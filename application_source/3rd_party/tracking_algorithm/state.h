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

@brief     Pomocne funkcie k praci s casticami
@details   Pomocne funkcie k praci s casticami. Subor je zalozeny na volne dostupnej implementacii (licencia nizsie).
@authors   Betko Peter (<pbetko@gmail.com>)
@date      11.10.2011
@note      This project was supported by MPO CR TIP FR-TI1/195.
@copyright (c) 2011 Betko Peter
@par       License

           The MIT License
  
           Copyright (c) 2008, Naotoshi Seo <sonots(at)sonots.com>
            
           Permission is hereby granted, free of charge, to any person obtaining a copy
           of this software and associated documentation files (the "Software"), to deal
           in the Software without restriction, including without limitation the rights
           to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
           copies of the Software, and to permit persons to whom the Software is
           furnished to do so, subject to the following conditions:
            
           The above copyright notice and this permission notice shall be included in
           all copies or substantial portions of the Software.
 
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

subor state.h - pomocne funkcie k praci s casticami
Subor je zalozeny na volne dostupnej implementacii (licencia nizsie).
*/

/*
 *
 * The MIT License
 * 
 * Copyright (c) 2008, Naotoshi Seo <sonots(at)sonots.com>
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 */
#ifndef CV_PARTICLE_ROTRECT_H
#define CV_PARTICLE_ROTRECT_H

#include "opencvx/cvparticle.h"
#include "opencvx/cvdrawrectangle.h"
#include "opencvx/cvcropimageroi.h"
#include "opencvx/cvrect32f.h"
#include <float.h>
//using namespace std;

/********************** Definition of a particle *****************************/

int num_states = 5;

/*! Definition of meanings of 5 states.
    This kinds of structures is not necessary to be defined, 
    but I recommend to define them because it makes clear meanings of states
*/
typedef struct CvParticleState {
    double x;        //!< Center coord of a rectangle
    double y;        //!< Center coord of a rectangle
    double width;    //!< Width of a rectangle
    double height;   //!< Height of a rectangle
    double angle;    //!< Rotation around center. degree
} CvParticleState;

/*! Definition of dynamics model:
    
    @code
       new_particle = cvMatMul( dynamics, particle ) + noise 
    @endcode
    
    @code
       curr_x =: curr_x + noise
    @endcode
*/
double dynamics[] = {
    1, 0, 0, 0, 0, 
    0, 1, 0, 0, 0, 
    0, 0, 1, 0, 0, 
    0, 0, 0, 1, 0, 
    0, 0, 0, 0, 1, 
};

/********************** Function Prototypes *********************************/

// Functions for CvParticleState structure ( constructor, getter, setter )
inline CvParticleState cvParticleState( double x, 
                                        double y, 
                                        double width, 
                                        double height, 
                                        double angle = 0 );
CvParticleState cvParticleStateFromMat( const CvMat* state );
void cvParticleStateToMat( const CvParticleState &state, CvMat* state_mat );
CvParticleState cvParticleStateGet( const CvParticle* p, int p_id );
void cvParticleStateSet( CvParticle* p, int p_id, const CvParticleState &state );

// Particle Filter configuration
void cvParticleStateConfig( CvParticle* p, CvSize imsize, CvParticleState& std );
void cvParticleStateAdditionalBound( CvParticle* p, CvSize imsize );

// Utility Functions
void cvParticleStateDisplay( const CvParticleState& state, IplImage* frame, CvScalar color );
void cvParticleStatePrint( const CvParticleState& state );

/****************** Functions for CvParticleState structure ******************/

// This kinds of state definitions are not necessary, 
// but helps readability of codes for sure.

/*!
 * Constructor
 */
inline CvParticleState cvParticleState( double x, 
                                        double y, 
                                        double width, 
                                        double height, 
                                        double angle )
{
    CvParticleState state = { x, y, width, height, angle }; 
    return state;
}

/*!
 * Convert a matrix state representation to a state structure
 *
 * @param state     num_states x 1 matrix
 */
CvParticleState cvParticleStateFromMat( const CvMat* state )
{
    CvParticleState s;
    s.x       = cvmGet( state, 0, 0 );
    s.y       = cvmGet( state, 1, 0 );
    s.width   = cvmGet( state, 2, 0 );
    s.height  = cvmGet( state, 3, 0 );
    s.angle   = cvmGet( state, 4, 0 );
    return s;
}

/*!
 * Convert a state structure to CvMat
 *
 * @param state        A CvParticleState structure
 * @param state_mat    num_states x 1 matrix
 * @return void
 */
void cvParticleStateToMat( const CvParticleState& state, CvMat* state_mat )
{
    cvmSet( state_mat, 0, 0, state.x );
    cvmSet( state_mat, 1, 0, state.y );
    cvmSet( state_mat, 2, 0, state.width );
    cvmSet( state_mat, 3, 0, state.height );
    cvmSet( state_mat, 4, 0, state.angle );
}

/*!
 * Get a state from a particle filter structure
 *
 * @param p         particle filter struct
 * @param p_id      particle id
 */
CvParticleState cvParticleStateGet( const CvParticle* p, int p_id )
{
    CvMat* state, hdr;
    state = cvGetCol( p->particles, &hdr, p_id );
    return cvParticleStateFromMat( state );
}

/*!
 * Set a state to a particle filter structure
 *
 * @param state     A CvParticleState structure
 * @param p         particle filter struct
 * @param p_id      particle id
 * @return void
 */
void cvParticleStateSet( CvParticle* p, int p_id, const CvParticleState& state )
{
    CvMat* state_mat, hdr;
    state_mat = cvGetCol( p->particles, &hdr, p_id );
    cvParticleStateToMat( state, state_mat );
}

/*************************** Particle Filter Configuration *********************************/

/*!
 * Configuration of Particle filter
 */
void cvParticleStateConfig( CvParticle* p, CvSize imsize, CvParticleState& std )
{
    // config dynamics model
    CvMat dynamicsmat = cvMat( p->num_states, p->num_states, CV_64FC1, dynamics );

    // config random noise standard deviation
    CvRNG rng = cvRNG( time( NULL ) );
    double stdarr[] = {
        std.x,
        std.y,
        std.width,
        std.height,
        std.angle
    };
    CvMat stdmat = cvMat( p->num_states, 1, CV_64FC1, stdarr );

    // config minimum and maximum values of states
    // lowerbound, upperbound, circular flag (useful for degree)
    // lowerbound == upperbound to express no bounding
    double boundarr[] = {
        0, imsize.width - 1, false,
        0, imsize.height - 1, false,
        1, imsize.width, false,
        1, imsize.height, false,
        0, 360, true
    };
    CvMat boundmat = cvMat( p->num_states, 3, CV_64FC1, boundarr );
    cvParticleSetDynamics( p, &dynamicsmat );
    cvParticleSetNoise( p, rng, &stdmat );
    cvParticleSetBound( p, &boundmat );
}

/*!
 * @todo
 * CvParticle does not support this type of bounding currently
 * Call after transition
 */
void cvParticleStateAdditionalBound( CvParticle* p, CvSize imsize )
{
    for( int np = 0; np < p->num_particles; np++ ) 
    {
        double x      = cvmGet( p->particles, 0, np );
        double y      = cvmGet( p->particles, 1, np );
        double width  = cvmGet( p->particles, 2, np ); 
        double height = cvmGet( p->particles, 3, np );
        width = MIN( width, imsize.width - (x) ); // another state x is used
        height = MIN( height, imsize.height - (y) ); // another state y is used
        cvmSet( p->particles, 2, np, width );
        cvmSet( p->particles, 3, np, height );
    }
}

void cvParticleRestoreSize(CvParticle *p, CvSize size)
{
	int i = 0;	

	if((cvmGet(p->std, 0, 2) == 0.0) && (cvmGet(p->std, 0, 3) == 0.0))
	{
		printf("aaa");
		for(i = 0; i < p->num_particles; i++)
		{
			cvmSet(p->particles, 2, i, size.width);
			cvmSet(p->particles, 3, i, size.height);
			
		}

	}
}

/***************************** Utility Functions ****************************************/

void cvParticleStateDisplay( const CvParticleState& state, IplImage* img, CvScalar color, int thickness )
{
    CvBox32f box32f = cvBox32f( state.x, state.y, state.width, state.height, state.angle );
    CvRect32f rect32f = cvRect32fFromBox32f( box32f );
    cvDrawRectangle( img, rect32f, cvPoint2D32f(0,0), color, thickness,8,0);
	//cvDrawRectangle( img, rect32f, cvPoint2D32f(0,0), color);
	
}

void cvParticleStatePrint( const CvParticleState& state )
{
    printf( "x :%.2f ", state.x );
    printf( "y :%.2f ", state.y );
    printf( "width :%.2f ", state.width );
    printf( "height :%.2f ", state.height );
    printf( "angle :%.2f\n", state.angle );
    fflush( stdout );
}

#endif
