//------------------------------------------------------------------------------
//
//  Project:   IOSS
//
//             Brno University of Technology
//             Faculty of Information Technology
//
//------------------------------------------------------------------------------
//
//             This project was financially supported by project 
//                  TIP FR-TI1/195 funds provided by MPO CR. 
//
//------------------------------------------------------------------------------
/*! 

@file      

@brief     Hlavny zdrojovy subor, funkcia main
@details   Hlavny zdrojovy subor, funkcia main
@authors   Betko Peter (<pbetko@gmail.com>)
@date      11.10.2011
@note      This project was supported by MPO CR TIP FR-TI1/195.
@copyright (c) 2011 Betko Peter

*/
/*
************************** SLEDOVANIE OBJEKTU VO VIDEU *************************
********************************* Peter Betko **********************************

autor: Peter Betko 
datum: 11.10.2011

Program na sledovanie objektu vo videu, s vyuzitim algoritmov zalozenych na baze
Casticoveho filtra (Particle filter). 

Pouzitie zdrojovych suborov upravuje zmluva uzatvorena medzi autorom, Petrom 
Betkom, a Vysokym Ucenim Technickym v Brne. Pouzitie na ine ucely ako uvedene
v zmluve, pripadne pouzitie inym subjektom ako Vysokym Ucenim Technickym v Brne, 
nie je dovolene. Pouzitie na komercne ucely je mozne az po dohode s autorom.

subor track.cpp - hlavny zdrojovy subor, funkcia main
*/

#ifdef _MSC_VER
#pragma warning(disable:4996)
#pragma warning(disable:4819)
#pragma warning(disable:4244)
#pragma comment(lib, "cv.lib")
#pragma comment(lib, "cxcore.lib")
#pragma comment(lib, "cvaux.lib")
#pragma comment(lib, "highgui.lib")
#endif

#include <stdio.h>
#include <stdlib.h>
#include "cv.h"
#include "cvaux.h"
#include "cxcore.h"
#include "highgui.h"

#include "opencvx/cvxmat.h"
#include "opencvx/cvxrectangle.h"
#include "opencvx/cvrect32f.h"
#include "opencvx/cvcropimageroi.h"
#include "opencvx/cvdrawrectangle.h"
#include "opencvx/cvparticle.h"
#include <time.h>

#include "state.h"
#include "observetemplate.h"

//sposob ohodnocovania castic
//! @addtogroup OHODNOCOVANIE_CASTIC Ohodnocovanie castic
//! Sposob ohodnocovania castic.
//! @{
#define DEFAULT 0
#define GRAY_HIST 1
#define RGB_HIST 2
#define HYBRID 3
//! @}

//minimalny a maximalny pocet castic, pouzite pri dyn. parametroch
//! @addtogroup POCET_CASTIC Pocet castic
//! Minimalny a maximalny pocet castic, pouzite pri dyn. parametroch.
//! @{
#define NUM_PARTICLES_MIN 100
#define NUM_PARTICLES_MAX 600
//! @}

//nastavenie sumu pri prekryvani a minimalne a maximalne nastavenie sumu
#define NOISE_OVERLAY 30
#define NOISE_MIN 5
#define NOISE_MAX 20

//navratove kody funkcie na nacitanie parametrov
#define EOK 0
#define HELP 1
#define EBAD_ARGC 2
#define EBAD_NUMBER 3

//dlzka pola bodov pre dynamicke parametre
#define DYN_PT_LEN 10

//realokacny blok pre vykreslovanie trajektorie
#define DRAW_PARTICLES_REALLOC 512

//testovacie vypisy
#define DEBUG 0

//premenna zabranujuca callbacku znovu nacitat vstupny region
bool regionSelected = FALSE;

/******************************* Structures ************************/

//struktura pre callback vyznacujuci region
typedef struct IcvMouseParam {
    CvPoint loc1;
    CvPoint loc2;
    char* win_name;
    IplImage* frame;
} IcvMouseParam;

//struktura pre callback vyznacujuci stred
typedef struct IcvMouseParamPt {
    CvPoint loc;    
    char* win_name;
    IplImage* frame;
} IcvMouseParamPt;

//struktura so vstupnymi parametrami
typedef struct inputParams {
	char* vidFile;		//zdrojovy subor
	int p;				//pocet particles
	int sx;				//x-ovy sum
	int sy;				//y-ovy sum
	int sw;				//sum pre sirku
	int sh;				//sum pre vysku
	int sr;				//sum pre rotaciu	
	CvSize resize;		//rozmery resize
	int evalType;		//typ ohodnocovania particles
	int m;				// 
	int n;				// pri hybrid ohodnocovani sa referencia a particles rozdelia na m x n casti
	bool dt;			// "draw trajectory"
	bool dap;			// "draw all particles"
	bool dpm;			//"dynamic parameters motion"
	bool dpe;			//"dynamic parameters evaluation"
	bool tl;			//"test learn"
	bool te;			//"test execute"
	int testStep;		//pocet framov, ktore sa preskocia pri testovani
	int pDyn;			//dynamicky pocet particles
	int overlapTresh;	//prahova hodnota detekujuca prekryvanie
		
} inputParams;

/**************************** Function Prototypes ******************/
/**							usage()
 ** Funkcia vypise na stdout napovedu programu.
 **/
void usage();

/**							argParse(...)
 ** Funkcia spracuje parametre prikazoveho riadku a ulozi ich do struktury 
 ** params.
 **
 ** argc: pocet parametrov
 ** argv: pole s parametrami
 ** params: struktura s parametrami, ktora bude funkciou naplnena 
 **/
int argParse( int argc, char** argv, inputParams * params);

/**							drawTrajectory(...)
 ** Funkcia vykresli do zadaneho obrazka (frame) trajektoriu, pospajanim bodov 
 ** (numPoints). Pouziva sa na postupne vykreslovanie trajektorie sledovaneho 
 **  objektu do kazdeho obrazku videa. 
 ** 
 ** frame: obrazok, do kt. sa bude vykreslovat
 ** points: pole bodov urcujucich trajektoriu
 ** numPoints: pocet bodov
 **/
void drawTrajectory(IplImage *frame, CvPoint *points, int numPoints);

/**							drawPoints(...)					
 ** Funkcia vykresli do zadaneho obrazka (frame) body (points). Pouziva sa
 ** vo faze testExecute, vykresluju sa body zadane
 ** 
 ** frame: obrazok, do kt. sa bude vykreslovat
 ** points: pole bodov, kt. sa budu vykreslovat
 ** numPoints: pocet bodov
 **/
void drawPoints(IplImage *frame, CvPoint *points, int numPoints);

/**							drawTestPointsRT(...)					
 ** Funkcia vykresli do zadaneho obrazka (frame) body (points) z realnej 
 ** trajektorie, ktore odpovedaju bodom zadanym uzivatelom v testovacej faze.
 ** 
 ** frame: obrazok, do kt. sa bude vykreslovat
 ** points: pole bodov, kt. sa budu vykreslovat
 ** numPoints: pocet bodov
 ** step: krok
 **/
void drawPointsRT(IplImage *frame, CvPoint *points, int numPoints, int step);

/**						evaluateSuccess(...)
 ** Funkcia sa vola vo faze testExecute. Z poloh telesa zadanych uzivatelom 
 ** vo faze testLearn (testPts) a poloh najlepsich castic pocas
 ** trackovania (particleCenters) vypocita priemernu odchylku od idealnej
 ** trajektorie. Odchylka je normalizovana velkostou castice.
 **
 ** particleCenters: polohy (stredy) castic, ziskanych samotnym trackovanim
 ** testPts: polohy objektu zadane uzivatelom vo faze testLearn
 ** numTestPts: pocet testovacich bodov
 ** step: dlzka kroku(pocet snimkov videa) medzi posebeiducimi polohami
 ** particleW: sirka castice
 ** particleH: vyska castice
 ** params: struktura so vstupnymi parametrami programu
 **/
void evaluateSuccess(CvPoint *particleCenters,CvPoint *testPts, int numTestPts, int step, int particleW, int particleH, inputParams *params);

/**						dynamicParamMotion(...)
 ** Funkcia podla aktualnej rychlosti sledovaneho objektu vypocita optimalny
 ** pocet a rozptyl castic. Hodnoty ulozi do struktury params.
 **
 ** p: struktura obsahujuca subor castic
 ** params: parametre programu (nacitane z prikazoveho riadka, nasledne 
 **		dynamicky menene)
 ** bestParticle: index najlepsej castice (z dovodu optimalizacie nedochadza
 **		k jeho vypoctu priamo vo funkcii, ale vyuzije sa skor ziskana hodnota) 
 **/
void dynamicParamMotion(CvParticle *p, inputParams *params, int bestParticle);

/**						dynamicParamEval(...)
 ** Funkcia na zaklade ohodnotenia najlepsej castice vypocita optimalny pocet
 ** a rozpty castic. Hodnoty ulozi do struktury params. Taktiez pri znizeni
 ** ohodnotenia pod prahovu uroven detekuje prekryvanie objektov a riesi ho.
 **
 ** p: struktura obsahujuca subor castic
 ** params: parametre programu (nacitane z prikazoveho riadka, nasledne 
 **		dynamicky menene)
 ** dynPt: pole bodov, oznacujucich polohu niekolkych poslednych castic
 **		s nadprahovym ohodnotenim
 ** bestPId: index aktualne najlepsej castice
 ** bestPEval: ohodnotenie aktualne najlepsej castice
 ** lastGood: castica, ktora ako posledna mala nadprahove ohodnotenie
 **/
bool dynamicParamEval(CvParticle *p, inputParams *params, CvPoint * dynPt, int bestPId, float bestPEval, CvParticleState *lastGood);

/**						setNoiseParams(...)
 **
 ** Funkcia nastavi rozptyl sumu ulozeny v struktorue params do struktury
 ** particle obsahujucej informacie o casticiach. Pouziva sa pri dynamickej
 ** zmene parametrov trackovania. 
 ** 
 ** p: struktura obsahujuca subor castic
 ** params: struktura obsahujuca parametre programu
 ** size: velkost obrazka
 **/
void setNoiseParams(CvParticle *p, inputParams *params, CvSize size);

/**						setNoiseParamsCond(...)
 **
 ** Podobne ako funkcia setNoiseParams(...) nastavuje rozptyl sumu, ale iba
 ** v pripade ze je vacsi ako ten aktualne nastaveny. Pouziva sa pri subeznom
 ** pouziti funkcii dynamicParamMotion(...) a dynamicParamEval(...), nastavene
 ** zostanu vyssie hodnoty sumu.
 **
 ** p: struktura obsahujuca subor castic
 ** params: struktura obsahujuca parametre programu
 ** size: velkost obrazka
 **/
void setNoiseParamsCond(CvParticle *p, inputParams *params, CvSize size);

/**						initDynPts(...)
 **
 ** Funkcia sluzi na inicializaciu bodov popisujucich posledne polohy najlepsej
 ** castice. Nainicializuje sa polohou najlepsej castice z prveho snimku videa.
 ** Body sa vyucizvaju vo funkciach dynamicParamMotion(...) a 
 ** dynamicParamEval(...)
 **
 ** dynPt: pole bodov
 ** bestParticle: najlepsia castica, ktorej poloha bude zapisana do pola bodov
 ** lastGood: posledna castica s nadprahovym ohodnotenim
 **/
void initDynPts(CvPoint * dynPt, CvParticleState bestParticle, CvParticleState *lastGood);

/**						updateDynPts(...)
 **
 ** Funkcia sluzi na ulozenie polohy najlepsej castice z aktualneho snimku
 ** do pola castic a vymazanie najstarsej. Pole sa da prirovnat k posuvnemu
 ** registru, ktory uchovava informaciu o poslednych DYN_PT_LEN polohach
 ** objektu.
 ** 
 ** dynPt: pole bodov
 ** bestParticle: najlepsia castica, ktorej poloha bude zapisana do pola bodov
 ** lastGood: posledna castica s nadprahovym ohodnotenim
 **/
void updateDynPts(CvPoint * dynPt, CvParticleState bestParticle, CvParticleState *lastGood);

/**						testLearn(...)
 **
 ** Funkcia zabezpecuje prvu fazu ohodnocovania presnosti programu. Nacita
 ** video specifikovane vo vstupnych parametroch a necha uzivatela kazdych
 ** n (tiez specifikovane v stupnych parametroch) snimkov znovu vyznacit polohu
 ** sledovaneho objektu. Spolu s rozmermi objektu a krokom n takto ziskane 
 ** informacie o polohe zapise do suboru.
 **
 ** params: vstupne parametre programu
 **/
void testLearn(inputParams *params);

/**						testExecute(...)
 **
 ** Funkcia nacita udaje zo suboru vytvoreneho funkciou testLearn(...).
 ** 
 ** params: vstupne parametre programu
 ** region: premenna, do ktorej budu ulozene rozmery (tym padom aj poloha)
 **		sledovaneho obejtku
 ** numTestPts: do premennej bude ulozeny pocet vyznacenych poloh objektu
 ** testPts: pole s jednotlivymi polohami objektu
 **/
void testExecute(inputParams *params, CvRect *region, int *numTestPts, CvPoint *testPts);
/**							icvGetRegion(...)
 ** 
 ** Funkcia zabezpecuje interaktivne vyznacenie objektu na sledovanie.
 ** Pomocou callback funkcie, reagujucej na akciu mysi sa vyberie 
 ** obdlznik a ulozi sa do premennej typu CvRect. 
 **/
void icvGetRegion( IplImage*, CvRect*, string);

/**							icvGetPoint(...)
 ** Funkcia zabezpecuje interaktivne vyznacenie objektu na sledovanie.
 ** Pomocou callback funkcie, reagujucej na akciu mysi sa vyberie 
 ** stred sledovaneho objektu a ulozi sa do premennej typu CvPoint.
 **/
void icvGetPoint( IplImage*, CvPoint*);

/**							icvMouseCallback(...)
 ** Callback funkcia volana z icvGetRegion(...)
 **/
void icvMouseCallback( int, int, int, int, void*);

/**							icvMouseCallbackPt(...)
 ** Callback funkcia volana z icvGetPoint(...)
 **/
void icvMouseCallbackPt( int, int, int, int, void* );

/**************************** Main ********************************/
int main( int argc, char** argv )
{
    IplImage *frame;
    CvCapture* video;	
    int frame_num = 0;
    int i = 0;
	int iterator = 0;	
	int numTestPts = 0;
	typedef struct CvVideoWriter CvVideoWriter;
	//struktura so vstupnymi parametrami sa nainicializuje defaultnymi hodnotami
	inputParams params = {NULL,300,5,5,0,0,0,cvSize(24,24),0,1,1,
		FALSE,FALSE,FALSE,FALSE,FALSE,FALSE,0,100,2400};		
	CvPoint *particleCenter = NULL;
	CvPoint testPts[128];		
	CvPoint dynPt[DYN_PT_LEN];	//pouzite pri dpm
	CvParticleState lastGood;		
	float bestPEval;
	int state = 0;
	int counter = 0;	
	int argParseResult;
	CvHistogram* histReferenceRed = NULL;
	CvHistogram* histReferenceGreen = NULL;
	CvHistogram* histReferenceBlue = NULL;
	CvHistogram* histReferenceGray = NULL;
	bool overlapping = FALSE;
	
	//nacitanie parametrov z prikazoveho riadku
    if((argParseResult = argParse( argc, argv , &params)) != EOK)
		switch(argParseResult)
		{
		case HELP:
			{
				usage();
				exit(0);
			}
			break;
		case EBAD_ARGC:
			{
				printf("ERROR: Incorrect number of parameters\nType \"-h\" for help\n");
				exit(0);
			}
			break;
		case EBAD_NUMBER:
			{
				printf("ERROR: Incorrect value of parameter(s)\nType \"-h\" for help\n");
				exit(0);
			}
		}

	
	if(params.tl) //test learn faza
	{
		testLearn(&params);
		exit(0);
	}

	//referencna matica pre hybridny pristup
	CvMat *matRefRGB = cvCreateMat(3, (256/DIVIDER) + 1, CV_8UC1);
	CvMat **matRef = (CvMat **)malloc((params.m + 1) * (params.n + 1) * sizeof(CvMat *));	
	for(int i = 0; i < (params.m + 1) * (params.n + 1); i++)
	{
		matRef[i] = cvCreateMat(3, (256/DIVIDER) + 1, CV_8UC1);
	}
	
	// pri dynamickom pocte castic nastav ich pocet defaultne na max hodnotu
	if(params.dpm || params.dpe)
		params.p = NUM_PARTICLES_MAX;
	params.pDyn = params.p;
	

	if((particleCenter = (CvPoint*)(malloc(DRAW_PARTICLES_REALLOC * sizeof(CvPoint)))) == NULL)
		return -1;

	if((!params.vidFile) && (params.te)) 
	{
		printf("V testovacom rezime nie je mozne pouzit video z webkamery");
		exit(0);
	}

    // read a video
    if( !params.vidFile || (isdigit(params.vidFile[0]) && params.vidFile[1] == '\0') )
        video = cvCaptureFromCAM( !params.vidFile ? 0 : params.vidFile[0] - '0' );
    else{
        video = cvCaptureFromAVI( params.vidFile ); 
        std::cout << cvGetCaptureProperty(video, CV_CAP_PROP_FPS) <<std::endl;
        }
    if( (frame = cvQueryFrame( video )) == NULL )
    {
        fprintf( stderr, "Video %s is not loadable.\n", params.vidFile );
        usage();
        exit(1);
    }

    // nechaj uzivatela vybrat objekt
    CvRect region;
	region.x = 1;
	region.y = 1;
	region.height = 1;
	region.width = 1;

	if(params.te) //test execute faza, uzivatel region nevybera
		testExecute(&params, &region, &numTestPts, testPts);
	else 
		icvGetRegion( frame, &region, "Select an initial region > SPACE > ESC" );
    std::cout << "JEDNAAAA" << endl;
	//CvVideoWriter *writer = cvCreateVideoWriter( "output.avi", CV_FOURCC('I', 'Y', 'U', 'V'), 25, cvGetSize(frame), 1);
	CvVideoWriter *writer = cvCreateVideoWriter( "output.avi", CV_FOURCC('D', 'I', 'V', 'X'), 25, cvGetSize(frame), 1 );
    std::cout << "DVAAAAAAAAAA" << endl;
	if(writer == NULL)
	{
		printf("Unable to save output file, please remove write protection\n");
		exit(0);
	}
	if((region.x < 0) || (region.x > frame->width) || (region.y < 0) || (region.y > frame->height) ||
		(region.width < 0) || (region.width > frame->width) || (region.height < 0) || (region.height > frame->height))
	{
		printf("Region not selected!\n");
		exit(0);
	}
	regionSelected = TRUE;


    // inicializacia particle filtra
    bool logprob = true;
    CvParticle *particle = cvCreateParticle( num_states, params.p, logprob );
    CvParticleState std = cvParticleState (
		params.sx,
        params.sy,
        params.sw,
        params.sh,
		params.sr
    );
    cvParticleStateConfig( particle, cvGetSize(frame), std );
    
    CvParticleState s;
    CvParticle *init_particle;
    init_particle = cvCreateParticle( num_states, 1 );
    CvRect32f region32f = cvRect32fFromRect( region );
    CvBox32f box = cvBox32fFromRect32f( region32f ); // centerize
    s = cvParticleState( box.cx, box.cy, box.width, box.height, 0.0 );
    cvParticleStateSet( init_particle, 0, s );
    cvParticleInit( particle, init_particle );
    cvReleaseParticle( &init_particle );

    //resizovanie referencneho obrazka
	IplImage* reference = cvCreateImage( params.resize, frame->depth, frame->nChannels );
	IplImage* tmp = cvCreateImage( cvSize(region.width,region.height), frame->depth, frame->nChannels );	
    cvCropImageROI( frame, tmp, region32f );	
    cvResize( tmp, reference );   	
    cvReleaseImage( &tmp );

	//inicializacia funkcii na ohodnocovanie castic
	if(params.evalType == RGB_HIST)		
		initializeRGBHist(reference, &histReferenceRed,&histReferenceGreen, &histReferenceBlue);
	if(params.evalType == GRAY_HIST)
		initializeGrayHist(reference, &histReferenceGray);
	if(params.evalType == HYBRID)
		initializeHyb(reference, matRef, params.m, params.n);

	i = 0;
	iterator = 0;	

	// cyklus iterujuci cez jednotlive framy videa
	while( ( frame = cvQueryFrame( video ) ) != NULL )
    {		
/**		if(DEBUG)
			printf("pocet particles: %d\n", params.pDyn);
*/		
		// prechod castic, pripocitaj sum pripade dynamiku
        cvParticleTransition( particle );		

		//ohodnot castice zvolenym pristupom
		if(params.evalType == DEFAULT)
			particleEvalDefault( particle, frame, reference, params.resize, params.pDyn);
/**		else if(params.evalType == GRAY_HIST)
			particleEvalGrayHist( particle, frame, histReferenceGray, params.resize, params.pDyn);
		else if(params.evalType == RGB_HIST)			
			particleEvalRGBHist( particle, frame, histReferenceRed,histReferenceGreen, histReferenceBlue, params.resize, params.pDyn);
		else if(params.evalType == HYBRID)			
			particleEvalHybrid( particle, frame, reference, matRef, params.m, params.n, params.resize, params.pDyn);
*/
    /**
		if(params.dap) //draw all particles
			for(i = 0; i < params.pDyn; i++ )
          {
              CvParticleState s = cvParticleStateGet( particle, i );
              cvParticleStateDisplay( s, frame, CV_RGB(0,0,255),1 );
          }
*/

//VVVV


		//zistenie polohy najlepsej castice a jej vykreslenie

        int maxp_id = cvParticleGetMax( particle );				
        CvParticleState maxs = cvParticleStateGet( particle, maxp_id );
		if(!overlapping)
			cvParticleStateDisplay( maxs, frame, CV_RGB(255,0,0), 4);
/**						
		//vykreslenie testovacich bodov
			if(params.te) //test execute
			{
				drawPoints(frame, testPts, numTestPts);
				drawPointsRT(frame, particleCenter, iterator, params.testStep);
			}
*/
		//vykreslenie trajektorie

/**
		if(((iterator % (DRAW_PARTICLES_REALLOC)) == 0) && (iterator != 0))	//je treba reallocovat?		
			if((particleCenter = (CvPoint *) realloc(particleCenter, (iterator + DRAW_PARTICLES_REALLOC) * sizeof(CvPoint))) == NULL)
				return -1;
		particleCenter[iterator].x = maxs.x;
		particleCenter[iterator].y = maxs.y;

*/
/**
		if(params.dt) //draw trajectory
		{
			drawTrajectory(frame, particleCenter, iterator);
		}
		if(DEBUG)
			printf("ohodnotenie najlepsej castice: %f\n", cvParticleGetMaxVal(particle));
*/			
/**		if(iterator == 0)
			initDynPts(dynPt, maxs, &lastGood);
*/

/**
		if(params.dpe) //inicializacia dynamic particle evaluation
		{			
			bestPEval = cvParticleGetMaxVal(particle);
			if(DEBUG)
				printf("Ohodnotenie najlepsej particle: %f\n", bestPEval);
		}
		if(params.dpe && params.dpm)	//aj evaluation aj motion
		{
			dynamicParamMotion(particle, &params, maxp_id);
			setNoiseParams(particle, &params, cvGetSize(frame));
			if(!(dynamicParamEval(particle, &params, dynPt, maxp_id, bestPEval, &lastGood))) //ak nedoslo k prekryvaniu
			{
				updateDynPts(dynPt, maxs, &lastGood);
				setNoiseParamsCond(particle, &params, cvGetSize(frame));			
				overlapping = FALSE;
			}
			else //doslo k prekryvaniu			
			{
				setNoiseParams(particle, &params, cvGetSize(frame));						
				overlapping = TRUE;
			}
		}
		else if(params.dpe && (!params.dpm))	//iba evaluation
		{
			if(!(dynamicParamEval(particle, &params, dynPt, maxp_id, bestPEval, &lastGood))) //ak nedoslo k prekryvaniu				
			{
				updateDynPts(dynPt, maxs, &lastGood);
				overlapping = FALSE;
			}
			else
			{
				overlapping = TRUE;				
			}

			setNoiseParams(particle, &params, cvGetSize(frame));
		}
		else if((!params.dpe) && params.dpm)	//iba motion
		{
			dynamicParamMotion(particle, &params, maxp_id);
			setNoiseParams(particle, &params, cvGetSize(frame));
		}
*/		
		
        cvShowImage( "Select an initial region > SPACE > ESC", frame );
		cvWriteFrame( writer, frame );

		//normalizacia castic
		cvParticleNormalize( particle);
	        
		//prevzorkovanie castic
        cvParticleResample( particle );

        char c = cvWaitKey( 1 );
        if(c == '\x1b')
            break;
/**
		if(DEBUG)
			printf("%d\n", iterator);
		iterator++;
*/
    }	//koniec while - nacitavania framov
/**
	if(params.te) //test execute
		evaluateSuccess(particleCenter,testPts, numTestPts, params.testStep, reference->width, reference->height, &params);
*/
	cvReleaseVideoWriter( &writer );
    //cvParticleObserveFinalize();
    cvDestroyWindow( "Select an initial region > SPACE > ESC" );
    cvReleaseImage( &reference );
    cvReleaseParticle( &particle );
    cvReleaseCapture( &video );	
	free(particleCenter);
    return 0;
	
}

/**
 * Print usage
 */
void usage()
{
    printf(	 "\n"
             "******************* track ********************\n"
			 "********** Object tracking in video **********\n"
			 "**************** Peter Betko *****************\n\n"
             "Usage: track\n"
			 " -et <def>|<gray>|<rgb>|<hyb m n>\n"
			 "     Particle evaluation type\n"
			 "     \"def\" for default\n"
			 "     \"gray\" for grayscale histogram\n"
			 "     \"rgb\" for RGB histogram\n"
			 "     \"hyb m n\" for hybrid with parameters m and n\n"
			 " -rs <width> <height>\n"
             "     Object template is resized into this size.\n"
             " -p <num_particles>\n"
             "     Number of particles. \n"
             " -sx <noise_std_for_x>\n"
             "     Noise for x coord. \n"
             " -sy <noise_std_for_y>\n"
             "     Noise for y coord.\n"
             " -sw <noise_std_for_width>\n"
             "     Noise for width.\n"
             " -sh <noise_std_for_height>\n"
             "     Noise for height.\n"
             " -sr <noise_std_for_rotate>\n"
             "     Noise for angle.\n"
			 " -dt\n"
			 "     Draw trajectory\n"
			 " -dpm\n"
			 "     Dynamic parameters based on motion\n"
			 " -dpe <overlay_threshold>\n"
			 "     Dynamic parameters based on evaluation\n"
			 " -tl <n_frames>\n"
			 "     Accuracy testing - 1. phase (Learn)\n"
			 "     <n_frames> - interval of object marking\n"
			 " -te\n"
			 "     Accuracy testing - 2. phase (Execute)\n"
             " -dap\n"
			 "     Draw all particles\n"
             " -h\n"
             "     Show this help\n"
             " <vidFile | camera_index>\n"
             );
}

/**
 * Parse command arguments
 */
int argParse( int argc, char** argv, inputParams * params)
{
    int i;
    for( i = 1; i < argc; i++ )
    {
        if( !strcmp( argv[i], "-h" ) )
			return HELP;
		else if(!strcmp( argv[i], "-et" ) )
		{
			if(argc <= ++i)
				return EBAD_ARGC;			
			if(!strcmp( argv[i], "def" ) )
				params->evalType = DEFAULT;
			else if(!strcmp( argv[i], "gray" ) )
				params->evalType = GRAY_HIST;
			else if(!strcmp( argv[i], "rgb" ) )
				params->evalType = RGB_HIST;
			else if(!strcmp( argv[i], "hyb" ) )
			{
				params->evalType = HYBRID;
				if(argc <= i+2)
					return EBAD_ARGC;
				params->m = atof(argv[++i]);
				params->n = atof(argv[++i]);
				if((params->m < 1) || (params->n < 1))
					return EBAD_NUMBER;
			}
		}
        else if( !strcmp( argv[i], "-rs" ) )
        {
			if(argc <= i+2)
				return EBAD_ARGC;
            int width = atoi(argv[++i]);
            int height = atoi(argv[++i]);
			if((width < 1) || (height < 1))
				return EBAD_NUMBER;
			params->resize = cvSize( width, height );
        }
        else if( !strcmp( argv[i], "-p" ) )
        {
			if(argc <= i+1)
				return EBAD_ARGC;
            params->p = atoi( argv[++i] );
			if(params->p < 1)
				return EBAD_NUMBER;
        }
        else if( !strcmp( argv[i], "-sx" ) )
        {
			if(argc <= i+1)			
			return EBAD_ARGC;
            params->sx = atof( argv[++i] );
			if(params->sx < 0)
				return EBAD_NUMBER;
        }
        else if( !strcmp( argv[i], "-sy" ) )
        {
			if(argc <= i+1)
				return EBAD_ARGC;
            params->sy = atof( argv[++i] );
			if(params->sy < 0)
				return EBAD_NUMBER;
        }
        else if( !strcmp( argv[i], "-sw" ) )
        {
			if(argc <= i+1)
				return EBAD_ARGC;
            params->sw = atof( argv[++i] );
			if(params->sw < 0)
				return EBAD_NUMBER;
        }
        else if( !strcmp( argv[i], "-sh" ) )
        {
			if(argc <= i+1)
				return EBAD_ARGC;
            params->sh = atof( argv[++i] );
			if(params->sh < 0)
				return EBAD_NUMBER;
        }
        else if( !strcmp( argv[i], "-sr" ) )
        {
			if(argc <= i+1)
				return EBAD_ARGC;
            params->sr = atof( argv[++i] );
			if(params->sr < 0)
				return EBAD_NUMBER;
        }
		else if( !strcmp( argv[i], "-dt" ) )
        {
            params->dt = TRUE;
        }
		else if( !strcmp( argv[i], "-dap" ) )
        {
            params->dap = TRUE;
        }
		else if( !strcmp( argv[i], "-dpm" ) )
        {
            params->dpm = TRUE;
        }
		else if( !strcmp( argv[i], "-dpe" ) )
        {
            params->dpe = TRUE;
			if(argc <= i+1)
				return EBAD_ARGC;
			params->overlapTresh = atof( argv[++i] );
			if(params->overlapTresh < 0)
				return EBAD_NUMBER;
        }
		else if( !strcmp( argv[i], "-tl" ) )
        {
            params->tl = TRUE;
			if(argc <= i+1)
				return EBAD_ARGC;
			params->testStep = atof(argv[++i]);
			if(params->testStep < 1)
				return EBAD_NUMBER;
        }
		else if( !strcmp( argv[i], "-te" ) )
        {
            params->te = TRUE;
        }
        else 
        {
			params->vidFile = argv[i];
        }
    }
	return EOK;
}

/**
 * Allows the user to interactively select the initial object region
 *
 * @param frame  The frame of video in which objects are to be selected
 * @param region A pointer to an array to be filled with rectangles
 * @param windowName Name of the window to display the frame.
 */
void icvGetRegion( IplImage* frame, CvRect* region, string windowName)
{
    IcvMouseParam p;
    
    /* use mouse callback to allow user to define object regions */
    //p.win_name = "Select an initial region > SPACE > ESC";
	p.win_name = (char *) windowName.c_str();
    p.frame = frame;	

    cvNamedWindow( p.win_name, 1 );
    cvShowImage( p.win_name, frame );
    cvSetMouseCallback( p.win_name, &icvMouseCallback, &p );
    cvWaitKey( 0 );
    //cvDestroyWindow( win_name );


    /* extract regions defined by user; store as a rectangle */
    region->x      = MIN( p.loc1.x, p.loc2.x );
    region->y      = MIN( p.loc1.y, p.loc2.y );
    region->width  = MAX( p.loc1.x, p.loc2.x ) - region->x + 1;
    region->height = MAX( p.loc1.y, p.loc2.y ) - region->y + 1;
}


void icvGetPoint( IplImage* frame, CvPoint* pt )
{
    IcvMouseParamPt p;

    // use mouse callback to allow user to define object regions
	p.win_name = "Select object > select centers";
    p.frame = frame;

    cvNamedWindow( p.win_name, 1 );
    cvShowImage( p.win_name, frame );
    cvSetMouseCallback( p.win_name, &icvMouseCallbackPt, &p );
    cvWaitKey( 0 );
    //cvDestroyWindow( win_name );

	pt->x = p.loc.x;
	pt->y = p.loc.y;
}



/**
 * Mouse callback function that allows user to specify the 
 * initial object region. 
 * Parameters are as specified in OpenCV documentation.
 */
void icvMouseCallback( int event, int x, int y, int flags, void* param)
{
	if(!regionSelected)
	{
		IcvMouseParam* p = (IcvMouseParam*)param;
		IplImage* clone;
		static int pressed = false;

		/* on left button press, remember first corner of rectangle around object */
		if( event == CV_EVENT_LBUTTONDOWN )
		{
			p->loc1.x = x;
			p->loc1.y = y;
			pressed = true;
		}

		/* on left button up, finalize the rectangle and draw it */
		else if( event == CV_EVENT_LBUTTONUP )
		{
			p->loc2.x = x;
			p->loc2.y = y;
			clone = (IplImage*)cvClone( p->frame );
			cvRectangle( clone, p->loc1, p->loc2, CV_RGB(255,255,255), 1, 8, 0 );
			cvShowImage( p->win_name, clone );
			cvReleaseImage( &clone );
			pressed = false;
		}

		/* on mouse move with left button down, draw rectangle */
		else if( event == CV_EVENT_MOUSEMOVE  &&  flags & CV_EVENT_FLAG_LBUTTON )
		{
			clone = (IplImage*)cvClone( p->frame );
			cvRectangle( clone, p->loc1, cvPoint(x, y), CV_RGB(255,255,255), 1, 8, 0 );
			cvShowImage( p->win_name, clone );
			cvReleaseImage( &clone );
		}
	}

}


void icvMouseCallbackPt( int event, int x, int y, int flags, void* param )
{
	IcvMouseParamPt* p = (IcvMouseParamPt*)param;
    // on left button press 
    if( event == CV_EVENT_LBUTTONDOWN )
    {
        p->loc.x = x;
        p->loc.y = y;        
    }
}


void drawTrajectory(IplImage *frame, CvPoint *points, int numPoints)
{
	int i;
	if(numPoints > 0)
		for(i=0; i<(numPoints - 1); i++)
			cvLine(frame, points[i], points[i+1], CV_RGB(255,0,0), 2, 8, 0);
}

void drawPoints(IplImage *frame, CvPoint *points, int numPoints)
{
	int i;
	if(numPoints > 0)
		for(i=0; i<numPoints; i++)
			cvCircle(frame,points[i],3,CV_RGB(0,255,0),3);
}

void drawPointsRT(IplImage *frame, CvPoint *points, int numPoints, int step)
{
	int i;
	if(numPoints > 0)
		for(i=0; i<numPoints; i++)
			if(i%step == 0)
				cvCircle(frame,points[i],3,CV_RGB(255,0,0),3);
}

void evaluateSuccess(CvPoint *particleCenters,CvPoint *testPts, int numTestPts, int step, int particleW, int particleH, inputParams *params)
{
	float distanceSum = 0;
	int i;
	float dx,dy;
	float distance;

	printf("\n========== Test results ==========\n");

	//rozmery castic sa nemenia, mozme normalizovat
	if((params->sw == 0) && (params->sh == 0))  
	{
		for(i = 0; i < numTestPts; i++)
		{
			dx = (float)(testPts[i].x - particleCenters[i*step].x) / particleW;
			dy = (float)(testPts[i].y - particleCenters[i*step].y) / particleH;		
			distance = sqrt(dx*dx + dy*dy);
			distanceSum += distance;
		//	printf("Distance for frame %d is: %.2f\n", i*step, distance);		
		}

		printf("\nSum of normalized distances: %.4f\n", distanceSum);
		printf("Average normalized distance: %.4f\n", distanceSum/numTestPts);
	}
	else //rozmery castic sa menia, nemozeme normalizovat
	{
		for(i = 0; i < numTestPts; i++)
		{
			dx = (testPts[i].x - particleCenters[i*step].x);
			dy = (testPts[i].y - particleCenters[i*step].y);		
			distance = sqrt(dx*dx + dy*dy);
			distanceSum += distance;
		//	printf("Distance for frame %d is: %.2f\n", i*step, distance);		
		}

		printf("\nSum of unnormalized distances: %.4fpx\n", distanceSum);
		printf("Average unnormalized distance: %.4fpx\n", distanceSum/numTestPts);
	}


	//docasne, zapis vysledkov do suboru
	//urcene na testovacie ucely
	/*FILE *f;	
	if((f = fopen("sample24_testResults.txt", "a")) == NULL)
		exit(0);
	fprintf(f, "-p: %d -sx: %d -sy: %d Average distance: %.4f\n", params->p, params->sx, params->sy, distanceSum/numTestPts);

	fclose(f);
	*/


}


void dynamicParamMotion(CvParticle *p, inputParams *params, int bestParticle)
{	
	static int xOld = cvmGet(p->particles, 0, bestParticle);			
	static int yOld = cvmGet(p->particles, 1, bestParticle);			
	int xNew = cvmGet(p->particles, 0, bestParticle);
	int yNew = cvmGet(p->particles, 1, bestParticle);
	int dx = abs(xOld - xNew);
	int dy = abs(yOld - yNew);

	params->sx = dx + 5;
	params->sy = dy + 5;

	//interval sumu 0->20 sa namapuje na interval poctu particles NUM_PARTICLES_MIN->NUM_PARTICLES_MAX
	params->pDyn = NUM_PARTICLES_MIN + ((float)(NUM_PARTICLES_MAX - NUM_PARTICLES_MIN) / NOISE_MAX) * ((dx + dy) / 2);	
	if(params->pDyn > NUM_PARTICLES_MAX )
		params->pDyn = NUM_PARTICLES_MAX ;
	

	xOld = xNew;
	yOld = yNew;
}

bool dynamicParamEval(CvParticle *p, inputParams *params, CvPoint * dynPt, int bestPId, float bestPEval, CvParticleState *lastGood)
{
	bool overlapping = FALSE;
	int dx,dy;
	static int counter = 0;
		
	if((params->evalType == DEFAULT) || (params->evalType == HYBRID)) //DEFAULT A HYBRID
	{
		if(bestPEval > (-1 * params->overlapTresh)) //nedoslo k prekryvaniu
		{
			params->sx = params->sy = NOISE_MIN	+ ((float)(NOISE_MAX-NOISE_MIN)/(params->overlapTresh)) * (-1 * bestPEval);
			params->pDyn = NUM_PARTICLES_MIN + ((float)(NUM_PARTICLES_MAX - NUM_PARTICLES_MIN) / (params->overlapTresh))
							* (-1 * bestPEval);										
			if(params->pDyn > NUM_PARTICLES_MAX)
				params->pDyn = NUM_PARTICLES_MAX;
			counter = 0; 
			overlapping = FALSE;
		}
		else
			overlapping = TRUE;
	}
	else//GRAY a RGB
	{
		if(bestPEval > (params->overlapTresh)) //nedoslo k prekryvaniu
		{
			params->sx = params->sy = NOISE_MIN	+ ((float)(NOISE_MAX-NOISE_MIN)/(params->overlapTresh)) * (bestPEval);
			params->pDyn = NUM_PARTICLES_MIN + ((float)(NUM_PARTICLES_MAX - NUM_PARTICLES_MIN) / (params->overlapTresh))
							* (bestPEval);										
			if(params->pDyn > NUM_PARTICLES_MAX)
				params->pDyn = NUM_PARTICLES_MAX;
			counter = 0; 
			overlapping = FALSE;
		}
		else
			overlapping = TRUE;
	}

	
	if(overlapping)
	{	
		if(DEBUG)
			printf("OVERLAPPING!");
		counter++;
		params->pDyn = NUM_PARTICLES_MAX;

		dx = dynPt[4].x - dynPt[DYN_PT_LEN - 1].x;
		dy = dynPt[4].y - dynPt[DYN_PT_LEN - 1].y;
					
		params->sx = NOISE_OVERLAY +  counter;
		params->sy = NOISE_OVERLAY +  counter;

		cvmSet(p->particles, 0, 0, lastGood->x + ((counter * dx) / (DYN_PT_LEN - 5)));
		cvmSet(p->particles, 1, 0, lastGood->y + ((counter * dy) / (DYN_PT_LEN - 5)));
		cvmSet(p->particles, 2, 0, lastGood->width);
		cvmSet(p->particles, 3, 0, lastGood->height);
		cvmSet(p->particles, 4, 0, lastGood->angle);
		if((params->evalType == DEFAULT) || (params->evalType == HYBRID))
			cvmSet(p->weights, 0, 0, -1.0);
		else
			cvmSet(p->weights, 0, 0, -9999.0);
	}
//	else
//		counter = 0;
	return overlapping;
}

void setNoiseParams(CvParticle *p, inputParams *params, CvSize size)
{
	CvParticleState std = cvParticleState (params->sx, params->sy, params->sw, params->sh, params->sr);
	cvParticleStateConfig( p, size, std );
}

void setNoiseParamsCond(CvParticle *p, inputParams *params, CvSize size)
{
	int tempX, tempY;
	tempX = cvmGet(p->std, 0, 0);
	tempY = cvmGet(p->std, 1, 0);	

	if(params->sx > cvmGet(p->std, 0, 0))
		tempX = params->sx;
	if(params->sy > cvmGet(p->std, 1, 0))
		tempY = params->sy;

	CvParticleState std = cvParticleState (tempX, tempY, params->sw, params->sh, params->sr);
	cvParticleStateConfig( p, size, std );
}
void initDynPts(CvPoint * dynPt, CvParticleState bestParticle, CvParticleState *lastGood)
{
	int i = 0;
	for(i=0; i<DYN_PT_LEN; i++)
	{
		dynPt[i].x = bestParticle.x;
		dynPt[i].y = bestParticle.y;
	}
	*lastGood = bestParticle;
}
void updateDynPts(CvPoint * dynPt, CvParticleState bestParticle, CvParticleState *lastGood)
{
	int i = 0;
	for(i=DYN_PT_LEN - 1; i>0;i--)
		dynPt[i] = dynPt[i-1];
	dynPt[0].x = bestParticle.x;
	dynPt[0].y = bestParticle.y;

	*lastGood = bestParticle;
}

void testLearn(inputParams *params)
{
	IplImage *frame;
	CvRect testRegion;
	int iterator = 0;	
	int i = 0;
	CvPoint center;
	CvRect position;
	int numTestPts = 0;
	CvPoint testPts[256];
	CvCapture* video;	


	if(params->vidFile)
    {
        video = cvCaptureFromAVI( params->vidFile); 
    }

	else
	{
        fprintf( stderr, "Video is not loadable.\n");
        usage();
        exit(1);
	}
    if( (frame = cvQueryFrame( video )) == NULL )
    {
        fprintf( stderr, "Video %s is not loadable.\n", params->vidFile );
        usage();
        exit(1);
    }

	
	icvGetRegion( frame, &testRegion, "Select object > select positions" );



	while( ( frame = cvQueryFrame( video ) ) != NULL )
	{
		
		drawPoints(frame, testPts, numTestPts);

		if((iterator % params->testStep) == 0)
		{
			//icvGetPoint(frame,&center);
			//testPts[numTestPts] = center;
			icvGetRegion( frame, &position, "Select object > select positions" );
			center.x = position.x + position.width/2; 
			center.y = position.y + position.height/2;
			testPts[numTestPts] = center;

			numTestPts++;
		}
		
		cvShowImage( "Select object > select positions", frame );
		char c = cvWaitKey( 1 );
        if(c == '\x1b')
            break;
		iterator++;

	}

	//zapis do suboru
	FILE *f;
	string fileName = params->vidFile;
	fileName += "_test.txt";
	if((f = fopen(fileName.c_str(), "w")) == NULL)
		exit(0);
	fprintf(f, "%d %d %d %d\n", testRegion.x, testRegion.y, testRegion.width, testRegion.height);
	fprintf(f, "%d\n", params->testStep);
	fprintf(f, "%d\n", numTestPts);
	for(i = 0; i< numTestPts; i++)
		fprintf(f, "%d %d\n", testPts[i].x, testPts[i].y);
	fclose(f);

	cvDestroyWindow( "Select object > select centers" );
	//exit(0);
}

void testExecute(inputParams *params, CvRect *region, int *numTestPts, CvPoint *testPts)
{
	FILE *f;
	int i;
	
	string fileName = params->vidFile;

	fileName += "_test.txt";
	if((f = fopen(fileName.c_str(), "r")) == NULL)
	{
		fprintf(stderr, "File not found\n");
		exit(0);
	}
	fscanf(f, "%d %d %d %d\n", &(region->x), &(region->y), &(region->width), &(region->height));
	fscanf(f, "%d", &params->testStep);
	fscanf(f, "%d", numTestPts);
	for(i = 0; i<(*numTestPts); i++)
		fscanf(f, "%d %d\n", &(testPts[i].x), &(testPts[i].y));
	fclose(f);
}
