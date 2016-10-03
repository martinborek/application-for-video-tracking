/**
 * @file ffmpegplayer.h
 * @author Martin Borek (mborekcz@gmail.com)
 * @date May, 2015
 */

#ifndef FFMPEGPLAYER_H
#define FFMPEGPLAYER_H

#include <string> // For video path (videoAddr)
#include "videoframe.h"

#include <QApplication>
#include <QProgressDialog>

//TODO: Are they needed in the header file or can they be moved to the source?
#include <cv.h>
#include <cvaux.h>
#include <cxcore.h>
#include <highgui.h>

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

struct OpenException : public std::exception{};
struct UserCanceledOpeningException : public std::exception{};

class FFmpegPlayer
{
public:
    /**
     * Constructor
     * @param videoAddr Path to a video file
     * @param progressDialog QT progress dialog for displaying information about video opening
     */
    FFmpegPlayer(std::string videoAddr, QProgressDialog const *progressDialog);

    /**
     * Destructor
     */
    ~FFmpegPlayer();

    /**
     * Returns information about frames per second.
     * @return Frames per second
     */
    double get_fps() const;

    /**
     * Returns information about number of frames.
     * @return Number of frames
     */
    unsigned long get_frame_count() const;

    /**
     * Returns information about video length.
     * @return Video length in milliseconds
     */
    unsigned long get_total_time() const;

    /**
     * Returns information about video height.
     * @return Video height
     */
    unsigned int get_height() const;

    /**
     * Returns information about video width.
     * @return Video width
     */
    unsigned int get_width() const;

    //int64_t get_frame_timestamp() const;
    //int get_time_position() const; // In milliseconds

    /**
     * Returns information about the input video stream.
     * @return Input video stream
     */
    AVStream const *get_video_stream();

    /**
     * Returns information about the input audio stream.
     * @return Input audio stream
     */
    AVStream const *get_audio_stream();

    /**
     * Reads a frame by its time position and returns it.
     * @param resultFrame Read frame
     * @param time Time position
     * @return True if successful
     */
    bool get_frame_by_time(VideoFrame *resultFrame, unsigned long time);

    /**
     * Reads a frame by its timestamp and returns it.
     * @param resultFrame Read frame
     * @param timestamp Timestamp
     * @return True if successful
     */
    bool get_frame_by_timestamp(VideoFrame *resultFrame, int64_t timestamp);

    /**
     * Reads a frame by its number (index) and returns it.
     * @param resultFrame Read frame
     * @param frameNumber Frame number (index)
     * @return True if successful
     */
    bool get_frame_by_number(VideoFrame *resultFrame, unsigned long frameNumber);

    /**
     * Reads a frame following to the current one returns it.
     * @param resultFrame Read frame
     * @return True if successful
     */
    bool get_next_frame(VideoFrame *resultFrame);

    /**
     * Reads a frame preceding to the current one returns it.
     * @param resultFrame Read frame
     * @return True if successful
     */
    bool get_previous_frame(VideoFrame *resultFrame);

    // reads new frame to this.newFrame
    /**
     * Reads a new frame from the input video stream. The new frame is stored in resultFrame.
     * @param packet Packet structure for storing the read packet; It is also used for returning audio packets
     * @param onlyVideoPackets If false - audio packets are read as well as video packets. If true - only video packets are read.
     * @param isAudio Is set to true if the function returned an audio packet in AVPacket &packet
     * @return True if successful
     */
    bool read_frame(AVPacket &packet, bool onlyVideoPackets=true, bool *isAudio=nullptr);

    /**
     * Returns the current frame that has already been read.
     * @param frame Read frame
     * @return True if successful
     */
    bool get_current_frame(VideoFrame *frame);

    /**
     * Rewinds the video to its beginning by seeking its first packet.
     * @return True if successful
     */
    bool seek_first_packet();

    /**
     * Returns the input file format name.
     * @return File format name
     */
    const char *get_format_name();

private:
    /**
     * Converts a given frame timestamp to a time position.
     * @param timestamp Timestamp
     * @return Time position
     */
    unsigned long get_time_position_by_timestamp(int64_t timestamp) const; // In miliseconds

    /**
     * Analyzes the opened video so that it can be seeked
     * @param progressDialog QT progress dialog for showing an information about analyzation progress.
     */
    void analyze_video(QProgressDialog const *progressDialog);

private:
    const int scalingMethod;
    int videoStreamID;
    int audioStreamID; // -1 => no audio stream found

    AVRational timeBase;

    AVPacket pkt;
    AVFormatContext *formatContext;
    AVFrame *newFrame;
    AVCodecContext *videoContext;
    AVCodecContext *videoContextOrig;
    int64_t firstTimestamp;
    int64_t firstTimestampSet;

    int64_t firstPts;
    int64_t firstPtsSet;
    int64_t lastTimestamp; // pkt_pts of last successfully read frame

    std::vector<int64_t> framesIndexVector;
    std::set<int64_t> framesTimestampSet;
};

#endif // FFMPEGPLAYER_H
