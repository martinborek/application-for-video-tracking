/**
 * @file ffmpegplayer.cpp
 * @author Martin Borek (mborekcz@gmail.com)
 * @date May, 2015
 */

#include "ffmpegplayer.h"

#include <QDebug>
#include <cassert>

#define VIDEOTRACKING_MS2DURATION 1000
#define VIDEOTRACKING_TIME_BASE_Q AVRational{1, AV_TIME_BASE} // original AV_TIME_BASE_Q gives syntax error
#define VIDEOTRACKING_MS_TIME_BASE_Q AVRational{1, 1000}

FFmpegPlayer::FFmpegPlayer(std::string videoAddr, QProgressDialog const *progressDialog) :
    scalingMethod(SWS_BILINEAR)
{
    //av_register_all();

    videoStreamID = -1; // -1 -> no video stream
    audioStreamID = -1; // -1 -> no audio stream
    formatContext = nullptr;
    newFrame = nullptr;
    videoContext = nullptr;
    videoContextOrig = nullptr;
    firstTimestamp = 0;
    firstTimestampSet = false;
    firstPts = 0;
    firstPtsSet = false;
    //firstPtsStream = 0;

    // Open video file
    if (avformat_open_input(&formatContext, videoAddr.c_str(), NULL, NULL) != 0)
    {
        qDebug() << "ffmpeg: Couldn't open the file.";
        throw OpenException();
    }

    // Retrieve stream information
    if (avformat_find_stream_info(formatContext, NULL)<0)
    {
        qDebug() << "ffmpeg: No stream information found.";
        throw OpenException();
    }
    av_dump_format(formatContext, 0, videoAddr.c_str(), 0);

    unsigned int i;

    // Find first video and first audio stream
    for (i = 0; i < formatContext->nb_streams; i++)
    {
        if (formatContext->streams[i]->codec->codec_type == AVMEDIA_TYPE_VIDEO && videoStreamID < 0)
            videoStreamID = i;

        if (formatContext->streams[i]->codec->codec_type == AVMEDIA_TYPE_AUDIO && audioStreamID < 0)
            audioStreamID = i;

        if (videoStreamID >= 0 && audioStreamID >= 0)
            break;
    }

    if (videoStreamID == -1)
    {
        qDebug() << "ffmpeg: Hasn't found any video stream.";
        throw OpenException();
    }

    // Get a pointer to the codec context for the video stream
    videoContextOrig = formatContext->streams[videoStreamID]->codec;
    //videoContext=formatContext->streams[videoStreamID]->codec;


    AVCodec *videoCodec = nullptr;

    // Find a decoder for the video stream
    videoCodec = avcodec_find_decoder(videoContextOrig->codec_id);
    if (videoCodec == nullptr)
    {
        qDebug() << "ffmpeg: Codec not supported.";
        throw OpenException();
    }

    // Copy context
    // ! Original AVCodecContext cannot be used directly, therefore avcodec_copy_context
    // ...must be copied first
    videoContext = avcodec_alloc_context3(videoCodec); // allocate memory
    if (videoContext == nullptr)
    {
        qDebug() << "ffmpeg error: Allocation.";
        throw OpenException();
    }

    if (avcodec_copy_context(videoContext, videoContextOrig) != 0)
    {
        qDebug() << "ffmpeg: Couldn't copy codec context.";
        throw OpenException();
    }

    // Open video codec
    if (avcodec_open2(videoContext, videoCodec, NULL) < 0)
    {
        qDebug() << "ffmpeg: Could not open codec.";
        throw OpenException();
    }

    // Allocate memory for a video frame that is used for reading new frames
    newFrame = av_frame_alloc();
    if (newFrame == nullptr)
    {
        qDebug() << "ffmpeg: Cannot allocate memory for newFrame";
        throw OpenException();
    }

    // get timeBase of the video stream
    timeBase = formatContext->streams[videoStreamID]->time_base;
    qDebug() << "frame count: " << get_frame_count();
    qDebug() << "keyframe every: " << videoContext->gop_size << "x frame";

   //analyze_video(qApplication);
   analyze_video(progressDialog);
}

FFmpegPlayer::~FFmpegPlayer()
{

    av_free(newFrame);

    // Close the codecs
 //   avcodec_close(enContext);
    avcodec_close(videoContext);
    avcodec_close(videoContextOrig);

    // Close the video file
    avformat_close_input(&formatContext);
    qDebug() << "ffmpeg deleted";

}

void FFmpegPlayer::analyze_video(QProgressDialog const *progressDialog)
{
    AVPacket packet;
    while (true)
    {
        //if (qApplication)
        //    qApplication->processEvents(); // Keeps progress bar active

        if (progressDialog->wasCanceled()) // User clicked "Cancel"
        {
            throw UserCanceledOpeningException();
        }
        qApp->processEvents(); // Keeps progress bar active

        if (av_read_frame(formatContext, &packet) < 0)
        {
            av_free_packet(&packet);

            break;
        }

        // Is this a packet from the video stream?
        if (packet.stream_index == videoStreamID)
        {

            if (!firstPtsSet)
            {
                //firstPts = packet.dts;
                firstPts = packet.pts;
                firstPtsSet = true;
                //firstPtsStream = packet.stream_index;

                if (!seek_first_packet())
                {
                    qDebug() << "analyze_video(): Cannot seek the first packet 1";
                    throw OpenException();
                }
                continue;
            }
            else
            {
                //static unsigned long frameID = 1;
                //framesIdMap[frameID] = packet.pts;
                //framesTimestampSet[packet.pts] = frameID++;
                framesTimestampSet.insert(packet.pts);
            }
        }
        // Free the packet that was allocated by av_read_frame
        av_free_packet(&packet);
    }

    for (const auto &timestamp: framesTimestampSet)
    {
       framesIndexVector.push_back(timestamp);
    }

    qDebug() << "number of frames:" << framesTimestampSet.size();
    qDebug() << "beginning:" << *(framesTimestampSet.begin());

    if (!seek_first_packet())
    {
        qDebug() << "analyze_video(): Cannot seek the first packet 2";
        throw OpenException();
    }
}


double FFmpegPlayer::get_fps() const
{
//    return formatContext->bit_rate;
//    return formatContext->streams[videoStreamID]->nb_frames * AV_TIME_BASE / static_cast<double>(formatContext->duration);
    return formatContext->streams[videoStreamID]->nb_frames * VIDEOTRACKING_MS2DURATION / static_cast<double>(get_total_time());
}

unsigned long FFmpegPlayer::get_frame_count() const
{
    return framesTimestampSet.size();

    //return formatContext->streams[videoStreamID]->nb_frames;
}

// in ms
unsigned long FFmpegPlayer::get_total_time() const
{
    // Time of the last frame
    return get_time_position_by_timestamp(*(framesTimestampSet.rbegin()));

    //return formatContext->duration / VIDEOTRACKING_MS2DURATION;
}

unsigned int FFmpegPlayer::get_height() const
{
    return videoContext->height;
}

unsigned int FFmpegPlayer::get_width() const
{
    return videoContext->width;
}

const AVStream *FFmpegPlayer::get_video_stream()
{
    if (videoStreamID < 0) // No video stream detected
        return nullptr;
    return formatContext->streams[videoStreamID];
}

const AVStream *FFmpegPlayer::get_audio_stream()
{
    if (audioStreamID < 0) // No audio stream detected
        return nullptr;
    else
        return formatContext->streams[audioStreamID];
}

/**
int64_t FFmpegPlayer::get_frame_timestamp() const
{
    //return videoContext->frame_number;
    return lastTimestamp;
}
*/

unsigned long FFmpegPlayer::get_time_position_by_timestamp(int64_t timestamp) const
{
    return av_rescale_q(timestamp, timeBase, AVRational{1, VIDEOTRACKING_MS2DURATION});
}
/**
unsigned long FFmpegPlayer::get_time_position() const
{
    //videoContext->ticks_per_frame;
    return av_rescale_q(get_frame_timestamp(), timeBase, AVRational{1, VIDEOTRACKING_MS2DURATION});
}
*/

bool FFmpegPlayer::get_current_frame(VideoFrame *frame)
{
    if (frame == nullptr)
        return false;

    if (!frame->set_frame(newFrame))
        return false;

    frame->set_timestamp(lastTimestamp);

    //frame->set_time_position(get_time_position_by_timestamp(lastTimestamp + newFrame->pkt_duration));

    //frame->set_frame_number(framesTimestampSet.find(lastTimestamp)->second);
    frame->set_frame_number(std::distance(framesTimestampSet.begin(), framesTimestampSet.find(lastTimestamp))+1);

    frame->set_time_position(get_time_position_by_timestamp(lastTimestamp));
    //qDebug() << "duration: " << newFrame->pkt_duration;

    return true;
}

bool FFmpegPlayer::get_frame_by_time(VideoFrame *resultFrame, unsigned long time)
{
    int64_t approximateTimestamp = av_rescale_q(time, VIDEOTRACKING_MS_TIME_BASE_Q, timeBase);

    // Find first timestamp that is not lower than the approximateTimestamp
    auto iterator = framesTimestampSet.lower_bound(approximateTimestamp);

    if (iterator == framesTimestampSet.end()) // approximateTimestamp is higher than all timestmaps; use the highest timestamp
        iterator--;

    int64_t exactTimestamp = *iterator;

    return get_frame_by_timestamp(resultFrame, exactTimestamp);
}

/** exactPosition - if frame with the exact timestamp does not exist, return false;
 *  onlyKeyFrames - Seek only to keyframes - faster; */
bool FFmpegPlayer::get_frame_by_timestamp(VideoFrame *resultFrame, int64_t timestamp)
{
    auto iterator = framesTimestampSet.find(timestamp);

    int64_t seekTimestamp = timestamp;

    while (true)
    {   // Seek KEYFRAME at desired timestamp or timestamp lower.
        // av_seek_frame() is not accurate. It might return frame at higher timestamp than
        // wanted. Thus, it is necessary to seek lower (if this happens) until returned KEYFRAME is not
        // higher than desired frame.

        if (av_seek_frame(formatContext, videoStreamID, seekTimestamp, AVSEEK_FLAG_BACKWARD) < 0) // Find first previous frame; might not be a keyframe
        //if (av_seek_frame(formatContext, videoStreamID, timestampPosition, 0) < 0)
        {
            qDebug() << "ERROR-get_frame_by_timestamp: seeking frame 1";
            return false;
        }

        avcodec_flush_buffers(videoContext);

        if (!read_frame(pkt))
            return false;

        if (lastTimestamp <= timestamp)
            break; // This is what we were looking for

        if (iterator == framesTimestampSet.begin())
            return false; // This is the first frame but the desired timestamp is lower

        seekTimestamp = *(--iterator);
    }


    //qDebug() << "I want" << timestamp << "I've got" << lastTimestamp;


  //while (lastTimestamp != previousTimestamp)
    while (lastTimestamp < timestamp)
    {

        if (!read_frame(pkt))
            return false;
    }

    //qDebug() << "final" << lastTimestamp;

    if (!get_current_frame(resultFrame))
        return false;
    return true;
}

bool FFmpegPlayer::get_frame_by_number(VideoFrame *resultFrame, unsigned long frameNumber)
{
    int64_t timestamp = framesIndexVector[frameNumber-1];

    return get_frame_by_timestamp(resultFrame, timestamp);

}

bool FFmpegPlayer::get_next_frame(VideoFrame *resultFrame)
{
    if (!read_frame(pkt))
        return false;

    if (!get_current_frame(resultFrame))
    {
        qDebug() << "Cannot get current frame";
        return false;
    }

    return true;
}

bool FFmpegPlayer::get_previous_frame(VideoFrame *resultFrame)
{
    int64_t currentTimestamp = lastTimestamp;
    //qDebug() << "get_previous_frame: currentTimestamp" << currentTimestamp;

    if (currentTimestamp == firstTimestamp) // There is no previous frame
        return false;

    auto iterator = framesTimestampSet.find(currentTimestamp);

    int64_t previousTimestamp = *(--iterator);
    //qDebug() << "get_previous_frame: previousTimestamp" << previousTimestamp << iterator->second;

    get_frame_by_timestamp(resultFrame, previousTimestamp);
    return true;
}

// read_frame cannot be called two times at the same time (at least until AVPacket is used)
// AVFrame *frame needs to be allocated before (no need for buffer)
bool FFmpegPlayer::read_frame(AVPacket &packet, bool onlyVideoPackets, bool *isAudio)
{
    assert(onlyVideoPackets || isAudio != nullptr); // isAudio needs to be defined if audioPackets should be searched too

    int frameFinished;
    bool lastFrames = false;

    while (true)
    {
        if (av_read_frame(formatContext, &packet) < 0)
        {
            av_free_packet(&packet);

            if (videoContext->codec->capabilities & CODEC_CAP_DELAY)
            { // delayed frames at the end of the video
                lastFrames = true;
                qDebug() << "last packets";
                packet.data = NULL;
                packet.size = 0;
                packet.stream_index = videoStreamID;
            }
            else
                return false;
        }



        // Is this a packet from the video stream?
        if (packet.stream_index == videoStreamID)
        {
            //qDebug() << "readFrame pts" <<newFrame->pts;

            // Decode video frame
            if (avcodec_decode_video2(videoContext, newFrame, &frameFinished, &packet) < 0)
            {
                av_free_packet(&packet);
                return false;
            }


            // frameFinished==0 -> no frame could be decompressed
            if (frameFinished)
            {
                lastTimestamp = newFrame->pkt_pts;
                av_free_packet(&packet);

                if (!firstTimestampSet)
                {
                    firstTimestamp = lastTimestamp;
                    qDebug() << "First timestamp is" << firstTimestamp;
                    firstTimestampSet = true;
                }

                if (!onlyVideoPackets)
                    *isAudio = false;

                return true;
            }
            else if (lastFrames)
            {
                qDebug() << "last frames finished";
                return false;
            }
        }
        else if(!onlyVideoPackets && packet.stream_index == audioStreamID)
        { // Do not forget to free packet after using it
            *isAudio = true;
            return true;

        }
        // Free the packet that was allocated by av_read_frame
        av_free_packet(&packet);
    }

    return false;
}

bool FFmpegPlayer::seek_first_packet()
{
    assert(firstPtsSet);
    qDebug() << "stream id" << videoStreamID;
    qDebug() << "first pts" << firstPts;

    //if (av_seek_frame(formatContext, videoStreamID, timestamp, AVSEEK_FLAG_BACKWARD) < 0)
    if (av_seek_frame(formatContext, videoStreamID, firstPts, AVSEEK_FLAG_BACKWARD) < 0)
    {
        qDebug() << "ERROR seek_first_packet()";
        return false;
    }

    avcodec_flush_buffers(videoContext);
    return true;
}

// Returns the name of input file format
char const *FFmpegPlayer::get_format_name()
{
    return formatContext->iformat->name;
}
