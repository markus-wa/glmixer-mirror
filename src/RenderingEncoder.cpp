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

#include <QSize>
#include <QFileInfo>
#include <QFileDialog>
#include <QMessageBox>
#include <QGLFramebufferObject>
#include <QThread>


class EncodingThread: public QThread {
public:
	EncodingThread(int bufsize = 10) : QThread(), rec(NULL), period(40), pictq_max(bufsize), pictq_size(0), pictq_rindex(0), pictq_windex(0){
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
				msleep(5);

		} else {

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

			// how long since last frame ?
			dt = period - (int) timer.restart();
			// wait for the time needed to be at the good frame rate (if more than 1 ms)
			if (dt > 1)
				msleep(dt);
		}
	}
}

RenderingEncoder::RenderingEncoder(QObject * parent): QObject(parent), started(false), elapseTimer(0), badframecount(0), fbohandle(0), update(40), displayupdate(33) {

	// set default format
	temporaryFileName = "glmixeroutput";
	setEncodingFormat(FORMAT_AVI_FFVHUFF);
	// init file saving dir
	sfa.setDirectory(QDir::currentPath());
	sfa.setAcceptMode(QFileDialog::AcceptSave);
	sfa.setFileMode(QFileDialog::AnyFile);
	// create encoding thread
	encoder = new EncodingThread();
}

RenderingEncoder::~RenderingEncoder() {
	free(encoder);
}

void RenderingEncoder::setEncodingFormat(encodingformat f){

	if (!started) {
		format = f;
	} else {
		qCritical("ERROR setting video recording format.\n\nRecorder is busy.");
	}
}


void RenderingEncoder::setActive(bool on)
{
	if (on) {
		if (!start())
			qCritical("ERROR starting video recording.\n\n%s\n", errormessage);
	} else {
		if (close())
			saveFileAs();
	}

	// inform if we could be activated
    emit activated(started);
}

// Start the encoding process
// - Create codec
// - Create the temporary file
bool RenderingEncoder::start(){

	if (started)
		return false;

	// if the temporary file already exists, delete it.
	if (QDir::temp().exists(temporaryFileName)){
		QDir::temp().remove(temporaryFileName);
	}

	// compute desired update frequency
	int freq = (int) ( 1000.0 / double(update) );
	int fps = RenderingManager::getRenderingWidget()->getFramerate();

	// show warning if frame rate is already too low
	if ( fps <  freq ) {
		 QMessageBox msgBox;
		 msgBox.setIcon(QMessageBox::Warning);
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

	// get access to the size of the renderer fbo
	QSize fbosize = RenderingManager::getInstance()->getFrameBufferResolution();
	fbohandle =  RenderingManager::getInstance()->getFrameBufferHandle();

	// setup update period for recording
	displayupdate = glRenderWidget::updatePeriod();
	glRenderWidget::setUpdatePeriod( update );

	// initialization of ffmpeg recorder
	recorder = video_rec_init(qPrintable( QDir::temp().absoluteFilePath(temporaryFileName)), format, fbosize.width(), fbosize.height(), freq, errormessage);
	if (recorder == NULL)
		return false;

	// init
	encoder->initialize(recorder, update);
	badframecount = 0;

	// start
    emit status(tr("Start recording."), 1000);
	timer.start();
	elapseTimer = startTimer(1000); // emit the duration of recording every second
	encoder->start();

	// set status
	started = true;

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

	if (!started || recorder == NULL)
		return;

	// bind rendering frame buffer object
	glBindFramebuffer(GL_READ_FRAMEBUFFER, fbohandle);
	// read the pixels and store into the temporary buffer queue
	// (get the pointer to the current writing buffer from the queue of the thread to know where to write)
	glReadPixels((GLint)0, (GLint)0, (GLint) recorder->width, (GLint) recorder->height, GL_BGRA, GL_UNSIGNED_BYTE, encoder->pictq_top());
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

    // free opengl buffer
//	free(tmpframe);

	// done
	killTimer(elapseTimer);
	started = false;

	// restore former display update period
	glRenderWidget::setUpdatePeriod( displayupdate );

	// show warning if too many frames were bad
	if ( float(badframecount) / float(framecount) > 0.7f  ) {

		 QMessageBox msgBox;
		 msgBox.setIcon(QMessageBox::Warning);
		 msgBox.setText(tr("The movie has been recorded, but %1 % of the frames were not synchronous.").arg((badframecount * 100) / (framecount)));
		 msgBox.setInformativeText(tr("Do you still want to save the movie ?"));
		 msgBox.setDetailedText( tr("This is because the recording frame rate was %1 fps on average instead of the targeted %2 fps.\n\n"
				 "The consequence is that timing of the movie will be incorrect (play too fast). "
				 "To avoid this, change the preferences to:\n"
				 "- a faster recording codec (VFFHUFF is the fastest)\n"
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

void RenderingEncoder::saveFileAs(){

	// Select file format
	switch (format) {
	case FORMAT_MPG_MPEG1:
		sfa.setFilter(tr("MPEG 1 Video (*.mpg *.mpeg)"));
		sfa.setDefaultSuffix("mpg");
		break;
	case FORMAT_MP4_MPEG4:
		sfa.setFilter(tr("MPEG 4 Video (*.mp4)"));
		sfa.setDefaultSuffix("mp4");
		break;
	case FORMAT_WMV_WMV2:
		sfa.setFilter(tr("Windows Media Video (*.wmv)"));
		sfa.setDefaultSuffix("wmv");
		break;
	case FORMAT_FLV_FLV1:
		sfa.setFilter(tr("Flash Video (*.flv)"));
		sfa.setDefaultSuffix("flv");
		break;
	default:
	case FORMAT_AVI_FFVHUFF:
		sfa.setFilter(tr("AVI Video (*.avi)"));
		sfa.setDefaultSuffix("avi");
	}

	// get file name
	if (sfa.exec()) {
	    QString newFileName = sfa.selectedFiles().front();
		// now we got a filename, save the file:
		if (!newFileName.isEmpty()) {

			// delete file if exists
			QFileInfo infoFileDestination(newFileName);
			if (infoFileDestination.exists()){
				infoFileDestination.dir().remove(infoFileDestination.fileName());
			}
			// move the temporaryFileName to newFileName
			QDir::temp().rename(temporaryFileName, newFileName);
			emit status(tr("File %1 saved.").arg(newFileName), 2000);
		}
	}

}
