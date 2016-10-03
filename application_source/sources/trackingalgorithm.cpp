/**
 * @file trackingalgorithm.cpp
 * @author Martin Borek (mborekcz@gmail.com)
 * @date May, 2015
 */

#include "trackingalgorithm.h"

#include "opencvx/cvxmat.h"
#include "opencvx/cvxrectangle.h"
#include "opencvx/cvrect32f.h"
#include "opencvx/cvcropimageroi.h"
#include "opencvx/cvdrawrectangle.h"
#include "opencvx/cvparticle.h"

#include "tracking_algorithm/observetemplate.h"
#include "tracking_algorithm/state.h"
#include <iostream>

#define DEFAULT 0

#define DRAW_PARTICLES_REALLOC 512

TrackingAlgorithm::TrackingAlgorithm(cv::Mat const &initialFrame, Selection const &initialPosition, Selection &centerizedPosition)
{
    IplImage *frame = new IplImage(initialFrame);

    resize = cvSize(24, 24);// size
    pDyn = 100;             // dynamic numer of particles
    int p = 300;            // number of particles
    int sx = 5;             // x
    int sy = 5;             // y
    int sw = 0;             // width
    int sh = 0;             // height
    int sr = 0;             // rotation

    CvPoint *particleCenter = NULL;
    if((particleCenter = (CvPoint*)(malloc(DRAW_PARTICLES_REALLOC * sizeof(CvPoint)))) == NULL)
        exit(1);

    CvRect region;
    region.x = initialPosition.x;
    region.y = initialPosition.y;
    region.height = initialPosition.height;
    region.width = initialPosition.width;

    bool logprob = true;
    particle = cvCreateParticle( num_states, p, logprob );

    CvParticleState std = cvParticleState (
                sx,
                sy,
                sw,
                sh,
                sr
                );

    cvParticleStateConfig( static_cast<CvParticle *>(particle), cvGetSize(frame), std );

    CvParticleState s;
    CvParticle *init_particle;

    init_particle = cvCreateParticle( num_states, 1 );
    CvRect32f region32f = cvRect32fFromRect( region );
    CvBox32f box = cvBox32fFromRect32f( region32f ); // Centerize
    s = cvParticleState( box.cx, box.cy, box.width, box.height, 0.0 );
    cvParticleStateSet( init_particle, 0, s );
    cvParticleInit( static_cast<CvParticle *>(particle), init_particle );
    cvReleaseParticle( &init_particle );

    // Resize reference image
    reference = cvCreateImage( resize, frame->depth, frame->nChannels );
    IplImage* tmp = cvCreateImage( cvSize(region.width,region.height), frame->depth, frame->nChannels );
    cvCropImageROI(frame, tmp, region32f );
    cvResize( tmp, reference );
    cvReleaseImage( &tmp );

    centerizedPosition.width = box.width;
    centerizedPosition.height = box.height;
    centerizedPosition.x = box.cx;
    centerizedPosition.y = box.cy;
    centerizedPosition.angle = initialPosition.angle;


}

TrackingAlgorithm::~TrackingAlgorithm()
{

}

Selection TrackingAlgorithm::track_next_frame(cv::Mat const  &nextImage)
{
    IplImage *frame = new IplImage(nextImage);

    cvParticleTransition( static_cast<CvParticle *>(particle) );

    particleEvalDefault( static_cast<CvParticle *>(particle), frame, reference, resize, pDyn);

    int maxp_id = cvParticleGetMax( static_cast<CvParticle *>(particle) );
    CvParticleState maxs = cvParticleStateGet( static_cast<CvParticle *>(particle), maxp_id );

    Selection objectPosition(maxs.x, maxs.y, maxs.width, maxs.height, maxs.angle);

    cvParticleNormalize( static_cast<CvParticle *>(particle));

    cvParticleResample( static_cast<CvParticle *>(particle) );

    return objectPosition;
}
