/*
 * VideoStream.cpp
 *
 *  Created on: Sept 12, 2016
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
 *   Copyright 2009, 2016 Bruno Herbelin
 *
 */

#include "VideoStream.moc"
#include "CodecManager.h"

extern "C"
{
#include <libavcodec/avcodec.h>
#include <libswscale/swscale.h>
#include <libavfilter/avfiltergraph.h>
#include <libavfilter/buffersink.h>
#include <libavfilter/buffersrc.h>
#include <libavutil/opt.h>
}

/**
 * Waiting time when update has nothing to do (ms)
 */
#define UPDATE_SLEEP_DELAY 10


class StreamOpeningThread: public videoStreamThread
{
public:
    StreamOpeningThread(VideoStream *video) : videoStreamThread(video)
    {
    }

    ~StreamOpeningThread()
    {
    }

    void run();

};

void StreamOpeningThread::run()
{
    if (!is->openStream())
        emit failed();
    else
        qDebug() << is->urlname << QChar(124).toLatin1() << tr("Connected to stream.");

}


class StreamDecodingThread: public videoStreamThread
{
public:
    StreamDecodingThread(VideoStream *video) : videoStreamThread(video)
    {
        // allocate a frame to fill
        _pFrame = av_frame_alloc();
        Q_CHECK_PTR(_pFrame);

        av_init_packet(&_nullPacket);
        _nullPacket.data = NULL;
        _nullPacket.size = 0;
    }

    ~StreamDecodingThread()
    {
        // free the allocated frame
        av_frame_free(&_pFrame);
    }

    void run();

private:
    AVFrame *_pFrame;
    AVPacket _nullPacket;
};



void StreamDecodingThread::run()
{
    AVPacket pkt1, *pkt   = &pkt1;
    int frameFinished = 0;
    double pts = 0.0; // Presentation time stamp
    int64_t dts = 0; // Decoding time stamp

    //TODO what about testing if stream 'isactive' ?


    while (is && !is->quit && !_forceQuit)
    {
        // start with clean frame
        av_frame_unref(_pFrame);

        /**
         *
         *   PARSING
         *
         * */
        // Read packet
        if ( av_read_frame(is->pFormatCtx, pkt) < 0)
        {
            // if could NOT read full frame, was it an error?
            if (is->pFormatCtx->pb && is->pFormatCtx->pb->error != 0) {
                qDebug() << is->urlname << QChar(124).toLatin1() << QObject::tr("Could not read frame!");

                avio_flush(is->pFormatCtx->pb);
                avformat_flush(is->pFormatCtx);

                // do not treat the error; just wait a bit for the end of the packet and continue
//                msleep(UPDATE_SLEEP_DELAY);
                continue;
            }

            // not really an error : read_frame reached the end of stream
            // send an empty frame with stop flag
            // (and pretending pts is one frame later)
            is->queue_picture(NULL, pts, VideoPicture::ACTION_STOP );

            // and go on to next packet
            msleep(UPDATE_SLEEP_DELAY);
            continue;

        }

        if ( pkt->stream_index != is->videoStream ) {
            // not a picture, go to next packet
            continue;
        }


        /**
         *
         *   DECODING
         *
         * */

        // send the packet to the decoder
        if ( avcodec_send_packet(is->video_dec, pkt) < 0 ) {
            //msleep(PARSING_SLEEP_DELAY);
            continue;
        }

        frameFinished = 0;
        while (frameFinished >= 0) {

            // get the packet from the decoder
            frameFinished = avcodec_receive_frame(is->video_dec, _pFrame);

            // no error, just try again
            if ( frameFinished == AVERROR(EAGAIN) || frameFinished == AVERROR_EOF ) {
                // continue in main loop.
                break;
            }
            // other kind of error
            else if ( frameFinished < 0 ) {
                fprintf(stderr, "\n%s - Could not decode frame.", qPrintable(is->urlname));
                // decoding error
                forceQuit();
                break;
            }

            // No error, but did we get a full video frame?
            VideoPicture::Action actionFrame = VideoPicture::ACTION_SHOW;

            // get packet decompression time stamp (dts)
            dts = 0;
            if (_pFrame->pts != (int64_t) AV_NOPTS_VALUE)
                dts = _pFrame->pts;
            else
                dts = _pFrame->pkt_dts;
            // compute PTS
            pts = double(dts) * av_q2d(is->video_st->time_base);

            // wait until we have space for a new pic
            // the condition is released in video_refresh_timer()
            is->pictq_mutex->lock();
            while ( !is->quit && (is->pictq.count() > is->pictq_max_count) )
                is->pictq_cond->wait(is->pictq_mutex);
            is->pictq_mutex->unlock();

            // default
            // add frame to the queue of pictures
            is->queue_picture(_pFrame, pts, actionFrame);

            // free internal buffers
            av_frame_unref(_pFrame);

        } // end while (frameFinished > 0)


        // free internal buffers
        av_packet_unref(pkt);

    } // end while

    // free internal buffers
    av_frame_unref(_pFrame);

#ifdef VIDEOSTREAM_DEBUG
        qDebug() << is->urlname << QChar(124).toLatin1() << tr("Decoding ended.");
#endif

}


VideoStream::VideoStream(QObject *parent, int destinationWidth, int destinationHeight) :
    QObject(parent),
    targetWidth(destinationWidth), targetHeight(destinationHeight)
{
    // first time a video file is created?
    CodecManager::registerAll();

    // Init some pointers to NULL
    videoStream = -1;
    video_st = NULL;
    pFormatCtx = NULL;
    video_dec = NULL;
    in_video_filter = NULL;
    out_video_filter = NULL;
    graph = NULL;
    pictq_max_count = PICTUREMAP_SIZE - 1;

    // Contruct some objects
    decod_tid = new StreamDecodingThread(this);
    Q_CHECK_PTR(decod_tid);
    QObject::connect(decod_tid, SIGNAL(failed()), this, SIGNAL(failed()));
    QObject::connect(decod_tid, SIGNAL(finished()), this, SLOT(onStop()));
    pictq_mutex = new QMutex;
    Q_CHECK_PTR(pictq_mutex);
    pictq_cond = new QWaitCondition;
    Q_CHECK_PTR(pictq_cond);
    open_tid = new StreamOpeningThread(this);
    Q_CHECK_PTR(open_tid);
    // forward signals
    QObject::connect(open_tid, SIGNAL(failed()), this, SIGNAL(failed()));
    QObject::connect(open_tid, SIGNAL(finished()), this, SIGNAL(openned()));

    // start playing as soon as it is openned
    QObject::connect(open_tid, SIGNAL(finished()), this, SLOT(start()));

    ptimer = new QTimer(this);
    Q_CHECK_PTR(ptimer);
    ptimer->setSingleShot(true);
    QObject::connect(ptimer, SIGNAL(timeout()), this, SLOT(video_refresh_timer()));

    // reset
    urlname = QString::null;
    codecname = QString::null;
    quit = true; // not running yet
}

VideoStream::~VideoStream()
{
    // make sure all is closed
    close();

    QObject::disconnect(this, 0, 0, 0);

    // delete threads
    if (open_tid->isRunning()) {
        QObject::disconnect(open_tid, 0, 0, 0);
        QObject::connect(open_tid, SIGNAL(terminated()), open_tid, SLOT(deleteLater()));
        open_tid->terminate();
    }
    else
        delete open_tid;

    if (decod_tid->isRunning()) {
        QObject::disconnect(decod_tid, 0, 0, 0);
        QObject::connect(decod_tid, SIGNAL(terminated()), decod_tid, SLOT(deleteLater()));
        decod_tid->terminate();
    }
    else
        delete decod_tid;

    delete pictq_mutex;
    delete pictq_cond;
    delete ptimer;

    // empty queue
    while (!pictq.isEmpty()) {
        VideoPicture *p = pictq.dequeue();
        delete p;
    }
}



void VideoStream::onStop()
{
    emit running(false);
}

void VideoStream::stop()
{
    if (decod_tid->isRunning())
    {
#ifdef VIDEOSTREAM_DEBUG
        qDebug() << urlname << QChar(124).toLatin1() << tr("Stopping.");
#endif

        // request quit
        quit = true;

        // stop play
//        if (pFormatCtx) {
//            av_read_pause(pFormatCtx);
//            avformat_flush(pFormatCtx);
//        }

        pictq_mutex->lock();
        // unlock all conditions
        pictq_cond->wakeAll();
        // wait for thread to end
        decod_tid->wait(100);
        pictq_mutex->unlock();

#ifdef VIDEOSTREAM_DEBUG
        qDebug() << urlname << QChar(124).toLatin1() << tr("Stopped.");
#endif
    }

}


void VideoStream::start()
{
    if ( !isOpen() )
        return;

    if (!decod_tid->isRunning())
    {
#ifdef VIDEOSTREAM_DEBUG
        qDebug() << urlname << QChar(124).toLatin1() << tr("Starting.");
#endif

        // reset quit flag
        quit = false;

        // start play
        if (pFormatCtx) {
            avformat_flush(pFormatCtx);
//            av_read_play(pFormatCtx);
        }

        // start timer and decoding threads
        ptimer->start();
        decod_tid->start();

        /* say if we are running */
        emit running(true);

#ifdef VIDEOSTREAM_DEBUG
        qDebug() << urlname << QChar(124).toLatin1() << tr("Started.");
#endif
    }

}

void VideoStream::play(bool startorstop)
{
//    // clear the picture queue
//    flush_picture_queue();
//    active = startorstop;

    flush_picture_queue();

    if (startorstop)
        start();
    else
        stop();
}


bool VideoStream::isOpen() const {
    return (pFormatCtx != NULL);
}

void VideoStream::open(QString url)
{
    if (pFormatCtx)
        close();

    // store url
    urlname = url;

    // request opening of thread in open thread
    qDebug() << urlname << QChar(124).toLatin1() << tr("Connecting to stream...");
    open_tid->start();

    // not running yet
    quit = true;
}

bool VideoStream::openStream()
{
    // re-open if alredy openned
    if (pFormatCtx)
        close();
    pFormatCtx = NULL;
    video_st = NULL;
    videoStream = -1;

    pFormatCtx = avformat_alloc_context();
    if ( !CodecManager::openFormatContext( &pFormatCtx, urlname) ){
        close();
        return false;
    }

    // get index of video stream
    AVCodec *codec;
    videoStream = av_find_best_stream(pFormatCtx, AVMEDIA_TYPE_VIDEO, -1, -1, &codec, 0);
    if (videoStream < 0) {
        // Cannot find video stream
        qWarning() << urlname << QChar(124).toLatin1()<< tr("Cannot find video stream.");
        close();
        return false;
    }

    // all ok, we can set the internal pointers to the good values
    video_st = pFormatCtx->streams[videoStream];

    // create video decoding context
    video_dec = avcodec_alloc_context3(codec);
    if (!video_dec) {
        CodecManager::printError(urlname, "Error creating decoder :", AVERROR(ENOMEM));
        // close
        close();
        return false;
    }

    int err = avcodec_parameters_to_context(video_dec, video_st->codecpar);
    if (err < 0) {
        CodecManager::printError(urlname, "Error configuring :", err);
        // close
        close();
        return false;
    }

    // options for decoder
    video_dec->workaround_bugs   = 1;
    video_dec->idct_algo         = FF_IDCT_AUTO;
    video_dec->skip_frame        = AVDISCARD_DEFAULT;
    video_dec->skip_idct         = AVDISCARD_DEFAULT;
    video_dec->skip_loop_filter  = AVDISCARD_DEFAULT;
    video_dec->error_concealment = 3;

    // set options for video decoder
    AVDictionary *opts = NULL;
    av_dict_set(&opts, "refcounted_frames", "1", 0);
    av_dict_set(&opts, "threads", "auto", 0);
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

    // set picture size : no argument means use picture size from video stream
    if (targetWidth == 0)
        targetWidth = video_st->codecpar->width;
    if (targetHeight == 0)
        targetHeight = video_st->codecpar->height;

    if (targetWidth == 0 || targetHeight == 0)
    {
        // Cannot initialize the conversion context!
        qWarning() << urlname << QChar(124).toLatin1()<< tr("Cannot read stream.");
        return false;
    }

    // Default targetFormat to PIX_FMT_RGB24
    targetFormat = AV_PIX_FMT_RGB24;

    // setup filtering
    if ( !setupFiltering() ) {
        // close file
        close();
        return false;
    }

    if (pFormatCtx) {
        av_read_play(pFormatCtx);
    }

    // all ok
    return true;
}

bool VideoStream::setupFiltering()
{
    // create conversion context
    const AVFilter *buffersrc  = avfilter_get_by_name("buffer");
    const AVFilter *buffersink = avfilter_get_by_name("buffersink");
    AVFilterInOut *outputs = avfilter_inout_alloc();
    AVFilterInOut *inputs  = avfilter_inout_alloc();

    graph = avfilter_graph_alloc();
    if (!outputs || !inputs || !graph) {
        qWarning() << urlname << QChar(124).toLatin1()
                   << tr("Cannot create conversion filter.");
        return false;
    }

    int64_t conversionAlgorithm = SWS_POINT; // optimal speed scaling for videos

    char sws_flags_str[128];
    snprintf(sws_flags_str, sizeof(sws_flags_str), "flags=%" PRId64, conversionAlgorithm);
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
        qWarning() << urlname << QChar(124).toLatin1()
                   << tr("Cannot create a INPUT conversion context.");
        return false;
    }

    // OUTPUT SINK
    if ( avfilter_graph_create_filter(&out_video_filter, buffersink,
                                      "out", NULL, NULL, graph) < 0)  {
        qWarning() << urlname << QChar(124).toLatin1()
                   << tr("Cannot create a OUTPUT conversion context.");
        return false;
    }

    enum AVPixelFormat pix_fmts[] = { targetFormat, AV_PIX_FMT_NONE };
    if ( av_opt_set_int_list(out_video_filter, "pix_fmts", pix_fmts,
                             AV_PIX_FMT_NONE, AV_OPT_SEARCH_CHILDREN) < 0 ){
        qWarning() << urlname << QChar(124).toLatin1()
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

    if ( avfilter_graph_parse_ptr(graph, filter_str, &inputs, &outputs, NULL) < 0 ){
        qWarning() << urlname << QChar(124).toLatin1()<< tr("Cannot parse filters.");
        return false;
    }

    // validate the filtering graph
    if ( avfilter_graph_config(graph, NULL) < 0){
        qWarning() << urlname << QChar(124).toLatin1()<< tr("Cannot configure conversion graph.");
        return false;
    }

    avfilter_inout_free(&inputs);
    avfilter_inout_free(&outputs);

    return true;
}


void VideoStream::close()
{
#ifdef VIDEOSTREAM_DEBUG
    qDebug() << urlname << QChar(124).toLatin1() << tr("Closing...");
#endif

    // Stop thread
    stop();

    // flush
    flush_picture_queue();

    // free filter
    if (graph)
        avfilter_graph_free(&graph);

    // free decoder
    if (video_dec)
        avcodec_free_context(&video_dec);

    // close & free format context
    if (pFormatCtx) {
        avformat_flush(pFormatCtx);
        // close file & free context and all its contents and set it to NULL.
        avformat_close_input(&pFormatCtx);
    }

    // reset pointers
    pFormatCtx = NULL;
    video_dec = NULL;
    graph = NULL;
    in_video_filter = NULL;
    out_video_filter = NULL;
    video_st = NULL;

    qDebug() << urlname << QChar(124).toLatin1() << tr("Stream closed.");
}


void VideoStream::video_refresh_timer()
{
    // by default timer will be restarted ASAP
    int ptimer_delay = UPDATE_SLEEP_DELAY;
    // by default do not quit
    bool quit_after_frame = false;
    // empty pointers
    VideoPicture *currentvp = NULL, *nextvp = NULL;

    // lock the thread to operate on the queue
    pictq_mutex->lock();

    // if all is in order, deal with the picture in the queue
    // (i.e. there is a stream, there is a picture in the queue
    if (!quit && video_st && pictq.size()>1 )
//        if (video_st && !pictq.empty())
    {
        // now working on the head of the queue, that we take off the queue
        currentvp = pictq.dequeue();

        // remember it if there is a next picture
        if (!pictq.empty())
            nextvp = pictq.head();

        // unblock the queue for the decoding thread
        // by informing it about the new size of the queue
        pictq_cond->wakeAll();
    }
    // release lock
    pictq_mutex->unlock();


    if (currentvp)
    {

//        fprintf(stderr, "video_refresh_timer pts %f \n", current_frame_pts);

        // if this frame was tagged as stopping frame
        if ( currentvp->hasAction(VideoPicture::ACTION_STOP) ) {
            // request to stop the video after this frame
            quit_after_frame = true;
        }

        // this frame is tagged to be displayed
        if ( currentvp->hasAction(VideoPicture::ACTION_SHOW) ) {

            // ask to show the current picture (and to delete it when done)
            currentvp->addAction(VideoPicture::ACTION_DELETE);
            emit frameReady(currentvp);

//              fprintf(stderr, "                         Display picture pts %f queue size %d\n", currentvp->getPts(), pictq.size());

            // before computing timer delay, set to default
            ptimer_delay = UPDATE_SLEEP_DELAY;

            // if there is a next picture
            // we can compute when to present the next frame
            if (nextvp) {

                double delay = 0.0;
                delay = nextvp->getPts() - currentvp->getPts() ;

                // if delay is correct
                if ( delay > 0 && delay < 1.0)
                    // schedule normal delayed display of next frame
                    ptimer_delay = (int) (delay * 1000.0);
            }

        }
        // NOT VISIBLE ? skip this frame...
        else {
            // delete the picture
            delete currentvp;
       }

    }

    // quit if requested
    if (quit_after_frame)
        stop();
    // normal behavior : restart the ptimer for next frame
    else
        ptimer->start( ptimer_delay );


//    fprintf(stderr, "video_refresh_timer update in %d \n", ptimer_delay);
}

void VideoStream::flush_picture_queue()
{
    pictq_mutex->lock();
    while (!pictq.isEmpty()) {
        VideoPicture *p = pictq.dequeue();
        delete p;
    }

    pictq_cond->wakeAll();
    pictq_mutex->unlock();
}

// called exclusively in Decoding Thread
void VideoStream::queue_picture(AVFrame *pFrame, double pts, VideoPicture::Action a)
{
    VideoPicture *vp = NULL;

    try {
        // convert
        if ( pFrame && av_buffersrc_add_frame_flags(in_video_filter, pFrame, AV_BUFFERSRC_FLAG_KEEP_REF) >= 0 )
        {
            // create vp as the picture in the queue to be written
            vp = new VideoPicture(out_video_filter, pts);
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
//    fprintf(stderr, "pictq size %d \n", pictq.size());

}







