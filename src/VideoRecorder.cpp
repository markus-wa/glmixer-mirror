
extern "C"
{
#include <libavcodec/avcodec.h>
#include <libswscale/swscale.h>
#include <libavutil/mathematics.h>
#include <libavutil/common.h>
#include <libavfilter/buffersink.h>
#include <libavfilter/buffersrc.h>
#include <libavutil/opt.h>
#include <libavutil/pixdesc.h>
}

#include <thread>

#include "VideoRecorder.h"
#include "CodecManager.h"


VideoRecorder *VideoRecorder::getRecorder(encodingformat f, QString filename, int w, int h, int fps, encodingquality quality = QUALITY_AUTO)
{
    CodecManager::registerAll();

    VideoRecorder *rec = 0;

    switch (f){

    case FORMAT_MP4_MPEG4:
        rec = new VideoRecorderMP4(filename, w, h, fps, quality);
        break;

    case FORMAT_MP4_H264:
        rec = new VideoRecorderH264(filename, w, h, fps, quality);
        break;

    case FORMAT_MPG_MPEG2:
        rec = new VideoRecorderMPEG2(filename, w, h, fps);
        break;

    case FORMAT_MPG_MPEG1:
        rec = new VideoRecorderMPEG1(filename, w, h, fps);
        break;

    case FORMAT_WMV_WMV2:
        rec = new VideoRecorderWMV(filename, w, h, fps);
        break;

    case FORMAT_FLV_FLV1:
        rec = new VideoRecorderFLV(filename, w, h, fps);
        break;

    case FORMAT_AVI_FFVHUFF:
        rec = new VideoRecorderFFVHUFF(filename, w, h, fps);
        break;

    case FORMAT_AVI_RAW:
        rec = new VideoRecorderRAW(filename, w, h, fps);
        break;

    default:
        VideoRecorderException("Encoding format not supported.").raise();
    }

    return rec;
}

VideoRecorder::VideoRecorder(QString filename, int w, int h, int fps) : fileName(filename), frameRate(fps), framenum(0)
{
    /* resolution must be a multiple of two */
    width = (w/2)*2;
    height = (h/2)*2;

    // not initialized
    format_context = NULL;
    codec_context = NULL;
    video_stream = NULL;
    codec = NULL;
    frame = NULL;
    in_video_filter = NULL;
    out_video_filter = NULL;
    graph = NULL;
    opts = NULL;

    targetFormat = AV_PIX_FMT_NONE;
    codecId = AV_CODEC_ID_NONE;
}

VideoRecorder::~VideoRecorder()
{
    // close codec context
    if (codec_context) {
        avcodec_close(codec_context);
        avcodec_free_context(&codec_context);
    }

    // close format context
    if (format_context)
        avformat_free_context(format_context);

    if (graph)
        avfilter_graph_free(&graph);

    if (frame) {
        av_frame_unref(frame);
        av_frame_free(&frame);
    }

    qDebug() << "VideoRecorder" << QChar(124).toLatin1() << "cleared.";
}

VideoRecorderMP4::VideoRecorderMP4(QString filename, int w, int h, int fps, encodingquality quality) : VideoRecorder(filename, w, h, fps)
{
    // specifics for this recorder
    suffix = "mp4";
    description = "MPEG 4 Video (*.mp4)";
    codecId = AV_CODEC_ID_MPEG4;
    targetFormat = AV_PIX_FMT_YUV420P;

    // allocate context
    setupContext("mp4");

    // default bit rate
    int64_t br = (width * height * av_get_bits_per_pixel( av_pix_fmt_desc_get(targetFormat)) * (frameRate));

    // MAX 24 Mbit/s max – AVCHD (using MPEG4 AVC compression)
    codec_context->bit_rate = FFMIN(br, 24000000);;

    if (quality != QUALITY_AUTO) {

        // not auto : set max rate and buffer
        codec_context->rc_max_rate = br;

        // set bit rate
        switch (quality) {
        case QUALITY_LOW:
            codec_context->bit_rate = codec_context->rc_max_rate / 100;
            break;
        case QUALITY_AUTO:
        case QUALITY_MEDIUM:
            codec_context->bit_rate = codec_context->rc_max_rate / 40;
            break;
        case QUALITY_HIGH:
            codec_context->bit_rate = codec_context->rc_max_rate;
            break;
        }

        // compute VBV buffer size ( snipet from mpegvideo_enc.c in ffmpeg )
        if (codec_context->rc_max_rate >= 15000000) {
            codec_context->rc_buffer_size = 320 + (codec_context->rc_max_rate - 15000000LL) * (760-320) / (38400000 - 15000000);
        } else if(codec_context->rc_max_rate >=  2000000) {
            codec_context->rc_buffer_size =  80 + (codec_context->rc_max_rate -  2000000LL) * (320- 80) / (15000000 -  2000000);
        } else if(codec_context->rc_max_rate >=   384000) {
            codec_context->rc_buffer_size =  40 + (codec_context->rc_max_rate -   384000LL) * ( 80- 40) / ( 2000000 -   384000);
        } else
            codec_context->rc_buffer_size = 40;
        codec_context->rc_buffer_size *= 16384;

        codec_context->bit_rate_tolerance = codec_context->bit_rate;
    }

    // OPTIONNAL
    // see https://github.com/savoirfairelinux/ring-daemon/blob/master/src/media/media_encoder.cpp
    codec_context->thread_count = FFMIN(8, std::thread::hardware_concurrency());

    setupFiltering();

    qDebug() << filename << QChar(124).toLatin1() << "Encoder" << avcodec_descriptor_get(codec_context->codec_id)->long_name << " ("<< codec_context->bit_rate / 1024 <<" kbit/s, "<<codec_context->rc_max_rate / 1024 <<" kbit/s max, VBV " << codec_context->rc_buffer_size/8192 << "kbyte)";
}

VideoRecorderH264::VideoRecorderH264(QString filename, int w, int h, int fps, encodingquality quality) : VideoRecorder(filename, w, h, fps)
{
    // specifics for this recorder
    suffix = "mov";
    description = "QuickTime Video (*.mov)";
    codecId = AV_CODEC_ID_H264;
    targetFormat = AV_PIX_FMT_YUV420P;

    // select variable bit rate quality factor
    int vbr = 54;   // default to 54% quality, default crf 23
    switch (quality) {
    case QUALITY_LOW:
        vbr = 40;;  // crf 30 : quite ugly but not that bad
        break;
    case QUALITY_MEDIUM:
        vbr = 64;;  // crf 18 : 'visually' lossless
        break;
    case QUALITY_HIGH:
        vbr = 90;   // crf 5 : almost lossless
        targetFormat = AV_PIX_FMT_YUV444P;  // higher quality color
        break;
    case QUALITY_AUTO:
        break;
    }

    // allocate context
    setupContext("mov");

    // configure encoder & quality
    av_dict_set(&opts, "preset", "ultrafast", 0);
    av_dict_set(&opts, "tune", "zerolatency", 0);
    if ((strcmp(codec->name, "h264_omx") == 0) || (strcmp(codec->name, "mpeg4_omx") == 0)) {
        // H264 OMX encoder quality can only be controlled via bit_rate
        vbr = (width * height * fps * vbr) >> 7;
        // Clip bit rate to min
        if (vbr < 4000) // magic number
            vbr = 4000;
        codec_context->profile = FF_PROFILE_H264_HIGH;
        codec_context->bit_rate = vbr;
    }
    else {
        // Control other H264 encoders quality via CRF
        // The total range is from 0 to 51, where 0 is lossless, 18 can be considered ‘visually lossless’,
        // and 51 is terrible quality. A sane range is 18-26, and the default is 23.
        char crf[10];
        vbr = (int)(( (100-vbr) * 51)/100);
        snprintf(crf, 10, "%d", vbr);
        av_dict_set(&opts, "crf", crf, 0);
    }

    // OPTIONNAL
    codec_context->thread_count = FFMIN(8, std::thread::hardware_concurrency());

    setupFiltering();

    char *buffer = NULL;
    av_dict_get_string(opts, &buffer, '=', ',');

    qDebug() << filename << QChar(124).toLatin1() << "Encoder" << avcodec_descriptor_get(codec_context->codec_id)->long_name << " ( "<< buffer <<  ")";

    av_freep(&buffer);
}

VideoRecorderFFVHUFF::VideoRecorderFFVHUFF(QString filename, int w, int h, int fps) : VideoRecorder(filename, w, h, fps)
{
    // specifics for this recorder
    suffix = "avi";
    description = "AVI Video (*.avi)";
    codecId = AV_CODEC_ID_FFVHUFF;
    targetFormat = AV_PIX_FMT_RGB24;

    // allocate context
    setupContext("avi");

    // SPECIFIC FFVHUFF
    codec_context->thread_count  = 1;

    setupFiltering();

    qDebug() << filename << QChar(124).toLatin1() << "Encoder" << avcodec_descriptor_get(codec_context->codec_id)->long_name ;
}

VideoRecorderRAW::VideoRecorderRAW(QString filename, int w, int h, int fps) : VideoRecorder(filename, w, h, fps)
{
    // specifics for this recorder
    suffix = "avi";
    description = "AVI Video (*.avi)";
    codecId = AV_CODEC_ID_RAWVIDEO;
    targetFormat = AV_PIX_FMT_BGR24;

    // allocate context
    setupContext("avi");

    setupFiltering();

    qDebug() << filename << QChar(124).toLatin1() << "Encoder" << avcodec_descriptor_get(codec_context->codec_id)->long_name ;
}

VideoRecorderMPEG1::VideoRecorderMPEG1(QString filename, int w, int h, int fps) : VideoRecorder(filename, w, h, fps)
{
    // specifics for this recorder
    suffix = "mpg";
    description = "MPEG Video (*.mpg *.mpeg)";
    codecId = AV_CODEC_ID_MPEG1VIDEO;
    targetFormat = AV_PIX_FMT_YUV420P;

    // allocate context
    setupContext("mpeg");

    // 9.8 Mbit/s max – DVD
    codec_context->rc_max_rate = 9800000;

    // bitrate
    int imagesize = width * height * av_get_bits_per_pixel( av_pix_fmt_desc_get(targetFormat));
    codec_context->bit_rate =  FFMIN(codec_context->rc_max_rate,  imagesize * frameRate);
    codec_context->rc_buffer_size = FFMAX(codec_context->bit_rate, 15000000) * 112L / 15000000 * 16384;

    // set the VBV buffer size of video stream
    // minimum default size is 230KB
    AVCPBProperties *props;
    props = (AVCPBProperties*) av_stream_new_side_data( video_stream, AV_PKT_DATA_CPB_PROPERTIES, sizeof(*props));
    props->buffer_size = FFMAX(imagesize/8, 235520);
    props->max_bitrate = 0; // auto
    props->min_bitrate = 0; // auto
    props->avg_bitrate = 0; // auto
    props->vbv_delay = UINT64_MAX; // auto

    // SPECIFIC MP1
    codec_context->max_b_frames = 1;
    codec_context->mb_decision = 2;
    // OPTIONNAL
    codec_context->thread_count = FFMIN(8, std::thread::hardware_concurrency());

    // needs filtering
    setupFiltering();

    qDebug() << filename << QChar(124).toLatin1() << "Encoder" << avcodec_descriptor_get(codec_context->codec_id)->long_name << " ("<< codec_context->bit_rate / 1024 <<" kbit/s, buffer "<<props->buffer_size /1024<<" kbytes)";
}

VideoRecorderMPEG2::VideoRecorderMPEG2(QString filename, int w, int h, int fps) : VideoRecorder(filename, w, h, fps)
{
    // specifics for this recorder
    suffix = "mpg";
    description = "MPEG Video (*.mpg *.mpeg)";
    codecId = AV_CODEC_ID_MPEG2VIDEO;
    targetFormat = AV_PIX_FMT_YUV420P;

    // allocate context
    setupContext("mpeg");

    // 25 Mbit/s approximate – HDV 1080i (using MPEG2 compression)
    codec_context->rc_max_rate = 25000000;

    // bitrate
    int imagesize = width * height * av_get_bits_per_pixel( av_pix_fmt_desc_get(targetFormat));
    codec_context->bit_rate =  FFMIN(codec_context->rc_max_rate,  imagesize * frameRate);
    codec_context->rc_buffer_size = FFMAX(codec_context->bit_rate, 15000000) * 112L / 15000000 * 16384;

    // set the VBV buffer size of video stream
    // minimum default size is 230KB
    AVCPBProperties *props;
    props = (AVCPBProperties*) av_stream_new_side_data( video_stream, AV_PKT_DATA_CPB_PROPERTIES, sizeof(*props));
    props->buffer_size = FFMAX(imagesize/8, 235520);;
    props->max_bitrate = 0; // auto
    props->min_bitrate = 0; // auto
    props->avg_bitrate = 0; // auto
    props->vbv_delay = UINT64_MAX; // auto

    // SPECIFIC MP2
    codec_context->max_b_frames = 2;
    // OPTIONNAL
    codec_context->thread_count = FFMIN(8, std::thread::hardware_concurrency());

    // needs filtering
    setupFiltering();

    qDebug() << filename << QChar(124).toLatin1() << "Encoder" << avcodec_descriptor_get(codec_context->codec_id)->long_name << " ("<< codec_context->bit_rate / 1024 <<" kbit/s, buffer "<<props->buffer_size /1024<<" kbytes)";
}

VideoRecorderWMV::VideoRecorderWMV(QString filename, int w, int h, int fps) : VideoRecorder(filename, w, h, fps)
{
    // specifics for this recorder
    suffix = "wmv";
    description = "Windows Media Video (*.wmv)";
    codecId = AV_CODEC_ID_WMV2;
    targetFormat = AV_PIX_FMT_YUV420P;

    // allocate context
    setupContext("avi");

    // bit_rate, maxi 25 Mbits/s
    codec_context->bit_rate = FFMIN(width * height * av_get_bits_per_pixel( av_pix_fmt_desc_get(targetFormat)) * frameRate, 25000000);

    // OPTIONNAL
    codec_context->thread_count = FFMIN(8, std::thread::hardware_concurrency());

    // needs filtering
    setupFiltering();

    qDebug() << filename << QChar(124).toLatin1() << "Encoder" << avcodec_descriptor_get(codec_context->codec_id)->long_name << " (" << codec_context->bit_rate / 1024 <<" kbit/s )";
}

VideoRecorderFLV::VideoRecorderFLV(QString filename, int w, int h, int fps) : VideoRecorder(filename, w, h, fps)
{
    // specifics for this recorder
    suffix = "flv";
    description = "Flash Video (*.flv)";
    codecId = AV_CODEC_ID_FLV1;
    targetFormat = AV_PIX_FMT_YUV420P;

    // allocate context
    setupContext("flv");

    // bit_rate, maxi 25 Mbits/s
    codec_context->bit_rate = FFMIN(width * height * av_get_bits_per_pixel( av_pix_fmt_desc_get(targetFormat)) * frameRate, 25000000);

    // OPTIONNAL
    codec_context->thread_count = FFMIN(8, std::thread::hardware_concurrency());

    // needs filtering
    setupFiltering();

    qDebug() << filename << QChar(124).toLatin1() << "Encoder" << avcodec_descriptor_get(codec_context->codec_id)->long_name << " (" << codec_context->bit_rate / 1024 <<" kbit/s )";
}

void VideoRecorder::addFrame(AVFrame *f)
{
    int retcd = 0;
    char errstr[128];

    if (!format_context)
        VideoRecorderException("Format context unavailable.").raise();

    // convert frame format & flip
    if (in_video_filter) {

        retcd = av_buffersrc_add_frame_flags(in_video_filter, f, AV_BUFFERSRC_FLAG_KEEP_REF);
        if (retcd < 0)
            VideoRecorderException(QString(av_make_error_string(errstr, sizeof(errstr),retcd))).raise();

        retcd = av_buffersink_get_frame(out_video_filter, frame);
        if (retcd < 0)
            VideoRecorderException(QString(av_make_error_string(errstr, sizeof(errstr),retcd))).raise();
    }
    else
        av_frame_ref(frame, f);

    // set Presentation time Stamp as frame number
    frame->pts = framenum;

    // send frame to codec encoder
    retcd = avcodec_send_frame(codec_context, frame);
    if (retcd < 0)
        VideoRecorderException(QString(av_make_error_string(errstr, sizeof(errstr),retcd))).raise();

    // free buffer in frame
    av_frame_unref(frame);

    // loop packet encoding
    while ( retcd >= 0 ) {

        AVPacket pkt;
        av_init_packet(&pkt);
        pkt.data = NULL;
        pkt.size = 0;

        // get packet from codec encoder
        retcd =  avcodec_receive_packet(codec_context, &pkt);

        // ignore if temporarily unavailable
        if (retcd == AVERROR(EAGAIN) || retcd == AVERROR_EOF)
            break;

        // interrupt on error
        if (retcd < 0)
            VideoRecorderException(QString(av_make_error_string(errstr, sizeof(errstr),retcd))).raise();

        // compute the presentation time stamp (pts)
        pkt.dts = av_rescale_q_rnd(pkt.dts, codec_context->time_base, video_stream->time_base, AV_ROUND_NEAR_INF);
        pkt.pts = av_rescale_q_rnd(pkt.pts, codec_context->time_base, video_stream->time_base, AV_ROUND_NEAR_INF);
        pkt.duration = av_rescale_q(1, codec_context->time_base, video_stream->time_base);

        // write frame
        retcd = av_write_frame(format_context, &pkt);
        if (retcd < 0)
            VideoRecorderException(QString(av_make_error_string(errstr, sizeof(errstr),retcd))).raise();

        av_packet_unref(&pkt);
    }

    // one more frame !
    framenum++;
}

void VideoRecorder::open()
{
    int retcd = 0;
    char errstr[128];

    if (!format_context)
        VideoRecorderException("Cannot open recording without format context.").raise();

    // open codec context
    retcd = avcodec_open2(codec_context, codec, &opts) ;
    if (retcd < 0)
        VideoRecorderException("Codec open " + QString(av_make_error_string(errstr, sizeof(errstr),retcd))).raise();

    // done with options
    av_dict_free(&opts);
    opts = NULL;

    retcd = avcodec_parameters_from_context( video_stream->codecpar, codec_context) ;
    if (retcd < 0)
        VideoRecorderException(QString(av_make_error_string(errstr, sizeof(errstr),retcd))).raise();

    // open file corresponding to the format context
    retcd = avio_open(&format_context->pb, qPrintable(fileName), AVIO_FLAG_WRITE);
    if (retcd < 0)
        VideoRecorderException("File open " + QString(av_make_error_string(errstr, sizeof(errstr),retcd))).raise();

    // start recording
    retcd = avformat_write_header(format_context, NULL);
    if (retcd < 0)
        VideoRecorderException("Write header " + QString(av_make_error_string(errstr, sizeof(errstr),retcd))).raise();

    framenum = 0;
}

int VideoRecorder::close()
{
    int retcd = 0;
    char errstr[128];

    if (!format_context)
        VideoRecorderException("Cannot close recording without format context.").raise();

    // end recording
    retcd = av_write_trailer(format_context);
    if (retcd < 0)
        VideoRecorderException("Write trailer " + QString(av_make_error_string(errstr, sizeof(errstr),retcd))).raise();

    // close file
    retcd = avio_close(format_context->pb);
    if (retcd < 0)
        VideoRecorderException("File close " + QString(av_make_error_string(errstr, sizeof(errstr),retcd))).raise();

    return framenum;
}

int VideoRecorder::estimateGroupOfPictureSize()
{
    // see http://www2.acti.com/download_file/Product/support/DesignSpec_Note_GOP_20091120.pdf
    if (frameRate < 5){
        return 24;
    } else if (frameRate < 15 ){
        return 60;
    } else {
        return frameRate * 4;
    }
}


void VideoRecorder::setupContext(QString formatname)
{
    // allocate frame
    frame = av_frame_alloc();

    // allocate format context
    format_context = avformat_alloc_context();
    if (!format_context)
        VideoRecorderException("Cannot allocate format context.").raise();

    // fill in format context
    snprintf(format_context->filename, sizeof(format_context->filename), "%s", qPrintable(fileName) );
    format_context->oformat = av_guess_format(qPrintable(formatname), NULL, NULL);
    if (!format_context->oformat)
        VideoRecorderException("Video format not found.").raise();

    if (format_context->oformat->video_codec == AV_CODEC_ID_NONE)
        VideoRecorderException("Codec unavailable.").raise();

    // create video stream
    codec = avcodec_find_encoder(codecId);
    if (!codec)
        VideoRecorderException("codec not found").raise();
    video_stream = avformat_new_stream(format_context, codec);
    if (!video_stream)
        VideoRecorderException("Cannot allocate stream.").raise();

    video_stream->time_base = av_make_q(1, frameRate);

    // allocate codec context
    codec_context = avcodec_alloc_context3(codec);
    if (!codec_context)
        VideoRecorderException("Cannot allocate codec context.").raise();

    // fill in codec context
    codec_context->codec_type    = AVMEDIA_TYPE_VIDEO;
    codec_context->codec_id      = codecId;
    codec_context->pix_fmt       = targetFormat;
    codec_context->width         = width;
    codec_context->height        = height;
    codec_context->framerate.num = frameRate;
    codec_context->framerate.den = 1;
    codec_context->time_base.num = 1;
    codec_context->time_base.den = frameRate;
    codec_context->gop_size      = estimateGroupOfPictureSize();

    if (format_context->oformat->flags & AVFMT_GLOBALHEADER)
        codec_context->flags     |= AV_CODEC_FLAG_GLOBAL_HEADER;
}

void VideoRecorder::setupFiltering()
{
    int retcd = 0;
    char errstr[128];

    // create conversion context
    const AVFilter *buffersrc  = avfilter_get_by_name("buffer");
    const AVFilter *buffersink = avfilter_get_by_name("buffersink");
    AVFilterInOut *outputs = avfilter_inout_alloc();
    AVFilterInOut *inputs  = avfilter_inout_alloc();

    graph = avfilter_graph_alloc();
    if (!outputs || !inputs || !graph)
        VideoRecorderException("cannot allocate filtering graph.").raise();

    int64_t conversionAlgorithm = SWS_POINT; // optimal speed scaling for videos
    char sws_flags_str[128];
    snprintf(sws_flags_str, sizeof(sws_flags_str), "flags=%d", (int) conversionAlgorithm);
    graph->scale_sws_opts = av_strdup(sws_flags_str);

    // INPUT BUFFER
    char buffersrc_args[256];
    snprintf(buffersrc_args, sizeof(buffersrc_args),
             "video_size=%dx%d:pix_fmt=%d:time_base=%d/%d:pixel_aspect=0/1",
             width, height, AV_PIX_FMT_RGB24, video_stream->time_base.num, video_stream->time_base.den);

    retcd = avfilter_graph_create_filter(&in_video_filter, buffersrc,
                                         "in", buffersrc_args, NULL, graph);
    if (retcd < 0)
        VideoRecorderException("Create in filter " + QString(av_make_error_string(errstr, sizeof(errstr),retcd))).raise();

    // OUTPUT SINK
    retcd = avfilter_graph_create_filter(&out_video_filter, buffersink,
                                         "out", NULL, NULL, graph);
    if (retcd < 0)
        VideoRecorderException("Create out filter " + QString(av_make_error_string(errstr, sizeof(errstr),retcd))).raise();

    enum AVPixelFormat pix_fmts[] = { targetFormat, AV_PIX_FMT_NONE };
    retcd = av_opt_set_int_list(out_video_filter, "pix_fmts", pix_fmts,
                             AV_PIX_FMT_NONE, AV_OPT_SEARCH_CHILDREN) ;
    if (retcd < 0)
        VideoRecorderException("Convert format " + QString(av_make_error_string(errstr, sizeof(errstr),retcd))).raise();

    // create another filter
    inputs->name       = av_strdup("out");
    inputs->filter_ctx = out_video_filter;
    inputs->pad_idx    = 0;
    inputs->next       = NULL;
    outputs->name       = av_strdup("in");
    outputs->filter_ctx = in_video_filter;
    outputs->pad_idx    = 0;
    outputs->next       = NULL;

    // no scaling to target size is necessary
    char filter_str[128];
    snprintf(filter_str, sizeof(filter_str), "vflip");
    retcd = avfilter_graph_parse_ptr(graph, filter_str, &inputs, &outputs, NULL);
    if (retcd < 0)
        VideoRecorderException("Add vflip " + QString(av_make_error_string(errstr, sizeof(errstr),retcd))).raise();

    // validate the filtering graph
    retcd = avfilter_graph_config(graph, NULL);
    if (retcd < 0)
        VideoRecorderException("Configure graph " + QString(av_make_error_string(errstr, sizeof(errstr),retcd))).raise();

    avfilter_inout_free(&inputs);
    avfilter_inout_free(&outputs);

}
