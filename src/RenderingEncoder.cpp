/*
 * RenderingEncoder.cpp
 *
 *  Created on: Mar 13, 2011
 *      Author: bh
 */

#include "RenderingEncoder.moc"

#include "common.h"
#include "RenderingManager.h"
#include "ViewRenderWidget.h"
#include "glmixer.h"

#include <QSize>
#include <QBuffer>
#include <QFileInfo>
#include <QMessageBox>
#include <QGLFramebufferObject>
#include <QThread>


class EncodingThread: public QThread {

public:

	EncodingThread(int bufsize = 10) : QThread(), rec(NULL), period(40), quit(true), pictq_max(bufsize),
		pictq_size(0), pictq_rindex(0), pictq_windex(0)
	{
		pictq = (char **) malloc( pictq_max * sizeof(char *) );
		// create mutex
	    pictq_mutex = new QMutex;
	    Q_CHECK_PTR(pictq_mutex);
	    pictq_cond = new QWaitCondition;
	    Q_CHECK_PTR(pictq_cond);
	}

	~EncodingThread(){
		free(pictq_mutex);
		free(pictq_cond);
		stop();
		free(pictq);
	}

    void initialize(video_rec_t *recorder, int update) {
    	rec = recorder;
    	period =  update;
    	// allocate buffer
    	for (int i = 0; i < pictq_max; ++i)
    		pictq[i] = (char *) malloc(rec->width * rec->height * 4);
    	// init variables
    	pictq_size = pictq_rindex = pictq_windex = 0;
    }

    void stop() {
    	if (quit)
    		return;
    	// end thread
    	quit = true;
    	// the thread now knows that it should end, we wait for it to terminate...
        wait();
    	// free buffer
    	for (int i = 0; i < pictq_max; ++i)
    		free(pictq[i]);
    }

    void pictq_push(){
    	// set to write index to next in queue
        if (++pictq_windex == pictq_max)
            pictq_windex = 0;
		/* now we inform our encoding thread that we have a picture ready */
        pictq_mutex->lock();
        pictq_size++;
        pictq_mutex->unlock();
    }

    char *pictq_top(){
    	// wait until we have space for a new picture
		// (this happens only when the queue is full)
		pictq_mutex->lock();
		while (pictq_size >= (pictq_max - 1) && !quit)
			pictq_cond->wait(pictq_mutex); // the condition is released in run()
		pictq_mutex->unlock();

        // Fill the queue with the given picture
        return pictq[pictq_windex];
    }

protected:
    void run();

    // ref to the recorder
	video_rec_t *rec;
	int period;

    // execution management
    bool quit;
    QMutex *pictq_mutex;
    QWaitCondition *pictq_cond;

    // picture queue management
    char** pictq;
    int pictq_max, pictq_size, pictq_rindex, pictq_windex;
    QTime timer;
};

void EncodingThread::run() {

	// prepare
	quit = false;
	int dt =  0;
	timer.start();

	// loop until break
	while (true) {

		if (pictq_size < 1) {
			// no picture ?
			// if it is because we shall quit, then terminate thread
			if (quit)
				break;
			// otherwise, quickly retry...
			else
				msleep(period / 2);

		} else {
			timer.restart();

			// record the picture to encode by calling the function specified in the recorder
			(*(rec)->pt2RecordingFunction)(rec, pictq[pictq_rindex]);

			/* update queue for next picture at the read index */
			if (++pictq_rindex == pictq_max)
				pictq_rindex = 0;

			pictq_mutex->lock();
			// decrease the number of frames in the queue
			pictq_size--;
			// tell main process that it can go on (in case it was waiting on a full queue)
			pictq_cond->wakeAll();
			pictq_mutex->unlock();

			// how long time remains ?
			dt = period - (int) timer.elapsed();
			if (dt > 0 && dt < period)
				msleep(dt);

		}
	}
}

RenderingEncoder::RenderingEncoder(QObject * parent): QObject(parent), started(false), paused(false),
													elapseTimer(0), badframecount(0), update(40), displayupdate(33) {

	// set default format
	temporaryFileName = "__temp__";
	setEncodingFormat(FORMAT_AVI_FFVHUFF);
    // init file saving
	setAutomaticSavingMode(false);
	// create encoding thread
	encoder = new EncodingThread();
    Q_CHECK_PTR(encoder);
}

RenderingEncoder::~RenderingEncoder() {
	free(encoder);
}

void RenderingEncoder::setEncodingFormat(encodingformat f){

	if (!started) {
		format = f;
	} else {
		qCritical() << "RenderingEncoder|" << tr ("Cannot change video recording format; Recorder is busy.");
	}
}


void RenderingEncoder::setActive(bool on)
{
	if (on) {
		if (!start())
			qCritical() << "RenderingEncoder|" << tr("Error starting video recording; %1.").arg(errormessage);
	} else {
		if (close()) {
			if (automaticSaving)
				saveFile();
			else
				saveFileAs();
		}
		video_rec_free(recorder);
	}

	// restore former display update period
	if (!started)
		glRenderWidget::setUpdatePeriod( displayupdate );

	// inform if we could be activated
    emit activated(started);
}


void RenderingEncoder::setPaused(bool on)
{
	static int elapsed = 0;

	// no pause if not active
	if (!started)
		return;

	// set pause
	paused = on;

	if (paused) {
		// remember timer
		elapsed = timer.elapsed();
		killTimer(elapseTimer);
		emit status(tr("Recording paused after %1 s").arg(elapsed/1000), 3000);
		qDebug() << "RenderingEncoder|" << tr("Paused (%1s).").arg(elapsed/1000);
	} else {
		// restart a timer
		timer = timer.addMSecs(timer.elapsed() - elapsed);
		elapseTimer = startTimer(1000);
	    emit status(tr("Recording time: %1 s").arg(timer.elapsed()/1000), 1000);
		qDebug() << "RenderingEncoder|" << tr("Resumed (%1).").arg(elapsed/1000);
	}

}

// Start the encoding process
// - Create codec
// - Create the temporary file
bool RenderingEncoder::start(){

	if (started) {
		QByteArray(errormessage, 256) = "Already recording.";
		return false;
	}

	// if the temporary file already exists, delete it.
	if (temporaryFolder.exists(temporaryFileName)){
		temporaryFolder.remove(temporaryFileName);
	}

	// compute desired update frequency
	int freq = (int) ( 1000.0 / double(update) );
	int fps = RenderingManager::getRenderingWidget()->getFramerate();

	// show warning if frame rate is already too low
	if ( fps <  freq ) {
		 QMessageBox msgBox;
		 msgBox.setIcon(QMessageBox::Question);
		 msgBox.setText(tr("Rendering frequency is lower than the requested %1 fps.").arg(freq));
		 msgBox.setInformativeText(tr("Do you still want to record at %1 fps ?").arg(fps));
		 msgBox.setDetailedText( tr("The rendering is currently at %1 fps on average, but your recording preferences are set to %2 fps.\n\n"
				 "You can either agree to record at this lower frame rate, or retry later after some optimizations:\n"
				 "- select a lower quality in your rendering preferences\n"
				 "- lower the resolution of some sources\n"
				 "- remove some sources\n").arg(fps).arg(freq) );

		 QPushButton *abortButton = msgBox.addButton(QMessageBox::Discard);
		 msgBox.addButton(tr("Record at lower frequency"), QMessageBox::AcceptRole);
		 msgBox.exec();
		 if (msgBox.clickedButton() == abortButton) {
		     return false;
		 }
		 // Continue anyway : set the recoding frequency to be at the fps of the rendering
		 freq = fps;
	}

	// setup update period for recording
	displayupdate = glRenderWidget::updatePeriod();
	glRenderWidget::setUpdatePeriod( update );

	// initialization of ffmpeg recorder
	recorder = video_rec_init(qPrintable(temporaryFolder.absoluteFilePath(temporaryFileName)), format, framesSize.width(), framesSize.height(), freq, errormessage);
	if (recorder == NULL)
		return false;

	// init
	encoder->initialize(recorder, update);
	badframecount = 0;
	emit selectAspectRatio(RenderingManager::getInstance()->getRenderingAspectRatio());

	// start
    emit status(tr("Start recording."), 1000);
	timer.start();
	elapseTimer = startTimer(1000); // emit the duration of recording every second
	encoder->start();
	encoder->setPriority(QThread::LowPriority);

	// set status
	started = true;
    qDebug() << "RenderingEncoder|" << tr("Start recording (%1).").arg(recorder->suffix);

	return true;
}

void RenderingEncoder::timerEvent(QTimerEvent *event)
{
    emit status(tr("Recording time: %1 s").arg(timer.elapsed()/1000), 1000);
}

int RenderingEncoder::getRecodingTime() {

	if (started)
		return timer.elapsed();
	else
		return 0;
}

// Add a frame to the stream
// This function is called with the rendering context active
// by the update method in the ViewRenderWidget
// it *should* be called at the desired frame rate
void RenderingEncoder::addFrame(){

	if (!started || paused || recorder == NULL)
		return;

#ifdef USE_GLREADPIXELS
	// read the pixels from the current frame buffer and store into the temporary buffer queue
	// (get the pointer to the current writing buffer from the queue of the thread to know where to write)
	glReadPixels((GLint)0, (GLint)0, (GLint) recorder->width, (GLint) recorder->height, GL_BGRA, GL_UNSIGNED_INT_8_8_8_8_REV, encoder->pictq_top());

#else
	// read the pixels from the texture
	glGetTexImage(GL_TEXTURE_2D, 0, GL_BGRA, GL_UNSIGNED_INT_8_8_8_8_REV, encoder->pictq_top());
#endif

	// inform the thread that a picture was pushed into the queue
	encoder->pictq_push();

	// increment the bad frame counter each time the frame rate is bellow 80% of the target frame rate
	if ( RenderingManager::getRenderingWidget()->getFramerate() <  800.0 / double(update) )
		badframecount++;

}

// Close the encoding process
bool RenderingEncoder::close(){

	if (!started)
		return false;

	// end encoder
	encoder->stop();

	// stop recorder
	int framecount = video_rec_stop(recorder);
	int duration = timer.elapsed();

	// done
	emit selectAspectRatio(ASPECT_RATIO_FREE);
    emit status(tr("Recorded %1 s").arg(timer.elapsed()/1000), 3000);
	killTimer(elapseTimer);
	started = false;

	// show warning if too many frames were bad
	if ( float(badframecount) / float(framecount) > 0.7f  ) {

		 QMessageBox msgBox;
		 msgBox.setIcon(QMessageBox::Warning);
		 msgBox.setText(tr("The movie has been recorded, but %1 % of the frames were not synchronous.").arg((badframecount * 100) / (framecount)));
		 msgBox.setInformativeText(tr("Do you still want to save the movie ?"));
		 msgBox.setDetailedText( tr("This is because the recording frame rate was %1 fps on average instead of the targeted %2 fps.\n\n"
				 "The consequence is that timing of the movie will be incorrect (play too fast). "
				 "To avoid this, change the preferences to:\n"
				 "- a faster recording codec\n"
				 "- a lower recording frame rate\n"
				 "- a lower rendering quality").arg((1000 * framecount)/duration).arg((int) ( 1000.0 / double(update) )) );

		 QPushButton *abortButton = msgBox.addButton(QMessageBox::Discard);
		 msgBox.addButton(tr("Save anyway"), QMessageBox::AcceptRole);

		 msgBox.exec();
		 if (msgBox.clickedButton() == abortButton) {
		     return false;
		 }
	}

	return true;
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


void RenderingEncoder::setAutomaticSavingFolder(QDir d) {

	savingFolder = d;

	setAutomaticSavingMode(automaticSaving);
}

void RenderingEncoder::saveFile(){

	QFileInfo infoFileDestination(savingFolder, QDateTime::currentDateTime().toString(Qt::ISODate) + '.' + recorder->suffix);

	if (infoFileDestination.exists()){
		infoFileDestination.dir().remove(infoFileDestination.fileName());
	}
	// move the temporaryFileName to newFileName
	temporaryFolder.rename(temporaryFileName, infoFileDestination.absoluteFilePath());
	emit status(tr("File %1 saved.").arg(infoFileDestination.absoluteFilePath()), 2000);
	qDebug() << infoFileDestination.absoluteFilePath() << tr("|Recording saved.");
}

void RenderingEncoder::saveFileAs(){

    QString suggestion = QString("glmixervideo %1 %2").arg(QDate::currentDate().toString()).arg(QTime::currentTime().toString());

    QString newFileName = GLMixer::getInstance()->getFileName(tr("Save recorded video"),
                                                              recorder->description,
                                                              recorder->suffix,
                                                              suggestion);

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
        qDebug() << newFileName << tr("|Recording saved.");
    }

}
