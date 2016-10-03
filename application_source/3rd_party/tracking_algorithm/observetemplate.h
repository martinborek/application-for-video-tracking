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

@brief     Meraci model, funkcie na ohodnocovanie castic
@details   Meraci model, funkcie na ohodnocovanie castic
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

subor observetemplate.h - meraci model, funkcie na ohodnocovanie castic
*/

#ifndef CV_PARTICLE_OBSERVE_TEMPLATE_H
#define CV_PARTICLE_OBSERVE_TEMPLATE_H

#include "opencvx/cvparticle.h"
#include "opencvx/cvrect32f.h"
#include "opencvx/cvcropimageroi.h"
#include "state.h"

#define HIST_SIZE 128
#define DIVIDER 4

using namespace std;


/******************** Function Prototypes **********************/
#ifndef NO_DOXYGEN

/*!
 ** Inicializacna funkcia pre ohodnocovanie castic pomocou farebneho 
 ** histogramu. Vytvoria sa histogramy pre R,G a B zlozku referencneho obrazka
 ** (ten sa pocas sledovania nemeni) a ulozia sa do predanych parametrov. 
 ** 
 ** @param reference Referencny obrazok.
 ** @param histReferenceRed Referencny histogram pre cervenu farbu.
 ** @param histReferenceGreen Referencny histogram pre zelenu farbu.
 ** @param histReferenceBlue Referencny histogram pre modru farbu.
 **/
void initializeRGBHist(IplImage *reference, CvHistogram** histReferenceRed, 
					   CvHistogram** histReferenceGreen, CvHistogram** histReferenceBlue);

/*!
 ** Inicializacna funkcia pre ohodnocovanie castic pomocou sedotonoveho 
 ** histogramu. Vytvori sa histogram referencneho obrazku prevedeneho 
 ** do odtienov sedej a ulozi sa do predaneho parametra (histReferenceGray)
 **
 ** @param reference Referencny obrazok.
 ** @param histReferenceGray Referencny histogram pre sedotonovy obrazok.
 **/
void initializeGrayHist(IplImage *reference, CvHistogram** histReferenceGray);

/*!
 ** Inicializacna funkcia pre ohodnocovanie castic hybridnym pristupom.
 ** Referencny obrazok bude rozdeleny na nx*ny casti a kazda z nich ohodnotena
 ** farebnym histogramom. Ten vsak nebude implementovany klasickym cvHistogram,
 ** ale polom matic. Kazdej casti ref. obrazka zodpoveda jedna matica. Ta ma 3
 ** riadky (pre 3 farene zlozky) a pocet stlpcov je rovny poctu "binov" 
 ** histogramu.
 **
 ** @param reference Referencny obrazok.
 ** @param matRef Pole matic reprezentujucich histogramy.
 ** @param nx Pocet casti, na ktore sa rozdeli obrazok vo vodorovnej osi.
 ** @param ny Pocet casti, na ktore sa rozdeli obrazok v zvislej osi. 
 **/
void initializeHyb(IplImage *reference, CvMat **matRef, int nx, int ny);

/*!
 ** Funkcia na ohodnocovanie castic s vyuzitim farebneho histogramu.
 ** Pre kazdu casticu sa vytvoria histogramy pre jednotlive farebne kanaly
 ** a tie su nasledne porovnavane s referencnymi histogramami.
 **
 ** @param p Struktura s casticami.
 ** @param frame Aktualny snimok videa. 
 ** @param histReferenceRed Referencny histogram pre cervenu farbu.
 ** @param histReferenceGreen Referencny histogram pre zelenu farbu.
 ** @param histReferenceBlue Referencny histogram pre modru farbu.
 ** @param featSize Rozmery, na ktore sa ma resizovat kazda castica.
 ** @param numParticlesDyn Aktualny pocet castic. 
 **/
void particleEvalRGBHist( CvParticle* p, IplImage* frame, CvHistogram* histReferenceRed, CvHistogram* histReferenceGreen, 
						 CvHistogram* histReferenceBlue, CvSize featSize, int numParticlesDyn);

/*!
 ** Funkcia na ohodnocovanie castic s vyuzitim sedotonoveho histogramu. 
 ** Aktualny snimok videa sa prevedie do odtienov sedej, pre kazdu casticu je 
 ** vypocitany sedotonovy historam, ktory je nasledne porovnany s tym
 ** referencnym.
 **
 ** @param p Struktura s casticami.
 ** @param frame Aktualny snimok videa. 
 ** @param histReferenceGray Referencny sedotonovy histogram. 
 ** @param featSize Rozmery, na ktore sa ma resizovat kazda castica.
 ** @param numParticlesDyn Aktualny pocet castic. 
 **/
void particleEvalGrayHist( CvParticle* p, IplImage* frame, CvHistogram* histReferenceGray, CvSize featSize, int numParticlesDyn);

/*!
 ** Funkcia na ohodnocovanie castic zakladnym sposobom "po pixeloch". Kazda
 ** castica sa porovna s referencnym obrazkom, pomocou funkcie cvNorm(...).
 **
 ** @param p Struktura s casticami.
 ** @param frame Aktualny snimok videa.
 ** @param reference Referencny obrazok.
 ** @param featSize Rozmery, na ktore sa ma resizovat kazda castica.
 ** @param numParticlesDyn Aktualny pocet castic.
 **/
void particleEvalDefault( CvParticle* p, IplImage* frame, IplImage *reference,CvSize featSize, int numParticlesDyn );

/*!
 ** Funkcia na ohodnocovanie castic "hybridnym" sposobom. Z kazdej castice sa 
 ** vytvori pole histogramov (matic)(vysvetlene v komentaroch pre funkciu 
 ** @ref initializeHyb) a tie su nasledne porovnane s referencnymi 
 ** histogramami.
 **
 ** @param p Struktura s casticami.
 ** @param frame Aktualny snimok videa.
 ** @param reference Referencny obrazok.
 ** @param matRef Pole matic s histogramami.
 ** @param nx Pocet casti, na ktore sa rozdeli obrazok vo vodorovnej osi.
 ** @param ny Pocet casti, na ktore sa rozdeli obrazok v zvislej osi. 
 ** @param featSize Rozmery, na ktore sa ma resizovat kazda castica.
 ** @param numParticlesDyn Aktualny pocet castic.
 **/
void particleEvalHybrid( CvParticle* p, IplImage* frame, IplImage *reference, CvMat **matRef, int nx, int ny,CvSize featSize, int numParticlesDyn );


/*
  Funkcie vo vyvoji. Alternativny pristup k ohodnocovaniu farebnym histogramom.
*/
void initializeRGB2(IplImage *reference, CvMat *matRef, int nx, int ny);
void particleEvalRGB2( CvParticle* p, IplImage* frame, IplImage *reference, CvMat *matRef,CvSize featSize, int numParticlesDyn );

#endif
/***************************************************************/



void initializeRGBHist(IplImage *reference, CvHistogram** histReferenceRed, 
					   CvHistogram** histReferenceGreen, CvHistogram** histReferenceBlue)
{
	int hist_size = HIST_SIZE;
	IplImage* referenceRed = cvCreateImage(cvSize(reference->width,reference->height), IPL_DEPTH_8U, 1);
    IplImage* referenceGreen = cvCreateImage(cvSize(reference->width,reference->height), IPL_DEPTH_8U, 1);
    IplImage* referenceBlue = cvCreateImage(cvSize(reference->width,reference->height), IPL_DEPTH_8U, 1);

	cvSplit(reference, referenceRed, referenceGreen, referenceBlue, NULL);	

    *histReferenceRed = cvCreateHist(1, &hist_size, CV_HIST_ARRAY);
    *histReferenceGreen = cvCreateHist(1, &hist_size, CV_HIST_ARRAY);
    *histReferenceBlue = cvCreateHist(1, &hist_size, CV_HIST_ARRAY);

	cvCalcHist( &referenceRed, *histReferenceRed, 0, NULL );
	cvCalcHist( &referenceGreen, *histReferenceGreen, 0, NULL );
	cvCalcHist( &referenceBlue, *histReferenceBlue, 0, NULL );


}

void initializeGrayHist(IplImage *reference, CvHistogram** histReferenceGray)
{	
	int hist_size = HIST_SIZE;
	IplImage* referenceGray = cvCreateImage(cvSize(reference->width,reference->height), IPL_DEPTH_8U, 1);
	cvCvtColor(reference, referenceGray, CV_BGR2GRAY);

	*histReferenceGray = cvCreateHist(1, &hist_size, CV_HIST_ARRAY);
	cvCalcHist(&referenceGray, *histReferenceGray, 0, NULL);	
}



void initializeHyb(IplImage *reference, CvMat **matRef, int nx, int ny)
{
	int a,b,x,y;
	int i = 0;			
	int stepX = reference->width/nx; 
	int stepY = reference->height/ny;	

    uchar *ptrRef = NULL;
	
		for(a = 0; a < ny; a++)
		{
			for(b = 0; b < nx; b++)
			{
				for(y = a * stepY; y < ((a + 1) * stepY); y++)
				{
					if(y < reference->height)
					{
						ptrRef = (uchar*) (reference->imageData + y * reference->widthStep);
						
						for(x = b * stepX; x < ((b + 1) * stepX); x++)
						{
							if(x < reference->width)
							{
								(*(matRef[i]->data.ptr + (*(ptrRef+3*x))/DIVIDER))++; //nulty riadok + intenzita cerveneho pixelu / DIVIDER (-> kvazihistogram)
								(*(matRef[i]->data.ptr + matRef[i]->cols + (*(ptrRef + 3*x+1))/DIVIDER))++; //prvy riadok
								(*(matRef[i]->data.ptr + matRef[i]->cols * 2 + (*(ptrRef + 3*x+2))/DIVIDER))++;  //druhy riadok
								
							}
						}
					}
				}
				i++;
			}
		}
}

void initializeRGB2(IplImage *reference, CvMat *matRef, int nx, int ny)
{
	int y,x;
	int i = 0;					

    uchar *ptrRef = NULL;

	for(y = 0; y < 24; y++)
	{	
		ptrRef = (uchar*) (reference->imageData + y * reference->widthStep);				
		for(x = 0; x < 24; x++)
		{			
			(*(matRef->data.ptr + (*(ptrRef+3*x))/DIVIDER))++; //nulty riadok + intenzita cerveneho pixelu / DIVIDER (-> kvazihistogram)
			(*(matRef->data.ptr + matRef->cols + (*(ptrRef + 3*x+1))/DIVIDER))++; //prvy riadok
			(*(matRef->data.ptr + matRef->cols * 2 + (*(ptrRef + 3*x+2))/DIVIDER))++;  //druhy riadok
		}
	}	
}


void particleEvalRGBHist( CvParticle* p, IplImage* frame, CvHistogram* histReferenceRed,CvHistogram* histReferenceGreen,
						 CvHistogram* histReferenceBlue, CvSize featSize, int numParticlesDyn )
{
    int i;
    double likeli;
	double likeliRed;
	double likeliGreen;
	double likeliBlue;
    IplImage *patch;
    IplImage *resize;
    resize = cvCreateImage( featSize, frame->depth, frame->nChannels );
    int hist_size = HIST_SIZE;
	IplImage* frameRed;
	IplImage* frameGreen;
	IplImage* frameBlue;
	CvHistogram* histFrameRed;
	CvHistogram* histFrameGreen;
	CvHistogram* histFrameBlue;


    for( i = 0; i < numParticlesDyn; i++ ) 
    {
        CvParticleState s = cvParticleStateGet( p, i );		
        CvBox32f box32f = cvBox32f( s.x, s.y, s.width, s.height, s.angle );
        CvRect32f rect32f = cvRect32fFromBox32f( box32f );
        CvRect rect = cvRectFromRect32f( rect32f );
        
        patch = cvCreateImage( cvSize(rect.width,rect.height), frame->depth, frame->nChannels );
        cvCropImageROI( frame, patch, rect32f );
        cvResize( patch, resize );

		frameRed = cvCreateImage(cvSize(resize->width,resize->height), IPL_DEPTH_8U, 1);
		frameGreen = cvCreateImage(cvSize(resize->width,resize->height), IPL_DEPTH_8U, 1);
		frameBlue = cvCreateImage(cvSize(resize->width,resize->height), IPL_DEPTH_8U, 1);
		
		cvSplit(resize, frameRed, frameGreen,frameBlue, NULL);

		histFrameRed = cvCreateHist(1, &hist_size, CV_HIST_ARRAY);
		histFrameGreen = cvCreateHist(1, &hist_size, CV_HIST_ARRAY);
		histFrameBlue = cvCreateHist(1, &hist_size, CV_HIST_ARRAY);

		cvCalcHist( &frameRed, histFrameRed, 0, NULL );
		cvCalcHist( &frameGreen, histFrameGreen, 0, NULL );
		cvCalcHist( &frameBlue, histFrameBlue, 0, NULL );

		likeliRed = cvCompareHist(histFrameRed, histReferenceRed, CV_COMP_INTERSECT);
		likeliGreen = cvCompareHist(histFrameGreen, histReferenceGreen, CV_COMP_INTERSECT);
		likeliBlue = cvCompareHist(histFrameBlue, histReferenceBlue, CV_COMP_INTERSECT);		

		/*
		//moznost pouzit ine sposoby porovnavania histogramov

		likeliRed = cvCompareHist(histFrameRed, histReferenceRed, CV_COMP_CHISQR);
		likeliGreen = cvCompareHist(histFrameGreen, histReferenceGreen, CV_COMP_CHISQR);
		likeliBlue = cvCompareHist(histFrameBlue, histReferenceBlue, CV_COMP_CHISQR);

		likeliRed = cvCompareHist(histFrameRed, histReferenceRed, CV_COMP_CORREL);
		likeliGreen = cvCompareHist(histFrameGreen, histReferenceGreen, CV_COMP_CORREL);
		likeliBlue = cvCompareHist(histFrameBlue, histReferenceBlue, CV_COMP_CORREL);

		likeliRed = cvCompareHist(histFrameRed, histReferenceRed, CV_COMP_BHATTACHARYYA);
		likeliGreen = cvCompareHist(histFrameGreen, histReferenceGreen, CV_COMP_BHATTACHARYYA);
		likeliBlue = cvCompareHist(histFrameBlue, histReferenceBlue, CV_COMP_BHATTACHARYYA);
		*/

		likeli = likeliRed + likeliBlue + likeliGreen;		
		//likeli = likeli * (-1); //odkomentovat pre chi-square a battacharyya
		
       cvmSet( p->weights, 0, i, likeli );
        
        cvReleaseImage( &patch );
		cvReleaseImage( &frameRed );
		cvReleaseImage( &frameGreen );
		cvReleaseImage( &frameBlue );
		cvReleaseHist(&histFrameRed);
		cvReleaseHist(&histFrameGreen);
		cvReleaseHist(&histFrameBlue);
		
    }
    cvReleaseImage( &resize );

	for(i = numParticlesDyn; i < p->num_particles; i++)
		cvmSet( p->weights, 0, i, -99999.0 );
}

void particleEvalGrayHist( CvParticle* p, IplImage* frame, CvHistogram* histReferenceGray, CvSize featSize, int numParticlesDyn )
{
    int i;
    double likeli;
    IplImage *patch;
    IplImage *resize;
	int hist_size = HIST_SIZE;
    resize = cvCreateImage( featSize, frame->depth, frame->nChannels );

    for( i = 0; i < numParticlesDyn; i++ ) 
    {
        CvParticleState s = cvParticleStateGet( p, i );
		//CvBox32f = The Constructor of Center Coordinate Floating Rectangle Structure.
        CvBox32f box32f = cvBox32f( s.x, s.y, s.width, s.height, s.angle );
        CvRect32f rect32f = cvRect32fFromBox32f( box32f );
        CvRect rect = cvRectFromRect32f( rect32f );
        
        patch = cvCreateImage( cvSize(rect.width,rect.height), frame->depth, frame->nChannels );
        cvCropImageROI( frame, patch, rect32f );
        cvResize( patch, resize );

		IplImage* grayResize = cvCreateImage(cvSize(resize->width,resize->height), IPL_DEPTH_8U, 1);
		cvCvtColor(resize, grayResize, CV_BGR2GRAY);
	
	    CvHistogram* histResize = cvCreateHist(1, &hist_size, CV_HIST_ARRAY);
    
		cvCalcHist( &grayResize, histResize, 0, NULL );

		/*
		//moznost pouzit ine sposoby porovnavania histogramov

		likeli = cvCompareHist(histResize, histReference, CV_COMP_CORREL);	
		likeli = cvCompareHist(histResize, histReferenceGray, CV_COMP_CHISQR);
		*/

		likeli = cvCompareHist(histResize, histReferenceGray, CV_COMP_INTERSECT);
	
		cvmSet( p->weights, 0, i, likeli );
        //cvmSet( p->weights, 0, i, -1 * likeli ); //odkomentovat pre chi-square
        
        cvReleaseImage( &patch );
    }
    cvReleaseImage( &resize );

	for(i = numParticlesDyn; i < p->num_particles; i++)
		cvmSet( p->weights, 0, i, -99999.0 );
}

void particleEvalDefault( CvParticle* p, IplImage* frame, IplImage *reference, CvSize featSize, int numParticlesDyn )
{
    int i;
    double likeli;
    IplImage *patch;
    IplImage *resize;
    resize = cvCreateImage( featSize, frame->depth, frame->nChannels );

    for( i = 0; i < numParticlesDyn; i++ ) 
    {
        CvParticleState s = cvParticleStateGet( p, i );		
		CvBox32f box32f = cvBox32f( s.x, s.y, s.width, s.height, s.angle );
        CvRect32f rect32f = cvRect32fFromBox32f( box32f );
    	CvRect rect = cvRectFromRect32f( rect32f );        
		patch = cvCreateImage( cvSize(rect.width,rect.height), frame->depth, frame->nChannels );
        cvCropImageROI( frame, patch, rect32f );
        cvResize( patch, resize );

        likeli = -cvNorm( resize, reference, CV_L2 ); 

        cvmSet( p->weights, 0, i, likeli );
        
        cvReleaseImage( &patch );
    }
    cvReleaseImage( &resize );

	for(i = numParticlesDyn; i < p->num_particles; i++)
		cvmSet( p->weights, 0, i, -99999.0 );
}


void particleEvalHybrid( CvParticle* p, IplImage* frame, IplImage *reference, CvMat **matRef, int nx, int ny, CvSize featSize, int numParticlesDyn )
{
	
    int i,a,b,x,y;
	int j = 0;
		
    double likeli;
    IplImage *patch;
    IplImage *resize;
    resize = cvCreateImage( featSize, frame->depth, frame->nChannels );

	int stepX = featSize.width/nx;
	int stepY = featSize.height/ny;
	
	CvMat *matRes = cvCreateMat(3, (256/DIVIDER) + 1, CV_8UC1);

	cvZero(matRes);

	//uchar *ptrRef = NULL;
	uchar *ptrRes = NULL;

    for( i = 0; i < numParticlesDyn; i++ ) 
    {
        CvParticleState s = cvParticleStateGet( p, i );
		//CvBox32f = The Constructor of Center Coordinate Floating Rectangle Structure.
        CvBox32f box32f = cvBox32f( s.x, s.y, s.width, s.height, s.angle );
        CvRect32f rect32f = cvRect32fFromBox32f( box32f );
        CvRect rect = cvRectFromRect32f( rect32f );
        
        patch = cvCreateImage( cvSize(rect.width,rect.height), frame->depth, frame->nChannels );
        cvCropImageROI( frame, patch, rect32f );
        cvResize( patch, resize );		

		likeli = 0;
		j = 0;

		for(a = 0; a < ny; a++)
		{
			for(b = 0; b < nx; b++)
			{
				for(y = a * stepY; y < ((a + 1) * stepY); y++)
				{
					if(y < featSize.height)
					{					
						ptrRes = (uchar*) (resize->imageData + y * resize->widthStep);
						for(x = b * stepX; x < ((b + 1) * stepX); x++)
						{
							if(x < featSize.width)
							{
								(*(matRes->data.ptr + (*(ptrRes+3*x))/DIVIDER))++; //nulty riadok + intenzita cerveneho pixelu / DIVIDER (-> kvazihistogram)
								(*(matRes->data.ptr + matRes->cols + (*(ptrRes + 3*x+1))/DIVIDER))++; //prvy riadok
								(*(matRes->data.ptr + matRes->cols * 2 + (*(ptrRes + 3*x+2))/DIVIDER))++;  //druhy riadok	
							}
						}
					}
				}				
				likeli += cvNorm( matRef[j], matRes, CV_L2 );								
				j++;

				cvZero(matRes);

			}
		}

		likeli *= -1;				

		//nastavi hodnotu matice p->weights na suradniciach [0,i] na hodnotu likeli
        cvmSet( p->weights, 0, i, likeli );

		cvZero(matRes);
		
        
        cvReleaseImage( &patch );
    }
    cvReleaseImage( &resize );

	for(i = numParticlesDyn; i < p->num_particles; i++)
		cvmSet( p->weights, 0, i, -99999.0 );
}

void particleEvalRGB2( CvParticle* p, IplImage* frame, IplImage *reference, CvMat *matRef,CvSize featSize, int numParticlesDyn )
{
	
    int i,x,y;	
		
    double likeli;
    IplImage *patch;
    IplImage *resize;
    resize = cvCreateImage( featSize, frame->depth, frame->nChannels );	
	
	CvMat *matRes = cvCreateMat(3, (512/DIVIDER) + 1, CV_8UC1);

	cvZero(matRes);

	//uchar *ptrRef = NULL;
	uchar *ptrRes = NULL;

    for( i = 0; i < numParticlesDyn; i++ ) 
    {
        CvParticleState s = cvParticleStateGet( p, i );
		//CvBox32f = The Constructor of Center Coordinate Floating Rectangle Structure.
        CvBox32f box32f = cvBox32f( s.x, s.y, s.width, s.height, s.angle );
        CvRect32f rect32f = cvRect32fFromBox32f( box32f );
        CvRect rect = cvRectFromRect32f( rect32f );
        
        patch = cvCreateImage( cvSize(rect.width,rect.height), frame->depth, frame->nChannels );
        cvCropImageROI( frame, patch, rect32f );
        cvResize( patch, resize );		

		likeli = 0;


		for(y = 0; y < 24; y++)
		{				
			ptrRes = (uchar*) (resize->imageData + y * resize->widthStep); 
			for(x = 0; x < 24; x++)
			{
				(*(matRes->data.ptr + (*(ptrRes+3*x))/DIVIDER))++; //nulty riadok + intenzita cerveneho pixelu / DIVIDER (-> kvazihistogram)
				(*(matRes->data.ptr + matRes->cols + (*(ptrRes + 3*x+1))/DIVIDER))++; //prvy riadok
				(*(matRes->data.ptr + matRes->cols * 2 + (*(ptrRes + 3*x+2))/DIVIDER))++;  //druhy riadok						
			}
		}				
		likeli = cvNorm( matRef, matRes, CV_L2 );								

		likeli *= -1;				

		//nastavi hodnotu matice p->weights na suradniciach [0,i] na hodnotu likeli
        cvmSet( p->weights, 0, i, likeli );

		cvZero(matRes);
		
        
        cvReleaseImage( &patch );
    }
    cvReleaseImage( &resize );

	for(i = numParticlesDyn; i < p->num_particles; i++)
		cvmSet( p->weights, 0, i, -99999.0 );
}


#endif
