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
 *   Copyright 2009, 2012 Bruno Herbelin
 *
 */

#include <stdint.h>

#ifndef INT64_MIN
#define INT64_MIN       (-0x7fffffffffffffffLL-1)
#endif

#ifndef INT64_MAX
#define INT64_MAX INT64_C(9223372036854775807)
#endif

extern "C"
{
#include <libavcodec/avcodec.h>
#include <libswscale/swscale.h>
#include <libavutil/mathematics.h>
#include <libavutil/common.h>
#if LIBAVCODEC_VERSION_INT > AV_VERSION_INT(52,30,0)
#include <libavutil/pixdesc.h>
#endif
}

#include "VideoFile.h"
#include "VideoFile.moc"

#include <QtGui/QButtonGroup>
#include <QtGui/QDialog>
#include <QtGui/QDialogButtonBox>
#include <QtGui/QHeaderView>
#include <QtGui/QTreeWidget>
#include <QtGui/QLabel>
#include <QtGui/QVBoxLayout>
#include <QThread>
#include <QDebug>
#include <QFileInfo>

// Buffer between Parsing thread and Decoding thread
#define MEGABYTE 1048576
#define MAX_VIDEOQ_SIZE (3 * 1048576)
#define GETTIME (double) av_gettime() * av_q2d(AV_TIME_BASE_Q)

bool VideoFile::ffmpegregistered = false;

/**
 * VideoFile::Clock
 */

VideoFile::Clock::Clock()  {
    _requested_speed = -1.0;
    _speed = 1.0;
    _frame_base = 0.04;
    _time_on_start = 0.0;
    _time_on_pause = 0.0;
    _paused = false;
    // minimum is 50 % of time base
    _min_frame_delay = 0.5;
    // maximum is 200 % of time base
    _max_frame_delay = 2.0;
}

void VideoFile::Clock::reset(double deltat, double timebase) {

    // set frame base time ratio when provided
    if (timebase > 0)
        _frame_base = timebase;

    // set new time on start
    _time_on_start = GETTIME - ( deltat / _speed );

    // trick to reset time on pause
    _time_on_pause = _time_on_start + ( deltat / _speed );

}

double VideoFile::Clock::time() const {

    if (_paused)
        return (_time_on_pause - _time_on_start) * _speed;
    else
        return (GETTIME - _time_on_start) * _speed;

}

void VideoFile::Clock::pause(bool p) {

    if (p)
        _time_on_pause = GETTIME;
    else
        _time_on_start += GETTIME - _time_on_pause;

    _paused = p;
}

bool VideoFile::Clock::paused() const {
    return _paused;
}

double VideoFile::Clock::speed() const {
    return _speed;
}

double VideoFile::Clock::timeBase() const {
    return _frame_base / _speed;
}

void VideoFile::Clock::setSpeed(double s) {

    // limit range
    s = qBound(0.1, s, 10.0);

    // request new speed
    _requested_speed = s;
}

void VideoFile::Clock::applyRequestedSpeed() {

    if ( _requested_speed > 0 ) {
        // trick to reset time on pause
        _time_on_pause = _time_on_start + ( time() / _speed );

        // replace time of start to match the change in speed
        _time_on_start = ( 1.0 - _speed / _requested_speed) * GETTIME + (_speed / _requested_speed) * _time_on_start;

        // set speed
        _speed = _requested_speed;
        _requested_speed = -1.0;
    }
}


double VideoFile::Clock::minFrameDelay() const{
    return _min_frame_delay * timeBase();
}

double VideoFile::Clock::maxFrameDelay() const {
    return  _max_frame_delay * timeBase();
}

class ParsingThread: public QThread
{
public:
	ParsingThread(VideoFile *video = 0) :
		QThread(), is(video)
	{
	}

	void run();
private:
	VideoFile *is;

};

class DecodingThread: public QThread
{
public:
    DecodingThread(VideoFile *video = 0) : QThread(), is(video)
	{
		// allocate a frame to fill
		_pFrame = avcodec_alloc_frame();
		Q_CHECK_PTR(_pFrame);
	}
	~DecodingThread()
	{
		// free the allocated frame
		av_free(_pFrame);
	}

	void run();
private:
	VideoFile *is;
	AVFrame *_pFrame;
};

//VideoPicture::VideoPicture() :
//    pts(0), width(0), height(0), convert_rgba_palette(false),  pixelformat(PIX_FMT_NONE), action(ACTION_SHOW)
//{
//	img_convert_ctx_filtering = NULL;
//	rgb.data[0] = NULL;
//}

VideoPicture::VideoPicture(SwsContext *img_convert_ctx, int w, int h,
        enum PixelFormat format, bool rgba_palette) : pts(0), width(w), height(h), convert_rgba_palette(rgba_palette),  pixelformat(format),  img_convert_ctx_filtering(img_convert_ctx), action(ACTION_SHOW)
{


    avpicture_alloc(&rgb, pixelformat, width, height);

}

VideoPicture::~VideoPicture()
{

    avpicture_free(&rgb);

}

void VideoPicture::saveToPPM(QString filename) const
{
    if (pixelformat != PIX_FMT_RGBA)
	{
		FILE *pFile;
		int y;

		// Open file
		pFile = fopen(filename.toUtf8().data(), "wb");
		if (pFile == NULL)
			return;

		// Write header
		fprintf(pFile, "P6\n%d %d\n255\n", width, height);

		// Write pixel data
		for (y = 0; y < height; y++)
			fwrite(rgb.data[0] + y * rgb.linesize[0], 1, width * 3, pFile);

		// Close file
		fclose(pFile);
	}
}

void VideoPicture::fill(AVPicture *frame, double timestamp)
{
    if (!frame || !img_convert_ctx_filtering)
		return;

	// remember pts
	pts = timestamp;

	if (!convert_rgba_palette)
    {
		// Convert the image with ffmpeg sws
		sws_scale(img_convert_ctx_filtering, frame->data, frame->linesize, 0,
				  height, (uint8_t**) rgb.data, (int *) rgb.linesize);

		return;
	}
	// I reimplement here sws_convertPalette8ToPacked32 which does not work with alpha channel (RGBA)...
	else
	{
		// get pointer to the palette
		uint8_t *palette = frame->data[1];
		if ( palette != 0 ) {
			// clear RGB to zeros when alpha is 0 (optional but cleaner)
			for (int i = 0; i < 4 * 256; i += 4)
			{
				if (palette[i + 3] == 0)
					palette[i + 0] = palette[i + 1] = palette[i + 2] = 0;
			}
			// copy BGR palette color from frame to RGBA buffer of VideoPicture
			uint8_t *map = frame->data[0];
			uint8_t *bgr = rgb.data[0];
			for (int y = 0; y < height; y++)
			{
				for (int x = 0; x < width; x++)
				{
					*bgr++ = palette[4 * map[x] + 2]; // B
					*bgr++ = palette[4 * map[x] + 1]; // G
					*bgr++ = palette[4 * map[x]];     // R
					if (pixelformat == PIX_FMT_RGBA)
						*bgr++ = palette[4 * map[x] + 3]; // A
				}
				map += frame->linesize[0];
			}
		}
	}

}


VideoFile::VideoFile(QObject *parent, bool generatePowerOfTwo,
		int swsConversionAlgorithm, int destinationWidth, int destinationHeight) :
	QObject(parent), filename(QString()), powerOfTwo(generatePowerOfTwo),
			targetWidth(destinationWidth), targetHeight(destinationHeight),
			conversionAlgorithm(swsConversionAlgorithm)
{

	if (!ffmpegregistered)
	{
		avcodec_register_all();
		av_register_all();
#ifdef NDEBUG
		av_log_set_level( AV_LOG_QUIET ); /* don't print warnings from ffmpeg */
#else
		av_log_set_level( AV_LOG_DEBUG  ); /* print debug info from ffmpeg */
#endif
		ffmpegregistered = true;
	}

	// Init some pointers to NULL
	videoStream = -1;
	video_st = NULL;
	deinterlacing_buffer = NULL;
	pFormatCtx = NULL;
    img_convert_ctx = NULL;
    firstPicture = NULL;
    blackPicture = NULL;
    resetPicture = NULL;
    filter = NULL;
    pictq_max_size = MAX_VIDEO_PICTURE_QUEUE_SIZE;

	// Contruct some objects
	parse_tid = new ParsingThread(this);
	Q_CHECK_PTR(parse_tid);
	decod_tid = new DecodingThread(this);
    Q_CHECK_PTR(decod_tid);
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
	seek_any = false; // NOT dirty seek
    loop_video = true; // loop by default
    restart_where_stopped = true; // by default restart where stopped
    ignoreAlpha = false; // by default ignore alpha channel

	// reset
    quit = true; // not running yet
	reset();
}

void VideoFile::close()
{

	// request ending
	quit = true;
	// wait for threads to end properly
    parse_tid->wait();
    pictq_cond->wakeAll();
    seek_cond->wakeAll();
	decod_tid->wait();

	// free context & filter
	if (img_convert_ctx)
		sws_freeContext(img_convert_ctx);
    if (filter)
        sws_freeFilter(filter);

	// close file
	if (pFormatCtx)

#if LIBAVCODEC_VERSION_INT > AV_VERSION_INT(52,100,0)
        avformat_close_input(&pFormatCtx);
#else
        av_close_input_file(pFormatCtx);
#endif
}

VideoFile::~VideoFile()
{
	// make sure all is closed
    close();

	// delete threads
	delete parse_tid;
	delete decod_tid;
	delete pictq_mutex;
	delete pictq_cond;
    delete seek_mutex;
    delete seek_cond;
	delete ptimer;

	if (deinterlacing_buffer)
		av_free(deinterlacing_buffer);

    QObject::disconnect(this, 0, 0, 0);

    fprintf(stderr, "Video file %s deleted\n", qPrintable(getFileName()) );
}

void VideoFile::reset()
{
    // reset variables to 0
    video_pts = 0.0;
    seek_pos = 0.0;
    seek_backward = false;
    pictq_flush_req = false;
    parsing_mode = VideoFile::PARSING_NORMAL;

    flush_picture_queue();

    if (video_st)
        _videoClock.reset(0.0, av_q2d(video_st->time_base));

}

void VideoFile::stop()
{
	if (!quit && pFormatCtx)
	{
        // remember where we are for next restart
        mark_stop = getCurrentFrameTime();

		// request quit
		quit = true;

		// wait fo threads to end properly
		parse_tid->wait();
        pictq_cond->wakeAll();
        seek_cond->wakeAll();
		decod_tid->wait();

        if (!restart_where_stopped)
        {
//            fprintf(stderr, "restart_where_stopped %f \n", mark_stop);
            current_frame_pts = fill_first_frame(true);
            // display firstPicture frame
            emit frameReady( resetPicture );
        }

        flush_picture_queue();

		/* say if we are running or not */
        emit running(!quit);

	}
}

void VideoFile::start()
{
	// nothing to play if there is ONE frame only...
    if ( getNumFrames() < 2)
		return;

	if (quit && pFormatCtx)
	{
        // reset internal state
        reset();

		// reset quit flag
		quit = false;

		// where shall we (re)start ?
		// enforces seek to the frame before ; it is needed because we may miss the good frame
		seek_backward = true;

        // restart where we where (if valid mark)
        if (restart_where_stopped && mark_stop < mark_out && mark_stop > mark_in)
           seek_pos =  mark_stop;
        // otherwise restart at beginning
        else
            seek_pos = mark_in;
        // request partsing thread to perform seek
        parsing_mode = VideoFile::PARSING_SEEKREQUEST;

        // start parsing and decoding threads
        ptimer->start();
		parse_tid->start();
        decod_tid->start();

		/* say if we are running or not */
        emit running(!quit);
	}

}

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
    // 0 % is 0.1 speed (1/5)
    // 50% is 1.0
    // 100% is 5.0
    if ( s != getPlaySpeedFactor() ){

        setPlaySpeed( exp( double(s -100) / 43.42 ) );
        emit playSpeedFactorChanged(s);
    }
}

int VideoFile::getPlaySpeedFactor()
{
    return (int) ( log(getPlaySpeed()) * 43.42 + 100.0  );
}

void VideoFile::setPlaySpeed(double s)
{
    // change the picture queue may size according to play speed
    // this is because, in principle, more frames are skipped when play faster
    // and we empty the queue faster
    double sizeq = qBound(2.0, (double) video_st->nb_frames * SEEK_STEP + 1.0, (double) MAX_VIDEO_PICTURE_QUEUE_SIZE);
    sizeq *= s;

    pictq_max_size = (int) sizeq;

    _videoClock.setSpeed( s );
    emit playSpeedChanged(s);
}

double VideoFile::getPlaySpeed()
{
    return _videoClock.speed();
}


VideoPicture *VideoFile::getResetPicture() const
{
    return (resetPicture);
}

bool VideoFile::open(QString file, double markIn, double markOut, bool ignoreAlphaChannel)
{
	int err = 0;
	AVFormatContext *_pFormatCtx = 0;

	filename = file;
	ignoreAlpha = ignoreAlphaChannel;

	// Check file
#if LIBAVCODEC_VERSION_INT > AV_VERSION_INT(52,100,0)
	_pFormatCtx = avformat_alloc_context();
	err = avformat_open_input(&_pFormatCtx, qPrintable(filename), NULL, NULL);
#else
	err = av_open_input_file(&_pFormatCtx, qPrintable(filename), NULL, 0, NULL);
#endif
	if (err < 0)
	{
		switch (err)
		{
		case AVERROR_INVALIDDATA:
            qWarning() << filename << QChar(124).toLatin1() << tr("Error while parsing header.");
			break;
		case AVERROR(EIO):
            qWarning() << filename << QChar(124).toLatin1()
                    << tr("I/O error. Usually that means that input file is truncated and/or corrupted");
			break;
		case AVERROR(ENOMEM):
            qWarning() << filename << QChar(124).toLatin1()<< tr("Memory allocation error.");
			break;
		case AVERROR(ENOENT):
            qWarning() << filename << QChar(124).toLatin1()<< tr("No such file.");
			break;
		default:
            qWarning() << filename << QChar(124).toLatin1()<< tr("Cannot open file.");
			break;
		}
		return false;
	}

#if LIBAVCODEC_VERSION_INT > AV_VERSION_INT(52,100,0)
    err = avformat_find_stream_info(_pFormatCtx, NULL);
#else
    err = av_find_stream_info(_pFormatCtx);
#endif
	if (err < 0)
	{
		switch (err)
		{
		case AVERROR_INVALIDDATA:
            qWarning() << filename << QChar(124).toLatin1()<< tr("Error while parsing header.");
			break;
		case AVERROR(EIO):
            qWarning() << filename<< QChar(124).toLatin1()
                    << tr("I/O error. Usually that means that input file is truncated and/or corrupted");
			break;
		case AVERROR(ENOMEM):
            qWarning() << filename << QChar(124).toLatin1()<< tr("Memory allocation error");
			break;
		case AVERROR(ENOENT):
            qWarning() << filename << QChar(124).toLatin1()<< tr("No such entry.");
			break;
		default:
            qWarning() << filename << QChar(124).toLatin1()<< tr("Unsupported format.");
			break;
		}
		return false;
	}

	// if video_index not set (no video stream found) or stream open call failed
	videoStream = stream_component_open(_pFormatCtx);
	if (videoStream < 0)
		//could not open Codecs (error message already emitted)
		return false;

	// Init the stream and timing info
	video_st = _pFormatCtx->streams[videoStream];

	// all ok, we can set the internal pointer to the good value
	pFormatCtx = _pFormatCtx;

    // make sure the number of frames is correctly counted (some files have no count=
    if (video_st->nb_frames == AV_NOPTS_VALUE || video_st->nb_frames < 1 )
        video_st->nb_frames = getDuration() / av_q2d(video_st->time_base);


    pictq_max_size = qBound(2, (int)( (double) video_st->nb_frames * SEEK_STEP) + 1, MAX_VIDEO_PICTURE_QUEUE_SIZE);


    // check the parameters for mark in and out and setup marking accordingly
    if (markIn < 0 || video_st->nb_frames < 2)
        mark_in = getBegin(); // default to start of file
    else
    {
        mark_in = qBound(getBegin(), markIn, getEnd());
        emit markingChanged();
    }

    if (markOut <= 0 || video_st->nb_frames < 2)
        mark_out = getEnd(); // default to end of file
    else
    {
        mark_out = qBound(mark_in, markOut, getEnd());
        emit markingChanged();
    }

	// init picture size
	if (targetWidth == 0)
		targetWidth = video_st->codec->width;
	if (targetHeight == 0)
		targetHeight = video_st->codec->height;

	// round target picture size to power of two size
	if (powerOfTwo)
	{
		targetWidth = VideoFile::roundPowerOfTwo(targetWidth);
		targetHeight = VideoFile::roundPowerOfTwo(targetHeight);
	}

	// Default targetFormat to PIX_FMT_RGB24, not using color palette
    targetFormat = PIX_FMT_RGB24;
    rgba_palette = false;

	// Change target format to keep Alpha channel if format requires
	if ( pixelFormatHasAlphaChannel()
		// this is a fix for some jpeg formats with YUVJ format
		|| av_pix_fmt_descriptors[video_st->codec->pix_fmt].log2_chroma_h > 0 )
	{
		// special case of PALETTE formats which have ALPHA channel in their colors
		if (video_st->codec->pix_fmt == PIX_FMT_PAL8) {
			// palette pictures are always treated as PIX_FMT_RGBA data
			targetFormat = PIX_FMT_RGBA;
			// if should NOT ignore alpha channel, use rgba palette (flag used in VideoFile)
			if (!ignoreAlpha)
				rgba_palette = true;
		}
		// general case (pictures have alpha channel)
		else {
			// if should NOT ignore alpha channel, use alpha channel
			if (!ignoreAlpha)
				targetFormat = PIX_FMT_RGBA;
		}
    }

	// Decide for optimal scaling algo if it was not specified
	// NB: the algo is used only if the conversion is scaled or with filter
	// (i.e. optimal 'unscaled' converter is used by default)
	if (conversionAlgorithm == 0)
	{
        if ( video_st->nb_frames < 2 )
			conversionAlgorithm = SWS_LANCZOS; // optimal quality scaling for 1 frame sources (images)
		else
			conversionAlgorithm = SWS_POINT;   // optimal speed scaling for videos
	}

#ifndef NDEBUG
	// print all info if in debug
	conversionAlgorithm |= SWS_PRINT_INFO;
#endif

	// MMX should be faster !
	conversionAlgorithm |= SWS_CPU_CAPS_MMX;
	conversionAlgorithm |= SWS_CPU_CAPS_MMX2;

    filter = NULL;
    // For single frames media or when ignorealpha flag is on, force filtering
    // (The ignore alpha flag is normally requested when the source is rgba
    // and in this case, optimal conversion from rgba to rgba is to do nothing : but
    // this means there is no conversion, and no brightness/contrast is applied)
    if (ignoreAlpha || (video_st->nb_frames < 2))
        // Setup a filter to enforce a per-pixel conversion (here a slight blur)
        filter = sws_getDefaultFilter(0.0, 1.0, 0.0, 0.0, 0.0, 0.0, 0);

	// create conversion context
	img_convert_ctx = sws_getCachedContext(NULL, video_st->codec->width,
					video_st->codec->height, video_st->codec->pix_fmt,
					targetWidth, targetHeight, targetFormat,
					conversionAlgorithm, filter, NULL, NULL);
	if (img_convert_ctx == NULL)
	{
		// Cannot initialize the conversion context!
        qWarning() << filename << QChar(124).toLatin1()<< tr("Cannot create a suitable conversion context.");
		return false;
	}

	// we may need a black Picture frame to return when stopped
    blackPicture = new VideoPicture(0, targetWidth, targetHeight);

	// we need a picture to display when not decoding
    firstPicture = new VideoPicture(img_convert_ctx, targetWidth, targetHeight, targetFormat, rgba_palette);

	// read firstPicture (not a big problem if fails; it would just be black)
    // (NB : seek in stream only if not reading the first frame)
    current_frame_pts = fill_first_frame( mark_in != getBegin() );
    mark_stop = current_frame_pts;

    // use first picture as reset picture
    resetPicture = firstPicture;

	// display a firstPicture frame ; this shows that the video is open
    emit frameReady( resetPicture );

	/* say if we are running or not */
    emit running(false);

	// tells everybody we are set !
    qDebug() << filename << QChar(124).toLatin1() <<  tr("Media opened (%1 frames).").arg(video_st->nb_frames);

	return true;
}

bool VideoFile::pixelFormatHasAlphaChannel() const
{
	if (!video_st)
		return false;

#if LIBAVCODEC_VERSION_INT > AV_VERSION_INT(52,30,0)
	return  (av_pix_fmt_descriptors[video_st->codec->pix_fmt].nb_components > 3)
			// special case of PALLETE and GREY pixel formats(converters exist for rgba)
			|| ( av_pix_fmt_descriptors[video_st->codec->pix_fmt].flags & PIX_FMT_PAL );
#else
	return (video_st->codec->pix_fmt == PIX_FMT_RGBA || video_st->codec->pix_fmt == PIX_FMT_BGRA ||
			video_st->codec->pix_fmt == PIX_FMT_ARGB || video_st->codec->pix_fmt == PIX_FMT_ABGR ||
			video_st->codec->pix_fmt == PIX_FMT_YUVJ420P );
#endif
}

double VideoFile::fill_first_frame(bool seek)
{
	AVPacket pkt1;
	AVPacket *packet = &pkt1;
    AVFrame *tmpframe = avcodec_alloc_frame();
    int frameFinished = 0;
    double pts = mark_in;
    int trial = 0;

    if (seek) {
        int64_t seek_target = av_rescale_q(mark_in, (AVRational){1, 1}, video_st->time_base);

        int flags = AVSEEK_FLAG_BACKWARD;
        if ( seek_any )
            flags |= AVSEEK_FLAG_ANY;

        av_seek_frame(pFormatCtx, videoStream, seek_target, flags);

        // flush seek stuff
        avcodec_flush_buffers(video_st->codec);

//        fprintf(stderr, "fill first frame seeked to %f (was at %f) \n", mark_in, getCurrentFrameTime());
    }

    while (!frameFinished && trial < 2)
	{
		// read a packet
        if (av_read_frame(pFormatCtx, packet) < 0){
            trial++; // give it one more chance
            continue;
        }

		// ignore non-video stream packets
		if (packet->stream_index != videoStream)
			continue;

		// if we can decode it
#if LIBAVCODEC_VERSION_INT > AV_VERSION_INT(52,30,0)
        if (avcodec_decode_video2(video_st->codec, tmpframe, &frameFinished, packet) >= 0)
		{
#else
            if ( avcodec_decode_video(video_st->codec, tmpframe, &frameFinished, packet->data, packet->size) >= 0)
			{
#endif
            // if the frame is full
            if (frameFinished) {

                // try to get a pts from the packet
                if (packet->dts != (int64_t) AV_NOPTS_VALUE)
                    pts =  double(packet->dts) * av_q2d(video_st->time_base);

                // we can now fill in the first picture with this frame
                firstPicture->fill((AVPicture *) tmpframe, pts);
            }
		}
	}

    // free memory
    av_free_packet(packet);
    av_free(tmpframe);

    return firstPicture->getPts();
}

int VideoFile::stream_component_open(AVFormatContext *pFCtx)
{

	AVCodecContext *codecCtx;
	AVCodec *codec;
	int stream_index = -1;

	// Find the first video stream index
	for (int i = 0; i < (int) pFCtx->nb_streams; i++)
	{
#if LIBAVCODEC_VERSION_INT > AV_VERSION_INT(53,0,0)
		if (pFCtx->streams[i]->codec->codec_type == AVMEDIA_TYPE_VIDEO)
#else
		if (pFCtx->streams[i]->codec->codec_type == CODEC_TYPE_VIDEO)
#endif
		{
			stream_index = i;
			break;
		}
	}

	if (stream_index < 0 || stream_index >= (int) pFCtx->nb_streams)
	{
        qWarning() << filename << QChar(124).toLatin1()<< tr("This is not a video or an image file.");
		return -1;
	}

	// Get a pointer to the codec context for the video stream
	codecCtx = pFCtx->streams[stream_index]->codec;

    codec = avcodec_find_decoder(codecCtx->codec_id);

#if LIBAVCODEC_VERSION_INT > AV_VERSION_INT(53,0,0)
    if (!codec || (avcodec_open2(codecCtx, codec, NULL) < 0))
#else
    if (!codec || (avcodec_open(codecCtx, codec) < 0))
#endif
	{
        qWarning() << filename << QChar(124).toLatin1()<< tr("The codec ") << codecCtx->codec_name
				<< tr("is not supported.");
		return -1;
    }

	codecname = QString(codec->name);

	return stream_index;
}

void VideoFile::video_refresh_timer()
{
    // by default timer will be restarted ASAP
    int ptimer_delay = 10;

    // if all is in order, deal with the picture in the queue
    // (i.e. there is a stream, there is a picture in the queue, and the clock is not paused)
    if (video_st && !pictq.empty() && (!_videoClock.paused() ||  pictq.head()->hasAction(VideoPicture::ACTION_PAUSE) ) )
    {
        _videoClock.applyRequestedSpeed();

        // now working on the head of the queue, that we take off the queue
        VideoPicture *currentvp = pictq.dequeue();

        // store time of this current frame
        current_frame_pts =  currentvp->presentationTime();

//        fprintf(stderr, "\t\tFRAME pts = %f \n", current_frame_pts);


        // detect if we over passed the marks or the end of stream
        // TODO : remove this catch which should be delt on the queue directly
//        if (current_frame_pts > mark_out || (current_frame_pts + av_q2d(video_st->time_base)) > getEnd() )
//        {
////            fprintf(stderr, "\n\n FOUND OVERDUE FRAME at %f \n", pictq[current_frame].pts);

//            // react according to loop mode
//            if (loop_video)
//                seekToPosition(mark_in);
//            else
//                // if loop mode off, stop
//                stop();
//        }

        // this frame was tagged as seeking frame
        if ( currentvp->hasAction(VideoPicture::ACTION_SEEK) ) {
            // reset clock to the time of the frame
            _videoClock.reset(current_frame_pts);

//            fprintf(stderr, "FOUND SEEK FRAME at t = %f for pts %f \n",_videoClock.time(), current_frame_pts);
        }


        if ( currentvp->hasAction(VideoPicture::ACTION_SHOW) ) {

            // ask to show the current picture and delete it
            currentvp->addAction(VideoPicture::ACTION_DELETE);
            emit frameReady(currentvp);

            // if there is a next picture
            if (!pictq.empty()) {

                // we now want to know when to present the next frame
                VideoPicture *nextvp = pictq.head();
                double delay = 0.0;

                // if next frame we will be seeking
                if ( nextvp->hasAction(VideoPicture::ACTION_SEEK) )
                    // update at normal fps, discarding computing of delay
                    delay = _videoClock.timeBase();
                else
                    // otherwise read presentation time and compute delay till next frame
                    delay = ( nextvp->presentationTime() - _videoClock.time() ) / _videoClock.speed() ;

//    fprintf(stderr, "at t = %f  next frame should be at  pts = %f  ( delay = %f   base = %f  speed = %f min = %f)\n",_videoClock.time(), nextvp->presentationTime(), delay, _videoClock.timeBase(), _videoClock.speed(), _videoClock.minFrameDelay());

                // if delay is correct
                if ( delay > _videoClock.minFrameDelay() ) {
                    // schedule normal delayed display of next frame
                    ptimer_delay = (int) (delay * 1000.0);

//                    fprintf(stderr, "NEXT FRAME in deltat = %f  = %d \n", delay, ptimer_delay);

                // delay is too small, or negative
                } else {
                    // retry shortly (but not too fast to avoid glitches)
                    ptimer_delay = (int) (_videoClock.minFrameDelay() * 1000.0);

                    // remove the show tag for that frame (i.e. skip it)
                    nextvp->removeAction(VideoPicture::ACTION_SHOW);
    //                fprintf(stderr, "NEXT FRAME at  pts = %f SKIPPED \n", nextvp->pts);
                }

            }

        }
        else {
//            fprintf(stderr, "SKIP frame for t = %f (pts was %f) \n",_videoClock.time(), currentvp->pts);

            delete currentvp;

            ptimer_delay = (int) (_videoClock.minFrameDelay() * 1000.0);
       }

        // this frame was tagged as stopping frame
        if ( currentvp->hasAction(VideoPicture::ACTION_STOP) ) {
//            fprintf(stderr, "FOUND STOP FRAME at t = %f for pts %f \n",_videoClock.time(), current_frame_pts);
            // stop the video
            stop();
        }

        // this frame was tagged as pause frame
        if ( currentvp->hasAction(VideoPicture::ACTION_PAUSE) ) {
//            fprintf(stderr, "FOUND PAUSE FRAME at t = %f for pts %f \n",_videoClock.time(), current_frame_pts);

            ptimer_delay = 10;

            // TODO : what for pause ?
            pause(true);
        }


        // unblock the queue for the decoding thread
        // block the video thread
        pictq_mutex->lock();
        // inform decoding thread about the new size of the queue
        pictq_size = pictq.size();
        pictq_cond->wakeAll();
        pictq_mutex->unlock();

    }

    // restart the ptimer for next frame
    if (!quit)
        ptimer->start(ptimer_delay);        


}

double VideoFile::getCurrentFrameTime() const
{
    return current_frame_pts;
}

double VideoFile::getTimefromFrame(int64_t  f) const
{
    return f * av_q2d(video_st->time_base);
}

double VideoFile::getFrameRate() const
{
    if (video_st)
    {
        // get average framerate from libav, if correct.
        if (video_st->avg_frame_rate.den > 0)
            return av_q2d(video_st->avg_frame_rate);

        // else get guessed framerate from libav, if correct. (deprecated)
#if FF_API_R_FRAME_RATE
        else if (video_st->r_frame_rate.den > 0)
            return av_q2d(video_st->r_frame_rate);
#endif
        // else compute global framerate from duration and number of frames
        else if (video_st->nb_frames > 1)
            return (  ((double)video_st->duration / av_q2d(video_st->time_base)) / (double)video_st->nb_frames ) ;

    }

    return 0.0;
}

void VideoFile::setOptionAllowDirtySeek(bool dirty)
{

	seek_any = dirty;

}

void VideoFile::setOptionRevertToBlackWhenStop(bool black)
{
	if (black)
        resetPicture = blackPicture;
	else
        resetPicture = firstPicture;
	// if the option is toggled while being stopped, then we should show the good frame now!
	if (quit)
        emit frameReady( resetPicture );
}

void VideoFile::seekToPosition(double t)
{
    if (pFormatCtx && !quit && parsing_mode == PARSING_NORMAL)
    {
        seek_mutex->lock();
        seek_pos = qBound(getBegin(), t, getEnd());
        parsing_mode = VideoFile::PARSING_SEEKREQUEST;
        seek_mutex->unlock();

        pictq_flush_req = true;


//        fprintf(stderr, "seekToPosition to %f\n\n", seek_pos);
	}
}

void VideoFile::seekBySeconds(double ss)
{
    seekToPosition( current_frame_pts + ss);

}

void VideoFile::seekByFrames(int si)
{
    seekToPosition( current_frame_pts + (double) si * av_q2d(video_st->time_base));
}


void VideoFile::seekForwardOneFrame()
{
    // tag this frame as Pause and as seek
    pictq.head()->addAction(VideoPicture::ACTION_SEEK & VideoPicture::ACTION_PAUSE);

}

QString VideoFile::getStringTimeFromtime(double time) const
{
	int s = (int) time;
	time -= s;
	int h = s / 3600;
	int m = (s % 3600) / 60;
	s = (s % 3600) % 60;
	int ds = (int) (time * 100.0);
	return QString("%1h %2m %3.%4s").arg(h, 2).arg(m, 2, 10, QChar('0')).arg(s,
			2, 10, QChar('0')).arg(ds, 2, 10, QChar('0'));
}

QString VideoFile::getStringFrameFromTime(double t) const
{
    return (QString("Frame %1").arg((int) ( t / av_q2d(video_st->time_base) )));
}


double VideoFile::getBegin() const
{
    if (video_st)
    {
        if (video_st && (video_st->start_time != AV_NOPTS_VALUE))
            return double(video_st->start_time) * av_q2d(video_st->time_base);

        return double(pFormatCtx->start_time) * av_q2d(AV_TIME_BASE_Q);
    }

    return 0.0;
}

double VideoFile::getDuration() const
{
    if (video_st)
    {
        if ( video_st->duration != AV_NOPTS_VALUE )
            return double(video_st->duration) * av_q2d(video_st->time_base);

        return double(pFormatCtx->duration) * av_q2d(AV_TIME_BASE_Q);
    }

    return 0.0;
}

double VideoFile::getEnd() const
{
    return getBegin() + getDuration();
}

void VideoFile::setMarkIn(double t)
{
    mark_in = qBound(getBegin(), t, mark_out);
	emit markingChanged();
}

void VideoFile::setMarkOut(double t)
{
    mark_out = qBound(mark_in, t, getEnd());
	emit markingChanged();

    // TODO : if we change the mark out but the queue already passed it
    // (i.e. it has marked the picture for stop or seek)
    //   -> we have to remove this part of the queue (including the bad frame)
    //  and restart parsing from there


}

double VideoFile::getStreamAspectRatio() const
{
	double aspect_ratio = 0;

	if (video_st->sample_aspect_ratio.num)
		aspect_ratio = av_q2d(video_st->sample_aspect_ratio);
	else if (video_st->codec->sample_aspect_ratio.num)
		aspect_ratio = av_q2d(video_st->codec->sample_aspect_ratio);
	else
		aspect_ratio = 0;
	if (aspect_ratio <= 0.0)
		aspect_ratio = 1.0;
	aspect_ratio *= (double) video_st->codec->width / video_st->codec->height;

	return aspect_ratio;
}

/**
 *
 * ParsingThread
 *
 */
void ParsingThread::run()
{

	AVPacket pkt1, *packet = &pkt1;

	while (is && !is->quit)
    {

        // seek stuff goes here
        int64_t seek_target = AV_NOPTS_VALUE;
        is->seek_mutex->lock();
        if (is->parsing_mode == VideoFile::PARSING_SEEKREQUEST) {
            // enter the seeking mode (disabled only when target reached)
            is->parsing_mode = VideoFile::PARSING_SEEKING;
            // compute dts of seek target from seek position
            seek_target = av_rescale_q(is->seek_pos, (AVRational){1, 1}, is->video_st->time_base);
        }
        is->seek_cond->wakeAll();
        is->seek_mutex->unlock();

        // decided to perform seek
        if (seek_target != AV_NOPTS_VALUE)
        {

            // configure seek options
			int flags = 0;
            if ( is->seek_any )
				flags |= AVSEEK_FLAG_ANY;
            if ( is->seek_backward || is->seek_pos <= is->getCurrentFrameTime() )
				flags |= AVSEEK_FLAG_BACKWARD;

//            fprintf(stderr, "\n\nPARSING seek to seektarget = %f  for seek pos = %f \n\n", seek_target * av_q2d(is->video_st->time_base), is->seek_pos );

            // request seek to libav
            if (av_seek_frame(is->pFormatCtx, is->videoStream, seek_target, flags) < 0)
			{
                qDebug() << is->filename << QChar(124).toLatin1()
                        << QObject::tr("Could not seek to frame (%1); jumping where I can!").arg(is->seek_pos);
			}

			// after seek,  we'll have to flush buffers
            // this also sends the target of seeking
            if (!is->videoq.flush())
                qWarning() << is->filename << QChar(124).toLatin1()<< QObject::tr("Flushing error.");

//            if (is->pictq_flush_req) {
////                fprintf(stderr, "ParsingThread flush_picture_queue");
//                is->flush_picture_queue();
//            }

            // revert one shot option
            is->seek_backward = false;

        }

        /* if the queue is full, no need to read more */
        if (is->videoq.isFull())
        {
            msleep(PARSING_SLEEP_DELAY);
            continue;
        }

		// MAIN call ; reading the frame
        if ( av_read_frame(is->pFormatCtx, packet) < 0)
		{
			// if could NOT read full frame, was it an error?
            if (is->pFormatCtx->pb && is->pFormatCtx->pb->error != 0)
                qWarning() << is->filename << QChar(124).toLatin1() << QObject::tr("Could not read frame.");
            else
            {
                // no error : the read frame reached the end of file
                is->videoq.endFile();
            }

			// no error; just wait a bit for the end of the packet and continue
            msleep(PARSING_SLEEP_DELAY);
			continue;
        }

		// 1) test if it was NOT a video stream packet : if yes, the OR ignores the second part and frees the packet
		// 2) if no, the OR tests the second part, which puts the video packet in the queue
		// 3) if this fails we free the packet anyway
		if (packet->stream_index != is->videoStream || !is->videoq.put(packet))
			// we need to free the packet if it was not put in the queue
			av_free_packet(packet);

	} // quit

	// request flushing of the video queue (this will end the decoding thread)
    if (!is->videoq.flush())
        qWarning() << is->filename << QChar(124).toLatin1() << QObject::tr("Flushing error at end.");

}


void VideoFile::flush_picture_queue()
{
    pictq_flush_req = false;

    pictq_mutex->lock();

    while (!pictq.isEmpty())
        delete pictq.dequeue();

    pictq_size = 0;

//    fprintf(stderr, "                    : \nREAD %d = %f        WRITE %d = %f\n", pictq_rindex, pictq[pictq_rindex].pts, pictq_windex, pictq[pictq_windex].pts);

    pictq_cond->wakeAll();
    pictq_mutex->unlock();

}


void VideoFile::requestSeekTo(double time)
{
    seek_mutex->lock();
    // TODO ; why do we need to wait here ?
    seek_cond->wait(seek_mutex);
    if ( parsing_mode == VideoFile::PARSING_NORMAL ) {
        seek_pos = time;
        parsing_mode = VideoFile::PARSING_SEEKREQUEST;
//        fprintf(stderr, "\n REQUEST SEEK at %f.\n", seek_pos);
        seek_cond->wait(seek_mutex);
//        fprintf(stderr, "\n done waiting \n" );
    }
    seek_mutex->unlock();

}

void VideoFile::queue_picture(AVFrame *pFrame, double pts, VideoPicture::Action a)
{

	/* wait until we have space for a new pic */
    pictq_mutex->lock();
    while ( (pictq_size > pictq_max_size) && !quit)
        pictq_cond->wait(pictq_mutex); // the condition is released in video_refresh_timer()
	pictq_mutex->unlock();

	// if have to quit, discard the rest but let the video thread continue
    if (quit) {
        flush_picture_queue();
		return;
	}

    // create vp as the picture in the queue to be written
    VideoPicture *vp = new VideoPicture(img_convert_ctx, targetWidth, targetHeight, targetFormat, rgba_palette);

    AVPicture *picture = (AVPicture*) pFrame;

    if (pFrame->interlaced_frame)
    {
        // create temporary picture the first time we need it
        if (deinterlacing_buffer == NULL)
        {
            int size = avpicture_get_size(video_st->codec->pix_fmt,
                    video_st->codec->width, video_st->codec->height);
            deinterlacing_buffer = (uint8_t *) av_malloc(size);
            avpicture_fill(&deinterlacing_picture, deinterlacing_buffer,
                    video_st->codec->pix_fmt, video_st->codec->width,
                    video_st->codec->height);
        }

        // try to deinterlace into the temporary picture
        if (avpicture_deinterlace(&deinterlacing_picture, picture,
                video_st->codec->pix_fmt, video_st->codec->width,
                video_st->codec->height) == 0)
            // if deinterlacing was successfull, use it
            picture = &deinterlacing_picture;
    }

    // Fill the Video Picture queue with the current frame
    vp->fill(picture, pts);

    // set the actions of this frame ; show frame + special option provided
    vp->resetAction();
    vp->addAction(a);

//    fprintf(stderr, "queue picture %f  action %d\n", pts, vp->action);

    /* now we inform our display thread that we have a pic ready */
    pictq_mutex->lock();
    // enqueue this picture in the queue
    pictq.enqueue(vp);
    // inform about the new size of the queue
    pictq_size = pictq.size();
    pictq_mutex->unlock();

}

/**
 * compute the exact PTS for the picture if it is omitted in the stream
 * @param pts1 the dts of the pkt / pts of the frame
 */
double VideoFile::synchronize_video(AVFrame *src_frame, double pts_)
{
    double pts = pts_;

	if (pts >= 0)
		/* if we have pts, set video clock to it */
        video_pts = pts;
	else
		/* if we aren't given a pts, set it to the clock */
		// this happens rarely (I noticed it on last frame)
        pts = video_pts;

    double frame_delay = av_q2d(video_st->codec->time_base);

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

	AVPacket pkt1, *packet = &pkt1;
	int len1 = 0, frameFinished = 0;
    double pts = 0.0; // Presentation time stamp
	int64_t dts = 0; // Decoding time stamp

    VideoPicture::Action actionSkipNextFrame = 0;

	while (is)
    {

		// get front packet (not blocking if we are quitting)
        if (!is->videoq.get(packet, !is->quit))
			// this is the exit condition
            break;

		// special case of flush packet
        // this happens when seeking in ParsingThread
		if (is->videoq.isFlush(packet))
        {
//            fprintf(stderr, "DecodingThread reached the seeked frame! \n");

            // flush buffers
            avcodec_flush_buffers(is->video_st->codec);

            // the next frame will be the result of the seeking process
            // its PTS is unknown, so we can only tag it for seeking action
            // (the ACTION_SEEK in video refresh timer will reset the clock)
            actionSkipNextFrame = VideoPicture::ACTION_SEEK;

            if (is->isPaused())
                actionSkipNextFrame |= VideoPicture::ACTION_PAUSE;

            // flush the picture queue on request (i.e. when seeking from setSeekTarget
            if (is->pictq_flush_req) {
//                fprintf(stderr, "DecodingThread flush_picture_queue");
                is->flush_picture_queue();
            }

            // reached the seeked frame! : can say we are not seeking anymore
            is->seek_mutex->lock();
            is->parsing_mode = VideoFile::PARSING_NORMAL;
            is->seek_mutex->unlock();
			// go on to next packet
            continue;
		}

        // special case of EndofFile packet
        // this happens when hitting end of file when reading
        if (is->videoq.isEndOfFile(packet))
        {
            // react according to loop mode
            if (is->loop_video)
                // if looping, request seek to begin
                is->requestSeekTo(is->mark_in);
            else
                // if stopping,  re-sends the previous frame pretending its too late
                is->queue_picture(_pFrame, pts + av_q2d(is->video_st->time_base), VideoPicture::ACTION_STOP);

            // go on to next packet
            continue;
        }

        // remember packet pts in case the decoding loose it
		is->video_st->codec->reordered_opaque = packet->pts;

        // Decode video frame
#if LIBAVCODEC_VERSION_INT > AV_VERSION_INT(52,30,0)
		len1 = avcodec_decode_video2(is->video_st->codec, _pFrame, &frameFinished, packet);
#else
		len1 = avcodec_decode_video(is->video_st->codec, _pFrame, &frameFinished, packet->data, packet->size);
#endif

		// loop on error
		if (len1 < 0)
			continue;

        // get packet decompression time stamp (dts)
        dts = 0;
        if (packet->dts != (int64_t) AV_NOPTS_VALUE)
            dts = packet->dts;
        else if (_pFrame->reordered_opaque != (int64_t) AV_NOPTS_VALUE)
            dts = _pFrame->reordered_opaque;

		// Did we get a full video frame?
		if (frameFinished)
        {
            // compute presentation time stamp
            pts = is->synchronize_video(_pFrame, double(dts) * av_q2d(is->video_st->time_base));

            // test if time will exceed limits (mark out or will pass the end of file)
            if ( pts > is->mark_out )
            {
//                fprintf(stderr, "decoding event : pts > is->mark_out\n");
                // react according to loop mode
                if (is->loop_video)
                    // if loop mode on, request seek to begin and do not queue the frame
                    is->requestSeekTo(is->mark_in);
                else
                    // if loop mode off, stop after this frame
                    is->queue_picture(_pFrame, pts, VideoPicture::ACTION_STOP);
            }
            else
                // add frame to the queue of pictures
                is->queue_picture(_pFrame, pts, actionSkipNextFrame);

            // reset skip tag
            actionSkipNextFrame = 0;

        }

		// packet was decoded, should be removed
		av_free_packet(packet);
	}

}

void VideoFile::pause(bool pause)
{
    if (!quit && pause != _videoClock.paused() )
    {
        _videoClock.pause(pause);

		emit paused(pause);
	}
}

bool VideoFile::isPaused() const {
    return _videoClock.paused();
}


/**
 * VideoFile::PacketQueue
 */

// put a packet at the tail of the queue.
bool VideoFile::PacketQueue::put(AVPacket *pkt)
{
    if (pkt != flush_pkt && pkt != eof_pkt && av_dup_packet(pkt) < 0)
        return false;

	AVPacketList *pkt1 = (AVPacketList*) av_malloc(sizeof(AVPacketList));
	Q_CHECK_PTR(pkt1);

	if (!pkt1)
		return false;

	pkt1->pkt = *pkt;
	pkt1->next = NULL;

	mutex->lock();

	if (!last_pkt)
		first_pkt = pkt1;
	else
		last_pkt->next = pkt1;

	last_pkt = pkt1;
	nb_packets++;
	size += pkt1->pkt.size;

	cond->wakeAll();
	mutex->unlock();

	return true;
}

// gets the front of the queue
// this blocks and repeats if block parameter is true
bool VideoFile::PacketQueue::get(AVPacket *pkt, bool block)
{
	bool ret = false;
	mutex->lock();

	for (;;)
	{
		AVPacketList *pkt1 = first_pkt;
		if (pkt1)
		{
			first_pkt = pkt1->next;
			if (!first_pkt)
				last_pkt = NULL;
			nb_packets--;
			size -= pkt1->pkt.size;
			*pkt = pkt1->pkt;
			av_free(pkt1);

			ret = true;
			break;
		}
        else if (!block)
            break;
        else
            cond->wait(mutex);
	}

	mutex->unlock();
	return ret;
}

VideoFile::PacketQueue::~PacketQueue()
{
	delete mutex;
	delete cond;
}

AVPacket *VideoFile::PacketQueue::flush_pkt = 0;
AVPacket *VideoFile::PacketQueue::eof_pkt = 0;

VideoFile::PacketQueue::PacketQueue()
{

	if (!flush_pkt)
	{
		flush_pkt = (AVPacket *) av_malloc(sizeof(AVPacket));
		Q_CHECK_PTR(flush_pkt);
		av_init_packet(flush_pkt);
		uint8_t *msg = (uint8_t *) av_malloc(sizeof(uint8_t));
		Q_CHECK_PTR(msg);
		msg[0] = 'F';
		flush_pkt->data = msg;
        flush_pkt->pts = 0;
    }

    if (!eof_pkt)
    {
        eof_pkt = (AVPacket *) av_malloc(sizeof(AVPacket));
        Q_CHECK_PTR(eof_pkt);
        av_init_packet(eof_pkt);
        uint8_t *msg = (uint8_t *) av_malloc(sizeof(uint8_t));
        Q_CHECK_PTR(msg);
        msg[0] = 'E';
        eof_pkt->data = msg;
        eof_pkt->pts = 0;
    }

	last_pkt = NULL;
	first_pkt = NULL;
	nb_packets = 0;
	size = 0;
	mutex = new QMutex();
	Q_CHECK_PTR(mutex);
	cond = new QWaitCondition();
	Q_CHECK_PTR(cond);
}

bool VideoFile::PacketQueue::flush()
{
	AVPacketList *pkt, *pkt1;

	mutex->lock();

	for (pkt = first_pkt; pkt != NULL; pkt = pkt1)
	{
		pkt1 = pkt->next;
		av_free_packet(&pkt->pkt);
		av_freep(&pkt);
	}
	last_pkt = NULL;
	first_pkt = NULL;
	nb_packets = 0;
	size = 0;

	mutex->unlock();

	return put(flush_pkt);
}

bool VideoFile::PacketQueue::isFlush(AVPacket *pkt) const
{
    return (pkt->data == flush_pkt->data);
}

bool VideoFile::PacketQueue::endFile()
{
    return put(eof_pkt);
}

bool VideoFile::PacketQueue::isEndOfFile(AVPacket *pkt) const
{
    return (pkt->data == eof_pkt->data);
}

bool VideoFile::PacketQueue::isFull() const
{
	return (size > MAX_VIDEOQ_SIZE);
}

int VideoFile::roundPowerOfTwo(int v)
{
	int k;
	if (v == 0)
		return 1;
	for (k = sizeof(int) * 8 - 1; ((static_cast<int> (1U) << k) & v) == 0; k--)
		;
	if (((static_cast<int> (1U) << (k - 1)) & v) == 0)
		return static_cast<int> (1U) << k;
	return static_cast<int> (1U) << (k + 1);
}

void VideoFile::displayFormatsCodecsInformation(QString iconfile)
{

	if (!ffmpegregistered)
	{
		avcodec_register_all();
		av_register_all();
		ffmpegregistered = true;
	}

	QVBoxLayout *verticalLayout;
	QLabel *label;
	QTreeWidget *availableFormatsTreeWidget;
	QLabel *label_2;
	QTreeWidget *availableCodecsTreeWidget;
	QDialogButtonBox *buttonBox;

	QDialog *ffmpegInfoDialog = new QDialog;
	QIcon icon;
	icon.addFile(iconfile, QSize(), QIcon::Normal, QIcon::Off);
	ffmpegInfoDialog->setWindowIcon(icon);
	ffmpegInfoDialog->resize(510, 588);
	verticalLayout = new QVBoxLayout(ffmpegInfoDialog);
	label = new QLabel(ffmpegInfoDialog);
	label_2 = new QLabel(ffmpegInfoDialog);

	availableFormatsTreeWidget = new QTreeWidget(ffmpegInfoDialog);
	availableFormatsTreeWidget->setProperty("showDropIndicator",
			QVariant(false));
	availableFormatsTreeWidget->setAlternatingRowColors(true);
	availableFormatsTreeWidget->setRootIsDecorated(false);
	availableFormatsTreeWidget->header()->setVisible(true);

	availableCodecsTreeWidget = new QTreeWidget(ffmpegInfoDialog);
	availableCodecsTreeWidget->setAlternatingRowColors(true);
	availableCodecsTreeWidget->setRootIsDecorated(false);
	availableCodecsTreeWidget->header()->setVisible(true);

	buttonBox = new QDialogButtonBox(ffmpegInfoDialog);
	buttonBox->setOrientation(Qt::Horizontal);
	buttonBox->setStandardButtons(QDialogButtonBox::Close);

	ffmpegInfoDialog->setWindowTitle(tr("FFMPEG formats and codecs"));
	label->setText(tr(
			"Compiled with libavcodec %1.%2.%3 \n\nReadable VIDEO codecs").arg(
			LIBAVCODEC_VERSION_MAJOR).arg(LIBAVCODEC_VERSION_MINOR).arg(
			LIBAVCODEC_VERSION_MICRO));
	label_2->setText(tr("Available formats"));

	QTreeWidgetItem *title = availableFormatsTreeWidget->headerItem();
	title->setText(1, tr("Description"));
	title->setText(0, tr("Name"));

	title = availableCodecsTreeWidget->headerItem();
	title->setText(1, tr("Description"));
	title->setText(0, tr("Name"));

	QObject::connect(buttonBox, SIGNAL(accepted()), ffmpegInfoDialog,
			SLOT(accept()));
	QObject::connect(buttonBox, SIGNAL(rejected()), ffmpegInfoDialog,
			SLOT(reject()));

	QMetaObject::connectSlotsByName(ffmpegInfoDialog);

	QTreeWidgetItem *formatitem;

	AVInputFormat *ifmt = NULL;
	AVCodec *p = NULL, *p2;
	const char *last_name;

	last_name = "000";
	for (;;)
	{
		const char *name = NULL;
		const char *long_name = NULL;

		while ((ifmt = av_iformat_next(ifmt)))
		{
			if ((name == NULL || strcmp(ifmt->name, name) < 0) && strcmp(
					ifmt->name, last_name) > 0)
			{
				name = ifmt->name;
				long_name = ifmt->long_name;
			}
		}
		if (name == NULL)
			break;
		last_name = name;

		formatitem = new QTreeWidgetItem(availableFormatsTreeWidget);
		formatitem->setText(0, QString(name));
		formatitem->setText(1, QString(long_name));
		availableFormatsTreeWidget->addTopLevelItem(formatitem);
	}

	last_name = "000";
	for (;;)
	{
		int decode = 0;
		int cap = 0;

		p2 = NULL;
		while ((p = av_codec_next(p)))
		{
			if ((p2 == NULL || strcmp(p->name, p2->name) < 0) && strcmp(
					p->name, last_name) > 0)
			{
				p2 = p;
				decode = cap = 0;
			}
			if (p2 && strcmp(p->name, p2->name) == 0)
			{
				if (p->decode)
					decode = 1;
				cap |= p->capabilities;
			}
		}
		if (p2 == NULL)
			break;
		last_name = p2->name;

#if LIBAVCODEC_VERSION_INT > AV_VERSION_INT(53,0,0)
		if (decode && p2->type == AVMEDIA_TYPE_VIDEO)
#else
		if (decode && p2->type == CODEC_TYPE_VIDEO)
#endif
		{
			formatitem = new QTreeWidgetItem(availableCodecsTreeWidget);
			formatitem->setText(0, QString(p2->name));
			formatitem->setText(1, QString(p2->long_name));
			availableCodecsTreeWidget->addTopLevelItem(formatitem);
		}
	}

	verticalLayout->addWidget(label);
	verticalLayout->addWidget(availableCodecsTreeWidget);
	verticalLayout->addWidget(label_2);
	verticalLayout->addWidget(availableFormatsTreeWidget);
	verticalLayout->addWidget(buttonBox);

	ffmpegInfoDialog->exec();

	delete verticalLayout;
	delete label;
	delete label_2;
	delete availableFormatsTreeWidget;
	delete availableCodecsTreeWidget;
	delete buttonBox;

	delete ffmpegInfoDialog;
}


#if LIBAVCODEC_VERSION_INT > AV_VERSION_INT(52,30,0)

QString VideoFile::getPixelFormatName(PixelFormat ffmpegPixelFormat) const
{

	if (ffmpegPixelFormat == PIX_FMT_NONE && video_st)
		ffmpegPixelFormat = video_st->codec->pix_fmt;

	QString pfn(av_pix_fmt_descriptors[ffmpegPixelFormat].name);
	pfn += QString(" (%1bpp)").arg(av_get_bits_per_pixel( &av_pix_fmt_descriptors[ffmpegPixelFormat]));

	return pfn;
}
#else
QString VideoFile::getPixelFormatName(PixelFormat ffmpegPixelFormat) const
{

	if (ffmpegPixelFormat == PIX_FMT_NONE)
	ffmpegPixelFormat =video_st->codec->pix_fmt;

	switch (ffmpegPixelFormat )
	{

		case PIX_FMT_YUV420P: return QString("planar YUV 4:2:0, 12bpp");
		case PIX_FMT_YUYV422: return QString("packed YUV 4:2:2, 16bpp");
		case PIX_FMT_RGB24:
		case PIX_FMT_BGR24: return QString("packed RGB 8:8:8, 24bpp");
		case PIX_FMT_YUV422P: return QString("planar YUV 4:2:2, 16bpp");
		case PIX_FMT_YUV444P: return QString("planar YUV 4:4:4, 24bpp");
		case PIX_FMT_RGB32: return QString("packed RGB 8:8:8, 32bpp");
		case PIX_FMT_YUV410P: return QString("planar YUV 4:1:0,  9bpp");
		case PIX_FMT_YUV411P: return QString("planar YUV 4:1:1, 12bpp");
		case PIX_FMT_RGB565: return QString("packed RGB 5:6:5, 16bpp");
		case PIX_FMT_RGB555: return QString("packed RGB 5:5:5, 16bpp");
		case PIX_FMT_GRAY8: return QString("Y,  8bpp");
		case PIX_FMT_MONOWHITE:
		case PIX_FMT_MONOBLACK: return QString("Y,  1bpp");
		case PIX_FMT_PAL8: return QString("RGB32 palette, 8bpp");
		case PIX_FMT_YUVJ420P: return QString("planar YUV 4:2:0, 12bpp (JPEG)");
		case PIX_FMT_YUVJ422P: return QString("planar YUV 4:2:2, 16bpp (JPEG)");
		case PIX_FMT_YUVJ444P: return QString("planar YUV 4:4:4, 24bpp (JPEG)");
		case PIX_FMT_XVMC_MPEG2_MC: return QString("XVideo Motion Acceleration (MPEG2)");
		case PIX_FMT_UYVY422: return QString("packed YUV 4:2:2, 16bpp");
		case PIX_FMT_UYYVYY411: return QString("packed YUV 4:1:1, 12bpp");
		case PIX_FMT_BGR32: return QString("packed RGB 8:8:8, 32bpp");
		case PIX_FMT_BGR565: return QString("packed RGB 5:6:5, 16bpp");
		case PIX_FMT_BGR555: return QString("packed RGB 5:5:5, 16bpp");
		case PIX_FMT_BGR8: return QString("packed RGB 3:3:2, 8bpp");
		case PIX_FMT_BGR4: return QString("packed RGB 1:2:1, 4bpp");
		case PIX_FMT_BGR4_BYTE: return QString("packed RGB 1:2:1,  8bpp");
		case PIX_FMT_RGB8: return QString("packed RGB 3:3:2,  8bpp");
		case PIX_FMT_RGB4: return QString("packed RGB 1:2:1,  4bpp");
		case PIX_FMT_RGB4_BYTE: return QString("packed RGB 1:2:1,  8bpp");
		case PIX_FMT_NV12:
		case PIX_FMT_NV21: return QString("planar YUV 4:2:0, 12bpp");
		case PIX_FMT_RGB32_1:
		case PIX_FMT_BGR32_1: return QString("packed RGB 8:8:8, 32bpp");
		case PIX_FMT_GRAY16BE:
		case PIX_FMT_GRAY16LE: return QString("Y, 16bpp");
		case PIX_FMT_YUV440P: return QString("planar YUV 4:4:0");
		case PIX_FMT_YUVJ440P: return QString("planar YUV 4:4:0 (JPEG)");
		case PIX_FMT_YUVA420P: return QString("planar YUV 4:2:0, 20bpp");
		case PIX_FMT_VDPAU_H264: return QString("H.264 HW");
		case PIX_FMT_VDPAU_MPEG1: return QString("MPEG-1 HW");
		case PIX_FMT_VDPAU_MPEG2: return QString("MPEG-2 HW");
		case PIX_FMT_VDPAU_WMV3: return QString("WMV3 HW");
		case PIX_FMT_VDPAU_VC1: return QString("VC-1 HW");
		case PIX_FMT_RGB48BE: return QString("packed RGB 16:16:16, 48bpp");
		case PIX_FMT_RGB48LE: return QString("packed RGB 16:16:16, 48bpp");
		case PIX_FMT_VAAPI_MOCO:
		case PIX_FMT_VAAPI_IDCT:
		case PIX_FMT_VAAPI_VLD: return QString("HW acceleration through VA API");
		default:
		return QString("unknown");
	}
}
#endif
