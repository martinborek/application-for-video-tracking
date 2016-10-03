/**
 * @file avwriter.h
 * @author Martin Borek (mborekcz@gmail.com)
 * @date May, 2015
 */

#ifndef AVWRITER_H
#define AVWRITER_H

extern "C"{
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>

#include <libavutil/common.h>
#include <libavutil/imgutils.h>
#include <libavutil/mathematics.h>
#include <libavutil/samplefmt.h>


#include <libavutil/avassert.h>
#include <libavutil/channel_layout.h>
#include <libavutil/opt.h>
#include <libavutil/timestamp.h>
#include <libswresample/swresample.h>
}

#include "videoframe.h"

class AVWriter
{
public:
    AVWriter();
    ~AVWriter();

    /**
     * Initializes contexts, opens codecs and opens the output file for writing.
     * @param filename Output filename
     * @param inVideoStream Input video stream
     * @param inAudioStream Input audio stream
     * @param inFormatName Name of the input file format
     * @param inFileExtension Extension of the input file
     * @return True if initialization successful
     */
    bool initialize_output(std::string filename, AVStream const *inVideoStream, AVStream const *inAudioStream,
                           char const *inFormatName, std::string inFileExtension);

    /**
     * Correct closing of the output file.
     * @return  True if successful
     */
    bool close_output();

    /**
     * Writes the video frame to the output video stream.
     * @param frame Frame to be written
     * @return True if successful
     */
    bool write_video_frame(VideoFrame &frame);

    /**
     * Writes the audio packet to the output audio stream.
     * @param pkt Packet to be written
     * @return True if successful
     */
    bool write_audio_packet(AVPacket &pkt);

    /**
     * Writes all frames remaining in the encoder; emptying buffers.
     * @return True if successful
     */
    bool write_last_frames();

private:
    /**
     * Adds a new stream (either audio or video) for creating an output media file.
     * @param outputStream Output stream that is initialized by this function
     * @param inputStream Input stream; it's values are used for outputStream initialization
     * @return True if successful
     */
    bool add_stream(AVStream **outputStream, AVStream const *inputStream);

private:
    AVFormatContext *formatContext;
    AVStream *videoStream;
    AVStream *audioStream;

    const int outputFormat = AV_PIX_FMT_YUV420P;
};

#endif // AVWRITER_H
