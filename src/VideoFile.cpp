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
 *   Copyright 2009, 2014 Bruno Herbelin
 *
 */

#include <stdint.h>

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
#include <QtGui/QPlainTextEdit>
#include <QThread>
#include <QDebug>
#include <QFileInfo>
#include <QDir>
#include <QDate>

/**
 * Size of a Mega Byte
 */
#define MEGABYTE 1048576
/**
 * Get time using libav
 */
#define GETTIME (double) av_gettime() * av_q2d(AV_TIME_BASE_Q)
/**
 * During parsing, the thread sleep for a little
 * in case there is an error or nothing to do (ms).
 */
#define PARSING_SLEEP_DELAY 100
/**
 * Waiting time when update has nothing to do (ms)
 */
#define UPDATE_SLEEP_DELAY 5

// memory policy
#define MIN_VIDEO_PICTURE_QUEUE_COUNT 3
#define MAX_VIDEO_PICTURE_QUEUE_COUNT 100
int VideoFile::memory_usage_policy = DEFAULT_MEMORY_USAGE_POLICY;
int VideoFile::maximum_packet_queue_size = MIN_PACKET_QUEUE_SIZE;
int VideoFile::maximum_video_picture_queue_size = MIN_VIDEO_PICTURE_QUEUE_SIZE;

// register ffmpeg / libav formats
bool VideoFile::ffmpegregistered = false;

// single instances of flush and end-of-file packets
AVPacket *VideoFile::PacketQueue::flush_pkt = 0;
AVPacket *VideoFile::PacketQueue::eof_pkt = 0;

#ifndef NDEBUG
int VideoFile::PacketCount = 0;
QMutex VideoFile::PacketCountLock;
int VideoFile::PacketListElementCount = 0;
QMutex VideoFile::PacketListElementCountLock;
int VideoPicture::VideoPictureCount = 0;
QMutex VideoPicture::VideoPictureCountLock;

csvLogger::csvLogger(QString filename) : QObject(0) {
    logFile.setFileName(filename + ".csv");
    logFile.open(QFile::WriteOnly | QFile::Truncate | QFile::Text);
    logStream.setDevice(&logFile);
    logStream << "videopicture,packet,packetlistsize\n";
}

void csvLogger::timerEvent(QTimerEvent *event)
{
    VideoPicture::VideoPictureCountLock.lock();
    logStream << VideoPicture::VideoPictureCount << ',';
    VideoPicture::VideoPictureCountLock.unlock();

    VideoFile::PacketCountLock.lock();
    logStream << VideoFile::PacketCount << ',';
    VideoFile::PacketCountLock.unlock();

    VideoFile::PacketListElementCountLock.lock();
    logStream << VideoFile::PacketListElementCount << '\n';
    VideoFile::PacketListElementCountLock.unlock();

    logStream.flush();
}
#else
csvLogger::csvLogger(QString filename) : QObject(0) {}
void csvLogger::timerEvent(QTimerEvent *event) {}
#endif

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
#if LIBAVCODEC_VERSION_INT < AV_VERSION_INT(55,60,0)
        _pFrame = avcodec_alloc_frame();
#else
        _pFrame = av_frame_alloc();
#endif
		Q_CHECK_PTR(_pFrame);
	}
	~DecodingThread()
	{
		// free the allocated frame
#if LIBAVCODEC_VERSION_INT < AV_VERSION_INT(55,60,0)
        av_free(_pFrame);
#else
        av_frame_free(&_pFrame);
#endif
	}

	void run();
private:
	VideoFile *is;
	AVFrame *_pFrame;
};


VideoPicture::VideoPicture(SwsContext *img_convert_ctx, int w, int h,
        enum PixelFormat format, bool rgba_palette) : pts(0), width(w), height(h), convert_rgba_palette(rgba_palette),  pixelformat(format),  img_convert_ctx_filtering(img_convert_ctx), action(ACTION_SHOW)
{
    avpicture_alloc(&rgb, pixelformat, width, height);

#ifndef NDEBUG
    VideoPicture::VideoPictureCountLock.lock();
    VideoPicture::VideoPictureCount++;
    VideoPicture::VideoPictureCountLock.unlock();
#endif

    // initialize buffer if no conversion context is provided
    if (!img_convert_ctx_filtering) {
        int nbytes = avpicture_get_size(pixelformat, width, height);
        for(int i = 0; i < nbytes; ++i)
            rgb.data[0][i] = 0;
    }
}

VideoPicture::~VideoPicture()
{
    avpicture_free(&rgb);

#ifndef NDEBUG
    VideoPicture::VideoPictureCountLock.lock();
    VideoPicture::VideoPictureCount--;
    VideoPicture::VideoPictureCountLock.unlock();
#endif
}

void VideoPicture::saveToPPM(QString filename) const
{
    if (pixelformat != AV_PIX_FMT_RGBA)
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
    if (!frame)
		return;

	// remember pts
	pts = timestamp;

    if (img_convert_ctx_filtering && !convert_rgba_palette)
    {
		// Convert the image with ffmpeg sws
        if ( 0 == sws_scale(img_convert_ctx_filtering, frame->data, frame->linesize, 0,
                  height, (uint8_t**) rgb.data, (int *) rgb.linesize) )
            // fail : set pointer to NULL (do not display)
            rgb.data[0] = NULL;

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
                    if (pixelformat == AV_PIX_FMT_RGBA)
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
    // first time a video file is created?
    if (!VideoFile::ffmpegregistered)
	{
        // register all codecs (do it once)
		avcodec_register_all();
		av_register_all();
        VideoFile::ffmpegregistered = true;

        // set log level for libav (do it once)
#ifdef NDEBUG
        av_log_set_level( AV_LOG_QUIET ); /* don't print warnings from ffmpeg */
#else
		av_log_set_level( AV_LOG_DEBUG  ); /* print debug info from ffmpeg */
#endif

#ifndef NDEBUG
        // uncomment to activate debug logs
        QString filevideologs = QFileInfo( QDir::home(), QString("glmixer_memory_logs_%1%2").arg(QDate::currentDate().toString("yyMMdd")).arg(QTime::currentTime().toString("hhmmss")) ).absoluteFilePath();
         csvLogger *debugingLogger = new csvLogger(filevideologs);
         debugingLogger->startTimer(500);
         qDebug() << "Starting CSV logger " << filevideologs;
#endif
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
    pictq_max_count = 0;

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
    first_picture_changed = true; // no mark_in set
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
    if ( !parse_tid->wait( 2 * PARSING_SLEEP_DELAY) )
        qWarning() << filename << QChar(124).toLatin1() << tr("Parsing interrupted unexpectedly.");
    pictq_cond->wakeAll();
    seek_cond->wakeAll();
    if ( !decod_tid->wait( 2 * PARSING_SLEEP_DELAY) )
        qWarning() << filename << QChar(124).toLatin1() << tr("Decoding interrupted unexpectedly.");

    if (pFormatCtx) {

        // does not hurt to ensure we flush buffers
        avcodec_flush_buffers(pFormatCtx->streams[videoStream]->codec);

        // Close codec (& threads inside)
        avcodec_close(pFormatCtx->streams[videoStream]->codec);

        // close file & free context  Free it and all its contents and set to NULL.
#if LIBAVCODEC_VERSION_INT > AV_VERSION_INT(52,100,0)
        avformat_close_input(&pFormatCtx);
#else
        av_close_input_file(pFormatCtx);
#endif
    }

    // empy pictq
    clear_picture_queue();

    // free context & filter
    if (img_convert_ctx)
        sws_freeContext(img_convert_ctx);
    if (filter)
        sws_freeFilter(filter);
    if (deinterlacing_buffer)
        av_free(deinterlacing_buffer);

    // free pictures
    if (blackPicture)
        delete blackPicture;
    if (firstPicture)
        delete firstPicture;

    // reset pointers
    img_convert_ctx = NULL;
    filter = NULL;
    pFormatCtx = NULL;
    deinterlacing_buffer = NULL;
    blackPicture = NULL;
    firstPicture = NULL;

}

VideoFile::~VideoFile()
{
	// make sure all is closed
    close();

    // delete threads
    parse_tid->quit();
    decod_tid->quit();
	delete parse_tid;
	delete decod_tid;
	delete pictq_mutex;
	delete pictq_cond;
    delete seek_mutex;
    delete seek_cond;
	delete ptimer;

    QObject::disconnect(this, 0, 0, 0);

}

void VideoFile::reset()
{
    // reset variables to 0
    fast_forward = false;
    video_pts = 0.0;
    seek_pos = 0.0;
    pictq_flush_req = false;
    parsing_mode = VideoFile::SEEKING_NONE;

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
        if ( !parse_tid->wait( 2 * PARSING_SLEEP_DELAY) )
            qWarning() << filename << QChar(124).toLatin1() << tr("Parsing interrupted unexpectedly.");
        pictq_cond->wakeAll();
        seek_cond->wakeAll();
        if ( !decod_tid->wait( 2 * PARSING_SLEEP_DELAY) )
            qWarning() << filename << QChar(124).toLatin1() << tr("Decoding interrupted unexpectedly.");

        if (!restart_where_stopped)
        {
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

        // restart at beginning
        seek_pos = mark_in;

        // except restart where we where (if valid mark)
        if (restart_where_stopped && mark_stop < mark_out && mark_stop > mark_in)
            seek_pos =  mark_stop;

        // request partsing thread to perform seek
        parsing_mode = VideoFile::SEEKING_PARSING_REQUEST;

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
    // change the picture queue size according to play speed
    // this is because, in principle, more frames are skipped when play faster
    // and we empty the queue faster
//    double sizeq = qBound(2.0, (double) video_st->nb_frames * SEEK_STEP + 1.0, (double) MAX_VIDEO_PICTURE_QUEUE_SIZE);

//    sizeq *= s;

//    pictq_max_count = qBound(2, (int) sizeq, (int) video_st->nb_frames);

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
    if (pFormatCtx)
        close();

	int err = 0;
	AVFormatContext *_pFormatCtx = 0;

	filename = file;
	ignoreAlpha = ignoreAlphaChannel;

#ifdef CONFIG_MPEG4_VDPAU_DECODER
#endif

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

        // free openned context
        avformat_free_context(_pFormatCtx);

		return false;
	}

	// if video_index not set (no video stream found) or stream open call failed
	videoStream = stream_component_open(_pFormatCtx);
    if (videoStream < 0) {
        // free openned context
        avformat_free_context(_pFormatCtx);
        //could not open Codecs (error message already sent)
		return false;
    }

    // all ok, we can set the internal pointers to the good values
    pFormatCtx = _pFormatCtx;
    video_st = pFormatCtx->streams[videoStream];

    // make sure the number of frames is correctly counted (some files have no count)
    if (video_st->nb_frames == (int64_t) AV_NOPTS_VALUE || video_st->nb_frames < 1 )
        video_st->nb_frames = (int64_t) (getDuration() / av_q2d(video_st->time_base));

    // disable multithreaded decoding for pictures
    if (video_st->nb_frames < 2)
        video_st->codec->thread_count = 1;

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

    // read picture width from video codec
    // (NB : if available, use coded width as some files have a width which is different)
    int actual_width = video_st->codec->coded_width > 0 ? video_st->codec->coded_width : video_st->codec->width;

    // fix non-aligned width (causing alignment problem in sws conversion)
    actual_width -= actual_width%16;

    // set picture size
    if (targetWidth == 0)
        targetWidth = actual_width;

    if (targetHeight == 0)
        targetHeight = video_st->codec->height;

    // round target picture size to power of two size
    if (powerOfTwo)
	{
		targetWidth = VideoFile::roundPowerOfTwo(targetWidth);
		targetHeight = VideoFile::roundPowerOfTwo(targetHeight);
	}

	// Default targetFormat to PIX_FMT_RGB24, not using color palette
    targetFormat = AV_PIX_FMT_RGB24;
    rgba_palette = false;


    // Change target format to keep Alpha channel if format requires
    if ( pixelFormatHasAlphaChannel() )
    {
        targetFormat = AV_PIX_FMT_RGBA;

        // special case of PALETTE formats which have ALPHA channel in their colors
        if (video_st->codec->pix_fmt == AV_PIX_FMT_PAL8 && !ignoreAlpha) {
            // if should NOT ignore alpha channel, use rgba palette (flag used in VideoFile)
            rgba_palette = true;
        }
    }
    // format description screen (for later)
    QString pfn(av_pix_fmt_desc_get(targetFormat)->name);

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

    filter = NULL;
    // For single frames media or when ignorealpha flag is on, force filtering
    // (The ignore alpha flag is normally requested when the source is rgba
    // and in this case, optimal conversion from rgba to rgba is to do nothing : but
    // this means there is no conversion, and no brightness/contrast is applied)
    if (ignoreAlpha || (video_st->nb_frames < 2))
        // Setup a filter to enforce a per-pixel conversion (here a slight blur)
        filter = sws_getDefaultFilter(0.0, 1.0, 0.0, 0.0, 0.0, 0.0, 0);

	// create conversion context
    // (use the actual width to match with targetWidth and avoid useless scaling)
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

    // we need a picture to display when not playing (also for single frame media)
    firstPicture = new VideoPicture(img_convert_ctx, targetWidth, targetHeight, targetFormat, rgba_palette);

	// read firstPicture (not a big problem if fails; it would just be black)
    // (NB : seek in stream only if not reading the first frame)
    current_frame_pts = fill_first_frame( mark_in != getBegin() );
    mark_stop = mark_in;

    // For videos only
    if (video_st->nb_frames > 1) {
        // we may need a black frame to return to when stopped
        blackPicture = new VideoPicture(0, targetWidth, targetHeight);

        // set picture queue maximum size
        recomputePictureQueueMaxCount();

        // tells everybody we are set !
        qDebug() << filename << QChar(124).toLatin1() <<  tr("Media opened (%1 frames, buffer of %2 MB for %3 %4 frames).").arg(video_st->nb_frames).arg((float) (pictq_max_count * firstPicture->getBufferSize()) / (float) MEGABYTE, 0, 'f', 1).arg( pictq_max_count).arg(pfn);

    }
    else {
        qDebug() << filename << QChar(124).toLatin1() <<  tr("Media opened (1 %1 frame).").arg(pfn);
    }

    // use first picture as reset picture
    resetPicture = firstPicture;

	// display a firstPicture frame ; this shows that the video is open
    emit frameReady( resetPicture );

    // say we are not running
    emit running(false);

    // all ok
	return true;
}

bool VideoFile::pixelFormatHasAlphaChannel() const
{
	if (!video_st)
		return false;

#if LIBAVCODEC_VERSION_INT > AV_VERSION_INT(55,60,0)
    return  (  (av_pix_fmt_desc_get(video_st->codec->pix_fmt)->nb_components > 3)
            // does the format has ALPHA ?
            || ( av_pix_fmt_desc_get(video_st->codec->pix_fmt)->flags & AV_PIX_FMT_FLAG_ALPHA )
            // special case of PALLETE and GREY pixel formats(converters exist for rgba)
            || ( av_pix_fmt_desc_get(video_st->codec->pix_fmt)->flags & AV_PIX_FMT_FLAG_PAL ) );
#elif LIBAVCODEC_VERSION_INT > AV_VERSION_INT(52,30,0)
	return  (av_pix_fmt_descriptors[video_st->codec->pix_fmt].nb_components > 3)
			// special case of PALLETE and GREY pixel formats(converters exist for rgba)
            || ( av_pix_fmt_descriptors[video_st->codec->pix_fmt].flags & PIX_FMT_PAL );
#else
	return (video_st->codec->pix_fmt == PIX_FMT_RGBA || video_st->codec->pix_fmt == PIX_FMT_BGRA ||
            video_st->codec->pix_fmt == PIX_FMT_ARGB || video_st->codec->pix_fmt == PIX_FMT_ABGR );
#endif
}

double VideoFile::fill_first_frame(bool seek)
{
    if (!first_picture_changed)
        return mark_in;

	AVPacket pkt1;
    AVPacket *packet = &pkt1;

#if LIBAVCODEC_VERSION_INT < AV_VERSION_INT(55,60,0)
    AVFrame *tmpframe = avcodec_alloc_frame();
#else
    AVFrame *tmpframe = av_frame_alloc();
#endif

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
    }

    // loop while we didn't finish the frame, maxi 10 times
    while (!frameFinished && trial < 10)
    {
        // read a packet
        if (av_read_frame(pFormatCtx, packet) < 0){
            // one trial less
            trial++;
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
                if (packet->dts != (int64_t) AV_NOPTS_VALUE) {
                    pts =  double(packet->dts) * av_q2d(video_st->time_base);

                    // if the obtained pts is before seeking mark
                    if (seek && pts < mark_in) {
                        // retry
                        frameFinished = false;
                    }
                }
            }
        }
    }

    if (frameFinished) {
        // we can now fill in the first picture with this frame
        firstPicture->fill((AVPicture *) tmpframe, pts);
    }
    else
        qWarning() << filename << QChar(124).toLatin1()<< tr("Could not read first frame.");

    // free memory
    av_free_packet(packet);
#if LIBAVCODEC_VERSION_INT < AV_VERSION_INT(55,60,0)
    av_free(tmpframe);
#else
    av_frame_free(&tmpframe);
#endif

    first_picture_changed = false;

    return pts;
}


// TODO : use av_find_best_stream instead of my own implementation ?
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
    if ( !codec || ( avcodec_open2(codecCtx, codec, NULL) < 0 ))
    {
        qWarning() << filename << QChar(124).toLatin1()<< tr("The codec ") << avcodec_descriptor_get(codecCtx->codec_id)->long_name << tr("is not supported.");
        return -1;
    }
#else
    if (!codec || (avcodec_open(codecCtx, codec) < 0))
	{
        qWarning() << filename << QChar(124).toLatin1()<< tr("The codec ") << codecCtx->codec_name
				<< tr("is not supported.");
		return -1;
    }
#endif


#ifndef NDEBUG
    if ( codec->capabilities & CODEC_CAP_HWACCEL_VDPAU) {

        qDebug() << filename << QChar(124).toLatin1()<< tr("This is H264 file with VDPAU capabilities.");
    }
#endif

    codecname = QString(codec->long_name);

	return stream_index;
}

void VideoFile::video_refresh_timer()
{
    // by default timer will be restarted ASAP
    int ptimer_delay = UPDATE_SLEEP_DELAY;
    bool quit_after_frame = false;
    VideoPicture *currentvp = NULL, *nextvp = NULL;

    // lock the thread to operate on the queue
    pictq_mutex->lock();

    // if all is in order, deal with the picture in the queue
    // (i.e. there is a stream, there is a picture in the queue, and the clock is not paused)
    // NB: if paused BUT the first pict in the queue is tagged for ACTION_RESET_PTS, then still proceed
    if (video_st && !pictq.empty() && (!_videoClock.paused() ||  pictq.head()->hasAction(VideoPicture::ACTION_RESET_PTS) ) )
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
        // deal with speed change before setting up the frame
        _videoClock.applyRequestedSpeed();

        // store time of this current frame
        current_frame_pts =  currentvp->presentationTime();

//        fprintf(stderr, "video_refresh_timer pts %f \n", current_frame_pts);

        // if this frame was tagged as stopping frame
        if ( currentvp->hasAction(VideoPicture::ACTION_STOP) ) {
            // request to stop the video after this frame
            quit_after_frame = true;
        }

        // this frame was tagged to reset the timer (seeking frame usually)
        if ( currentvp->hasAction(VideoPicture::ACTION_RESET_PTS) ) {
            // reset clock to the time of the frame
            _videoClock.reset(current_frame_pts);
            // inform that seeking is done
            emit seekEnabled(true);
        }

        // this frame is tagged to be displayed
        if ( currentvp->hasAction(VideoPicture::ACTION_SHOW) ) {

            // ask to show the current picture (and to delete it when done)
            currentvp->addAction(VideoPicture::ACTION_DELETE);
            emit frameReady(currentvp);

            // if there is a next picture
            // we can compute when to present the next frame
            if (nextvp) {
                double delay = 0.0;

                // if next frame we will be seeking
                if ( nextvp->hasAction(VideoPicture::ACTION_RESET_PTS) )
                    // update at normal fps, discarding computing of delay
                    delay = _videoClock.timeBase();
                else
                    // otherwise read presentation time and compute delay till next frame
                    delay = ( nextvp->presentationTime() - _videoClock.time() ) / _videoClock.speed() ;

                // if delay is correct
                if ( delay > _videoClock.minFrameDelay() ) {
                    // schedule normal delayed display of next frame
                    ptimer_delay = (int) (delay * 1000.0);

                // delay is too small, or negative
                } else {

                    // retry shortly (but not too fast to avoid glitches)
                    ptimer_delay = (int) (_videoClock.minFrameDelay() * 1000.0);

                    // remove the show tag for that frame (i.e. skip it)
                    nextvp->removeAction(VideoPicture::ACTION_SHOW);
                }

            }

        }
        // NOT VISIBLE ? skip this frame...
        else {
            // delete the picture
            delete currentvp;
            // loop ASAP
            ptimer_delay = UPDATE_SLEEP_DELAY;
       }

       if (fast_forward) {
           _videoClock.reset(current_frame_pts);
           ptimer_delay = UPDATE_SLEEP_DELAY;
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

double VideoFile::getFrameDuration() const
{
    if (video_st)
    {
        // from average framerate from libav, if correct.
        if (video_st->avg_frame_rate.num > 0)
            return (double) video_st->avg_frame_rate.den / (double) video_st->avg_frame_rate.num;

        // else :  inverse of frame rate
        else
            return 1.0 / getFrameRate();
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
    if (pFormatCtx && video_st && !quit)
    {
        t = qBound(getBegin(), t, getEnd());

        //  try to go to the frame in the decoding queue if looping
        if ( !loop_video || !decodingSeekRequest(t) )  {
            // if not looping or if couln't seek with decoder only
            // request to seek in the parser, with flushing of the buffer
            pictq_flush_req = true;
            parsingSeekRequest(t);
        }

        emit seekEnabled(false);
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

void VideoFile::seekByFrames(int seekFrameStep)
{
    // seek by how many seconds the number of frames corresponds to
    seekBySeconds( (double) seekFrameStep * av_q2d(video_st->time_base));
}


void VideoFile::seekForwardOneFrame()
{
    if (!pictq.isEmpty())
        // tag this frame as a RESET frame ; this enforces its processing in video_refresh_timer
        pictq.head()->addAction(VideoPicture::ACTION_RESET_PTS);
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
    if (video_st && video_st->start_time != (int64_t) AV_NOPTS_VALUE )
        return double(video_st->start_time) * av_q2d(video_st->time_base);

    if (pFormatCtx && pFormatCtx->start_time != (int64_t) AV_NOPTS_VALUE )
        return double(pFormatCtx->start_time) * av_q2d(AV_TIME_BASE_Q);

    return 0.0;
}

double VideoFile::getDuration() const
{
    if (video_st && video_st->duration != (int64_t) AV_NOPTS_VALUE )
        return double(video_st->duration) * av_q2d(video_st->time_base);

    if (pFormatCtx && pFormatCtx->duration != (int64_t) AV_NOPTS_VALUE )
        return double(pFormatCtx->duration) * av_q2d(AV_TIME_BASE_Q);

    return 0.0;
}

double VideoFile::getEnd() const
{
    return getBegin() + getDuration();
}

void VideoFile::cleanupPictureQueue(double time) {

    if (time < getBegin())
        time = getEnd();

    // get hold on the picture queue
    pictq_mutex->lock();

    // loop to find a mark frame in the queue
    int i =  0;
    while ( i < pictq.size() && !pictq[i]->hasAction(VideoPicture::ACTION_MARK) && pictq[i]->getPts() < time)
        i++;

    // found a mark frame in the queue
    if ( pictq.size() > i ) {
        // remove all what is after
        while ( pictq.size() > i)
            delete pictq.takeLast();

        // sanity check (but should never be the case)
        if (! pictq.empty())
            // restart filling in at the last pts of the cleanned queue
            parsingSeekRequest( pictq.takeLast()->getPts() );
    }
    // done with the cleanup
    pictq_mutex->unlock();

}

void VideoFile::setLoop(bool loop) {

    loop_video = loop;

    // Cleanup the queue until the next mark it contains
    cleanupPictureQueue();

}

void VideoFile::recomputePictureQueueMaxCount()
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
    mark_in = qBound(getBegin(), time, mark_out - av_q2d(video_st->time_base));

    // if requested mark_in is after current time
    if ( !(mark_in < current_frame_pts) )
        // seek to mark in
        seekToPosition(mark_in);
    // else mark in is before the current time, but the queue might already have looped
    else
        // Cleanup the queue until the next mark it contains
        cleanupPictureQueue();

    // inform about change in marks
    emit markingChanged();

    // change the picture queue size to match the new interval
    recomputePictureQueueMaxCount();

    // remember that we have to update the first picture
    first_picture_changed = true;

}



void VideoFile::setMarkOut(double time)
{
    // set the mark_out
    // reserve at least 1 frame interval with mark in
    mark_out = qBound(mark_in + av_q2d(video_st->time_base), time, getEnd());

    // if requested mark_out is before current time or
    if ( !(mark_out > current_frame_pts) )
        // seek to mark in
        seekToPosition(mark_out);
    // else mark in is before the current time, but the queue might already have looped
    else
        // Cleanup the queue until the next mark it contains, or until the new mark_out
        cleanupPictureQueue(mark_out);

    // inform about change in marks
    emit markingChanged();

    // charnge the picture queue size to match the new interval
    recomputePictureQueueMaxCount();
}

bool VideoFile::timeInQueue(double time)
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
    if (video_st && video_st->codec) {

        // base computation of aspect ratio
        double aspect_ratio = (double) video_st->codec->width / (double) video_st->codec->height;

        // use correction of aspect ratio if available in the video stream
        if (video_st->codec->sample_aspect_ratio.num > 1)
            aspect_ratio *= av_q2d(video_st->codec->sample_aspect_ratio);

        return aspect_ratio;
    }

    // default case
    return firstPicture->getAspectRatio();
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
        if (is->parsing_mode == VideoFile::SEEKING_PARSING_REQUEST) {
            // enter the seeking mode (disabled only when target reached)
            is->parsing_mode = VideoFile::SEEKING_DECODING_REQUEST;
            // compute dts of seek target from seek position
            seek_target = av_rescale_q(is->seek_pos, (AVRational){1, 1}, is->video_st->time_base);
        }
        is->seek_cond->wakeAll();
        is->seek_mutex->unlock();

        // decided to perform seek
        if (seek_target != (int64_t) AV_NOPTS_VALUE)
        {
            // configure seek options
            // always seek for the frame before the target
            int flags = AVSEEK_FLAG_BACKWARD;
            if ( is->seek_any )
				flags |= AVSEEK_FLAG_ANY;

            // request seek to libav
            if (av_seek_frame(is->pFormatCtx, is->videoStream, seek_target, flags) < 0)
                qDebug() << is->filename << QChar(124).toLatin1()
                        << QObject::tr("Could not seek to frame (%1); jumping where I can!").arg(is->seek_pos);

			// after seek,  we'll have to flush buffers
            // this also queues a flush packet
            if (!is->videoq.flush())
                qWarning() << is->filename << QChar(124).toLatin1()<< QObject::tr("Flushing error.");

            // hack: because when paused, the decoding thread is waiting for space in the queue
            // flush the queue to unblock it
            if (is->isPaused() && is->pictq_flush_req) {
                is->flush_picture_queue();
                // but restore flush request
                is->pictq_flush_req = true;
            }

            continue;
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
                // no error : the read frame reached the end of file
                is->videoq.endFile();

			// no error; just wait a bit for the end of the packet and continue
            msleep(PARSING_SLEEP_DELAY);
			continue;
        }

        // test if it was NOT a video stream packet : free the packet
        if (packet->stream_index != is->videoStream) {

            av_free_packet(packet);

        }
        else {

#ifndef NDEBUG
            VideoFile::PacketCountLock.lock();
            VideoFile::PacketCount++;
            VideoFile::PacketCountLock.unlock();
#endif

            if ( !is->videoq.put(packet) ) {
                // we need to free the packet if it was not put in the queue
                av_free_packet(packet);
#ifndef NDEBUG
                VideoFile::PacketCountLock.lock();
                VideoFile::PacketCount--;
                VideoFile::PacketCountLock.unlock();
#endif
            }
        }

    }
    // quit

	// request flushing of the video queue (this will end the decoding thread)
    if (!is->videoq.flush())
        qWarning() << is->filename << QChar(124).toLatin1() << QObject::tr("Flushing error at end.");

}

void VideoFile::clear_picture_queue() {

    while (!pictq.isEmpty()) {
        VideoPicture *p = pictq.dequeue();
        delete p;
    }

}

void VideoFile::flush_picture_queue()
{
    pictq_flush_req = false;

    pictq_mutex->lock();
    clear_picture_queue();
    pictq_cond->wakeAll();
    pictq_mutex->unlock();

}


void VideoFile::parsingSeekRequest(double time)
{
    if ( parsing_mode != VideoFile::SEEKING_PARSING_REQUEST )
    {
        seek_mutex->lock();
        seek_pos = time;
        parsing_mode = VideoFile::SEEKING_PARSING_REQUEST;
        // wait for the parsing thread to aknowledge the seek request
        seek_cond->wait(seek_mutex);
        seek_mutex->unlock();
    }
}



bool VideoFile::decodingSeekRequest(double time)
{
    // do not attempt to seek if queue is too small or if already seeking
    if ( pictq.size() < 2 || parsing_mode != VideoFile::SEEKING_NONE)
        return false;

    // discard too small seek
    if ( qAbs( time - pictq.head()->getPts() ) < av_q2d(video_st->time_base) )
        return false;

    // Is there a picture with the seeked time into the queue ?
    if ( ! timeInQueue(time)  )
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

void VideoFile::queue_picture(AVFrame *pFrame, double pts, VideoPicture::Action a)
{
    // create vp as the picture in the queue to be written
    VideoPicture *vp = new VideoPicture(img_convert_ctx, targetWidth, targetHeight, targetFormat, rgba_palette);

    if (!vp)
        return;

    // cast frame pointer to picture
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

    /* now we inform our display thread that we have a pic ready */
    pictq_mutex->lock();
    // enqueue this picture in the queue
    pictq.enqueue(vp);
    // inform about the new size of the queue
    pictq_mutex->unlock();

//    fprintf(stderr, "queued pict pts %f action %d\n", pts, a);
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
    int frameFinished = 0;
    double pts = 0.0; // Presentation time stamp
    int64_t dts = 0; // Decoding time stamp

	while (is)
    {

		// get front packet (not blocking if we are quitting)
        if (!is->videoq.get(packet, !is->quit))
            // this is the exit condition
            // (does not have a packet to free)
            break;

		// special case of flush packet
        // it is put after seeking in ParsingThread
        if (VideoFile::PacketQueue::isFlush(packet))
        {
            // flush buffers
            avcodec_flush_buffers(is->video_st->codec);

            // now asking the decoding thread to find the seeked frame
            is->seek_mutex->lock();
            is->parsing_mode = VideoFile::SEEKING_DECODING_REQUEST;
            is->seek_mutex->unlock();

            // go on to next packet (do not free flush packet)
            continue;
		}

        // special case of EndofFile packet
        // this happens when hitting end of file when parsing
        if (VideoFile::PacketQueue::isEndOfFile(packet))
        {

            // restart parsing at beginning in any case
            // (avoid leaving the parsing stuck at end of file
            // and forces the pictq to be freed)
            is->parsingSeekRequest(is->mark_in);

            // react according to loop mode
            if ( is->loop_video) {

                // go on to next packet (do not free eof packet)
                continue;
            }
            else {
                // if stopping,  re-sends the previous frame with stop flag
                // and pretending it is one frame later
                is->queue_picture(_pFrame, pts + is->getFrameDuration(), VideoPicture::ACTION_STOP | VideoPicture::ACTION_MARK);
                // stop here (do not free eof packet)
                break;
            }

        }

        // remember packet pts in case the decoding loose it
		is->video_st->codec->reordered_opaque = packet->pts;

        // Decode video frame
#if LIBAVCODEC_VERSION_INT > AV_VERSION_INT(52,30,0)
        if ( avcodec_decode_video2(is->video_st->codec, _pFrame, &frameFinished, packet) >= 0 ) {
#else
        if ( avcodec_decode_video(is->video_st->codec, _pFrame, &frameFinished, packet->data, packet->size) >= 0) {
#endif

            // Did we get a full video frame?
            if (frameFinished > 0)
            {
                VideoPicture::Action actionFrame = 0;

                // get packet decompression time stamp (dts)
                dts = 0;
                if (packet->dts != (int64_t) AV_NOPTS_VALUE)
                    dts = packet->dts;
                else if (_pFrame->reordered_opaque != (int64_t) AV_NOPTS_VALUE)
                    dts = _pFrame->reordered_opaque;

                // compute presentation time stamp
                pts = is->synchronize_video(_pFrame, double(dts) * av_q2d(is->video_st->time_base));

                // if seeking in decoded frames
                if (is->parsing_mode == VideoFile::SEEKING_DECODING_REQUEST) {

                    // Skip all pFrame which didn't reach the seeking position
                    // Stop seeking when seeked time is reached
                    if ( !(pts < is->seek_pos) ) {

                        // flush the picture queue on request
                        // (i.e. when done seeking from setSeekTarget)
                        if (is->pictq_flush_req)
                            is->flush_picture_queue();

                        // this frame is the result of the seeking process
                        // (the ACTION_RESET_PTS in video refresh timer will reset the clock)
                        actionFrame = VideoPicture::ACTION_RESET_PTS;

                        // reached the seeked frame! : can say we are not seeking anymore
                        is->seek_mutex->lock();
                        is->parsing_mode = VideoFile::SEEKING_NONE;
                        is->seek_mutex->unlock();

    //                    fprintf(stderr, "DecodingThread reached pts %f queue size %d\n", pts, is->pictq.size());
                        // if the seek position we reached equals the mark_in
                        if ( qAbs( is->seek_pos - is->mark_in ) < is->getFrameDuration() )
                            // tag the frame as a MARK frame
                            actionFrame |= VideoPicture::ACTION_MARK;

                    }
                    else {
                        // save a bit of time for filling in the first frame : detect it
                        // (time between pts and mark in is less than time for one frame)
                        if ( is->first_picture_changed &&  (qAbs(pts - is->mark_in) < is->getFrameDuration() ) ) {
                            is->firstPicture->fill((AVPicture*) _pFrame, pts);
                            is->first_picture_changed = false;

                        }
                    }
                }

                // if not seeking, queue picture for display
                // (not else of previous if because it could have unblocked this frame)
                if (is->parsing_mode != VideoFile::SEEKING_DECODING_REQUEST ) {

                    // test if time will exceed limits (mark out or will pass the end of file)
                    if ( pts > is->mark_out )
                    {
                        // react according to loop mode
                        if (is->loop_video)
                            // if loop mode on, request seek to begin and do not queue the frame
                            is->parsingSeekRequest(is->mark_in);
                        else
                            // if loop mode off, stop after this frame
                            is->queue_picture(_pFrame, pts, VideoPicture::ACTION_STOP | VideoPicture::ACTION_MARK);

                    }
                    else
                        // default
                        // add frame to the queue of pictures
                        is->queue_picture(_pFrame, pts, actionFrame);


                    // wait until we have space for a new pic
                    is->pictq_mutex->lock();
                    // the condition is released in video_refresh_timer()
                    while ( (is->pictq.count() > is->pictq_max_count) && !is->quit )
                        is->pictq_cond->wait(is->pictq_mutex);
                    is->pictq_mutex->unlock();

                    // if have to quit
                    if (is->quit)
                        is->flush_picture_queue();
                }
            }
        }

		// packet was decoded, should be removed
        av_free_packet(packet);

#ifndef NDEBUG
        VideoFile::PacketCountLock.lock();
        VideoFile::PacketCount--;
        VideoFile::PacketCountLock.unlock();
#endif
	}

    // if normal exit through break (couldn't get any more packet)
    if (is)
        // clear the queue
        is->videoq.clear();

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
//    if (pkt != VideoFile::PacketQueue::flush_pkt &&
//            pkt != VideoFile::PacketQueue::eof_pkt && av_dup_packet(pkt) < 0)
//        return false;

	AVPacketList *pkt1 = (AVPacketList*) av_malloc(sizeof(AVPacketList));
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

#ifndef NDEBUG
    VideoFile::PacketListElementCountLock.lock();
    if ( !VideoFile::PacketQueue::isFlush( &(pkt1->pkt) ) && !VideoFile::PacketQueue::isEndOfFile( &(pkt1->pkt) ) )
        VideoFile::PacketListElementCount += pkt1->pkt.size;
    VideoFile::PacketListElementCountLock.unlock();
#endif

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

#ifndef NDEBUG
    VideoFile::PacketListElementCountLock.lock();
    if ( !VideoFile::PacketQueue::isFlush( &(pkt1->pkt) ) && !VideoFile::PacketQueue::isEndOfFile( &(pkt1->pkt) ) )
        VideoFile::PacketListElementCount -= pkt1->pkt.size;
    VideoFile::PacketListElementCountLock.unlock();
#endif

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
    clear();

    delete mutex;
    delete cond;
}


VideoFile::PacketQueue::PacketQueue()
{

    if (!VideoFile::PacketQueue::flush_pkt)
	{
        VideoFile::PacketQueue::flush_pkt = (AVPacket *) av_malloc(sizeof(AVPacket));
        Q_CHECK_PTR(VideoFile::PacketQueue::flush_pkt);
        av_init_packet(VideoFile::PacketQueue::flush_pkt);
		uint8_t *msg = (uint8_t *) av_malloc(sizeof(uint8_t));
		Q_CHECK_PTR(msg);
		msg[0] = 'F';
        VideoFile::PacketQueue::flush_pkt->data = msg;
        VideoFile::PacketQueue::flush_pkt->pts = 0;
    }

    if (!VideoFile::PacketQueue::eof_pkt)
    {
        VideoFile::PacketQueue::eof_pkt = (AVPacket *) av_malloc(sizeof(AVPacket));
        Q_CHECK_PTR(VideoFile::PacketQueue::eof_pkt);
        av_init_packet(VideoFile::PacketQueue::eof_pkt);
        uint8_t *msg = (uint8_t *) av_malloc(sizeof(uint8_t));
        Q_CHECK_PTR(msg);
        msg[0] = 'E';
        VideoFile::PacketQueue::eof_pkt->data = msg;
        VideoFile::PacketQueue::eof_pkt->pts = 0;
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

void VideoFile::PacketQueue::clear()
{
    AVPacketList *pkt, *pkt1;

    for (pkt = first_pkt; pkt != NULL; pkt = pkt1)
    {
        pkt1 = pkt->next;

        // do not free flush or eof packets
        if ( !VideoFile::PacketQueue::isFlush( &(pkt->pkt) ) && !VideoFile::PacketQueue::isEndOfFile( &(pkt->pkt) ) ) {

#ifndef NDEBUG
            VideoFile::PacketListElementCountLock.lock();
            VideoFile::PacketListElementCount -= pkt->pkt.size;
            VideoFile::PacketListElementCountLock.unlock();
#endif
            av_free_packet(&(pkt->pkt));

#ifndef NDEBUG
            VideoFile::PacketCountLock.lock();
            VideoFile::PacketCount--;
            VideoFile::PacketCountLock.unlock();
#endif

        }

        // free list element
        av_freep(&pkt);

    }

    last_pkt = NULL;
    first_pkt = NULL;
    nb_packets = 0;
    size = 0;
}

bool VideoFile::PacketQueue::flush()
{
    mutex->lock();
    // clear the queue
    clear();
    mutex->unlock();

    return put(VideoFile::PacketQueue::flush_pkt);;
}

bool VideoFile::PacketQueue::isFlush(AVPacket *pkt)
{
    return (pkt->data == VideoFile::PacketQueue::flush_pkt->data);
}

bool VideoFile::PacketQueue::endFile()
{
    bool ret = false;

    mutex->lock();
    // discard if already a eof packet at end
    if ( last_pkt && isEndOfFile(&last_pkt->pkt) )
        ret = true;
    mutex->unlock();

    if (!ret)
        // put a eof packet at end
        ret = put(VideoFile::PacketQueue::eof_pkt);

    return ret;
}

bool VideoFile::PacketQueue::isEndOfFile(AVPacket *pkt)
{
    return (pkt->data == VideoFile::PacketQueue::eof_pkt->data);
}

bool VideoFile::PacketQueue::isFull() const
{
    return (size > (VideoFile::maximum_packet_queue_size * MEGABYTE));
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

void VideoFile::setMemoryUsagePolicy(int percent)
{
    VideoFile::memory_usage_policy = percent;
    double p = qBound(0.0, (double) percent / 100.0, 1.0);
    VideoFile::maximum_packet_queue_size = MIN_PACKET_QUEUE_SIZE + (int)( p * (MAX_PACKET_QUEUE_SIZE - MIN_PACKET_QUEUE_SIZE));
    VideoFile::maximum_video_picture_queue_size = MIN_VIDEO_PICTURE_QUEUE_SIZE + (int)( p * (MAX_VIDEO_PICTURE_QUEUE_SIZE - MIN_VIDEO_PICTURE_QUEUE_SIZE));

}

int VideoFile::getMemoryUsagePolicy()
{
    return VideoFile::memory_usage_policy;
}

int VideoFile::getMemoryUsageMaximum(int policy)
{
    double p = qBound(0.0, (double) policy / 100.0, 1.0);
    int max = MIN_PACKET_QUEUE_SIZE + (int)( p * (MAX_PACKET_QUEUE_SIZE - MIN_PACKET_QUEUE_SIZE));
    max += MIN_VIDEO_PICTURE_QUEUE_SIZE + (int)( p * (MAX_VIDEO_PICTURE_QUEUE_SIZE - MIN_VIDEO_PICTURE_QUEUE_SIZE));

    return max;
}


void VideoFile::displayFormatsCodecsInformation(QString iconfile)
{

    if (!VideoFile::ffmpegregistered)
	{
		avcodec_register_all();
		av_register_all();
        VideoFile::ffmpegregistered = true;
	}

    QVBoxLayout *verticalLayout;
    QLabel *label;
    QPlainTextEdit *options;
    QLabel *label_2;
	QTreeWidget *availableFormatsTreeWidget;
    QLabel *label_3;
	QTreeWidget *availableCodecsTreeWidget;
    QDialogButtonBox *buttonBox;

	QDialog *ffmpegInfoDialog = new QDialog;
	QIcon icon;
	icon.addFile(iconfile, QSize(), QIcon::Normal, QIcon::Off);
	ffmpegInfoDialog->setWindowIcon(icon);
    ffmpegInfoDialog->resize(450, 600);
    verticalLayout = new QVBoxLayout(ffmpegInfoDialog);
    label = new QLabel(ffmpegInfoDialog);
    label_2 = new QLabel(ffmpegInfoDialog);
    label_3 = new QLabel(ffmpegInfoDialog);

    availableFormatsTreeWidget = new QTreeWidget(ffmpegInfoDialog);
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
    QObject::connect(buttonBox, SIGNAL(rejected()), ffmpegInfoDialog, SLOT(reject()));

    ffmpegInfoDialog->setWindowTitle(tr("About Libav formats and codecs"));

    label->setWordWrap(true);
    label->setOpenExternalLinks(true);
    label->setTextFormat(Qt::RichText);
    label->setText(tr( "<H3>About Libav</H3>"
                       "<br>This program uses <b>libavcodec %1.%2.%3</b>."
                       "<br><br><b>Libav</b> provides cross-platform tools and libraries to convert, manipulate and "
                       "stream a wide range of multimedia formats and protocols."
                       "<br>For more information : <a href=\"http://www.libav.org\">http://www.libav.org</a>"
                       "<br><br>Compilation options:").arg(LIBAVCODEC_VERSION_MAJOR).arg(LIBAVCODEC_VERSION_MINOR).arg( LIBAVCODEC_VERSION_MICRO));

    options = new QPlainTextEdit( QString(avcodec_configuration()), ffmpegInfoDialog);

    label_2->setText(tr("Available codecs:"));
    label_3->setText(tr("Available formats:"));

	QTreeWidgetItem *title = availableFormatsTreeWidget->headerItem();
	title->setText(1, tr("Description"));
	title->setText(0, tr("Name"));

	title = availableCodecsTreeWidget->headerItem();
	title->setText(1, tr("Description"));
	title->setText(0, tr("Name"));

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

    verticalLayout->setSpacing(10);
    verticalLayout->addWidget(label);
    verticalLayout->addWidget(options);
    verticalLayout->addWidget(label_2);
	verticalLayout->addWidget(availableCodecsTreeWidget);
    verticalLayout->addWidget(label_3);
	verticalLayout->addWidget(availableFormatsTreeWidget);
	verticalLayout->addWidget(buttonBox);

	ffmpegInfoDialog->exec();

	delete verticalLayout;
    delete label;
    delete label_2;
    delete label_3;
    delete options;
    delete availableFormatsTreeWidget;
    delete availableCodecsTreeWidget;
    delete buttonBox;

    delete ffmpegInfoDialog;
}


#if LIBAVCODEC_VERSION_INT > AV_VERSION_INT(55,60,0)
QString VideoFile::getPixelFormatName() const
{
    QString pfn(av_pix_fmt_desc_get(video_st->codec->pix_fmt)->name);
    pfn += QString(" (%1 bpp)").arg(av_get_bits_per_pixel( av_pix_fmt_desc_get(video_st->codec->pix_fmt)) );

    return pfn;
}
#elif LIBAVCODEC_VERSION_INT > AV_VERSION_INT(52,30,0)
QString VideoFile::getPixelFormatName() const
{
    QString pfn(av_pix_fmt_descriptors[video_st->codec->pix_fmt].name);
    pfn += QString(" (%1bpp)").arg(av_get_bits_per_pixel( &av_pix_fmt_descriptors[video_st->codec->pix_fmt]));

	return pfn;
}
#else
QString VideoFile::getPixelFormatName(PixelFormat ffmpegPixelFormat) const
{
    PixelFormat ffmpegPixelFormat =video_st->codec->pix_fmt;

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
