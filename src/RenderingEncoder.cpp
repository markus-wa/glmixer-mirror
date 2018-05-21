/*
 * RenderingEncoder.cpp
 *
 *  Created on: Mar 13, 2011
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

#include "RenderingEncoder.moc"

#include "common.h"
#include "defines.h"
#include "RenderingManager.h"
#include "ViewRenderWidget.h"
#include "glmixer.h"

#include <QSize>
#include <QBuffer>
#include <QFileInfo>
#include <QMessageBox>
#include <QGLFramebufferObject>
#include <QThread>


EncodingThread::EncodingThread() : QThread(), recorder(NULL), _quit(true), time(0),
    pictq_max_count(0), pictq_size_count(0), pictq_rindex(0), pictq_windex(0),
    frameq(NULL), framewidth(0), frameheight(0) //, recordingTimeStamp(0)
{
    // create mutex
    pictq_mutex = new QMutex;
    Q_CHECK_PTR(pictq_mutex);
    pictq_cond = new QWaitCondition;
    Q_CHECK_PTR(pictq_cond);
}

EncodingThread::~EncodingThread()
{
    clear();

    delete pictq_mutex;
    delete pictq_cond;

  //  qDebug() << "EncodingThread" << QChar(124).toLatin1() << tr("Done.");
}

void EncodingThread::initialize(VideoRecorder *rec, int width, int height, unsigned long bufSize)

{
    // clear buffer in case its a re-initialization
    clear();

    // set recorder
    recorder = rec;

    // set frames
    framewidth = width;
    frameheight = height;

    // compute buffer count from size of buffer over the size of RGB images
    // store buffer count as maximum buffer size
    pictq_max_count = (int) ( (long double) bufSize / (long double) (width * height * 3) );

    // init variables
    pictq_size_count = pictq_rindex = pictq_windex = 0;

    // allocate & initialize array to zero
    frameq = (AVFrame **) calloc( pictq_max_count, sizeof(AVFrame *) );
}

void EncodingThread::clear() {

    // free picture queue array
    int freedmemory = 0;
    if (frameq) {
        // free buffer
        for (int i = 0; i < pictq_max_count; ++i) {
            if (frameq[i]) {
                av_frame_unref(frameq[i]);
                av_frame_free(&frameq[i]);
                freedmemory++;
            }

        }
        // free pictq array
        free(frameq);
        frameq = NULL;

        qDebug() << "EncodingThread" << QChar(124).toLatin1() << tr("Buffer cleared (%1 frames).").arg(freedmemory);
    }

    recorder = NULL;
    time = 0;
}

void EncodingThread::stop() {

    // end thread
    _quit = true;

}

void EncodingThread::releaseAndPushFrame(int elapsedtime)
{
    // store time
    time = elapsedtime;

    // set to write index to next in queue
    if (++pictq_windex == pictq_max_count)
        pictq_windex = 0;

    /* now we inform our encoding thread that we have a picture ready */
    pictq_size_count++;
    pictq_mutex->unlock();

}

AVBufferRef *EncodingThread::lockFrameAndGetBuffer()
{
    // wait until we have space for a new picture
    // (this happens only when the queue is full)
    pictq_mutex->lock();
    while ( frameq_full() && !_quit)
        pictq_cond->wait(pictq_mutex); // the condition is released in run()

    // allocate frame if not already done
    if (frameq[pictq_windex] == 0) {

        // allocate and setup frame
        frameq[pictq_windex] = av_frame_alloc();
        frameq[pictq_windex]->format = AV_PIX_FMT_RGB24;
        frameq[pictq_windex]->width  = framewidth;
        frameq[pictq_windex]->height = frameheight;

        // allocate buffer
        av_frame_get_buffer(frameq[pictq_windex], 24);
        av_frame_make_writable(frameq[pictq_windex]);
    }

    // return ref to buffer of frame
    return av_frame_get_plane_buffer(frameq[pictq_windex], 0);
}

bool EncodingThread::frameq_full() {
    return (pictq_size_count >= (pictq_max_count - 1));
}


void EncodingThread::run() {

    if (!recorder)
        return;

    // prepare
    _quit = false;
    int pictq_usage = 0, picq_size_usage = 0;
    double wait_duration =  400.0 / (double) recorder->getFrameRate(); // 40% of fps

    // loop until break
    while (true) {

        if (pictq_size_count < 1) {
            // no picture ?
            // if it is because we shall quit, then terminate thread
            if (_quit)
                break;

        } else {

            try {
                recorder->addFrame(frameq[pictq_rindex]);
            }
            catch (VideoRecorderException &e){
                qWarning() << "EncodingThread" << QChar(124).toLatin1() << e.message();
                break;
            }

            /* update queue for next picture at the read index */
            if (++pictq_rindex == pictq_max_count)
                pictq_rindex = 0;

            pictq_mutex->lock();
            // remember usage
            pictq_usage = MAXI(pictq_usage, pictq_rindex + 1);
            picq_size_usage = MAXI(picq_size_usage, pictq_size_count + 1);
            // decrease the number of frames in the queue
            pictq_size_count--;
            // tell main process that it can go on (in case it was waiting on a full queue)
            pictq_cond->wakeAll();
            pictq_mutex->unlock();

        }

        // maintain a slow use of ressources for encoding during the recording
        if (!_quit)
            // sleep at least 1 ms, plus an amount of time proportionnal to the remaining buffer
            msleep( 1 + (int)( wait_duration * (double)  (pictq_max_count - pictq_size_count)  / (double) pictq_max_count ) );
        else
            // conversely, after quit recording, loop as fast as possible to finish quickly
            // (but leave time for thread to acquire lock)
            msleep( 1 );

    }

    // inform Rendering Encoder that the process is over (call close())
    emit encodingFinished(_quit);

    // normal exit
    if (_quit)
    {
        qDebug() << "EncodingThread" << QChar(124).toLatin1()  << tr("Encoding finished (%1 % of buffer was used, %2 % was really necessary).").arg((int) (100.f * (float)pictq_usage/(float)pictq_max_count)).arg((int) (100.f * (float)picq_size_usage/(float)pictq_max_count));
    }
    else
        qWarning() << "EncodingThread" << QChar(124).toLatin1()  << tr("Encoding interrupted.");

}

RenderingEncoder::RenderingEncoder(QObject * parent): QObject(parent), started(false), paused(false), elapsed_time(0), skipframecount(0), update(40), displayupdate(33), bufferSize(DEFAULT_RECORDING_BUFFER_SIZE)
{
    // set default format
    format = FORMAT_MP4_H264;
    // set default quality
    quality = QUALITY_AUTO;
    // init file saving
    temporaryFileName = "__temp__";
    savingFolder =  QDesktopServices::storageLocation(QDesktopServices::MoviesLocation);
    temporaryFolder = "";
    setAutomaticSavingMode(false);
    // encoder & recorder not created yet
    encoder = NULL;
    recorder = NULL;
}

RenderingEncoder::~RenderingEncoder() {

    if (encoder)
        delete encoder;

    qDebug() << "RenderingEncoder" << QChar(124).toLatin1() << "All clear.";
}

void RenderingEncoder::setEncodingFormat(encodingformat f){

    if (!started) {
        format = f;
    } else {
        qCritical() << tr ("Cannot change video recording format; Recorder is busy.");
    }
}

void RenderingEncoder::setEncodingQuality(encodingquality q) {

    if (!started) {
        quality = q;
    } else {
        qCritical() << tr ("Cannot change video recording quality; Recorder is busy.");
    }
}


void RenderingEncoder::setActive(bool on)
{
    if (on) {
        // activate if not already active
        if (!started) {
            // create encoding thread
            encoder = new EncodingThread();
            Q_CHECK_PTR(encoder);
            // connect encoder end to closing slot
            connect(encoder, SIGNAL(encodingFinished(bool)), this, SLOT(close(bool)));
            connect(encoder, SIGNAL(finished()), encoder, SLOT(deleteLater()));

            // start encoding thread
            started = start();
            if (started) {
                // inform about starting of recording
                emit selectAspectRatio(RenderingManager::getInstance()->getRenderingAspectRatio());
                emit status(tr("Recording.."), 2000);

                // log
                qDebug() << recorder->getFilename() << QChar(124).toLatin1()  << tr("Recording started (%1 at %2 fps, buffer of %3 frames in %4 ).").arg(recorder->getSuffix()).arg(recorder->getFrameRate()).arg(encoder->getFrameQueueSize()).arg(getByteSizeString(bufferSize));
            }
            else
                qCritical() << tr("Recording aborted. ") << errormessage;
        }
        emit activated(started);
        emit timing( "" );
    }
    else {
        // deactivate if previously started
        if (started) {
            // request stop to encoder
            encoder->stop();
            // stop recording
            started = false;
            // inform GUI the encoder is still processing
            emit processing(true);
        }
        // restore rendering fps
        glRenderWidget::setUpdatePeriod( displayupdate );
    }

}


void RenderingEncoder::setPaused(bool on)
{
    // no pause if not active
    if (!started)
        return;

    // set pause
    paused = on;

    // just inform on the time of pause
    QString duration = getStringFromTime( (double) elapsed_time / 1000.0 );
    if (paused) {
        emit status(tr("Recording paused at %1").arg(duration), 2000);
        qDebug() << "RenderingEncoder" << QChar(124).toLatin1() << tr("Recording paused at %1").arg(duration);
    }
    else {
        emit status(tr("Recording resumed at %1").arg(duration), 2000);
        qDebug() << "RenderingEncoder" << QChar(124).toLatin1() << tr("Recording resumed at %1.").arg(duration);
    }

    // restart timer
    timer.start();
}

// Start the encoding process
// - Create codec
// - Create the temporary file
bool RenderingEncoder::start(){

    // prevent re-start
    if (started) {
        errormessage = "Already recording.";
        return false;
    }

    // init
    skipframecount = 0;
    errormessage.clear();

    // if the temporary file already exists, delete it.
    if (temporaryFolder.exists(temporaryFileName)){
        temporaryFolder.remove(temporaryFileName);
    }

    // compute desired update frequency
    int desired_fps = (int) ( 1000.0 / double(update) );

    // read current frame rate
    int fps = RenderingManager::getRenderingWidget()->getFramerate();

    // show warning if frame rate is already too low
    if ( fps <  (desired_fps-2) ) {
         QMessageBox msgBox;
         msgBox.setIcon(QMessageBox::Question);
         msgBox.setText(tr("Rendering frequency is lower than the recording %1 fps.").arg(desired_fps));
         msgBox.setInformativeText(tr("Do you still want to record at %1 fps ?").arg(fps));
         msgBox.setDetailedText( tr("The rendering is currently at %1 fps on average, but your recording preferences are set to %2 fps.\n\n"
                 "You can either agree to record at this lower frame rate, or retry later after some optimizations:\n"
                 "- select a lower quality in your rendering preferences\n"
                 "- lower the resolution of some sources\n"
                 "- remove some sources\n").arg(fps).arg(desired_fps) );

         QPushButton *abortButton = msgBox.addButton(QMessageBox::Discard);
         msgBox.addButton(tr("Record at lower frequency"), QMessageBox::AcceptRole);
         msgBox.exec();
         if (msgBox.clickedButton() == abortButton) {
             errormessage = "Recording aborted.";
             return false;
         }
         // Continue anyway : set the recoding frequency to be at the fps of the rendering
         desired_fps = fps;
    }

    // remember current update display period
    displayupdate = glRenderWidget::updatePeriod();

    // setup new display update period to match recording update
    // adjust update to match the fps
    glRenderWidget::setUpdatePeriod( (int) ( 1000.0 / double(desired_fps)) );

    // initialization of ffmpeg recorder
    QString filename = temporaryFolder.absoluteFilePath(temporaryFileName);
    QSize framesSize = RenderingManager::getInstance()->getFrameBufferResolution();
    try {
        // allocate recorder
        recorder = VideoRecorder::getRecorder(format, filename, framesSize.width(), framesSize.height(), desired_fps, quality);
        // open recorder
        recorder->open();
    }
    catch (VideoRecorderException &e){
        recorder = NULL;
        errormessage = QString("Failed to initiate recoding. %1").arg(e.message());
        return false;
    }

    // initialize encoder
    encoder->initialize(recorder, framesSize.width(), framesSize.height(), bufferSize);
    // start the encoding thread
    encoder->start();

    // start the timers
    elapsed_time = 0;
    timer.start();

    return true;
}


int RenderingEncoder::getRecodingTime()
{
    return elapsed_time;
}

bool RenderingEncoder::acceptFrame()
{
    // is the encoder at work?
    if (started && !paused) {

        // if the recorder cannot follow
        if ( encoder && encoder->frameq_full() ) {
            // remember amount of skipped frames
            skipframecount++;
            // skip the frame
            return false;
        }
        return true;
    }
    return false;
}


// Add a frame to the stream
// This function is called with the rendering context active
// by the update method in the ViewRenderWidget
// it *should* be called at the desired frame rate
void RenderingEncoder::addFrame(uint8_t *data){

    // just to be sure
    if (!started || encoder == NULL)
        return;

//    // if the recorder cannot follow
//    if ( encoder->frameq_full() )
//    {
//        // remember amount of skipped frames
//        skipframecount++;

//        // skip the frame
//        return;
//    }

    // lock access to frame and get buffer
    // (get the pointer to the current writing buffer from the queue of the thread to know where to write)
    AVBufferRef *buf = encoder->lockFrameAndGetBuffer();
    if (!buf) {
        // failed
        skipframecount++;
        return;
    }

    if (data) {
        // read the pixels from the given buffer and store into the temporary buffer queue
        memmove( buf->data, data, qMin( buf->size, encoder->getFrameWidth() * encoder->getFrameHeight() * 3) );

    } else
#ifdef RECORDING_READ_PIXEL
        // ReadPixel of _fbo
        glReadPixels(0, 0, encoder->getFrameWidth(), encoder->getFrameHeight(), GL_RGB, GL_UNSIGNED_BYTE, buf->data);
#else
        // read the pixels from the texture
        glGetTexImage(GL_TEXTURE_2D, 0, GL_RGB, GL_UNSIGNED_BYTE, buf->data);
#endif

    // elapsed time
    elapsed_time += timer.restart();

    // inform the thread that a picture was pushed into the queue
    encoder->releaseAndPushFrame( elapsed_time );

    // display record time
    emit timing( getStringFromTime( (double) elapsed_time / 1000.0) );
}

void RenderingEncoder::kill(){
    // deactivate if previously started
    if (started) {
        // request stop to encoder
        encoder->terminate();
        encoder->wait(1000);
    }
}

// Close the encoding process
void RenderingEncoder::close(bool success){

    // temporarily store information for log
    int framecount = 0;
    int fps = recorder->getFrameRate();
    QString filename = recorder->getFilename();
    QString suffix_file = recorder->getSuffix();
    QString description_file = recorder->getDescription();
    QString duration = getStringFromTime( (double) elapsed_time / 1000.0 );

    // stop recorder
    try {
        framecount = recorder->close();
    }
    catch (VideoRecorderException &e){
        errormessage = tr("Error closing recording. %1").arg(e.message());
        success = false;
    }

    // inform we are off
    started = false;
    emit selectAspectRatio(ASPECT_RATIO_FREE);
    emit activated(false);
    emit timing( "" );

    // encoder is automatically deleted
    encoder = NULL;

    // free recorder
    delete recorder;
    recorder = NULL;

    // inform GUI that processing is over
    emit processing(false);

    // If there was no error
    if (success) {

        // Log
        qDebug() << filename << QChar(124).toLatin1() << tr("Recording finished (%1 frames in %2).").arg(framecount).arg(duration);

        // show warning if too many frames were bad
        bool savefile = true;
        float percent = float(skipframecount) / float(framecount + skipframecount);
        if ( percent > 0.03f  ) {

            QMessageBox msgBox;
            msgBox.setIcon(QMessageBox::Warning);
            msgBox.setText(tr("A movie has been recorded, but your system couldn't record at %2 fps and %1 % of the frames were lost.").arg( 100.f * percent ).arg(fps));
            msgBox.setInformativeText(tr("Do you still want to save it ?"));
            msgBox.setDetailedText( tr("Only %1 of %2 frames were recorded. "
                                       "Playback of the movie might be jerky.\n"
                                       "To avoid this, change the preferences to:\n"
                                       " - another rendering resolution\n"
                                       " - a lower recording frame rate\n"
                                       " - a larger recording buffer size\n"
                                       " - another recording codec").arg(framecount).arg(framecount + skipframecount));

            QPushButton *abortButton = msgBox.addButton(QMessageBox::Discard);
            msgBox.addButton(tr("Save anyway"), QMessageBox::AcceptRole);

            // display dialog warning
            msgBox.exec();
            if (msgBox.clickedButton() == abortButton) {
                savefile = false;
            }
        }

        // save file
        if (savefile) {
            if (automaticSaving)
                saveFile(suffix_file);
            else
                saveFileAs(suffix_file, description_file);
        }
        else
            qDebug() << tr("Recording not saved.");

    }
    else {
        // Log
        qCritical() << "RenderingEncoder" << QChar(124).toLatin1() << tr("Recording failed. %1").arg(errormessage);
    }

}


void RenderingEncoder::setAutomaticSavingMode(bool on) {

    automaticSaving = on;

    // ensure the temporary file is in the same folder as the destination file
    // to avoid copy (rename) of file accross drives
    if (automaticSaving)
        temporaryFolder = savingFolder;
    else
        temporaryFolder = QDir::temp();

}


void RenderingEncoder::setAutomaticSavingFolder(QString d) {

    QDir directory(d);

    if ( d.isEmpty() || !directory.exists())
        savingFolder = QDesktopServices::storageLocation(QDesktopServices::MoviesLocation);
    else
        savingFolder =  directory;

    setAutomaticSavingMode(automaticSaving);
}

void RenderingEncoder::saveFile(QString suffix, QString filename){

    if (filename.isNull())
        filename = QString("glmixervideo%1%2").arg(QDate::currentDate().toString("yyMMdd")).arg(QTime::currentTime().toString("hhmmss")) + '.' + suffix;

    QFileInfo infoFileDestination(savingFolder, filename);

    // delete file if exists
    if (infoFileDestination.exists()){
        infoFileDestination.dir().remove(infoFileDestination.fileName());
    }

    // move the temporaryFileName to newFileName
    if (!temporaryFolder.rename(temporaryFileName, infoFileDestination.fileName()) )
        qWarning() << infoFileDestination.absoluteFilePath() << QChar(124).toLatin1() << tr("Could not save file (file exists already?).");
    else {
        emit status(tr("File %1 saved.").arg(infoFileDestination.absoluteFilePath()), 2000);
        qDebug() << infoFileDestination.absoluteFilePath() << QChar(124).toLatin1() << tr("File saved.");
    }
}

void RenderingEncoder::saveFileAs(QString suffix, QString description){

    QString suggestion = QString("glmixervideo%1%2").arg(QDate::currentDate().toString("yyMMdd")).arg(QTime::currentTime().toString("hhmmss"));

    QString newFileName = GLMixer::getInstance()->getFileName(tr("Save recorded video"),
                                                              description, suffix, suggestion);
    // if we got a filename, save the file:
    if (!newFileName.isEmpty()) {

        // delete file if exists
        QFileInfo infoFileDestination(newFileName);
        if (infoFileDestination.exists()){
            infoFileDestination.dir().remove(infoFileDestination.fileName());
        }
        // move the temporaryFileName to newFileName
        temporaryFolder.rename(temporaryFileName, newFileName);
        emit status(tr("File %1 saved.").arg(newFileName), 2000);
        qDebug() << newFileName << QChar(124).toLatin1() << tr("Recording saved.");
    }

}

void RenderingEncoder::setBufferSize(unsigned long bytes){

    bufferSize = CLAMP(bytes, MIN_RECORDING_BUFFER_SIZE, MAX_RECORDING_BUFFER_SIZE);
}

unsigned long RenderingEncoder::getBufferSize() {

    return bufferSize;
}

unsigned long RenderingEncoder::computeBufferSize(int percent) {

    long double p = (double) CLAMP(percent, 0, 100) / 100.0;
    unsigned long megabytes = MIN_RECORDING_BUFFER_SIZE;
    megabytes += (unsigned long) ( p * (long double)(MAX_RECORDING_BUFFER_SIZE - MIN_RECORDING_BUFFER_SIZE));

    return megabytes;
}

int RenderingEncoder::computeBufferPercent(unsigned long bytes) {

    unsigned long b = bytes - MIN_RECORDING_BUFFER_SIZE;
    double p = (double) b / (double)(MAX_RECORDING_BUFFER_SIZE - MIN_RECORDING_BUFFER_SIZE);

    return ( (int) qRound(p * 100.0) );
}
