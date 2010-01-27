/*
 * OpencvSource.cpp
 *
 *  Created on: Dec 13, 2009
 *      Author: bh
 */

#include <OpencvSource.moc>

#include <QThread>
#include <QTime>

class CameraThread: public QThread {
public:
	CameraThread(OpencvSource *source) :
        QThread(), cvs(source), end(false) {
    }
    ~CameraThread() {
    }

    void run();

    OpencvSource* cvs;
    bool end;

};

void CameraThread::run(){

	QTime t;
	int f = 0;

	t.start();
	while (!end) {

		cvs->mutex->lock();
		if (!cvs->frameChanged) {
			cvs->frame = cvQueryFrame( cvs->capture );
			cvs->frameChanged = true;
			cvs->cond->wait(cvs->mutex);
		}
		cvs->mutex->unlock();

		if ( ++f == 100 )  // hundred frames to average the frame rate
			cvs->framerate = 100000.0 / (double) t.elapsed();
	}
}

OpencvSource::OpencvSource(int opencvIndex, QGLWidget *context) : Source(context), frameChanged(false), framerate(0.0)
{

	opencvCameraIndex = opencvIndex;
	capture = cvCreateCameraCapture(opencvCameraIndex);
    Q_CHECK_PTR(capture);
//	if (!capture) {
//		// TODO Through exception
//	}

	width = (int) cvGetCaptureProperty(capture, CV_CAP_PROP_FRAME_WIDTH);
	height = (int) cvGetCaptureProperty(capture, CV_CAP_PROP_FRAME_HEIGHT);
	aspectratio = (float)width / (float)height;

	// create thread
	mutex = new QMutex;
    Q_CHECK_PTR(mutex);
    cond = new QWaitCondition;
    Q_CHECK_PTR(cond);
	thread = new CameraThread(this);
    Q_CHECK_PTR(thread);
	thread->start();
}


OpencvSource::~OpencvSource() {

	thread->end = true;
	mutex->lock();
	cond->wakeAll();
	mutex->unlock();
    thread->wait(500);
	delete thread;
	delete cond;
	delete mutex;

	if (capture)
		cvReleaseCapture(&capture);

}


void OpencvSource::play(bool on){

	if ( on ) { // start play
		if (! isRunning() ) {
			thread->end = false;
			thread->start();
		}
	} else { // stop play
		if ( isRunning() ) {
			thread->end = true;
			mutex->lock();
			cond->wakeAll();
		    frameChanged = false;
			mutex->unlock();
		    thread->wait(500);
		}
	}
}

bool OpencvSource::isRunning(){

	return !thread->end;

}

void OpencvSource::update(){

	if( frameChanged )
	{
    	// update the texture
        glBindTexture(GL_TEXTURE_2D, textureIndex);
		glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

		mutex->lock();
		frameChanged = false;
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, frame->width, frame->height,0, GL_BGR, GL_UNSIGNED_BYTE, (unsigned char*) frame->imageData);
		cond->wakeAll();
		mutex->unlock();
	}

}
