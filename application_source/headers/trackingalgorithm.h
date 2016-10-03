/**
 * @file trackingalgorithm.h
 * @author Martin Borek (mborekcz@gmail.com)
 * @date May, 2015
 */

#ifndef TRACKINGALGORITHM_H
#define TRACKINGALGORITHM_H

#include "selection.h"

#include <cv.h>
#include <cvaux.h>
#include <cxcore.h>
#include <highgui.h>

class TrackingAlgorithm
{

public:
    /**
     * Constructor
     * @param initialFrame Data of the first frame
     * @param initialPosition Position of the object in the first frame
     * @param centerizedPosition Returned position of the object
     */
    TrackingAlgorithm(cv::Mat const &initialFrame, Selection const &initialPosition, Selection &centerizedPosition);

    /**
     * Destructor
     */
    ~TrackingAlgorithm();

    /**
     * Tracks the next provided frame.
     * @param nextImage The next image for tracking
     * @return Position of the object
     */
    Selection track_next_frame(cv::Mat const &nextImage);

private:
    void *particle;
    IplImage* reference;
    CvSize resize;
    int pDyn;
};

#endif // TRACKINGALGORITHM_H
