/*
 * VideoFile.cpp
 *
 *  Created on: Jul 10, 2009
 *      Author: bh
 *
 *  This file is part of GLMixer.
 *
 *   GLMixer is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation, either version 3 of the License, or
 *   (at your option) any later version.
 *
 *   GLMixer is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with GLMixer.  If not, see <http://www.gnu.org/licenses/>.
 *
 *   Copyright 2009, 2018 Bruno Herbelin
 *
 */

//#include <stdint.h>

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

#include "VideoFile.moc"

#include "CodecManager.h"

#include <QtGui/QButtonGroup>
#include <QtGui/QDialog>
#include <QtGui/QDialogButtonBox>
#include <QtGui/QHeaderView>
#include <QtGui/QTreeWidget>
#include <QtGui/QLabel>
#include <QtGui/QVBoxLayout>
#include <QtGui/QPlainTextEdit>
#include <QThread>
#include <QDebug>
#include <QFileInfo>
#include <QDir>
#include <QDate>

/**
 * During parsing, the thread sleep for a little
 * in case there is an error or nothing to do (ms).
 */
#define PARSING_SLEEP_DELAY 100
/**
 * Waiting time when update has nothing to do (ms)
 */
#define UPDATE_SLEEP_DELAY 5
/**
 * Waiting timout when trying to acquire lock
 */
#define LOCKING_TIMEOUT 500

// memory policy
#define MIN_VIDEO_PICTURE_QUEUE_COUNT 3
#define MAX_VIDEO_PICTURE_QUEUE_COUNT 100
int VideoFile::memory_usage_policy = DEFAULT_MEMORY_USAGE_POLICY;
int VideoFile::maximum_video_picture_queue_size = MIN_VIDEO_PICTURE_QUEUE_SIZE;



void VideoFile::play(bool startorstop)
{
    if (startorstop)
        start();
    else
        stop();
}

void VideoFile::setPlaySpeedFactor(int s)
{
    // exponential scale of speed
    // 0 % is x 0.1 speed (1/5)
    // 100% is x 1.0
    // 200% is x 10.0
    if ( s != getPlaySpeedFactor() ){

        setPlaySpeed( exp( double(s -100) / 43.42 ) );
        emit playSpeedFactorChanged(s);
    }
}

int VideoFile::getPlaySpeedFactor()
{
    return (int) ( log(getPlaySpeed()) * 43.42 + 100.0  );
}

double VideoFile::getPlaySpeed()
{
    return play_speed;
}

void VideoFile::setPlaySpeed(double s)
{
    // change the picture queue size according to play speed
    // this is because, in principle, more frames are skipped when play faster
    // and we empty the queue faster
    //    double sizeq = qBound(2.0, (double) nb_frames * SEEK_STEP + 1.0, (double) MAX_VIDEO_PICTURE_QUEUE_SIZE);
    //    sizeq *= s;
    //    pictq_max_count = qBound(2, (int) sizeq, (int) nb_frames);

    play_speed = s;
    pclock->setSpeed( play_speed );
    emit playSpeedChanged( play_speed );
}


/**
 * *
 * *  NEW IMPLEMENTATION FOR RECENT LIBAV CODEC > 57
 * *
 * *
 * */

class DecodingThread: public videoFileThread
{
public:
    DecodingThread(VideoFile *video) : videoFileThread(video)
    {
        // allocate a frame to fill
        _pFrame = av_frame_alloc();
        Q_CHECK_PTR(_pFrame);
        _tmpFrame = av_frame_alloc();
        Q_CHECK_PTR(_tmpFrame);
    }
    ~DecodingThread()
    {
        // free the allocated frame
        av_frame_free(&_pFrame);
        av_frame_free(&_tmpFrame);
    }

    void run();

private:
    AVFrame *_pFrame, *_tmpFrame;
};



VideoFile::VideoFile(QObject *parent, bool generatePowerOfTwo,
                     int destinationWidth, int destinationHeight) :
    QObject(parent), filename(QString()), powerOfTwo(generatePowerOfTwo),
    targetWidth(destinationWidth), targetHeight(destinationHeight)
{
    // first time a video file is created?
    CodecManager::registerAll();

    // Init some pointers to NULL
    videoStream = -1;
    video_st = NULL;
    pFormatCtx = NULL;
    video_dec = NULL;
    graph = NULL;
    in_video_filter = NULL;
    out_video_filter = NULL;
    firstPicture = NULL;
    blackPicture = NULL;
    pictq_max_count = 0;
    duration = 0.0;
    frame_rate = 0.0;
    nb_frames = 0;
    mark_in = 0.0;
    mark_out = 0.0;
    mark_stop = 0.0;
    play_speed = 1.0;
    targetFormat = AV_PIX_FMT_RGB24;

    // Contruct some objects
    decod_tid = new DecodingThread(this);
    Q_CHECK_PTR(decod_tid);
    QObject::connect(decod_tid, SIGNAL(failed()), this, SIGNAL(failed()));
    pictq_mutex = new QMutex;
    Q_CHECK_PTR(pictq_mutex);
    pictq_cond = new QWaitCondition;
    Q_CHECK_PTR(pictq_cond);
    seek_mutex = new QMutex;
    Q_CHECK_PTR(seek_mutex);
    seek_cond = new QWaitCondition;
    Q_CHECK_PTR(seek_cond);

    ptimer = new QTimer(this);
    Q_CHECK_PTR(ptimer);
    ptimer->setSingleShot(true);
    QObject::connect(ptimer, SIGNAL(timeout()), this, SLOT(video_refresh_timer()));

    // initialize behavior
    filename = QString::null;
    codecname = QString::null;
    first_picture_changed = true; // no mark_in set
    loop_video = true; // loop by default
    restart_where_stopped = true; // by default restart where stopped
    stop_to_black = false;
    ignoreAlpha = false; // by default do not ignore alpha channel
    hasHwCodec = false;  // by default do not use hardware codec
    pHardwareCodec = NULL;

    // initialize clock control
    pclock = new VideoClock(this);
    Q_CHECK_PTR(pclock);
    smooth_pause_animation = new QPropertyAnimation(pclock, "speed");
    Q_CHECK_PTR(smooth_pause_animation);
    smooth_pause_animation->setDuration(100);

    // reset
    quit = true; // not running yet
    reset();
}


void VideoFile::close()
{
#ifdef VIDEOFILE_DEBUG
    fprintf(stderr, "\n%s - Closing Media...", qPrintable(filename));
#endif

    // Stop playing
    stop();

    // free filter
    if (graph)
        avfilter_graph_free(&graph);

    // free decoder
    if (video_dec)
        avcodec_free_context(&video_dec);

#if LIBAVCODEC_VERSION_INT > AV_VERSION_INT(58,0,0)
    if (pHardwareCodec)
        av_buffer_unref(&pHardwareCodec);
#endif

    // close & free format context
    if (pFormatCtx) {
        avformat_flush(pFormatCtx);
        // close file & free context and all its contents and set it to NULL.
        avformat_close_input(&pFormatCtx);
    }

    // free pictures
    if (firstPicture)
        delete firstPicture;
    if (blackPicture)
        delete blackPicture;

    // reset pointers
    pFormatCtx = NULL;
    video_dec = NULL;
    graph = NULL;
    in_video_filter = NULL;
    out_video_filter = NULL;
    video_st = NULL;
    blackPicture = NULL;
    firstPicture = NULL;

    qDebug() << filename << QChar(124).toLatin1() << tr("Media closed.");
#ifdef VIDEOFILE_DEBUG
    fprintf(stderr, "\n%s - Media closed.", qPrintable(filename));
#endif
}

VideoFile::~VideoFile()
{
    // end silently
    QObject::disconnect(this, 0, 0, 0);
    // just to make sure stop() will not fill first frame
    restart_where_stopped = true;

    // make sure all is closed
    close();
    clear_picture_queue();

    // delete threads
    delete decod_tid;
    delete pictq_mutex;
    delete pictq_cond;
    delete seek_mutex;
    delete seek_cond;
    delete ptimer;
    delete pclock;
    delete smooth_pause_animation;

#ifdef VIDEOFILE_DEBUG
    fprintf(stderr, "\n%s - Media deleted.", qPrintable(filename));
#endif
}

void VideoFile::reset()
{
    // reset variables to 0
    current_frame_pts = 0.0;
    video_pts = 0.0;
    seek_pos = 0.0;
    parsing_mode = VideoFile::SEEKING_NONE;
    fast_forward = false;

    // if reset after openning, reset clock and indicate time base
    if (video_st && frame_rate > 0)
        pclock->reset(0.0, 1.0 / frame_rate);
}

void VideoFile::stop()
{
    if (!quit)
    {
#ifdef VIDEOFILE_DEBUG
        fprintf(stderr, "\n%s - Stopping video...", qPrintable(filename));
#endif

        // request quit
        quit = true;

        // remember where we are for next restart
        mark_stop = getCurrentFrameTime();

        // unlock all conditions
        pictq_cond->wakeAll();
        seek_cond->wakeAll();
        decod_tid->wait();

        if (!restart_where_stopped)
        {
            // recreate first picture in case begin has changed
            current_frame_pts = fill_first_frame(true);
            emit frameReady( firstPicture );
            emit timeChanged( current_frame_pts );
        }

        if (stop_to_black)
            emit frameReady( blackPicture );

        ptimer->stop();

#ifdef VIDEOFILE_DEBUG
        fprintf(stderr, "\n%s - Stopped.", qPrintable(filename));
#endif
    }

    /* say if we are running or not */
    emit running(!quit);
}

void VideoFile::start()
{
    if ( !isOpen() )
        return;

    // nothing to play if there is ONE frame only...
    if ( getNumFrames() < 2)
        return;

    if (quit)
    {
#ifdef VIDEOFILE_DEBUG
        fprintf(stderr, "\n%s - Starting .", qPrintable(filename));
#endif
        // reset internal state
        reset();

        // reset quit flag
        quit = false;

        // restart at beginning
        seek_pos = mark_in;

        // except restart where we where (if valid mark)
        if (restart_where_stopped && mark_stop < (mark_out - getFrameDuration()) && mark_stop > mark_in)
            seek_pos =  mark_stop;

        current_frame_pts = seek_pos;

        // request parsing thread to perform seek
        parsing_mode = VideoFile::SEEKING_PARSING;

        // start timer and decoding threads
        ptimer->start();
        decod_tid->start();

#ifdef VIDEOFILE_DEBUG
        fprintf(stderr, "\n%s - Started.", qPrintable(filename));
#endif
    }

    /* say if we are running or not */
    emit running(!quit);
}



int VideoFile::getNumFrames() const {
    return nb_frames;
}

bool VideoFile::isOpen() const {
    return (pFormatCtx != NULL);
}



bool VideoFile::open(QString file, bool tryHardwareCodec, bool ignoreAlphaChannel, double markIn, double markOut)
{
    // re-open if alredy openned
    if (pFormatCtx)
        close();
    quit = true;

    filename = file;

    // allocate context
    pFormatCtx = avformat_alloc_context();
    //Flags modifying the (de)muxer behaviour.  Set by the user before avformat_open_input().
    pFormatCtx->flags |= AVFMT_FLAG_GENPTS; //Generate missing pts even if it requires parsing future frames.
    if ( !CodecManager::openFormatContext( &pFormatCtx, filename) ) {
        // close
        close();
        return false;
    }

    // get index of video stream
    AVCodec *codec;
    videoStream = av_find_best_stream(pFormatCtx, AVMEDIA_TYPE_VIDEO, -1, -1, &codec, 0);
    if (videoStream < 0) {
        // Cannot find video stream
        qWarning() << filename << QChar(124).toLatin1() << tr("Cannot find video stream.");
        close();
        return false;
    }

    // keep reference to video stream
    video_st = pFormatCtx->streams[videoStream];
    nb_frames = video_st->nb_frames;

    // if video decoding context not already created (i.e. no hardware accelerated)
    if (!video_dec) {
        // create video decoding context with default codec
        video_dec = avcodec_alloc_context3(codec);
    }

    // in any case, complain if no decoding context could be created
    if (!video_dec) {
        CodecManager::printError(filename, "Error creating decoder :", AVERROR(ENOMEM));
        // close
        close();
        return false;
    }

    int err = avcodec_parameters_to_context(video_dec, video_st->codecpar);
    if (err < 0) {
        CodecManager::printError(filename, "Error configuring :", err);
        // close
        close();
        return false;
    }

    // reset hardware capability
    hasHwCodec = false;

#if LIBAVCODEC_VERSION_INT > AV_VERSION_INT(58,0,0)
    pHardwareCodec = NULL;

    // test hardware acceleration
    const AVCodecHWConfig *hwconfig = CodecManager::getCodecHardwareAcceleration(codec);
    if ( hwconfig && nb_frames > 2) {
         // this codec has hardware acceleration capabilities
         hasHwCodec = true;
     }

    // try to use hardware acceleration
    if (tryHardwareCodec && hasHwCodec) {

        pHardwareCodec = CodecManager::applyCodecHardwareAcceleration(video_dec, hwconfig);
        if ( pHardwareCodec != NULL ) {

            qDebug() << filename << QChar(124).toLatin1()
                     <<  tr("Using Codec %1 %2 hardware acceleration.").arg(codec->name).arg(av_hwdevice_get_type_name(hwconfig->device_type));

        }
        else
            qWarning() << filename << QChar(124).toLatin1()
                     <<  tr("Failed to use %2 hardware acceleration for %1 codec.").arg(codec->name).arg(av_hwdevice_get_type_name(hwconfig->device_type));

    }
#endif

    // options for decoder
    video_dec->workaround_bugs   = FF_BUG_AUTODETECT;
    video_dec->idct_algo         = FF_IDCT_AUTO;
    video_dec->skip_frame        = AVDISCARD_DEFAULT;
    video_dec->skip_idct         = AVDISCARD_DEFAULT;
    video_dec->skip_loop_filter  = AVDISCARD_DEFAULT;
    video_dec->error_concealment = FF_EC_GUESS_MVS | FF_EC_DEBLOCK;
    video_dec->flags2           |= AV_CODEC_FLAG2_FAST;

    // get the duration and frame rate of the video stream
    frame_rate = CodecManager::getFrameRateStream(pFormatCtx, videoStream);
    duration = CodecManager::getDurationStream(pFormatCtx, videoStream);

    // make sure the numbers match !
    if (nb_frames == (int64_t) AV_NOPTS_VALUE || nb_frames < 1 )
        nb_frames =  (int64_t) ( duration * frame_rate );

    // set options for video decoder
    AVDictionary *opts = NULL;
    av_dict_set(&opts, "refcounted_frames", "1", 0);
    // do not use auto threads for single image files
    av_dict_set(&opts, "threads", nb_frames < 2 ? "1" : "auto", 0);

    // init the video decoder
    if ( avcodec_open2(video_dec, codec, &opts) < 0 ) {
        qWarning() << avcodec_descriptor_get(video_dec->codec_id)->long_name
                   << QChar(124).toLatin1() << tr("Unsupported Codec.");
        close();
        av_dict_free(&opts);
        return false;
    }
    av_dict_free(&opts);

    // store name of codec
    codecname = avcodec_descriptor_get(video_dec->codec_id)->long_name;

    // special case of GIF : they do not indicate duration
    if (video_dec->codec_id == AV_CODEC_ID_GIF) {
        nb_frames = CodecManager::countFrames(pFormatCtx, videoStream, video_dec);
        duration = nb_frames / frame_rate;
    }

    // check the parameters for mark in and out and setup marking accordingly
    if (markIn < 0 || nb_frames < 2)
        mark_in = getBegin(); // default to start of file
    else
    {
        mark_in = qBound(getBegin(), markIn, getEnd());
        emit markInChanged(mark_in);
    }

    if (markOut <= 0 || nb_frames < 2)
        mark_out = getEnd(); // default to end of file
    else
    {
        mark_out = qBound(mark_in, markOut, getEnd());
        emit markOutChanged(mark_out);
    }

    // set picture size : no argument means use picture size from video stream
    if (targetWidth == 0)
        targetWidth = video_st->codecpar->width;
    if (targetHeight == 0)
        targetHeight = video_st->codecpar->height;

    // round target picture size to power of two dimensions if requested
    if (powerOfTwo)
        CodecManager::convertSizePowerOfTwo(targetWidth, targetHeight);

    // Default targetFormat to PIX_FMT_RGB24
    targetFormat = AV_PIX_FMT_RGB24;

    // Change target format to keep Alpha channel if format requires it
    ignoreAlpha = ignoreAlphaChannel;
    if ( hasAlphaChannel() && !ignoreAlpha )
        targetFormat = AV_PIX_FMT_RGBA;

    // setup filtering
    if ( !setupFiltering() ) {
        // close file
        close();
        return false;
    }

    // we need a picture to display when not playing (also for single frame media)
    // create firstPicture (and get actual pts of first picture)
    // (NB : seek in stream only if not reading the first frame)
    first_picture_changed = true;
    current_frame_pts = fill_first_frame( mark_in != getBegin() );

    // make sure the first picture was filled
    if (!firstPicture) {
        qWarning() << filename << QChar(124).toLatin1()<< tr("Could not create first picture.");
        close();
        return false;
    }

    mark_stop = mark_in;

    // For videos only
    if (nb_frames > 1) {

        // we may need a black frame to return to when stopped
        blackPicture = new VideoPicture(targetWidth, targetHeight);
        blackPicture->addAction(VideoPicture::ACTION_SHOW | VideoPicture::ACTION_RESET_PTS);

        // set picture queue maximum size
        recompute_max_count_picture_queue();

        // tells everybody we are set !
        qDebug() << filename << QChar(124).toLatin1()
                 <<  tr("Media opened (%1 frames, buffer of %2 MB for %3 %4 frames).").arg(nb_frames).arg((float) (pictq_max_count * firstPicture->getBufferSize()) / (float) MEGABYTE, 0, 'f', 1).arg( pictq_max_count).arg(CodecManager::getPixelFormatName(targetFormat));

    }
    else {
        qDebug() << filename << QChar(124).toLatin1()
                 <<  tr("Media opened (1 %1 frame).").arg(CodecManager::getPixelFormatName(targetFormat));
    }

    // display a firstPicture frame ; this shows that the video is open
    emit frameReady( firstPicture );

    // say we are not running
    emit running(false);

    // all ok
    return true;
}

bool VideoFile::setupFiltering()
{
    // create conversion context
    const AVFilter *buffersrc  = avfilter_get_by_name("buffer");
    const AVFilter *buffersink = avfilter_get_by_name("buffersink");
    AVFilterInOut *outputs = avfilter_inout_alloc();
    AVFilterInOut *inputs  = avfilter_inout_alloc();

    graph = avfilter_graph_alloc();
    if (!outputs || !inputs || !graph) {
        qWarning() << filename << QChar(124).toLatin1()
                   << tr("Cannot create conversion filter.");
        return false;
    }

    // Decide for optimal scaling algo
    // NB: the algo is used only if the conversion is scaled or with filter
    // (i.e. optimal 'unscaled' converter is used by default)
    int64_t conversionAlgorithm = SWS_POINT; // optimal speed scaling for videos
    if ( nb_frames < 2 )
        conversionAlgorithm = SWS_LANCZOS; // optimal quality scaling for 1 frame sources (images)

    char sws_flags_str[128];
    snprintf(sws_flags_str, sizeof(sws_flags_str), "flags=%d", (int) conversionAlgorithm);
    graph->scale_sws_opts = av_strdup(sws_flags_str);

    // INPUT BUFFER
    char buffersrc_args[256];
    snprintf(buffersrc_args, sizeof(buffersrc_args),
             "video_size=%dx%d:pix_fmt=%d:time_base=%d/%d:pixel_aspect=%d/%d",
             video_dec->width, video_dec->height, video_dec->pix_fmt,
             video_st->time_base.num, video_st->time_base.den,
             video_dec->sample_aspect_ratio.num, video_dec->sample_aspect_ratio.den);

    if ( avfilter_graph_create_filter(&in_video_filter, buffersrc,
                                      "in", buffersrc_args, NULL, graph) < 0)  {
        qWarning() << filename << QChar(124).toLatin1()
                   << tr("Cannot create a INPUT conversion context.");
        return false;
    }

    // OUTPUT SINK
    if ( avfilter_graph_create_filter(&out_video_filter, buffersink,
                                      "out", NULL, NULL, graph) < 0)  {
        qWarning() << filename << QChar(124).toLatin1()
                   << tr("Cannot create a OUTPUT conversion context.");
        return false;
    }

    enum AVPixelFormat pix_fmts[] = { targetFormat, AV_PIX_FMT_NONE };
    if ( av_opt_set_int_list(out_video_filter, "pix_fmts", pix_fmts,
                             AV_PIX_FMT_NONE, AV_OPT_SEARCH_CHILDREN) < 0 ){
        qWarning() << filename << QChar(124).toLatin1()
                   << tr("Cannot set output pixel format");
        return false;
    }

    // create another filter
    inputs->name       = av_strdup("out");
    inputs->filter_ctx = out_video_filter;
    inputs->pad_idx    = 0;
    inputs->next       = NULL;
    outputs->name       = av_strdup("in");
    outputs->filter_ctx = in_video_filter;
    outputs->pad_idx    = 0;
    outputs->next       = NULL;

    // performs scaling to target size if necessary
    char filter_str[128];
    if ( targetWidth != video_dec->width || targetHeight != video_dec->height)
        snprintf(filter_str, sizeof(filter_str), "scale=w=%d:h=%d", targetWidth, targetHeight);
    else
        // null filter does nothing
        snprintf(filter_str, sizeof(filter_str), "null");

    //    // TODO : Deinterlacing filter if frame->interlaced_frame
    // : append yadif to filter "yadif=1:-1:0"

    if ( avfilter_graph_parse_ptr(graph, filter_str, &inputs, &outputs, NULL) < 0 ){
        qWarning() << filename << QChar(124).toLatin1()
                   << tr("Cannot parse filters.");
        return false;
    }

    // validate the filtering graph
    if ( avfilter_graph_config(graph, NULL) < 0){
        qWarning() << filename << QChar(124).toLatin1()
                   << tr("Cannot configure conversion graph.");
        return false;
    }

    avfilter_inout_free(&inputs);
    avfilter_inout_free(&outputs);

    return true;
}

bool VideoFile::hasAlphaChannel() const
{
    if (!video_st)
        return false;

    return CodecManager::pixelFormatHasAlphaChannel(video_dec->pix_fmt);
}

bool VideoFile::useHardwareCodec() const {

#if LIBAVCODEC_VERSION_INT > AV_VERSION_INT(58,0,0)
    return pHardwareCodec != NULL;
#else
    return false;
#endif

}


double VideoFile::fill_first_frame(bool seek)
{
    int retcd = 0;
    char errstr[128];

    if (!first_picture_changed)
        return mark_in;

    if (firstPicture)
        delete firstPicture;
    firstPicture = NULL;

    AVPacket pkt1, *pkt = &pkt1;
    AVFrame *tmpframe = av_frame_alloc();
    AVFrame *hwframe = av_frame_alloc();

    bool frameFilled = false;
    double pts = mark_in;
    int trial = 0;


    if (seek) {
        int64_t seek_target = AV_NOPTS_VALUE;
        seek_target = av_rescale_q(mark_in, (AVRational){1, 1}, video_st->time_base);
#ifdef VIDEOFILE_DEBUG
            fprintf(stderr, "\n%s - fill_first_frame seek to %d.", qPrintable(filename), (int) seek_target);
#endif
        // seek back to begining
        av_seek_frame(pFormatCtx, videoStream, seek_target, AVSEEK_FLAG_BACKWARD);
        // flush decoder
        avcodec_flush_buffers(video_dec);
    }

    // loop while we didn't finish the frame, or looped for too long
    while (!frameFilled && trial < 500 )
    {
        // unreference buffers
        av_frame_unref(tmpframe);
        trial++;

        // read a packet
        retcd = av_read_frame(pFormatCtx, pkt);
        if ( retcd < 0 ) {
#ifdef VIDEOFILE_DEBUG
            fprintf(stderr, "\n%s - Error read frame (%s).", qPrintable(filename), av_make_error_string(errstr, sizeof(errstr),retcd));
#endif
            continue;
        }

        // ignore non-video stream packets
        if (pkt->stream_index != videoStream)
            continue;

        if ( avcodec_send_packet(video_dec, pkt) < 0 ){
#ifdef VIDEOFILE_DEBUG
            fprintf(stderr, "\n%s - Error send packet (%s).", qPrintable(filename), av_make_error_string(errstr, sizeof(errstr),retcd));
#endif
            continue;
        }

        // read until the frame is finished
        int frameFinished = AVERROR(EAGAIN);;
        while ( frameFinished < 0 )
        {
            // read frame
            frameFinished = avcodec_receive_frame(video_dec, tmpframe);

            if ( frameFinished == AVERROR(EAGAIN) )
                break;
            else if ( frameFinished == AVERROR_EOF ||  frameFinished < 0 ) {
#ifdef VIDEOFILE_DEBUG
                fprintf(stderr, "\n%s - Error receive frame (%s).", qPrintable(filename), av_make_error_string(errstr, sizeof(errstr),retcd));
#endif
                trial = 499;
                break;
            }

            // got the frame
            frameFilled = true;

            // try to get a pts from the packet
            if (tmpframe->pts != AV_NOPTS_VALUE)
                pts = double(tmpframe->pts) * av_q2d(video_st->time_base);
            else if (pkt->dts != AV_NOPTS_VALUE)
                pts =  double(tmpframe->pkt_dts) * av_q2d(video_st->time_base);

            // if the obtained pts is before seeking mark,
            // read forward until reaching the mark_in (target)
            if (seek && pts < mark_in)
                frameFilled = false; // retry

        }

        // free packet
        av_packet_unref(pkt);

    }

    // we got the frame
    if (frameFilled) {

        // reference the tmp frame filled
        AVFrame *frame = tmpframe;

#if LIBAVCODEC_VERSION_INT > AV_VERSION_INT(58,0,0)
        if ( pHardwareCodec ) {
            /* retrieve data from GPU to CPU */
            if ( av_hwframe_transfer_data(hwframe, tmpframe, 0) >= 0) {
                // reference the hardware frame
                frame = hwframe;
            }
            // error
            else {
                fprintf(stderr, "Error transferring the data to system memory\n");
                frame = NULL;
            }
        }
#endif

        // convert it and copy content of frame
        if ( frame && av_buffersrc_add_frame_flags(in_video_filter, frame, AV_BUFFERSRC_FLAG_KEEP_REF) >= 0 ) {

            try {
                // create the picture
                firstPicture = new VideoPicture(out_video_filter, pts);
                firstPicture->addAction(VideoPicture::ACTION_SHOW | VideoPicture::ACTION_RESET_PTS);

            } catch (AllocationException &e){
                qWarning() << tr("Cannot create picture; ") << e.message();
                frameFilled = false;
            }
        }
        else
            frameFilled = false;

    }

    if (!frameFilled)
        qWarning() << filename << QChar(124).toLatin1() << tr("Could not create first frame.") << trial;
#ifdef VIDEOFILE_DEBUG
    else
        fprintf(stderr, "\n%s - First frame updated (%d).", qPrintable(filename), trial);
#endif


    av_frame_free(&tmpframe);
    av_frame_free(&hwframe);
//    avcodec_flush_buffers(video_dec);

    first_picture_changed = false;

    return pts;
}


void VideoFile::video_refresh_timer()
{
    // by default timer will be restarted ASAP
    int ptimer_delay = UPDATE_SLEEP_DELAY;
    // by default do not quit
    bool quit_after_frame = false;
    // empty pointers
    VideoPicture *currentvp = NULL, *nextvp = NULL;

    // lock the thread to operate on the queue
    if ( pictq_mutex->tryLock(LOCKING_TIMEOUT) )
    {

        // if all is in order, deal with the picture in the queue
        // (i.e. there is a stream, there is a picture in the queue, and the clock is not paused)
        // NB: if paused BUT the first pict in the queue is tagged for ACTION_RESET_PTS, then still proceed
        if (video_st && !pictq.empty() && (!pclock->paused() ||  pictq.head()->hasAction(VideoPicture::ACTION_RESET_PTS) ) )
        {

            // now working on the head of the queue, that we take off the queue
            currentvp = pictq.dequeue();

            // remember it if there is a next picture
            if (!pictq.empty())
                nextvp = pictq.head();

//            fprintf(stderr, "video_refresh_timer current %f has next %d \n", currentvp->getPts(), nextvp ? 1 : 0);

            // unblock the queue for the decoding thread
            // by informing it about the new size of the queue
            pictq_cond->wakeAll();
        }

        // release lock
        pictq_mutex->unlock();
    }
    // NB: if either of the above failed, currentvp is still null and we just retry

    if (currentvp)
    {
        // deal with speed change before setting up the frame
        pclock->applyRequestedSpeed();

        // store time of this current frame
        current_frame_pts =  currentvp->getPts();

//                fprintf(stderr, "video_refresh_timer pts %f time %f \n", current_frame_pts, _videoClock.time());

        // if this frame was tagged as stopping frame
        if ( currentvp->hasAction(VideoPicture::ACTION_STOP) ) {
            // request to stop the video after this frame
            quit_after_frame = true;
        }

        // this frame was tagged to reset the timer (seeking frame usually)
        if ( currentvp->hasAction(VideoPicture::ACTION_RESET_PTS) ) {
            // reset clock to the time of the frame
            pclock->reset(current_frame_pts);
            // inform that seeking is done
            emit seekEnabled(true);
        }

        // this frame is tagged to be displayed
        if ( currentvp->hasAction(VideoPicture::ACTION_SHOW) ) {

            // ask to show the current picture (and to delete it when done)
            currentvp->addAction(VideoPicture::ACTION_DELETE);
            emit frameReady(currentvp);
            emit timeChanged(current_frame_pts);

            //            currentvp->saveToPPM(tr("%1.ppm").arg(currentvp->getPts()));

            // if there is a next picture
            // we can compute when to present the next frame
            if (nextvp) {
                double delay = 0.0;

                // if next frame we will be seeking
                if ( nextvp->hasAction(VideoPicture::ACTION_RESET_PTS) )
                    // update at normal fps, discarding computing of delay
                    delay = pclock->timeBase();
                else
                    // otherwise read presentation time and compute delay till next frame
                    delay = ( nextvp->getPts() - pclock->time() ) / pclock->speed() ;

                // if delay is correct
                if ( delay > pclock->minFrameDelay() ) {
                    // schedule normal delayed display of next frame
                    ptimer_delay = (int) (delay * 1000.0);

                    // delay is too small, or negative
                } else {

                    // retry shortly (but not too fast to avoid glitches)
                    ptimer_delay = (int) (pclock->minFrameDelay() * 1000.0);

                    // remove the show tag for that frame (i.e. skip it)
                    nextvp->removeAction(VideoPicture::ACTION_SHOW);
                }

//                fprintf(stderr, "                         NEXT    picture pts %f delay %d\n", nextvp->getPts(), ptimer_delay);

            }
            else {

//                _videoClock.reset(current_frame_pts);
                ptimer_delay = (int) (pclock->timeBase() * 1000.0);

            }
        }
        // NOT VISIBLE ? skip this frame...
        else {
            // delete the picture
            delete currentvp;
        }

        if (fast_forward) {
            pclock->reset(current_frame_pts);
            ptimer_delay = UPDATE_SLEEP_DELAY;
        }

    }

    // quit if requested
    if (quit_after_frame)
        stop();
    // normal behavior : restart the ptimer for next frame
    else
        ptimer->start( ptimer_delay );

//        fprintf(stderr, "video_refresh_timer update in %d \n", ptimer_delay);
}

double VideoFile::getCurrentFrameTime() const
{
    return current_frame_pts;
}


double VideoFile::getFrameDuration() const
{
    if (frame_rate > 0.0)
        return 1.0 / frame_rate;
    return 0.0;
}

void VideoFile::setOptionRevertToBlackWhenStop(bool on)
{
    stop_to_black = on;

    if (quit && stop_to_black)
        emit frameReady( blackPicture );
}


void VideoFile::setOptionRestartToMarkIn(bool on)
{
    restart_where_stopped = !on;

}

void VideoFile::seekToPosition(double t)
{
    if (pFormatCtx && video_st && !quit)
    {

        t = qBound(getBegin(), t, getEnd());

        //  try to jump to the frame in the picture queue
        if (  !jump_in_picture_queue(t) )  {
            // if couln't seek within picture queue
            // request to seek in the parser.

            // need to unlock and flush the picture queue
            flush_picture_queue();

            // request seek with lock
            requestSeek(t, true);
        }

        emit seekEnabled(false);

        // if paused, unpause for 1 frame
        if ( pclock->paused() )
            seekForwardOneFrame();
    }
}

void VideoFile::seekBySeconds(double seekStep)
{
    double position = current_frame_pts + seekStep;

    // if the video is in loop mode
    if (loop_video) {
        // bound and loop the seeking position inside the [mark_in mark_out]
        if (position > mark_out)
            position = mark_in + (position - mark_out);
        else if (position < mark_in)
            position = mark_out - (mark_in - position);
    }
    else {
        // only bound the seek position inside the [mark_in mark_out]
        position = qBound(mark_in, position, mark_out);
    }

    // call seeking to the computed position
    seekToPosition( position );

}

void VideoFile::seekForwardOneFrame()
{
    if (!pictq.isEmpty())
        // tag this frame as a RESET frame ; this enforces its processing in video_refresh_timer
        pictq.head()->addAction(VideoPicture::ACTION_RESET_PTS);
}

double VideoFile::getBegin() const
{
    if (video_st && video_st->start_time != (int64_t) AV_NOPTS_VALUE )
        return double(video_st->start_time) * av_q2d(video_st->time_base);

    if (pFormatCtx && pFormatCtx->start_time != (int64_t) AV_NOPTS_VALUE )
        return double(pFormatCtx->start_time) * av_q2d(AV_TIME_BASE_Q);

    return 0.0;
}

double VideoFile::getEnd() const
{
    return getBegin() + getDuration();
}

int VideoFile::getStreamFrameWidth() const
{
    if (video_dec)
        return video_dec->width;
    else
        return targetWidth;
}

int VideoFile::getStreamFrameHeight() const
{
    if (video_dec)
        return video_dec->height;
    else
        return targetHeight;
}

void VideoFile::clean_until_time_picture_queue(double time) {

    if (pictq.empty())
        return;

    if (time < getBegin())
        time = getEnd();

    // get hold on the picture queue
    pictq_mutex->lock();

    // loop to find a mark frame in the queue
    int i =  0;
    while ( i < pictq.size()
            && !pictq[i]->hasAction(VideoPicture::ACTION_MARK)
            && pictq[i]->getPts() < time)
        i++;

    // found a mark frame in the queue
    if ( pictq.size() > i ) {
        // remove all what is after
        while ( pictq.size() > i )
            delete pictq.takeLast();

        // restart filling in at the last pts of the cleanned queue
        if (! pictq.empty()) // sanity check (but should never be the case)
        {
            requestSeek( pictq.last()->getPts() );
        }
    }

    // done with the cleanup
    pictq_mutex->unlock();

}

void VideoFile::setLoop(bool loop) {

    loop_video = loop;

    // Cleanup the queue until the next mark it contains
    clean_until_time_picture_queue();

}

void VideoFile::recompute_max_count_picture_queue()
{
    // the number of frames allowed in order to fit into the maximum picture queue size (in MB)
    int max_count = (int) ( (float) (VideoFile::maximum_video_picture_queue_size * MEGABYTE) / (float) firstPicture->getBufferSize() );

    // limit this maximum number of frames within the number of frames into the [begin end] interval
    max_count = qMin(max_count, 1 + (int) ((mark_out - mark_in) * getFrameRate() ));

    // bound the max count within the [MIN_VIDEO_PICTURE_QUEUE_COUNT MAX_VIDEO_PICTURE_QUEUE_COUNT] interval
    pictq_max_count = qBound( MIN_VIDEO_PICTURE_QUEUE_COUNT, max_count, MAX_VIDEO_PICTURE_QUEUE_COUNT );
}

void VideoFile::setMarkIn(double time)
{
    // set the mark in
    // reserve at least 1 frame interval with mark out
    mark_in = qBound(getBegin(), time, mark_out - getFrameDuration());

    // if requested mark_in is after current time
    if ( !(mark_in < current_frame_pts) )
        // seek to mark in
        seekToPosition(mark_in);
    // else mark in is before the current time, but the queue might already have looped
    else
        // Cleanup the queue until the next mark it contains
        clean_until_time_picture_queue();

    // inform about change in marks
    emit markInChanged(mark_in);

    // change the picture queue size to match the new interval
    recompute_max_count_picture_queue();

    // remember that we have to update the first picture
    first_picture_changed = true;
}



void VideoFile::setMarkOut(double time)
{
    // set the mark_out
    // reserve at least 1 frame interval with mark in
    mark_out = qBound(mark_in + getFrameDuration(), time, getEnd());

    // if requested mark_out is before current time
    if ( !(current_frame_pts + getFrameDuration() < mark_out) ) {
       // react according to loop mode
       if (loop_video)
           // seek to mark in
           seekToPosition(mark_in);
       else
           stop();
    }
    // else mark out is after the current time, but the queue might already have looped
    else
        // Cleanup the queue until the new mark_out
        clean_until_time_picture_queue(mark_out);

    // inform about change in marks
    emit markOutChanged(mark_out);

    // charnge the picture queue size to match the new interval
    recompute_max_count_picture_queue();
}

bool VideoFile::time_in_picture_queue(double time)
{
    // obvious answer
    if (pictq.empty())
        return false;

    // Is there a picture with that time into the queue ?
    // (first in queue is the head to be displayed first, last is the last one enqueued)
    // Normal case : the queue is contiguous, and the first is less than the last
    // Loop case : the queue is looping after mark out to mark in : this cause the first to be after the last

    // does the queue loop ?
    bool loopingbuffer = pictq.first()->getPts() > pictq.last()->getPts();

    // either in normal case the given time is between first and last frames
    return ( (!loopingbuffer && ( time > pictq.first()->getPts() && time < pictq.last()->getPts()) )
             // or, in loop case, the given time is before mark out
             || ( loopingbuffer && (time > pictq.first()->getPts() && time < mark_out) )
             // or, in loop case, the given time passed the mark out, back to mark in
             || ( loopingbuffer &&  (time > mark_in && time < pictq.last()->getPts()) )   );

}


double VideoFile::getStreamAspectRatio() const
{
    // read information from the video stream if avaialble
    if (video_dec) {

        // base computation of aspect ratio
        double aspect_ratio = (double) video_dec->width / (double) video_dec->height;

        // use correction of aspect ratio if available in the video stream
        if (video_dec->sample_aspect_ratio.num > 1)
            aspect_ratio *= av_q2d(video_dec->sample_aspect_ratio);

        return aspect_ratio;
    }

    // default case
    if (video_st) {
        return (double) video_st->codecpar->width / (double) video_st->codecpar->height;
    }

    return 1.0;
}


void VideoFile::clear_picture_queue() {

#ifdef VIDEOFILE_DEBUG
    fprintf(stderr, "\n%s - Clear Picture queue N = %d.", qPrintable(filename), pictq.size());
#endif

    while (!pictq.isEmpty()) {
        VideoPicture *p = pictq.dequeue();
        delete p;
    }
}

void VideoFile::flush_picture_queue()
{
    pictq_mutex->lock();
    clear_picture_queue();
    pictq_cond->wakeAll();
    pictq_mutex->unlock();
}


void VideoFile::requestSeek(double time, bool lock)
{
    if ( parsing_mode != VideoFile::SEEKING_PARSING )
    {

        seek_mutex->lock();
        seek_pos = time;

        parsing_mode = VideoFile::SEEKING_PARSING;
        if (lock)
            // wait for the thread to aknowledge the seek request
            seek_cond->wait(seek_mutex);
        seek_mutex->unlock();

    }
}


bool VideoFile::jump_in_picture_queue(double time)
{
    // discard too small seek (return true to indicate the jump is already effective)
    if ( qAbs( time - current_frame_pts ) < qAbs( getDuration() / (double) nb_frames)  )
        return true;

    // Is there a picture with the seeked time into the queue ?
    if ( ! time_in_picture_queue(time)  )
        // no we cannot seek into decoder picture queue
        return false;

    // now for sure the seek time is in the queue
    // get hold on the picture queue

    pictq_mutex->lock();

    // does the queue loop ?
    bool loopingbuffer = pictq.first()->getPts() > pictq.last()->getPts();
    // in this case, remove all till mark out
    if ( loopingbuffer &&  (time > mark_in && time < pictq.last()->getPts()) ) {
        // remove all frames till the mark out, ie every frame above the last
        while ( pictq.size() > 1 && pictq.first()->getPts() > pictq.last()->getPts() ) {
            VideoPicture *p = pictq.dequeue();
            delete p;
        }
    }

    // remove all frames before the time of seek (except if queue is empty)
    while ( pictq.size() > 1 && time > pictq.first()->getPts() ){
        VideoPicture *p = pictq.dequeue();
        delete p;
    }

    // mark this frame for reset of time
    pictq.first()->addAction(VideoPicture::ACTION_RESET_PTS);

    // inform about the new size of the queue
    pictq_cond->wakeAll();
    pictq_mutex->unlock();

    // yes we could seek in decoder picture queue
    return true;

}

// called exclusively in Decoding Thread
void VideoFile::queue_picture(AVFrame *pFrame, double pts, VideoPicture::Action a)
{
    VideoPicture *vp = NULL;

    try {
        // convert given frame
        if ( pFrame && av_buffersrc_add_frame_flags(in_video_filter, pFrame, AV_BUFFERSRC_FLAG_KEEP_REF) >= 0 ) {

            // create VP containing an AVFrame created by the buffer sink
            vp = new VideoPicture(out_video_filter, pts);

//            // alternative implementation (slower), copy the data of the frame into a VP
//            if (  av_buffersink_get_frame(out_video_filter, pFrame) >= 0 )
//                vp = new VideoPicture(pFrame, pts);

        }
        else
            vp = new VideoPicture(targetWidth, targetHeight, pts);

        // set the actions of this frame
        vp->resetAction();
        vp->addAction(a);

        /* now we inform our display thread that we have a pic ready */
        pictq_mutex->lock();
        // enqueue this picture in the queue
        pictq.enqueue(vp);
        // inform about the new size of the queue
        pictq_mutex->unlock();

    } catch (AllocationException &e){
        qWarning() << tr("Cannot queue picture; ") << e.message();
    }

}

/**
 * compute the exact PTS for the picture if it is omitted in the stream
 * @param pts1 the dts of the pkt / pts of the frame
 */
double VideoFile::synchronize_video(AVFrame *src_frame, double dts)
{
    double pts = dts;
    double frame_delay = av_q2d(video_dec->time_base);

    if (pts < 0)
        /* if we aren't given a pts, set it to the clock */
        // this happens rarely (I noticed it on last frame, or in GIF files)
        pts = video_pts;
    else
        /* if we have dts, set video clock to it */
        video_pts = pts;

    /* for MPEG2, the frame can be repeated, so we update the clock accordingly */
    frame_delay +=  (double) src_frame->repeat_pict * (frame_delay * 0.5);

    /* update the video clock */
    video_pts += frame_delay;

    return pts;
}

/**
 * DecodingThread
 */


void DecodingThread::run()
{
    AVPacket pkt1, *pkt   = &pkt1;
    int frameFinished = 0;
    double pts = 0.0; // Presentation time stamp
    int64_t intdts = 0; // Decoding time stamp
    int64_t previous_intpts = 0; // Previous decoding time stamp (checking for continuity)
    int error_count = 0;
    bool eof = false;
    AVFrame *frame = NULL;

    while (is && !is->quit && !_forceQuit)
    {
        // start with clean frame
        av_frame_unref(_pFrame);
        av_frame_unref(_tmpFrame);

        eof = false;
        /**
         *
         *   PARSING
         *
         * */

        // seek stuff goes here
        int64_t seek_target = AV_NOPTS_VALUE;
        is->seek_mutex->lock();
        if (is->parsing_mode == VideoFile::SEEKING_PARSING) {
            // compute dts of seek target from seek position
            seek_target = av_rescale_q(is->seek_pos, (AVRational){1, 1}, is->video_st->time_base);

        }
        is->seek_cond->wakeAll();
        is->seek_mutex->unlock();

        // decided to perform seek
        if (seek_target != AV_NOPTS_VALUE)
        {
            // request seek to libav
            // seek BACK to make sure we will not overshoot
            // (frames before the seek position will be discarded when decoding)

            if (av_seek_frame(is->pFormatCtx, is->videoStream, seek_target, AVSEEK_FLAG_BACKWARD) < 0) {
                qDebug() << is->filename << QChar(124).toLatin1()
                         << QObject::tr("Could not seek to frame (%1).").arg(is->seek_pos);
            }

            // todo replace by avformat_seek_file

            previous_intpts = seek_target;

            // flush buffers after seek
            avcodec_flush_buffers(is->video_dec);

            // enter the decoding seeking mode (disabled only when target reached)
            is->parsing_mode = VideoFile::SEEKING_DECODING;
        }


        // Read packet
        int ret = av_read_frame(is->pFormatCtx, pkt);
        if ( ret < 0 )
        {
            // not an error : read_frame have reached the end of file
            if ( ret == AVERROR_EOF || (is->pFormatCtx->pb && is->pFormatCtx->pb->eof_reached) )  {
                eof = true;
#ifdef VIDEOFILE_DEBUG
            fprintf(stderr, "\n%s - EOF packet.", qPrintable(is->filename));
#endif
            }
            // if could NOT read full frame, was it an error?
            if (is->pFormatCtx->pb && is->pFormatCtx->pb->error) {
#ifdef VIDEOFILE_DEBUG
                fprintf(stderr, "\n%s - Error reading frame.", qPrintable(is->filename));
#endif
                // forget error
                avio_flush(is->pFormatCtx->pb);

                error_count++;
                if (error_count < 10) {
                    // do not treat the error; just wait a bit for the end of the packet and continue
                    msleep(PARSING_SLEEP_DELAY);
                    continue;
                }

                // recurrent decoding error
                forceQuit();
                break;
            }
        }

        // we have a packet is it a video packets?
        if ( pkt->stream_index == is->videoStream ) {

            // send the packet to the decoder
            if ( avcodec_send_packet(is->video_dec, pkt) < 0 ) {
#ifdef VIDEOFILE_DEBUG
                fprintf(stderr, "\n%s - Could not send packet.", qPrintable(is->filename));
#endif
                //msleep(PARSING_SLEEP_DELAY);
                continue;
            }

            // read all pending frames
            frameFinished = 0;
            while (frameFinished >= 0) {

                // get the packet from the decoder
                frameFinished = avcodec_receive_frame(is->video_dec, _pFrame);

                // no error, just try again
                if ( frameFinished == AVERROR(EAGAIN) ) {
                    // continue in main loop.
                    break;
                }

                // reached end of file ?
                else if ( frameFinished == AVERROR_EOF ) {
#ifdef VIDEOFILE_DEBUG
                    fprintf(stderr, "\n%s - Decoded End of File.", qPrintable(is->filename));
#endif
                    eof = true;
                    break;
                }
                // other kind of error
                else if ( frameFinished < 0 ) {
#ifdef VIDEOFILE_DEBUG
                    fprintf(stderr, "\n%s - Could not decode frame.", qPrintable(is->filename));
#endif
                    error_count++;
                    if (error_count < 10)
                        continue;

                    // recurrent decoding error
                    forceQuit();
                    break;
                }

                // change var name
                frame = _pFrame;

#if LIBAVCODEC_VERSION_INT > AV_VERSION_INT(58,0,0)
                if ( is->pHardwareCodec ) {
                    /* retrieve data from GPU to CPU */
                    if ( av_hwframe_transfer_data(_tmpFrame, _pFrame, 0) < 0) {
#ifdef VIDEOFILE_DEBUG
                        fprintf(stderr, "\n%s - Error transferring the data to system memory.", qPrintable(is->filename));
#endif
                        error_count++;
                        if (error_count < 10)
                            continue;

                        // recurrent decoding error
                        break;
                    }
                    else
                        // all ok, use hw decoded frame
                        frame = _tmpFrame;
                }
#endif

                // by default, a frame will be displayed
                VideoPicture::Action actionFrame = VideoPicture::ACTION_SHOW;

                // get packet decompression time stamp (dts)
                intdts = 0;
                if (frame->pts != AV_NOPTS_VALUE)
                    intdts = frame->pts; // good case
                else if (frame->pkt_dts != AV_NOPTS_VALUE)
                    // bad case
                    intdts = frame->pkt_dts;
                else
                    // increment if no valid pts given
                    intdts = previous_intpts + pkt->duration;

                // remember previous dts
                previous_intpts = intdts;

                // this frame is the first of the stream
                if (intdts==0)
                    // (the ACTION_RESET_PTS in video refresh timer will reset the clock)
                    actionFrame |= VideoPicture::ACTION_RESET_PTS;

                // compute presentation time stamp
                pts = is->synchronize_video(frame, double(intdts) * av_q2d(is->video_st->time_base));

//                fprintf(stderr, "intdts = %d   pts = %f   %d", (int)intdts, pts, (int)pkt->duration);

                // ?? WHY ? it is in ffplay...
                if (is->video_st->sample_aspect_ratio.num) {
                    frame->sample_aspect_ratio = is->video_st->sample_aspect_ratio;
                }

                // if seeking in decoded frames
                if (is->parsing_mode == VideoFile::SEEKING_DECODING) {

                    // Skip pFrame if it didn't reach the seeking position
                    // Stop seeking when seeked time is reached
                    if ( !(pts < is->seek_pos) ) {

                        // this frame is the result of the seeking process
                        // (the ACTION_RESET_PTS in video refresh timer will reset the clock)
                        actionFrame |= VideoPicture::ACTION_RESET_PTS;

                        // reached the seeked frame! : can say we are not seeking anymore
                        is->seek_mutex->lock();
                        is->parsing_mode = VideoFile::SEEKING_NONE;
                        is->seek_mutex->unlock();

                        // if the seek position we reached equals the mark_in
                        if ( qAbs( is->seek_pos - is->mark_in ) < is->getFrameDuration() )
                            // tag the frame as a MARK frame
                            actionFrame |= VideoPicture::ACTION_MARK;

                    }
                }

                // if not seeking, queue picture for display
                // (not else of previous if because it could have unblocked this frame)
                if (is->parsing_mode == VideoFile::SEEKING_NONE)
                {
                    // wait until we have space for a new pic
                    // to add a picture in the queue
                    // (the condition is released in video_refresh_timer() )
                    is->pictq_mutex->lock();
                    while ( !is->quit && (is->pictq.count() > is->pictq_max_count) )
                        is->pictq_cond->wait(is->pictq_mutex);
                    is->pictq_mutex->unlock();

                    // ignore frame if seek has been asked while waiting
                    // (appens when user asks for seek)
                    if (is->parsing_mode != VideoFile::SEEKING_NONE)
                        continue;

                    // test the end of file
                    double lastpts = is->duration;
                    if (eof && is->video_st->last_dts_for_order_check > 0) {
                        // almost at end of file : will be when reaching last dts computed
                        eof = false;
                        lastpts = (double) (is->video_st->last_dts_for_order_check) * av_q2d(is->video_st->time_base);
                    }
                    else {
                        // no end of file detected :
                        // will be reached when at last frame dts (1 frame before duration)
                        lastpts -= is->getFrameDuration() ;
                    }

                    // test if time will exceed one of the limits
                    if ( !(pts < is->mark_out) || !(pts < is->getEnd() ) || !(pts < lastpts)  )
                    {

                        // react according to loop mode
                        if ( is->loop_video ) {
                            // if loop mode on, request seek to begin
                            eof = true;
                        }
                        else {
                            // if loop mode off, stop after this frame
                            actionFrame |= VideoPicture::ACTION_STOP | VideoPicture::ACTION_MARK;
                            pts = is->mark_out;
                        }
                    }

                    // add frame to the queue of pictures
                    is->queue_picture(frame, pts, actionFrame);
//                    fprintf(stderr, "queue pic  pts = %f   ", pts);
                }

                // clean frame
                av_frame_unref(_pFrame);
                av_frame_unref(_tmpFrame);

            } // end while (frameFinished)

        } // end if (is->videoStream)

        // End of file detected and not handled as last video image
        if (eof) {

            is->requestSeek(is->mark_in);
            previous_intpts = 0;

            // react according to loop mode
            if ( !is->loop_video )
                // if stopping, send an empty frame with stop flag
                // (and pretending pts is one frame later)
                is->queue_picture(NULL, is->duration, VideoPicture::ACTION_STOP | VideoPicture::ACTION_MARK);
        }

        // free internal buffers
        av_packet_unref(pkt);

    } // end while

    // clean frame
    av_frame_unref(_pFrame);
    av_frame_unref(_tmpFrame);

    // if normal exit
    if (is) {

        // clear the picture queue
        is->clear_picture_queue();

        if (_forceQuit) {
            qWarning() << is->filename << QChar(124).toLatin1() << tr("Decoding interrupted unexpectedly.");
            emit failed();
        }
#ifdef VIDEOFILE_DEBUG
        else
            fprintf(stderr, "\n%s - Decoding ended.", qPrintable(is->filename));
#endif

    }

}


void VideoFile::suspend()
{
    pclock->pause(true);
    emit paused(true);
}

void VideoFile::resume()
{
    pclock->pause(false);
    emit paused(false);
}

void VideoFile::pause(bool pause, PauseMode mode, int duration)
{
    if (!quit && pause != pclock->paused() )
    {

        // cancel all animation
        if (smooth_pause_animation->state() == QAbstractAnimation::Running )
            smooth_pause_animation->stop();

        // using smoothing
        if (mode > PAUSE_INSTANTANEOUS && duration > 20) {

            // set duration
            smooth_pause_animation->setDuration(duration);
            // init to current speed

            // smooth suspend¨
            if (pause)
            {
                // suspend at the end
                QObject::connect(smooth_pause_animation, SIGNAL(finished()), this, SLOT(suspend()) );

                // set mode
                switch (mode) {
                case PAUSE_QUARTIC:
                    smooth_pause_animation->setEasingCurve(QEasingCurve::OutQuart);
                    break;
                case PAUSE_EXPONENTIAL:
                    smooth_pause_animation->setEasingCurve(QEasingCurve::OutExpo);
                    break;
                case PAUSE_CIRCULAR:
                    smooth_pause_animation->setEasingCurve(QEasingCurve::OutCirc);
                    break;
                default:
                case PAUSE_LINEAR:
                    smooth_pause_animation->setEasingCurve(QEasingCurve::Linear);
                    break;
                }

                // set targets
                smooth_pause_animation->setStartValue( pclock->speed() );
                smooth_pause_animation->setEndValue( 0.04 );

            }
            // smooth resume
            else {

                // do not suspend at end
                QObject::disconnect(smooth_pause_animation, SIGNAL(finished()), this, SLOT(suspend()) );

                // set mode
                switch (mode) {
                case PAUSE_QUARTIC:
                    smooth_pause_animation->setEasingCurve(QEasingCurve::InQuart);
                    break;
                case PAUSE_EXPONENTIAL:
                    smooth_pause_animation->setEasingCurve(QEasingCurve::InExpo);
                    break;
                case PAUSE_CIRCULAR:
                    smooth_pause_animation->setEasingCurve(QEasingCurve::InCirc);
                    break;
                default:
                case PAUSE_LINEAR:
                    smooth_pause_animation->setEasingCurve(QEasingCurve::Linear);
                    break;
                }

                // set targets
                smooth_pause_animation->setStartValue( 0.04 );
                smooth_pause_animation->setEndValue( play_speed );

                // start playing
                resume();
            }

            // initiate animation
            smooth_pause_animation->start();

        }
        // not using smoothing
        else {

            pclock->pause(pause);
            pclock->setSpeed(play_speed);
            emit paused(pause);
        }

    }
}

bool VideoFile::isPaused() const {
    return pclock->paused();
}

void VideoFile::setMemoryUsagePolicy(int percent)
{
    VideoFile::memory_usage_policy = percent;
    double p = qBound(0.0, (double) percent / 100.0, 1.0);
    VideoFile::maximum_video_picture_queue_size = MIN_VIDEO_PICTURE_QUEUE_SIZE + (int)( p * (MAX_VIDEO_PICTURE_QUEUE_SIZE - MIN_VIDEO_PICTURE_QUEUE_SIZE));

}

int VideoFile::getMemoryUsagePolicy()
{
    return VideoFile::memory_usage_policy;
}

int VideoFile::getMemoryUsageMaximum(int policy)
{
    double p = qBound(0.0, (double) policy / 100.0, 1.0);
    int max = MIN_VIDEO_PICTURE_QUEUE_SIZE + (int)( p * (MAX_VIDEO_PICTURE_QUEUE_SIZE - MIN_VIDEO_PICTURE_QUEUE_SIZE));

    return max;
}

QString VideoFile::getPixelFormatName() const
{
    QString pfn = "Invalid";

    if (video_st)
        pfn = CodecManager::getPixelFormatName(video_dec->pix_fmt);

    return pfn;
}

