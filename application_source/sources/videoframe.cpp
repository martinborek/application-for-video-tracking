/**
 * @file videoframe.cpp
 * @author Martin Borek (mborekcz@gmail.com)
 * @date May, 2015
 */

#include "videoframe.h"

#include <QDebug>

VideoFrame::VideoFrame():
    scalingMethod(VIDEOTRACKING_DEFAULT_SCALING_METHOD)
{
    matFrame = nullptr;
    avFrame = nullptr;
    width = 0;
    height = 0;
}

VideoFrame::VideoFrame(unsigned int width, unsigned int height):
    scalingMethod(VIDEOTRACKING_DEFAULT_SCALING_METHOD),
    width(width),
    height(height)
{
    matFrame = nullptr;
    avFrame = nullptr;
}

//VideoFrame::VideoFrame(cv::Mat const &newFrame, int64_t timestamp, unsigned long timePosition):
// Does not copy avFrame because it is not needed and would make it slower
VideoFrame::VideoFrame(VideoFrame const &obj):
    scalingMethod(VIDEOTRACKING_DEFAULT_SCALING_METHOD)
{
    avFrame = nullptr;

    if (obj.matFrame)
        matFrame = new cv::Mat(*(obj.matFrame));
    else
        matFrame = nullptr;

    height = obj.height;
    width = obj.width;
    timestamp = obj.timestamp;
    timePosition = obj.timePosition;
}

VideoFrame::~VideoFrame()
{
    if (matFrame)
    {
        delete matFrame;
        matFrame = nullptr;
    }
    if (avFrame)
    {
        av_free(avFrame->data[0]);
        av_frame_free(&avFrame);
        avFrame = nullptr;
    }
}

bool VideoFrame::set_frame(AVFrame const *newFrame)
{
    if (!matFrame) // matFrame is nullptr and was not allocated yet
    {
        if (!width || !height) // equals one of them zero?
            return false;
        else
            matFrame = new cv::Mat(height, width, CV_8UC3); // CV_8UC3->3 channels of unsigned 8-bit int
    }

    return AVFrame2Mat(newFrame, matFrame);
}



unsigned long VideoFrame::get_time_position() const
{
    return timePosition;
}

int64_t VideoFrame::get_timestamp() const
{
    return timestamp;
}

unsigned long VideoFrame::get_frame_number() const
{
    return frameNumber;
}

unsigned int VideoFrame::get_width() const
{
    return width;
}

unsigned int VideoFrame::get_height() const
{
    return height;
}

void VideoFrame::set_size(unsigned int newWidth, unsigned int newHeight)
{
    width = newWidth;
    height = newHeight;

    // size changed -> allocated memory incorrect; free memory; it will be allocated when needed
    if (matFrame)
    {
        delete matFrame;
        matFrame = nullptr;
    }
}

void VideoFrame::set_timestamp(int64_t newTimestamp)
{
    timestamp = newTimestamp;
}

void VideoFrame::set_time_position(unsigned long newTimePosition)
{
    timePosition = newTimePosition;
}

void VideoFrame::set_frame_number(unsigned long newFrameNumber)
{
    frameNumber = newFrameNumber;
}

AVFrame const *VideoFrame::get_av_frame()
//AVFrame *VideoFrame::get_av_frame()
{
    if (matFrame == nullptr) // There is not a valid frame that could be converted to AVFrame
        return nullptr;

    if (!avFrame)
    {
        avFrame = av_frame_alloc();
        if (avFrame == nullptr)
        {
            qDebug() << "Not allocated";
            exit(1);
        }
        int numBytes = avpicture_get_size((enum PixelFormat)outputFormat, width, height);
        if (numBytes < 0)
        {
            qDebug() << "Error: avpicture_get_size()";
            exit(1);
        }

        uint8_t *buffer = (uint8_t *)av_malloc(numBytes * sizeof(uint8_t));
        if (buffer == nullptr)
        {
            qDebug() << "Not allocated";
            exit(1);
        }

        if (avpicture_fill((AVPicture *)avFrame, buffer, (enum PixelFormat)outputFormat, width, height) < 0)
        {
            qDebug() << "Error: avpicture_fill()";
            exit(1);
        }
    }

    Mat2AVFrame(*matFrame, avFrame, outputFormat);
    avFrame->pts = timestamp;
    return avFrame;
}

cv::Mat const *VideoFrame::get_mat_frame() const
{
    return matFrame;
}

// dstMat must be allocated before
bool VideoFrame::AVFrame2Mat(AVFrame const *src, cv::Mat *dstMat) const
{
    assert(src != nullptr);
    //assert(dstMat != nullptr);

    AVFrame *dst = nullptr;
    dst = av_frame_alloc(); // Calls also memset, xal

    dst->data[0] = (uint8_t *)dstMat->data; //dstMat->data is used as a buffer

    // note: avpicture_fill does not perform a deep copy
    if (avpicture_fill((AVPicture *)dst, dst->data[0], AV_PIX_FMT_BGR24, src->width, src->height) < 0)
    {
        qDebug() << "Error - AVFrame2Mat: avpicture_fill";
        av_frame_free(&dst);
        dst = nullptr;
        return false;
    }

    SwsContext *conversionContext = nullptr; // Context is needed for sws_scale
    conversionContext = sws_getContext(src->width, src->height, (enum PixelFormat)src->format,
                                 src->width, src->height, AV_PIX_FMT_BGR24,
                                 scalingMethod, NULL, NULL, NULL); // xal


    sws_scale(conversionContext, src->data, src->linesize, 0, src->height,
                    dst->data, dst->linesize);

    // free allocated memories
    sws_freeContext(conversionContext);
    conversionContext = nullptr;
    av_frame_free(&dst);
    dst = nullptr;

    return true;
}

bool VideoFrame::Mat2AVFrame(cv::Mat const &src, AVFrame *dstAVFrame, const int dstFormat) const
{
    //assert(src != nullptr);
    assert(dstAVFrame != nullptr);

    static const int width = src.cols;
    static const int height = src.rows;

    AVFrame *srcAV = av_frame_alloc(); // Calls also memset, xal
    if (srcAV == nullptr)
    {
        qDebug() << "Not allocated";
        return false;
    }

    avpicture_fill((AVPicture *)srcAV, (uint8_t *)src.data, AV_PIX_FMT_BGR24, width, height);

    // all frames have same width, height, format ...
    SwsContext *conversionContext = sws_getContext(width, height, PIX_FMT_BGR24,
                                 width, height, (enum PixelFormat)dstFormat,
                                 scalingMethod, NULL, NULL, NULL); // xal


    sws_scale(conversionContext, srcAV->data, srcAV->linesize, 0, height,
                    dstAVFrame->data, dstAVFrame->linesize);

    // free allocated memories
    sws_freeContext(conversionContext);
    conversionContext = nullptr;
    av_frame_free(&srcAV);
    srcAV = nullptr;

    return true;
}
