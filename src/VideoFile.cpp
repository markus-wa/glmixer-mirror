/*
 * VideoFile.cpp
 *
 *  Created on: Jul 10, 2009
 *      Author: bh
 */

//#define QT_NO_DEBUG_OUTPUT
//#define QT_NO_WARNING_OUTPUT
#define QT_FATAL_WARNINGS

#define __STDC_CONSTANT_MACROS
#include <stdint.h>

#ifndef INT64_MIN
#define INT64_MIN       (-0x7fffffffffffffffLL-1)
#endif

#ifndef INT64_MAX
#define INT64_MAX INT64_C(9223372036854775807)
#endif

extern "C" {
#include <libavcodec/avcodec.h>
#include <libswscale/swscale.h>
#include <libavutil/mathematics.h>
#include <libavutil/common.h>
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
#include <QFileInfo>

class ParsingThread: public QThread {
public:
    ParsingThread(VideoFile *video = 0) :
        QThread((QObject *) video), is(video) {
    }
    ~ParsingThread() {
    }

    void run();
private:
    VideoFile *is;

};

class DecodingThread: public QThread {
public:
    DecodingThread(VideoFile *video = 0) :
        QThread((QObject *) video), is(video) {
    }
    ~DecodingThread() {
    }

    void run();
private:
    VideoFile *is;
};


VideoPicture::VideoPicture() :
		rgb(NULL), width(0), height(0), allocated(false), pts(0.0) {

	img_converter = NULL;
}

bool VideoPicture::allocate(int w, int h, enum PixelFormat format) {

    // if we already have one, make another
    if (rgb) {
        // free image
        if (rgb->data)
            av_freep(rgb->data);
        av_freep(rgb);
    }

    // Allocate a place to put our image
    width = w;
    height = h;
    pixelformat = format;

    // Determine required buffer size and allocate buffer
    int numBytes = avpicture_get_size(pixelformat, width, height+1);
    uint8_t *buffer = static_cast<uint8_t*>(av_mallocz(numBytes + FF_INPUT_BUFFER_PADDING_SIZE));

    Q_CHECK_PTR(buffer);

    if (buffer != NULL) {
        // initialize buffer to black
        for (int i = 0; i < numBytes; ++i)
            buffer[i] = 0;
        // create the frame
        rgb = avcodec_alloc_frame();
        // setup the Video Picture buffer
        if (rgb != NULL) {
            avpicture_fill( reinterpret_cast<AVPicture*>(rgb), buffer, pixelformat, width, height+1);
			allocated = true;
        } else
            return false;

    } else
        return false;

	// a software converter to apply brightness & contrast filters
	img_converter = sws_getContext(width, height+1, pixelformat, width, height, pixelformat, SWS_POINT, NULL, NULL, NULL);
	// NB: when both dimensions and formats are equal, a copy converter is automatically selected by libswscale (optimal choice)
	// but here, we DO want to process the pixels with the swscale to apply filtering. So, the trick is to consider an extra line
	// (allocated) in the src frame so that the destination height is not the same.
	// The copy operator takes it into account by NOT copying it (srcSliceY =1)

    return true;
}

VideoPicture::~VideoPicture() {
    if (rgb) {
        // free image
        if (rgb->data)
            av_freep(rgb->data);
        av_freep(rgb);
    }
}

VideoPicture& VideoPicture::operator=(VideoPicture const &original) {
    if (!allocated)
        allocate(original.width, original.height, original.pixelformat);

    if (allocated && original.allocated && width == original.width && height == original.height && pixelformat == original.pixelformat) {
    	if (!img_converter) {
    		// no converter ? just copy;
			int numBytes = avpicture_get_size(pixelformat, width, height);
			for (int i=0; i<numBytes; ++i)
				rgb->data[0][i] = original.rgb->data[0][i];
			pts = original.pts;
    	} else
			// apply the modified converter to copy (with the srcSliceY = 1 to avoid extra line)
			sws_scale(img_converter, original.rgb->data, original.rgb->linesize, 1, height, rgb->data, rgb->linesize);
    }
    return *this;
}


void VideoPicture::setCopyFiltering(int b, int c, int s){
	// check values
	if ( !img_converter || ( b < -100 && b > 100) || ( c < -100 && c > 100) || ( s < -100 && s > 100) )
		return;

	// Modify the sws converter used internally
	int *inv_table, srcrange, *table, dstrange, brightness, contrast, saturation;
	if ( -1 != sws_getColorspaceDetails(img_converter, &inv_table, &srcrange, &table, &dstrange, &brightness, &contrast, &saturation) ) {
		// ok, can modify the converter
        brightness = (( b <<16) + 50)/100;
        contrast = ((( c +100)<<16) + 50)/100;
		saturation = ((( s +100)<<16) + 50)/100;
		sws_setColorspaceDetails(img_converter, inv_table, srcrange, table, dstrange, brightness, contrast, saturation);
	}
}


void VideoPicture::saveToPPM(QString filename) const {

    if ( allocated && rgb ) {
        FILE *pFile;
        int  y;

        // Open file
        pFile = fopen(filename.toUtf8().data(), "wb");
        if(pFile==NULL)
            return;

        // Write header
        fprintf(pFile, "P6\n%d %d\n255\n", width, height);

        // Write pixel data
        for(y=0; y<height; y++)
            fwrite(rgb->data[0] + y*rgb->linesize[0], 1, width*3, pFile);

        // Close file
        fclose(pFile);
    }
}



bool VideoFile::ffmpegregistered = false;

VideoFile::VideoFile(QObject *parent, bool generatePowerOfTwo, int swsConversionAlgorithm, int destinationWidth, int destinationHeight) :
    QObject(parent), filename(QString()) {

    if (!ffmpegregistered) {
        avcodec_register_all();
        av_register_all();
#ifdef NDEBUG
        av_log_set_level( AV_LOG_QUIET ); /* don't print warnings from ffmpeg */
#endif
        ffmpegregistered = true;
    }

    // read software conversion parameters
    targetWidth = destinationWidth;
    targetHeight = destinationHeight;
    targetFormat = PIX_FMT_RGB24;
    powerOfTwo = generatePowerOfTwo;
    conversionAlgorithm = swsConversionAlgorithm;

    // Init some pointers to NULL
    videoStream = -1;
    video_st = NULL;
    pFormatCtx = NULL;
    img_convert_ctx = NULL;
    resetPicture = NULL;

    // Contruct some objects
    parse_tid = new ParsingThread(this);
    Q_CHECK_PTR(parse_tid);
    decod_tid = new DecodingThread(this);
    Q_CHECK_PTR(decod_tid);
    pictq_mutex = new QMutex;
    Q_CHECK_PTR(pictq_mutex);
    pictq_cond = new QWaitCondition;
    Q_CHECK_PTR(pictq_cond);

    QObject::connect(parse_tid, SIGNAL(finished()), this, SLOT(thread_terminated()));
    QObject::connect(decod_tid, SIGNAL(finished()), this, SLOT(thread_terminated()));

    ptimer = new QTimer(this);
    Q_CHECK_PTR(ptimer);
    ptimer->setSingleShot(true);
    QObject::connect(ptimer, SIGNAL(timeout()), this, SLOT(video_refresh_timer()));

    // initialize behavior
    play_speed = 1.0; // normal play speed
    seek_any = false; // NOT dirty seek
    seek_backward = false;
    pause_video = false; // not paused by default
    loop_video = true; // loop by default
    quit = true; // not running yet
    restart_where_stopped = true;
    mark_stop = 0;
    is_synchronized = false;

    // reset
    reset();
}

VideoFile::~VideoFile() {

    // tries to end properly
    quit = true;
    // wait for threads to end properly
    parse_tid->wait();
    pictq_cond->wakeAll();
    decod_tid->wait();

    if (img_convert_ctx)
        sws_freeContext(img_convert_ctx);

    if (pFormatCtx)
        av_close_input_file(pFormatCtx);

    // delete threads
    delete parse_tid;
    delete decod_tid;
    delete pictq_mutex;
    delete pictq_cond;
    if (!is_synchronized) {
    	delete ptimer;
    }

	QObject::disconnect(this,0,0,0);
}


void VideoFile::sendInfo(QString m){
	emit info( QString("%1 : %2").arg(filename).arg(m) );
}

void VideoFile::synchroniseWithVideo(VideoFile *vf){

    if (ptimer)
    	delete ptimer;
	this->ptimer = vf->ptimer;
	is_synchronized = true;
    QObject::connect(ptimer, SIGNAL(timeout()), this, SLOT(video_refresh_timer()));
}


void VideoFile::reset() {

    // reset variables to 0
    frame_last_pts = 0.0;
    video_clock = 0.0;
    video_current_pts = 0.0;
    pictq_windex = 0;
    pictq_size = 0;
    pictq_rindex = 0;
    seek_pos = 0;
    seek_req = false;

    // init clock
    last_time = av_gettime();
    last_virtual_time = 0;
    video_current_pts_time = last_time;
    frame_timer = (double) (video_current_pts_time) / (double) AV_TIME_BASE;
    frame_last_delay = 40e-3; // 40 ms

}

void VideoFile::stop() {

    if (!quit && pFormatCtx) {
        // request quit
        quit = true;
        // wait fo threads to end properly
        parse_tid->wait();
        pictq_cond->wakeAll();
        decod_tid->wait();

        // remember where we are for next restart
        mark_stop = getCurrentFrameTime();
        if (!restart_where_stopped) {
            fill_first_frame(true);
            // display firstPicture frame
            emit frameReady(-1);
        }

        /* say if we are running or not */
        emit running(!quit);
        emit info(tr("%1 stopped.").arg(filename));

        // clear logs (should be done sometime; when stopped is good as logs are used to explain a crash during play)
        logmessage.clear();
    }

}

void VideoFile::start() {

    if (quit && pFormatCtx) {
        // reset quit flag
        quit = false;
        // reset internal state
        reset();

        // where shall we (re)start ?
        if (restart_where_stopped && mark_stop > 0) {
            // enforces seek to the frame before ; it is needed because we may miss the good frame
            seek_backward = true;
            // restart where we where
            seekToPosition(mark_stop);
        } else
        // restart at beginning
        if (mark_in > getBegin()) {
            // enforces seek to the frame before ; it is needed because we may miss the good frame
            seek_backward = true;
            seekToPosition(mark_in);
        }

        if (!pause_video) {
            // unpause
            pause_video = true; // hack because otherwise pause() does nothing...
            pause(false);
            pause_video_last = false;
        }

        // start parsing and decoding threads
        parse_tid->start();
        decod_tid->start();

        /* say if we are running or not */
        emit running(!quit);
        emit info(tr("%1 started.").arg(filename));
    }

}

void VideoFile::play(bool startorstop) {

    if (startorstop) {
        start();
    } else {
        stop();
    }
}

void VideoFile::setPlaySpeed(int playspeed) {

    switch (playspeed) {

    case SPEED_QUARTER:
        play_speed = 0.25;
        break;
    case SPEED_THIRD:
        play_speed = 0.333;
        break;
    case SPEED_HALF:
        play_speed = 0.5;
        break;
    case SPEED_DOUBLE:
        play_speed = 2.0;
        break;
    case SPEED_TRIPLE:
        play_speed = 3.0;
        break;
    case SPEED_NORMAL:
    default:
        play_speed = 1.0;
    }

}

int VideoFile::getPlaySpeed(){

	if (play_speed - 0.25 < 0.1 )
		return SPEED_QUARTER;
	if (play_speed - 0.333 < 0.1 )
		return SPEED_THIRD;
	if (play_speed - 0.5 < 0.1 )
		return SPEED_HALF;
	if (play_speed - 1.0 < 0.1 )
		return SPEED_NORMAL;
	if (play_speed - 2.0 < 0.1 )
		return SPEED_DOUBLE;
	if (play_speed - 3 < 0.1 )
		return SPEED_TRIPLE;

	return SPEED_NORMAL;
}

void VideoFile::thread_terminated() {

    // recieved this message while 'quit' was not requested ?
    if (!quit) {
        //  this means an error occured...
        emit error(tr("Error reading %1 !\n\nLogs;\n%2Decoding interrupted. ").arg(filename).arg(logmessage));

        // stop properly if possible
        stop();
    }
}

const VideoPicture *VideoFile::getPictureAtIndex(int index) const {

    // return a firstPicture picture if wrong index
    if (index < 0 || index > VIDEO_PICTURE_QUEUE_SIZE) {

        return (resetPicture);
    }

    return (&pictq[index]);
}

bool VideoFile::open(QString file, int64_t markIn, int64_t markOut) {

    AVFormatContext *_pFormatCtx;

    // tells everybody we are set !
    emit info(tr("Opening %1...").arg(filename) );

    // Check file
    filename = file;
    if (!QFileInfo(filename).isFile()){
        emit error(tr("Error opening %1:\nFile does not exist.").arg(file));
    	return false;
    }


    int err = av_open_input_file(&_pFormatCtx, getFileName(), NULL, 0, NULL);
    if (err < 0) {
        switch (err) {
        case AVERROR_NUMEXPECTED:
            emit error(tr("Error opening %1:\nIncorrect numbered image sequence syntax.").arg(file));
            break;
        case AVERROR_INVALIDDATA:
            emit error(tr("Error opening %1:\nError while parsing header.").arg(file));
            break;
        case AVERROR_NOFMT:
            emit error(tr("Error opening %1:\nUnknown format.").arg(file));
            break;
        case AVERROR(EIO):
            emit error(tr("Error opening %1:\n"
                        "I/O error. Usually that means that input file is truncated and/or corrupted.").arg(file));
            break;
        case AVERROR(ENOMEM):
            emit error(tr("Error opening %1:\nMemory allocation error.").arg(file));
            break;
        case AVERROR(ENOENT):
            emit error(tr("Error opening %1:\nNo such file.").arg(file));
            break;
        default:
            emit error(tr("Error opening %1:\nCouldn't open the file.").arg(file));
            break;
        }
        return false;
    }

    if (av_find_stream_info(_pFormatCtx) < 0) {
        // Couldn't find stream information
        emit error(tr("Error opening %1:\nCouldn't find any stream information in this file.").arg(filename));
        return false;
    }

    // if video_index not set (no video stream found) or stream open call failed
    videoStream = stream_component_open(_pFormatCtx);
    if (videoStream < 0) {
        //could not open codecs (errors already emited)
        return false;
    }

    // Init the stream and timing info
    video_st = _pFormatCtx->streams[videoStream];

    // all ok, we can set the internal pointer to the good value
    pFormatCtx = _pFormatCtx;

    // check the parameters for mark in and out and setup marking accordingly
    if (markIn == 0)
        mark_in = getBegin();
    else
        mark_in = qBound(getBegin(), markIn, getEnd());

    if (markOut == 0)
        mark_out = getEnd();  // default to end of file
    else
    	mark_out = qBound(mark_in, markOut, getEnd());

    // emit that marking changed only once
    if (markIn != 0 || markOut != 0)
    	emit markingChanged();

    // init picture size
    if (targetWidth == 0 || targetHeight == 0) {
        targetWidth = video_st->codec->width;
        targetHeight = video_st->codec->height;
    }

    // round target picture size to power of two size
    if ( powerOfTwo ) {
        targetWidth = VideoFile::roundPowerOfTwo(targetWidth);
        targetHeight = VideoFile::roundPowerOfTwo(targetHeight);
    }

    // automatic optimal selection of target format
    if (video_st->codec->pix_fmt == PIX_FMT_RGB32 || video_st->codec->pix_fmt == PIX_FMT_BGR32)
    	targetFormat = PIX_FMT_RGBA;
    else
    	targetFormat = PIX_FMT_RGB24;


    /* allocate the buffers */
    for (int i = 0; i < VIDEO_PICTURE_QUEUE_SIZE; ++i) {
        if (!pictq[i].allocate(targetWidth, targetHeight, targetFormat)) {
            // Cannot allocate Video Pictures!
            emit error(tr("Error opening %1:\nCannot allocate pictures buffer.").arg(filename));
            return false;
        }
    }

    // we may need a black Picture frame to return when stopped
    if (!blackPicture.allocate(targetWidth, targetHeight, targetFormat)) {
        // Cannot allocate Video Pictures!
        emit error(tr("Error opening %1:\nCannot allocate picture buffer.").arg(filename));
        return false;
    }
    // we need a picture to display when not decoding
    if (!firstPicture.allocate(targetWidth, targetHeight, targetFormat)) {
        // Cannot allocate Video Pictures!
        emit error(tr("Error opening %1:\nCannot allocate picture buffer.").arg(filename));
        return false;
    }


    // create conversion context
    img_convert_ctx = sws_getContext(video_st->codec->width, video_st->codec->height, video_st->codec->pix_fmt,
                targetWidth, targetHeight, targetFormat, conversionAlgorithm, NULL, NULL, NULL);

    if (img_convert_ctx == NULL) {
        // Cannot initialize the conversion context!
        emit error(tr("Error opening %1:\nCannot create a suitable conversion context to RGB.").arg(filename));
        return false;
    }

    // read firstPicture (not a big problem if fails; it would just be black)
    fill_first_frame(false);
    resetPicture = &firstPicture;

    // display a firstPicture frame ; this shows that the video is open
    emit frameReady(-1);

    /* say if we are running or not */
    pause_video = false; // not paused when loaded
    emit running(!quit);

    // tells everybody we are set !
    emit info(tr("File %1 opened (%2 frames).").arg(filename).arg(getExactFrameFromFrame(getEnd())));

    return true;
}


void VideoFile::fill_first_frame(bool seek) {
    AVPacket pkt1;
    AVPacket *packet = &pkt1;
    int frameFinished = 0;

    if (seek)
        av_seek_frame(pFormatCtx, videoStream, mark_in, AVSEEK_FLAG_BACKWARD);

    while (!frameFinished) {
        if (av_read_frame(pFormatCtx, packet) != 0)
            break;
        if (packet->stream_index != videoStream)
            continue;
        // create an empty frame
        AVFrame *pFrame = avcodec_alloc_frame();
        Q_CHECK_PTR(pFrame);
        // if we can decode it
#if LIBAVCODEC_VERSION_INT > AV_VERSION_INT(52,30,0)
        if (avcodec_decode_video2(video_st->codec, pFrame, &frameFinished, packet) >= 0) {
#else
        if ( avcodec_decode_video(video_st->codec, pFrame, &frameFinished, packet->data, packet->size) >= 0) {
#endif
            // and if the frame is finished (should be in one shot for video stream)
        	if (frameFinished) {
                // convert it to the firtPicture
                sws_scale(img_convert_ctx, pFrame->data, pFrame->linesize, 0, video_st->codec->height,
                            firstPicture.rgb->data, firstPicture.rgb->linesize);
        	}
        }
        // free frame
        av_free(pFrame);
    }
    // free packet
    av_free_packet(packet);

    if(seek)
        avcodec_flush_buffers(video_st->codec);
}


int VideoFile::stream_component_open(AVFormatContext *pFCtx) {

    AVCodecContext *codecCtx;
    AVCodec *codec;
    int stream_index = -1;

    // Find the first video stream index
    for (int i = 0; i < (int) pFCtx->nb_streams; i++) {
        if (pFCtx->streams[i]->codec->codec_type == CODEC_TYPE_VIDEO) {
            stream_index = i;
            break;
        }
    }

    if (stream_index < 0 || stream_index >= (int) pFCtx->nb_streams) {
        emit error(tr("Error opening %1:\nNot a video or image file.").arg(filename));
        return -1;
    }

    // Get a pointer to the codec context for the video stream
    codecCtx = pFCtx->streams[stream_index]->codec;

    codec = avcodec_find_decoder(codecCtx->codec_id);
    if (!codec || (avcodec_open(codecCtx, codec) < 0)) {
        emit error(tr("Error opening %1:\nUnsupported codec (%2).").arg(filename).arg(codec->name));
        return -1;
    }

    codecname = QString(codec->name);

    return stream_index;
}

void VideoFile::video_refresh_timer() {

    VideoPicture *vp;
    double actual_delay, delay;

    if (video_st) {

        if (pictq_size < 1) {
            // no picture yet ? Quickly retry...
        	if (!is_synchronized)
        		ptimer->start(10);

        } else {

            // use vp as the current picture at the read index
            vp = &pictq[pictq_rindex];

			// update time to now
			video_current_pts = vp->pts;
			video_current_pts_time = av_gettime();

			// how long since last frame ?
			delay = video_current_pts - frame_last_pts; /* the pts from last time */
			if (delay <= 0 || delay >= 1.0)
				delay = frame_last_delay;               /* if incorrect delay, use previous one */
			else
				frame_last_delay = delay;               /* save for next time */

			frame_last_pts = video_current_pts;

			// the next frame should be displayed at frame_timer
			frame_timer += delay;

			/* compute the REAL delay */
			actual_delay = frame_timer - ((double) av_gettime() / (double) AV_TIME_BASE);
			if (actual_delay < 0.010) {
				/* Really it should skip the picture instead (quick retry) */
				actual_delay = 0.010;
			}

			/* show the picture at the current read index */
			emit frameReady(pictq_rindex);

			// knowing the delay (in seconds), we can schedule the next display (in ms)
			if (!pause_video && !is_synchronized)
				ptimer->start((int) (actual_delay * 1000 + 0.5));

//			// if we just paused on this frame, keep it as reference
//			else
//				vp->setReferenceFrame();

            /* update queue for next picture at the read index */
            if (++pictq_rindex == VIDEO_PICTURE_QUEUE_SIZE)
                pictq_rindex = 0;

            pictq_mutex->lock();
            pictq_size--;
            // tell video thread that it can go on...
            pictq_cond->wakeAll();
            pictq_mutex->unlock();

        }

    } else {
        // wait for a video_st to be created
    	if (!is_synchronized)
    		ptimer->start(100);
    }
}

int64_t VideoFile::getCurrentFrameTime() const {

    return ((int64_t)(play_speed * video_current_pts / av_q2d(video_st->time_base)));
}

float VideoFile::getFrameRate() const {

//    if (video_st && video_st->avg_frame_rate.den > 0)
//        return ((float) (video_st->avg_frame_rate.num )/ (float) video_st->avg_frame_rate.den);
//    else
	if (video_st && video_st->r_frame_rate.den > 0)
        return ((float) (video_st->r_frame_rate.num )/ (float) video_st->r_frame_rate.den);
    else
        return 0;
}

void VideoFile::setOptionAllowDirtySeek(bool dirty) {

    seek_any = dirty;

}

void VideoFile::setOptionRevertToBlackWhenStop(bool black) {
    if (black)
        resetPicture = &blackPicture;
    else
        resetPicture = &firstPicture;
    if (quit)
        emit frameReady(-1);
}

void VideoFile::seekToPosition(int64_t t) {

    if (pFormatCtx && !quit && !seek_req) {
        seek_pos = qBound(getBegin(), t, getEnd());
        seek_req = true;
    }
}

void VideoFile::seekBySeconds(double ss) {

    if (pFormatCtx && !quit && !seek_req) {
        int64_t pos = (int64_t)((get_clock() + ss) * AV_TIME_BASE * play_speed);
        pos = av_rescale_q(pos, AV_TIME_BASE_Q, video_st->time_base);

        seekToPosition(pos);
    }
}


void VideoFile::seekByFrames(int64_t ss) {

    if (pFormatCtx && !quit && !seek_req) {
        int64_t pos = getCurrentFrameTime() + ss;

        seekToPosition(pos);
    }
}


QString VideoFile::getTimeFromFrame(int64_t t) const {

    double time = (double) (t- video_st->start_time) * av_q2d(video_st->time_base) / play_speed;
    int s = (int) time;
    time -= s;
    int h = s / 3600;
    int m = (s % 3600) / 60;
    s = (s % 3600) % 60;
    int ds = (int) (time * 100.0);
    return QString("%1h %2m %3.%4s").arg(h, 2).arg(m, 2, 10, QChar('0')).arg(s, 2, 10, QChar('0')).arg(ds, 2, 10, QChar('0'));
}


QString VideoFile::getExactFrameFromFrame(int64_t t) const {

	if ( video_st->nb_frames > 0 )
		return (QString("Frame: %1").arg( (int)( video_st->nb_frames * double(t - video_st->start_time) / double(video_st->duration) ) ) );
	 else
		return (QString("Frame: %1").arg( (int)( float (t - video_st->start_time) / getFrameRate()) ));
}

int64_t VideoFile::getFrameFromTime(QString t) const {

    QStringList list1 = t.split(":");

    if (list1.size() > 3) {
        int h = list1.at(0).toInt();
        int m = list1.at(1).toInt();
        double s = list1.at(2).toDouble();

        double time = (double) h * 3600.0 + (double) m * 60 +  s ;
        time = (time * play_speed) / av_q2d(video_st->time_base);
        return ((int64_t) time);
    }

    return -1;
}

int64_t VideoFile::getBegin() const {
    return ((video_st && video_st->start_time != (int64_t) AV_NOPTS_VALUE) ? video_st->start_time : 0);
}

double VideoFile::getDuration() const {

    return (pFormatCtx ? (double) pFormatCtx->duration / (double) AV_TIME_BASE : 0.0);
}

int64_t VideoFile::getEnd() const {

    int64_t e = 0;
    if (video_st) {
        if ( video_st->duration != (int64_t) AV_NOPTS_VALUE)
            e = video_st->duration;
        else
            e = (int64_t) ( getDuration() * video_st->time_base.den / video_st->time_base.num );
    }

    return e;
}

void VideoFile::setMarkIn(int64_t t) {
    mark_in = qBound(getBegin(), t, mark_out);
    emit markingChanged();
}

void VideoFile::setMarkOut(int64_t t) {
    mark_out = qBound(mark_in, t, getEnd());
    emit markingChanged();
}


float VideoFile::getStreamAspectRatio() const {
    float aspect_ratio = 0;

    if (video_st->sample_aspect_ratio.num)
        aspect_ratio = av_q2d(video_st->sample_aspect_ratio);
    else if (video_st->codec->sample_aspect_ratio.num)
        aspect_ratio = av_q2d(video_st->codec->sample_aspect_ratio);
    else
        aspect_ratio = 0;
    if (aspect_ratio <= 0.0)
        aspect_ratio = 1.0;
    aspect_ratio *= (float)video_st->codec->width / video_st->codec->height;

    return aspect_ratio;
}

/**
 *
 * ParsingThread
 *
 */
void ParsingThread::run() {

    AVPacket pkt1, *packet = &pkt1;
    bool eof = false;

    while (is && !is->quit) {

        if (is->pause_video != is->pause_video_last) {
            is->pause_video_last = is->pause_video;
            if (is->pause_video)
                av_read_pause(is->pFormatCtx);
            else
                av_read_play(is->pFormatCtx);
        }

        // seek stuff goes here
        if (is->seek_req) {

            int flags = 0;
            if (is->seek_any)
                flags |= AVSEEK_FLAG_ANY;
            if (is->seek_pos <= is->getCurrentFrameTime() || is->seek_backward)
                flags |= AVSEEK_FLAG_BACKWARD;

            if (av_seek_frame(is->pFormatCtx, is->videoStream, is->seek_pos, flags) < 0) {
                is->logmessage += tr("ParsingThread:: Seeking error.\n");
                is->sendInfo( tr("Could not seek to frame (%1); jumping where I can!").arg(is->seek_pos));
            }
			// after seek,  we'll have to flush buffers
			if (!is->videoq.flush())
				is->logmessage += tr("ParsingThread:: Flushing error.\n");

            is->seek_backward = false;
            is->seek_req = false;
            eof = false;
        }

        /* if the queue is full, no need to read more */
        if (is->videoq.isFull()) {
            msleep(10);
            continue;
        }

        //  end of file management
        if (url_feof(is->pFormatCtx->pb) || eof) {
            av_init_packet(packet);
            packet->data = NULL;
            packet->size = 0;
            packet->stream_index = is->videoStream;
            packet->dts = is->mark_out;
            is->videoq.put(packet);
            msleep(40);
            continue;
        }

        int ret = av_read_frame(is->pFormatCtx, packet);
        Q_CHECK_PTR(packet);

        if (ret < 0) {
            //  end of file detection
            if (ret == AVERROR_EOF) {
                eof = true;
            }

            if (url_ferror(is->pFormatCtx->pb)) {
                // error ; exit
                is->logmessage += tr("ParsingThread:: Couldn't read frame.\n");
                is->sendInfo( tr("Could not read frame.") );
                break;
            }
            /* no error; just wait a bit for the end of the packet and continue*/
            msleep(100);
            continue;
        }

        // Is this a packet from the video stream?
        if (packet->stream_index == is->videoStream) {
            // yes, so put it into the video queue
            if (!is->videoq.put(packet)) {
                // we need to free the packet if it was not put in the queue? (not done in example)
                is->logmessage += tr("ParsingThread:: Couldn't put packet..\n");
                av_free_packet(packet);
            }
        } else {
            // no, so delete it ( no use for it )
            av_free_packet(packet);
        }

    }

    // request flushing of the video queue (this will end the decoding thread)
	if (!is->videoq.flush())
		is->logmessage += tr("ParsingThread:: Flushing error at end.\n");

}

void VideoFile::queue_picture(AVFrame *pFrame, double pts) {

    /* wait until we have space for a new pic */
    pictq_mutex->lock();
    while (pictq_size >= (VIDEO_PICTURE_QUEUE_SIZE - 1) && !quit)
        pictq_cond->wait(pictq_mutex); // the condition is released in VideoFile::video_refresh_timer()
    pictq_mutex->unlock();

    // if have to quit, discard the rest but let the video thread continue
    if (quit) {
        pictq_mutex->lock();
        pictq_size = 0;
        pictq_mutex->unlock();
        return;
    }

    // set vp as the picture in the queue to be written
    // (write index is set to 0 initially)
    VideoPicture *vp = &pictq[pictq_windex];

    if (vp->rgb && img_convert_ctx && pictq_size <= VIDEO_PICTURE_QUEUE_SIZE) {

        // Convert the image
        sws_scale(img_convert_ctx, pFrame->data, pFrame->linesize, 0, video_st->codec->height, vp->rgb->data,
                    vp->rgb->linesize);

        // remember pts
        vp->pts = pts;

        // set to write indexto next in queue
        if (++pictq_windex == VIDEO_PICTURE_QUEUE_SIZE)
            pictq_windex = 0;

        /* now we inform our display thread that we have a pic ready */
        pictq_mutex->lock();
        pictq_size++;
        pictq_mutex->unlock();
    }

}

/**
 * compute the exact PTS for the picture if it is omitted in the stream
 * @param pts1 the dts of the pkt / pts of the frame
 */
double VideoFile::synchronize_video(AVFrame *src_frame, double pts1) {

    double frame_delay, pts;
    pts = pts1;

    if (pts != 0) {
        /* if we have pts, set video clock to it */
        video_clock = pts;
    } else {
        /* if we aren't given a pts, set it to the clock */
        pts = video_clock;
    }
    frame_delay = av_q2d(video_st->codec->time_base);

    /* for MPEG2, the frame can be repeated, so we update the clock accordingly */
    frame_delay += src_frame->repeat_pict * (frame_delay * 0.5);
    /* update the video clock */
    video_clock += frame_delay;

    return pts;
}

/**
 * DecodingThread
 */

void DecodingThread::run() {

    AVPacket pkt1;
    AVPacket *packet = &pkt1;
    int len1 = 0, frameFinished = 0;
    double pts = 0.0;					// Presentation time stamp
    int64_t dts = 0;					// Decoding time stamp
    AVFrame *pFrame = avcodec_alloc_frame();
    Q_CHECK_PTR(pFrame);

//    is->logmessage += tr("DecodingThread:: run.\n");

    while (is) {
        // sleep a bit if paused
        if (is->pause_video && !is->quit) {
            msleep(100);
            continue;
        }

        // get front packet (not blocking if we are quitting)
        if (!is->videoq.get(packet, !is->quit)) {
            // means we quit getting packets
            is->logmessage += tr("DecodingThread:: No more packets.\n");
            break;
        }
        Q_CHECK_PTR(packet);

        // special case of flush packet
        if (is->videoq.isFlush(packet)) {
            // means we have to flush buffers
            avcodec_flush_buffers(is->video_st->codec);
            continue;
        }

        // Decode video frame
        is->video_st->codec->reordered_opaque = packet->pts;

#if LIBAVCODEC_VERSION_INT > AV_VERSION_INT(52,30,0)
        len1 = avcodec_decode_video2(is->video_st->codec, pFrame, &frameFinished, packet);
#else
        len1 = avcodec_decode_video(is->video_st->codec, pFrame, &frameFinished, packet->data, packet->size);
#endif
        // get decompression time stamp
        // this is the code in ffplay:
        dts = 0.0;
        if (packet->dts != (int64_t) AV_NOPTS_VALUE) {
            dts = packet->dts;
        } else if (pFrame->reordered_opaque != (int64_t) AV_NOPTS_VALUE) {
            dts = pFrame->reordered_opaque;
        }
        // this is the code from tutorial :
        //    else if (pFrame->opaque && *(uint64_t*) pFrame->opaque != AV_NOPTS_VALUE) {
        //                pts = *(uint64_t *) pFrame->opaque;
        //            }

        // test if this is the last frame
        if (dts >= is->mark_out || is->videoq.isEmpty()) {
            if (!is->loop_video)
                is->pause(true);
            else {
                is->seekToPosition(is->mark_in);
            }
        }
        // compute presentation time stamp
        //        pts = dts * av_q2d(is->video_st->time_base);
        pts = dts * av_q2d(is->video_st->time_base) / is->play_speed;

        // Did we get a full video frame?
        if (frameFinished) {
            pts = is->synchronize_video(pFrame, pts);
            // yes, frame is ready ; add it to the queue of pictures
            is->queue_picture(pFrame, pts);
        }

        // packet was decoded, can be removed
        av_free_packet(packet);
    }

    // free the locally allocated variable
    av_free(pFrame);

//    is->logmessage += tr("DecodingThread:: ended.\n");
}

void VideoFile::pause(bool pause) {

    if (!quit && pause != pause_video) {
        pause_video = pause;

        if (!pause_video) {
            video_current_pts = get_clock();
            frame_timer += (av_gettime() - video_current_pts_time) / (double) AV_TIME_BASE;

        	if (!is_synchronized)
        		ptimer->start(10);
        }

        emit paused(pause);
    }
}

double VideoFile::get_clock() {

    double delta;

    if (pause_video)
        delta = 0.0;
    else
        delta = (double) (av_gettime() - video_current_pts_time) / (double) AV_TIME_BASE;

    return video_current_pts + delta;
}


/**
 * VideoFile::PacketQueue
 */

bool VideoFile::PacketQueue::put(AVPacket *pkt) {

    if (pkt != flush_pkt && av_dup_packet(pkt) < 0)
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

bool VideoFile::PacketQueue::get(AVPacket *pkt, bool block) {

    bool ret = false;
    mutex->lock();

    for (;;) {

        AVPacketList *pkt1 = first_pkt;
        if (pkt1) {
            first_pkt = pkt1->next;
            if (!first_pkt)
                last_pkt = NULL;
            nb_packets--;
            size -= pkt1->pkt.size;
            *pkt = pkt1->pkt;
            av_free(pkt1);

            ret = true;
            break;
        } else if (!block) {
            break;
        } else {
            cond->wait(mutex);
        }
    }

    mutex->unlock();
    return ret;
}

VideoFile::PacketQueue::~PacketQueue() {
    delete mutex;
    delete cond;
}

AVPacket *VideoFile::PacketQueue::flush_pkt = 0;

VideoFile::PacketQueue::PacketQueue() {

    if (!flush_pkt) {
        flush_pkt = (AVPacket *) av_malloc(sizeof(AVPacket));
        Q_CHECK_PTR(flush_pkt);
        av_init_packet(flush_pkt);
        uint8_t *msg = (uint8_t *) av_malloc(sizeof(uint8_t));
        Q_CHECK_PTR(msg);
        msg[0] = 'F';
        flush_pkt->data = msg;
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

bool VideoFile::PacketQueue::flush() {
    AVPacketList *pkt, *pkt1;

    mutex->lock();

    for (pkt = first_pkt; pkt != NULL; pkt = pkt1) {
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

bool VideoFile::PacketQueue::isFlush(AVPacket *pkt) {
    return (pkt->data == flush_pkt->data);
}

#define MAX_VIDEOQ_SIZE (5 * 256 * 1024)

bool VideoFile::PacketQueue::isFull() {
    return (size > MAX_VIDEOQ_SIZE);
}

int VideoFile::roundPowerOfTwo(int v){

    int k;
    if (v == 0)
            return 1;
    for (k = sizeof(int) * 8 - 1; ((static_cast<int>(1U) << k) & v) == 0; k--);
    if (((static_cast<int>(1U) << (k - 1)) & v) == 0)
            return static_cast<int>(1U) << k;
    return static_cast<int>(1U) << (k + 1);

}

void VideoFile::displayFormatsCodecsInformation(QString iconfile) {

    if (!ffmpegregistered) {
        avcodec_register_all();
        av_register_all();
        ffmpegregistered = true;
    }

    QVBoxLayout *verticalLayout;
    QLabel *label;
    QTreeWidget *treeWidget_2;
    QLabel *label_2;
    QTreeWidget *treeWidget_3;
    QDialogButtonBox *buttonBox;

    QDialog *ffmpegInfoDialog = new QDialog;
    QIcon icon;
    icon.addFile(iconfile, QSize(), QIcon::Normal, QIcon::Off);
    ffmpegInfoDialog->setWindowIcon(icon);
    ffmpegInfoDialog->resize(510, 588);
    verticalLayout = new QVBoxLayout(ffmpegInfoDialog);
    label = new QLabel(ffmpegInfoDialog);

    verticalLayout->addWidget(label);

    treeWidget_2 = new QTreeWidget(ffmpegInfoDialog);
    treeWidget_2->setProperty("showDropIndicator", QVariant(false));
    treeWidget_2->setAlternatingRowColors(true);
    treeWidget_2->setRootIsDecorated(false);
    treeWidget_2->header()->setVisible(true);

    verticalLayout->addWidget(treeWidget_2);

    label_2 = new QLabel(ffmpegInfoDialog);

    verticalLayout->addWidget(label_2);

    treeWidget_3 = new QTreeWidget(ffmpegInfoDialog);
    treeWidget_3->setAlternatingRowColors(true);
    treeWidget_3->setRootIsDecorated(false);
    treeWidget_3->header()->setVisible(true);

    verticalLayout->addWidget(treeWidget_3);

    buttonBox = new QDialogButtonBox(ffmpegInfoDialog);
    buttonBox->setOrientation(Qt::Horizontal);
    buttonBox->setStandardButtons(QDialogButtonBox::Close);

    verticalLayout->addWidget(buttonBox);

    ffmpegInfoDialog->setWindowTitle(tr("FFMPEG formats and codecs"));
    label->setText(tr("Available formats"));
    label_2->setText(tr("Readable VIDEO codecs"));

    QTreeWidgetItem *___qtreewidgetitem = treeWidget_2->headerItem();
    ___qtreewidgetitem->setText(1, tr("Description"));
    ___qtreewidgetitem->setText(0, tr("Name"));

    QTreeWidgetItem *___qtreewidgetitem3 = treeWidget_3->headerItem();
    ___qtreewidgetitem3->setText(1, tr("Description"));
    ___qtreewidgetitem3->setText(0, tr("Name"));

    QObject::connect(buttonBox, SIGNAL(accepted()), ffmpegInfoDialog, SLOT(accept()));
    QObject::connect(buttonBox, SIGNAL(rejected()), ffmpegInfoDialog, SLOT(reject()));

    QMetaObject::connectSlotsByName(ffmpegInfoDialog);

    QTreeWidgetItem *formatitem;

    AVInputFormat *ifmt = NULL;
    AVCodec *p = NULL, *p2;
    const char *last_name;

    last_name = "000";
    for (;;) {
        const char *name = NULL;
        const char *long_name = NULL;

        while ((ifmt = av_iformat_next(ifmt))) {
            if ((name == NULL || strcmp(ifmt->name, name) < 0) && strcmp(ifmt->name, last_name) > 0) {
                name = ifmt->name;
                long_name = ifmt->long_name;
            }
        }
        if (name == NULL)
            break;
        last_name = name;

        formatitem = new QTreeWidgetItem(treeWidget_2);
        formatitem->setText(0, QString(name));
        formatitem->setText(1, QString(long_name));
        treeWidget_2->addTopLevelItem(formatitem);
    }

    last_name = "000";
    for (;;) {
        int decode = 0;
        int cap = 0;

        p2 = NULL;
        while ((p = av_codec_next(p))) {
            if ((p2 == NULL || strcmp(p->name, p2->name) < 0) && strcmp(p->name, last_name) > 0) {
                p2 = p;
                decode = cap = 0;
            }
            if (p2 && strcmp(p->name, p2->name) == 0) {
                if (p->decode)
                    decode = 1;
                cap |= p->capabilities;
            }
        }
        if (p2 == NULL)
            break;
        last_name = p2->name;

        if (decode && p2->type == CODEC_TYPE_VIDEO) {
            formatitem = new QTreeWidgetItem(treeWidget_3);
            formatitem->setText(0, QString(p2->name));
            formatitem->setText(1, QString(p2->long_name));
            treeWidget_3->addTopLevelItem(formatitem);
        }
    }

    ffmpegInfoDialog->exec();

    delete verticalLayout;
    delete label;
    delete label_2;
    delete treeWidget_2;
    delete treeWidget_3;
    delete buttonBox;

    delete ffmpegInfoDialog;
}



int VideoFile::getBrightness(){
    int *inv_table, srcrange, *table, dstrange, brightness, contrast, saturation;

    if ( -1 != sws_getColorspaceDetails(img_convert_ctx, &inv_table, &srcrange, &table, &dstrange, &brightness, &contrast, &saturation) ) {

        return (((brightness*100) + (1<<15))>>16);
    }
    else return 0;

}
int VideoFile::getContrast(){
    int *inv_table, srcrange, *table, dstrange, brightness, contrast, saturation;

    if ( -1 != sws_getColorspaceDetails(img_convert_ctx, &inv_table, &srcrange, &table, &dstrange, &brightness, &contrast, &saturation) ) {

        return ((((contrast  *100) + (1<<15))>>16) - 100);
    }
    else return 0;

}
int VideoFile::getSaturation(){
    int *inv_table, srcrange, *table, dstrange, brightness, contrast, saturation;

    if ( -1 != sws_getColorspaceDetails(img_convert_ctx, &inv_table, &srcrange, &table, &dstrange, &brightness, &contrast, &saturation) ) {

        return ((((saturation*100) + (1<<15))>>16) - 100);
    }
    else return 0;
}


void VideoFile::setBrightness(int b){

	int *inv_table, srcrange, *table, dstrange, brightness, contrast, saturation;
	if ( b >= -100 && b <= 100 &&  -1 != sws_getColorspaceDetails(img_convert_ctx, &inv_table, &srcrange, &table, &dstrange, &brightness, &contrast, &saturation) ) {
		// ok, got all the details, modify one:
		brightness = (( b <<16) + 50)/100;
		// apply it
		sws_setColorspaceDetails(img_convert_ctx, inv_table, srcrange, table, dstrange, brightness, contrast, saturation);
	}

	emit prefilteringChanged();
}


void VideoFile::setContrast(int c){

	int *inv_table, srcrange, *table, dstrange, brightness, contrast, saturation;
	if ( c >= -99 && c <= 100 &&  -1 != sws_getColorspaceDetails(img_convert_ctx, &inv_table, &srcrange, &table, &dstrange, &brightness, &contrast, &saturation) ) {
		// ok, got all the details, modify one:
		contrast   = ((( c +100)<<16) + 50)/100;
		// apply it
		sws_setColorspaceDetails(img_convert_ctx, inv_table, srcrange, table, dstrange, brightness, contrast, saturation);
	}

	emit prefilteringChanged();
}



void VideoFile::setSaturation(int s){

	int *inv_table, srcrange, *table, dstrange, brightness, contrast, saturation;
	if ( s >= -100 && s <= 100 &&  -1 != sws_getColorspaceDetails(img_convert_ctx, &inv_table, &srcrange, &table, &dstrange, &brightness, &contrast, &saturation) ) {
		// ok, got all the details, modify one:
		saturation = ((( s +100)<<16) + 50)/100;
		// apply it
		sws_setColorspaceDetails(img_convert_ctx, inv_table, srcrange, table, dstrange, brightness, contrast, saturation);
	}

	emit prefilteringChanged();
}

