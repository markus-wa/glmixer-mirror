
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

// HOWTO avconv command
// List encoders : avconv -encoders
// List options  : avconv -hide_banner -h encoder=<encodername>

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

    case FORMAT_MKV_HEVC:
        rec = new VideoRecorderHEVC(filename, w, h, fps, quality);
        break;

    case FORMAT_WEB_WEBM:
        rec = new VideoRecorderWebM(filename, w, h, fps, quality);
        break;

    case FORMAT_MOV_PRORES:
        rec = new VideoRecorderProRes(filename, w, h, fps, quality);
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

    case FORMAT_AVI_FFV3:
        rec = new VideoRecorderFFV(filename, w, h, fps);
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
    enum AVPixelFormat targetFormat = AV_PIX_FMT_YUV420P;

    // allocate context
    QStringList codeclist;
    codeclist  << "mpeg4"<< "libxvid" << "msmpeg4";
    setupContext(codeclist, "mp4", targetFormat);

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
            codec_context->bit_rate = codec_context->rc_max_rate / 90;
            break;
        case QUALITY_AUTO:
        case QUALITY_MEDIUM:
            codec_context->bit_rate = codec_context->rc_max_rate / 20;
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

    qDebug() << filename << QChar(124).toLatin1() << "Encoder" << avcodec_descriptor_get(codec_context->codec_id)->long_name << " ("<< QString(codec->name)  << codec_context->bit_rate / 1024 <<" kbit/s, "<<codec_context->rc_max_rate / 1024 <<" kbit/s max, VBV " << codec_context->rc_buffer_size/8192 << "kbyte)";
}

VideoRecorderH264::VideoRecorderH264(QString filename, int w, int h, int fps, encodingquality quality) : VideoRecorder(filename, w, h, fps)
{
    // specifics for this recorder
    suffix = "mp4";
    description = "MPEG H264 Video (*.mp4)";
    enum AVPixelFormat targetFormat = AV_PIX_FMT_YUV420P;

    // select variable bit rate quality factor (percent)
    unsigned long vbr = 54;   // default to 54% quality, default crf 23
    switch (quality) {
    case QUALITY_LOW:
        vbr = 40;   // crf 30 : quite ugly but not that bad
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
    QStringList codeclist;
    codeclist << "h264_nvenc"<< "libx264"  << "h264_omx"  ;
    setupContext(codeclist, "mp4", targetFormat);

    if ((strcmp(codec->name, "libx264") == 0)) {
        // configure libx264 encoder quality
        // see https://trac.ffmpeg.org/wiki/Encode/H.264
        av_dict_set(&opts, "preset", "ultrafast", 0);
        av_dict_set(&opts, "tune", "zerolatency", 0);

        if (quality==QUALITY_HIGH)
            av_dict_set(&opts, "profile", "high444", 0); // high444p

        // Control x264 encoder quality via CRF
        // The total range is from 0 to 51, where 0 is lossless, 18 can be considered ‘visually lossless’,
        // and 51 is terrible quality. A sane range is 18-26, and the default is 23.
        char crf[10];
        vbr = (int)(( (100-vbr) * 51)/100);
        snprintf(crf, 10, "%d", (int) vbr);
        av_dict_set(&opts, "crf", crf, 0);
    }
    else if ((strcmp(codec->name, "h264_nvenc") == 0)) {
        // configure nvenc_h264 encoder quality
        // see https://superuser.com/questions/1296374/best-settings-for-ffmpeg-with-nvenc
        av_dict_set(&opts, "preset", "3", 0); // fast
        av_dict_set(&opts, "zerolatency", "1", 0); // active

        if (quality==QUALITY_HIGH)
            av_dict_set(&opts, "profile", "3", 0); // high444p

        // encoder quality controlled via quantization parameter
        av_dict_set(&opts, "rc", "0", 0); // Constant QP mode

        // Constant quantization parameter rate control method (from -1 to 51)
        char crf[10];
        vbr = (int)(( (100-vbr) * 51)/100);
        snprintf(crf, 10, "%d", (int) vbr);
        av_dict_set(&opts, "qp", crf, 0);
    }
    else if ((strcmp(codec->name, "h264_omx") == 0)) {
        // H264 OMX encoder quality can only be controlled via bit_rate
        vbr = (width * height * fps * vbr) >> 7;
        // Clip bit rate to min
        if (vbr < 4000) // magic number
            vbr = 4000;
        codec_context->profile = FF_PROFILE_H264_HIGH;
        codec_context->bit_rate = vbr;
    }

    // OPTIONNAL
    codec_context->thread_count = FFMIN(8, std::thread::hardware_concurrency());

    setupFiltering();

    char *buffer = NULL;
    av_dict_get_string(opts, &buffer, '=', ',');

    qDebug() << filename << QChar(124).toLatin1() << "Encoder" << avcodec_descriptor_get(codec_context->codec_id)->long_name << " ( "<< QString(codec->name)  << buffer << vbr << ")";

    av_freep(&buffer);
}


VideoRecorderHEVC::VideoRecorderHEVC(QString filename, int w, int h, int fps, encodingquality quality) : VideoRecorder(filename, w, h, fps)
{
    // specifics for this recorder
    suffix = "mkv";
    description = "High Efficiency Video Codec (*.mkv)";
    enum AVPixelFormat targetFormat = AV_PIX_FMT_YUV420P;
    QStringList codeclist;
    codeclist << "hevc_nvenc"<< "libx265";

    // select variable bit rate quality factor (percent)
    unsigned long vbr = 54;   // default to 54% quality, default crf 23
    switch (quality) {
    case QUALITY_LOW:
        vbr = 40;   // crf 30 : quite ugly but not that bad
        break;
    case QUALITY_MEDIUM:
        vbr = 64;;  // crf 18 : 'visually' lossless
        break;
    case QUALITY_HIGH:
        vbr = 90;   // crf 5 : almost lossless
        codeclist.takeFirst(); // nvenc does not support yuv444
        targetFormat = AV_PIX_FMT_YUV444P;  // higher quality color
        break;
    case QUALITY_AUTO:
        break;
    }

    // allocate context
    setupContext(codeclist, "matroska", targetFormat);

    if ((strcmp(codec->name, "libx265") == 0)) {
        // configure libx264 encoder quality
        // see https://trac.ffmpeg.org/wiki/Encode/H.265
        av_dict_set(&opts, "preset", "ultrafast", 0);
        av_dict_set(&opts, "tune", "zerolatency", 0);

        // Control Constant Rate Factor (CRF)
        // The total range is from 0 to 51, where 0 is lossless, 18 can be considered ‘visually lossless’,
        // and 51 is terrible quality. A sane range is 18-26, and the default is 23.
        char crf[10];
        vbr = (int)(( (100-vbr) * 51)/100);
        snprintf(crf, 10, "%d", (int) vbr);
        av_dict_set(&opts, "crf", crf, 0);
    }
    else if ((strcmp(codec->name, "hevc_nvenc") == 0)) {
        // configure nvenc_h265 encoder quality
        av_dict_set(&opts, "preset", "3", 0); // fast
        av_dict_set(&opts, "zerolatency", "1", 0); // active

        if (quality==QUALITY_HIGH)
            av_dict_set(&opts, "profile", "3", 0); // high444p

        // encoder quality controlled via quantization parameter
        av_dict_set(&opts, "rc", "0", 0); // Constant QP mode

        // Constant quantization parameter rate control method (from -1 to 51)
        char crf[10];
        vbr = (int)(( (100-vbr) * 51)/100);
        snprintf(crf, 10, "%d", (int) vbr);
        av_dict_set(&opts, "qp", crf, 0);
    }

    // OPTIONNAL
    codec_context->thread_count = FFMIN(8, std::thread::hardware_concurrency());

    setupFiltering();

    char *buffer = NULL;
    av_dict_get_string(opts, &buffer, '=', ',');

    qDebug() << filename << QChar(124).toLatin1() << "Encoder" << avcodec_descriptor_get(codec_context->codec_id)->long_name << " ( "<< QString(codec->name)  << buffer << vbr << ")";

    av_freep(&buffer);
}

VideoRecorderFFV::VideoRecorderFFV(QString filename, int w, int h, int fps) : VideoRecorder(filename, w, h, fps)
{
    // specifics for this recorder
    suffix = "avi";
    description = "AVI FF Video (*.avi)";

    // allocate context
    QStringList codeclist;
    codeclist << "ffv1";
    setupContext(codeclist, "avi", AV_PIX_FMT_YUV444P);

    // optimized options
    // see https://trac.ffmpeg.org/wiki/Encode/FFV1
    av_dict_set(&opts, "level", "3", 0);
    av_dict_set(&opts, "coder", "0", 0);
    av_dict_set(&opts, "slices", "24", 0);
    av_dict_set(&opts, "slicecrc", "1", 0);
    av_dict_set(&opts, "threads", "6", 0);
    av_dict_set(&opts, "g", "6", 0);

    setupFiltering();

    qDebug() << filename << QChar(124).toLatin1() << "Encoder" << avcodec_descriptor_get(codec_context->codec_id)->long_name ;
}

VideoRecorderRAW::VideoRecorderRAW(QString filename, int w, int h, int fps) : VideoRecorder(filename, w, h, fps)
{
    // specifics for this recorder
    suffix = "avi";
    description = "AVI Video (*.avi)";

    // allocate context
    QStringList codeclist;
    codeclist << "rawvideo";
    setupContext(codeclist, "avi", AV_PIX_FMT_BGR24);

    setupFiltering();

    qDebug() << filename << QChar(124).toLatin1() << "Encoder" << avcodec_descriptor_get(codec_context->codec_id)->long_name ;
}

VideoRecorderMPEG1::VideoRecorderMPEG1(QString filename, int w, int h, int fps) : VideoRecorder(filename, w, h, fps)
{
    // specifics for this recorder
    suffix = "mpg";
    description = "MPEG Video (*.mpg *.mpeg)";
    enum AVPixelFormat targetFormat = AV_PIX_FMT_YUV420P;

    // allocate context
    QStringList codeclist;
    codeclist << "mpeg1video";
    setupContext(codeclist, "mpeg", targetFormat);

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

    qDebug() << filename << QChar(124).toLatin1() << "Encoder" << avcodec_descriptor_get(codec_context->codec_id)->long_name << " ("<< QString(codec->name)  << codec_context->bit_rate / 1024 <<" kbit/s, buffer "<<props->buffer_size /1024<<" kbytes)";
}

VideoRecorderMPEG2::VideoRecorderMPEG2(QString filename, int w, int h, int fps) : VideoRecorder(filename, w, h, fps)
{
    // specifics for this recorder
    suffix = "mpg";
    description = "MPEG Video (*.mpg *.mpeg)";
    enum AVPixelFormat targetFormat = AV_PIX_FMT_YUV420P;

    // allocate context
    QStringList codeclist;
    codeclist << "mpeg2video";
    setupContext(codeclist, "mpeg", targetFormat);

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

    qDebug() << filename << QChar(124).toLatin1() << "Encoder" << avcodec_descriptor_get(codec_context->codec_id)->long_name << " ("<< QString(codec->name)  << codec_context->bit_rate / 1024 <<" kbit/s, buffer "<<props->buffer_size /1024<<" kbytes)";
}

VideoRecorderWMV::VideoRecorderWMV(QString filename, int w, int h, int fps) : VideoRecorder(filename, w, h, fps)
{
    // specifics for this recorder
    suffix = "wmv";
    description = "Windows Media Video (*.wmv)";
    enum AVPixelFormat targetFormat = AV_PIX_FMT_YUV420P;

    // allocate context
    QStringList codeclist;
    codeclist << "wmv2" << "wmv1";
    setupContext(codeclist, "avi", targetFormat);

    // bit_rate, maxi 25 Mbits/s
    codec_context->bit_rate = FFMIN(width * height * av_get_bits_per_pixel( av_pix_fmt_desc_get(targetFormat)) * frameRate, 25000000);

    // OPTIONNAL
    codec_context->thread_count = FFMIN(8, std::thread::hardware_concurrency());

    // needs filtering
    setupFiltering();

    qDebug() << filename << QChar(124).toLatin1() << "Encoder" << avcodec_descriptor_get(codec_context->codec_id)->long_name << " (" << QString(codec->name)  << codec_context->bit_rate / 1024 <<" kbit/s )";
}

VideoRecorderFLV::VideoRecorderFLV(QString filename, int w, int h, int fps) : VideoRecorder(filename, w, h, fps)
{
    // specifics for this recorder
    suffix = "flv";
    description = "Flash Video (*.flv)";
    enum AVPixelFormat targetFormat = AV_PIX_FMT_YUV420P;

    // allocate context
    QStringList codeclist;
    codeclist << "flv";
    setupContext(codeclist, "flv", targetFormat);

    // bit_rate, maxi 25 Mbits/s
    codec_context->bit_rate = FFMIN(width * height * av_get_bits_per_pixel( av_pix_fmt_desc_get(targetFormat)) * frameRate, 25000000);

    // OPTIONNAL
    codec_context->thread_count = FFMIN(8, std::thread::hardware_concurrency());

    // needs filtering
    setupFiltering();

    qDebug() << filename << QChar(124).toLatin1() << "Encoder" << avcodec_descriptor_get(codec_context->codec_id)->long_name << " (" << QString(codec->name)  << codec_context->bit_rate / 1024 <<" kbit/s )";
}

VideoRecorderWebM::VideoRecorderWebM(QString filename, int w, int h, int fps, encodingquality quality) : VideoRecorder(filename, w, h, fps)
{
    // specifics for this recorder
    suffix = "webm";
    description = "WebM Video (*.webm)";
    enum AVPixelFormat targetFormat = AV_PIX_FMT_YUV420P;

    // select constant quality (CQ) mode
    // The CRF value can be from 0–63. Lower values mean better quality.
    // Recommended values range from 15–35, with 31 being recommended for 1080p HD video
    // see https://developers.google.com/media/vp9/settings/vod/
    int crf = 31;
    switch (quality) {
    case QUALITY_LOW:
        crf = 35;
        break;
    case QUALITY_MEDIUM:
        crf = 25;
        break;
    case QUALITY_HIGH:
        crf = 18;
        targetFormat = AV_PIX_FMT_YUV444P;
        break;
    case QUALITY_AUTO:
        break;
    }

    // allocate context
    QStringList codeclist;
    codeclist  << "libvpx-vp9" << "libvpx";
    setupContext(codeclist, "webm", targetFormat);

    // see https://trac.ffmpeg.org/wiki/Encode/VP9
    // Time to spend encoding, in microseconds. <int> (from INT_MIN to INT_MAX)
    // best / good / realtime (default good)
    av_dict_set( &opts, "deadline", "realtime", 0);
    // Quality/Speed ratio modifier (from -8 to 8) (default 1)
    av_dict_set( &opts, "cpu-used", "8", 0);
    // Select the quality for constant quality mode (from -1 to 63) (default -1)
    char ocrf[10];
    snprintf(ocrf, 10, "%d", crf);
    av_dict_set( &opts, "crf", ocrf, 0);

    // OPTIONNAL
    codec_context->bit_rate = 0; // force not used;
    codec_context->thread_count = FFMIN(8, std::thread::hardware_concurrency());

    // needs filtering
    setupFiltering();

    char *buffer = NULL;
    av_dict_get_string(opts, &buffer, '=', ',');

    qDebug() << filename << QChar(124).toLatin1() << "Encoder" << avcodec_descriptor_get(codec_context->codec_id)->long_name << " ( "<< QString(codec->name) << buffer <<  ")";

    av_freep(&buffer);
}

VideoRecorderProRes::VideoRecorderProRes(QString filename, int w, int h, int fps, encodingquality quality) : VideoRecorder(filename, w, h, fps)
{
    // specifics for this recorder
    suffix = "mov";
    description = "Apple ProRes Video (*.mov)";

    // see https://documentation.apple.com/en/finalcutpro/professionalformatsandworkflows/chapter_10_section_0.html
    // prores profiles : [0 - apco, 1 - apcs, 2 - apcn (default), 3 - apch]
    // 0 Proxy: 'apco' Roughly 30 percent of the data rate of Apple ProRes 422
    // 1 LT: 'apcs' Roughly 70 percent of the data rate of Apple ProRes 422
    // 2 Standard Definition Apple ProRes 422: 'apcn'
    // 3 High Quality: 'apch'

    // select profile
    switch (quality) {
    case QUALITY_LOW:
        av_dict_set( &opts, "profile", "0", 0); // LT
        break;
    case QUALITY_MEDIUM:
        av_dict_set( &opts, "profile", "2", 0); // standard
        break;
    case QUALITY_HIGH:
        av_dict_set( &opts, "profile", "3", 0); // hq
        // CANNOT USE PRORES_KS : TOO SLOW ENCODER
        // see https://ffmpeg.org/ffmpeg-codecs.html#ProRes
//        codeclist << "prores_ks";
//        targetFormat = AV_PIX_FMT_YUV444P10LE;
//        av_dict_set( &opts, "profile", "3", 0); // hq
//        av_dict_set( &opts, "quant_mat", "2", 0); // quantiser matrix : standard
//        // desired bits per macroblock (from 0 to 8192) : the higher the faster
//        av_dict_set( &opts, "bits_per_mb", "8000", 0);

        break;
    case QUALITY_AUTO:
        av_dict_set( &opts, "profile", "1", 0);
        break;
    }

    // allocate context
    QStringList codeclist;
    codeclist << "prores" << "prores_aw";
    setupContext(codeclist, "mov", AV_PIX_FMT_YUV422P10LE);

    // OPTIONNAL
    codec_context->thread_count = FFMIN(8, std::thread::hardware_concurrency());

    // needs filtering
    setupFiltering();


    char *buffer = NULL;
    av_dict_get_string(opts, &buffer, '=', ',');

    qDebug() << filename << QChar(124).toLatin1() << "Encoder" << avcodec_descriptor_get(codec_context->codec_id)->long_name << " ( "<< QString(codec->name) << buffer <<  ")";

    av_freep(&buffer);
}


bool VideoRecorder::addFrame(AVFrame *f)
{
    int retcd = 0;
    char errstr[128];

    if (!format_context)
        VideoRecorderException("Format context unavailable.").raise();

    if (f != NULL ) {

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

        // one more frame !
        framenum++;
    }
    else
        avcodec_send_frame(codec_context, NULL);

    // loop packet encoding
    while ( retcd >= 0 ) {

        AVPacket pkt;
        av_init_packet(&pkt);
        pkt.data = NULL;
        pkt.size = 0;

        // get packet from codec encoder
        retcd =  avcodec_receive_packet(codec_context, &pkt);

        // ignore if temporarily unavailable
        if (retcd == AVERROR(EAGAIN))
            break;

        // done end of buffer
        if (retcd == AVERROR_EOF)
            return false;

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

    return true;
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


void VideoRecorder::setupContext(QStringList codecnames, QString formatname, AVPixelFormat pixelformat)
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

    // find codec encoder
    QStringList listcodecs(codecnames);
    codec=NULL;
    while (!listcodecs.isEmpty()) {
        codec = avcodec_find_encoder_by_name(qPrintable(listcodecs.takeFirst()));
        if (codec != NULL)
            break;
    }
    if (codec==NULL)
        VideoRecorderException( QString("Codec not found (%1)").arg(codecnames.join(", ")) ).raise();

    // create video stream
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
    codec_context->codec_id      = codec->id;
    codec_context->pix_fmt       = pixelformat;
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

    enum AVPixelFormat pix_fmts[] = { codec_context->pix_fmt, AV_PIX_FMT_NONE };
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
