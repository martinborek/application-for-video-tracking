/**
 * @file videoframe.h
 * @author Martin Borek (mborekcz@gmail.com)
 * @date May, 2015
 */

#ifndef VIDEOFRAME_H
#define VIDEOFRAME_H

#include <cv.h>
#include <cvaux.h>
#include <cxcore.h>
#include <highgui.h>

extern "C"{
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
}

#define VIDEOTRACKING_DEFAULT_SCALING_METHOD SWS_BILINEAR
#define VIDEOTRACKING_OUTPUT_FORMAT AV_PIX_FMT_YUV420P

class VideoFrame
{
public:
    /**
     * Constructor
     */
    VideoFrame();
    /**
     * Copy constructor
     * @param obj Original
     */
    VideoFrame(const VideoFrame &obj);

    /**
     * Constructor
     * @param width Frame width
     * @param height Frame height
     */
    VideoFrame(unsigned int width, unsigned int height);

    /**
     * Destructor
     */
    ~VideoFrame();

    /**
     * Sets a new frame
     * @param newFrame New frame data
     * @return True if successful
     */
    bool set_frame(AVFrame const *newFrame);

    unsigned long get_time_position() const;
    /**
     * Returns frame timestamp.
     * @return Frame timestamp
     */
    int64_t get_timestamp() const;

    /**
     * Returns frame number (index).
     * @return Frame number
     */
    unsigned long get_frame_number() const;

    /**
     * Returns width of the frame.
     * @return Frame width
     */
    unsigned int get_width() const;

    /**
     * Returns height of the frame.
     * @return Frame height
     */
    unsigned int get_height() const;

    /**
     * Sets frame size.
     * @param newWidth New frame width
     * @param newHeight New frame height
     */
    void set_size(unsigned int newWidth, unsigned int newHeight);

    /**
     * Sets timestamp.
     * @param newTimestamp New timestamp
     */
    void set_timestamp(int64_t newTimestamp);

    /**
     * Sets time position.
     * @param newTimePosition New time Position
     */
    void set_time_position(unsigned long newTimePosition);

    /**
     * Sets frame number.
     * @param newFrameNumber New frame number
     */
    void set_frame_number(unsigned long newFrameNumber);

    /**
     * Returns the frame in AVFrame.
     * @return Frame
     */
    AVFrame const *get_av_frame();

    /**
     * Returns the frame in cv::Mat.
     * @return Frame
     */
    cv::Mat const *get_mat_frame() const;

    /**
     * Converts AVFrame to cv::Mat.
     * @param src Source AVFrame
     * @param dstMat Destination cv::Mat; It mus be allocated before using this function.
     * @return True if successful
     */
    bool AVFrame2Mat(AVFrame const *src, cv::Mat *dstMat) const;

    /**
     * Converts cv::Mat to AVFrame.
     * @param src Source cv::at
     * @param dstAVFrame Destination AVFrame
     * @param dstFormat Destination PixelFormat; it must be allocated before, but must not have allocated buffer (is allocated inside of this function). The buffer needs to be later freed manually.
     * @return True if successful
     */
    bool Mat2AVFrame(cv::Mat const &src, AVFrame *dstAVFrame, const int dstFormat) const;

public:
    cv::Mat *matFrame; // Be careful using it directly! If possible, use get_mat_frame() instead
private:
    AVFrame *avFrame;

    const int outputFormat = VIDEOTRACKING_OUTPUT_FORMAT;
    const int scalingMethod;
    int64_t timestamp;
    unsigned long timePosition;
    unsigned long frameNumber;
    unsigned int width;
    unsigned int height;

};

#endif // VIDEOFRAME_H
