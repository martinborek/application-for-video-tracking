/**
 * @file avwriter.cpp
 * @author Martin Borek (mborekcz@gmail.com)
 * @date May, 2015
 */

#include "avwriter.h"
#include <QDebug>
#include <cassert>

AVWriter::AVWriter()
{
    formatContext = nullptr;
    videoStream = nullptr;
    audioStream = nullptr;
}

AVWriter::~AVWriter()
{
    if (videoStream)
        avcodec_close(videoStream->codec);
    if (audioStream)
        avcodec_close(audioStream->codec);

    if (formatContext)
    {
        avformat_free_context(formatContext);
        formatContext = nullptr;
    }
}

bool AVWriter::initialize_output(std::string filename, AVStream const *inVideoStream, AVStream const *inAudioStream, char const *inFormatName, std::string inFileExtension)
{
    /** choose extension according the file format:
    std::string str(formatContext->iformat->name);
    std::string ext = str.substr(0, str.find(','));
    filename += ".";
    filename += ext;
    qDebug() << ext.c_str();
    avformat_alloc_output_context2(&formatContext2, NULL, ext.c_str(), NULL); // allocation of output context
      */


    /** Output video format deduction form the (output) filename.
        It is disabled because format should match the input format so as extra settings is not needed.
        Encoding to some other formats might not be valid especially because mixed PTS and DTS

    std::string originalFilename = filename; // "filename" might change the extension if needed; "originalFilename" stores the argument
    // Allocating output media context
    avformat_alloc_output_context2(&formatContext, nullptr, nullptr, filename.c_str()); // allocation of output context

    if (!formatContext || formatContext->oformat->video_codec == AV_CODEC_ID_NONE)
    {
        if (!formatContext)
        {
            qDebug() << "Warning: Could not deduce output format from file extension. Deducing from input format instead.";
        }
        else
        {
            qDebug() << "Warning: File format deduced from file extension does not support video. Deducing from input format instead.";
            avformat_free_context(formatContext);
            formatContext = nullptr;
        }

        // Try allocate formatContext according to input format name and set appropriate extension
        // The new extension is added after the previous one (user might want to have a dot in the filename)
        std::string str(inFormatName);
        std::string ext = str.substr(0, str.find(',')); // Finds first extension that belongs to given file format
        if (!ext.empty()) // Appropriate extension found
        {
            filename = originalFilename + "." + ext;
            qDebug() << ext.c_str();
            avformat_alloc_output_context2(&formatContext, NULL, ext.c_str(), NULL); // allocation of output context
        }
    }
    */

    // Try allocate formatContext according to input format name and set appropriate extension
    // If a user changed the file extension, add the correct one
    std::string str(inFormatName);

    // Find first extension that belongs to given file format
    std::string ext = "";
    auto extPos = str.find(',');
    if (extPos != std::string::npos)
         ext = str.substr(0, extPos); // Finds first extension that belongs to given file format
    else
        ext = str;

    if (!ext.empty()) // Appropriate extension found
    {

        std::string outFileExtension = "";
        auto outExtensionPosition  = filename.find_last_of('.');
        if (outExtensionPosition != std::string::npos)
            outFileExtension = filename.substr(outExtensionPosition);


        qDebug() << "outFileExtension" << QString::fromStdString(outFileExtension);
        qDebug() << "inFileExtension" << QString::fromStdString(inFileExtension);
        if (outFileExtension.compare(inFileExtension)) // extensions are not the same, suffix the correct one
            filename +=  inFileExtension;

        qDebug() << ext.c_str();
        avformat_alloc_output_context2(&formatContext, NULL, ext.c_str(), NULL); // allocation of output context
    }

    if (!formatContext || formatContext->oformat->video_codec == AV_CODEC_ID_NONE)
    {   //Deduced file format does not support video. Using MPEG
        if (!formatContext)
        {
            qDebug() << "Warning: Deduced file format does not support video. Using MPEG.";
        }
        else
        {
            qDebug() << "Warning: Could not deduce output format, using MPEG";
            avformat_free_context(formatContext);
            formatContext = nullptr;
        }

        filename = filename + ".mpg";
        avformat_alloc_output_context2(&formatContext, NULL, "mpeg", filename.c_str());
    }

    if (!formatContext)
    {
        qDebug() << "Error: avformat_alloc_output_context2";
        return false;
    }

    AVOutputFormat *fmt = formatContext->oformat;

    // Add video stream
    if (fmt->video_codec != AV_CODEC_ID_NONE)
    {
        if (!add_stream(&videoStream, inVideoStream))
        {
            qDebug() << "ERROR: Adding video stream";
            return false;
        }
    }
    else
    { // Video is needed
        qDebug() << "ERROR: This format is not a video format.";
        return false;
    }

    // Add audio format
    if (fmt->audio_codec != AV_CODEC_ID_NONE)
    {

        if (!add_stream(&audioStream, inAudioStream))
        {
            qDebug() << "ERROR: Adding audio stream";
            return false;
        }
    }

    /* Now that all the parameters are set, we can open the audio and
     * video codecs and allocate the necessary encode buffers. */

    av_dump_format(formatContext, 0, filename.c_str(), 1);

    // Open the output file
    if (!(fmt->flags & AVFMT_NOFILE)) {
        if (avio_open(&formatContext->pb, filename.c_str(), AVIO_FLAG_WRITE) < 0)
        {
            qDebug() << "Error could not open output file.";
            return false;
        }
    }

    // Write the stream header
    if (avformat_write_header(formatContext, nullptr) < 0)
    {
        qDebug() << "Error writing header";
        return false;
    }

    // Everything is correctly set, prepared for writing
    return true;

}

bool AVWriter::close_output()
{
    /* Write the trailer, if any. The trailer must be written before you
     * close the CodecContexts open when you wrote the header; otherwise
     * av_write_trailer() may try to use memory that was freed on
     * av_codec_close(). */
    av_write_trailer(formatContext);

    if (!(formatContext->oformat->flags & AVFMT_NOFILE))
        avio_closep(&formatContext->pb);

    return true;
}

bool AVWriter::add_stream(AVStream **outputStream, const AVStream *inputStream)
{
 //   AVStream *inputStream = formatContext->streams[inputStreamID]; // careful, it's different formatContext!

    AVCodecContext *codecContext;
    AVCodec *codec;

    /* find the encoder */
    qDebug() << "codec id:" << inputStream->codec->codec_id;
    codec = avcodec_find_encoder(inputStream->codec->codec_id);
    if (!codec) {
        qDebug() << "Error finding encoder";
        return false;
    }

    if (codec->type == AVMEDIA_TYPE_VIDEO)
        *outputStream = avformat_new_stream(formatContext, codec); // Creates new stream (also increases nb_streams)
    else
        *outputStream = avformat_new_stream(formatContext, inputStream->codec->codec); // Creates new stream (also increases nb_streams)

    if (!*outputStream) {
        qDebug() << "Error allocating stream";
        return false;
    }
    (*outputStream)->id = formatContext->nb_streams - 1;
    codecContext = (*outputStream)->codec;

    /** Not used because of c->qmin, qmax and qcompress - in mp4 format copied values are invalid
    if (avcodec_copy_context((*outputStream)->codec, inputStream->codec) < 0) {
        qDebug() << "Failed to copy context from input to output stream codec context";
        return false;
    }
    */

    switch (codec->type)
    {
        case AVMEDIA_TYPE_AUDIO:
            if (avcodec_copy_context((*outputStream)->codec, inputStream->codec) < 0)
            {
                qDebug() << "Failed to copy context from input to output stream codec context";
                return false;
            }
            (*outputStream)->codec->codec_tag = 0;

        /** No need to encode because audio frames are not decoded
            codecContext->sample_fmt = codec->sample_fmts ?
                codec->sample_fmts[0] : AV_SAMPLE_FMT_FLTP;
            codecContext->bit_rate = 64000;
            codecContext->sample_rate = 44100;
            if (codec->supported_samplerates) {
                codecContext->sample_rate = codec->supported_samplerates[0];
                for (i = 0; codec->supported_samplerates[i]; i++) {
                    if (codec->supported_samplerates[i] == 44100)
                        codecContext->sample_rate = 44100;
                }
            }
            codecContext->channels = av_get_channel_layout_nb_channels(codecContext->channel_layout);
            codecContext->channel_layout = AV_CH_LAYOUT_STEREO;
            if (codec->channel_layouts) {
                codecContext->channel_layout = codec->channel_layouts[0];
                for (i = 0; codec->channel_layouts[i]; i++) {
                    if (codec->channel_layouts[i] == AV_CH_LAYOUT_STEREO)
                        codecContext->channel_layout = AV_CH_LAYOUT_STEREO;
                }
            }
            codecContext->channels = av_get_channel_layout_nb_channels(codecContext->channel_layout);
            (*outputStream)->time_base = AVRational{ 1, codecContext->sample_rate };
            */
            break;

        case AVMEDIA_TYPE_VIDEO:
            codecContext->codec_id = inputStream->codec->codec_id;

            // this is commented because for some video formats causes the first frame not being a keyframe
            //codecContext->bit_rate = inputStream->codec->bit_rate;

            codecContext->width = inputStream->codec->width;
            codecContext->height = inputStream->codec->height;

            (*outputStream)->time_base = inputStream->time_base;

            codecContext->time_base = (*outputStream)->time_base;

            codecContext->gop_size = inputStream->codec->gop_size; // one intra frame emitted every X frames at most

            codecContext->pix_fmt = (enum PixelFormat)outputFormat;

            codecContext->max_b_frames = inputStream->codec->max_b_frames;

            codecContext->mb_decision = inputStream->codec->mb_decision;
            break;
        default:
            break;
    }

    /* Some formats want stream headers to be separate. */
    if (formatContext->oformat->flags & AVFMT_GLOBALHEADER)
        codecContext->flags |= CODEC_FLAG_GLOBAL_HEADER;

    if(codec->type == AVMEDIA_TYPE_VIDEO)
    {
        if (avcodec_open2((*outputStream)->codec, codec, NULL) < 0)
        {
            qDebug() << "Error opening codec";
            return false;
        }
    }
    return true;
}

bool AVWriter::write_video_frame(VideoFrame &frame)
{
    assert(frame.get_mat_frame() != nullptr);

    int got_packet = 0;

    AVPacket pkt;
    //AVPacket pkt = { 0 };

    // Set .data and .size to 0; That is needed for avcodec_encode_video2() to allocate buffer
    pkt.data = nullptr;
    pkt.size = 0;
    av_init_packet(&pkt);

    AVFrame const *avFrame = frame.get_av_frame(); // frame converted from cv::Mat (part of VideoFrame)
    if (!avFrame)
    {
       qDebug() << "Error: Cannot get frame converted to AVFrame";
       return false;
    }

    /** Commented because not tested
    if (formatContext->oformat->flags & AVFMT_RAWPICTURE)
    { // avoiding data copy with some raw video muxers
        qDebug() << "raw";
        pkt.flags |= AV_PKT_FLAG_KEY;
        pkt.stream_index = videoStream->index;
        pkt.data = (uint8_t *)avFrame;
        pkt.size = sizeof(AVPicture);
        pkt.pts = pkt.dts = frame.get_timestamp();
    }
    else
    {
    */

    //avFrame->pts = av_rescale_q(avFrame->pts, videoStream->time_base, videoStream->codec->time_base);

    if (avcodec_encode_video2(videoStream->codec, &pkt, avFrame, &got_packet) < 0)
    {
        qDebug() << "Error encoding video frame";
        return false;
    }
    if (!got_packet)
    { // packet is empty -> avcodec_encode_video2() needs more frames to begin decoding
        qDebug() << "Encoding: packet is empty";
        return true;
    }

   /** } */


    // rescale output packet timestamp values from codec to stream timebase
    av_packet_rescale_ts(&pkt, videoStream->codec->time_base, videoStream->time_base);
    pkt.stream_index = videoStream->index;

    // Write the compressed frame to the media file.
    if (av_interleaved_write_frame(formatContext, &pkt) < 0)
    {
        qDebug() << "Error: av_interleaved_write_frame.";
        return false;
    }

    av_free_packet(&pkt);

    return true;
}

bool AVWriter::write_last_frames()
{
    int got_packet = 1;
    AVPacket pkt;
    //AVPacket pkt = { 0 };

    // Set .data and .size to 0; It is needed for avcodec_encode_video2() to allocate buffer
    pkt.data = nullptr;
    pkt.size = 0;
    av_init_packet(&pkt);

    // Get delayed frames - empty the buffer
    while (got_packet)
    {
        qDebug() << "Writing a last frame";
        if (avcodec_encode_video2(videoStream->codec, &pkt, NULL, &got_packet) < 0)
        {
            qDebug() << "Error encoding video frame when writing last frames";
            return false;
        }

        if (got_packet)
        {
            av_packet_rescale_ts(&pkt, videoStream->codec->time_base, videoStream->time_base);

            pkt.stream_index = videoStream->index;

            // Write the compressed frame to the media file.
            if (av_interleaved_write_frame(formatContext, &pkt) < 0)
            {
                qDebug() << "Error: av_interleaved_write_frame() when writing last frames";
                return false;
            }
            av_free_packet(&pkt);
        }
    }

    return true;
}

bool AVWriter::write_audio_packet(AVPacket &pkt)
{
    pkt.pos = -1;
    pkt.stream_index = audioStream->index;
    if (av_interleaved_write_frame(formatContext, &pkt) < 0)
    {
        qDebug() << "Error: Writing audio packet";
        return false;
    }
    return true;
}
