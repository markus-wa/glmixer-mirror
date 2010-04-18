/*
 * OpencvSource.cpp
 *
 *  Created on: Dec 13, 2009
 *      Author: bh
 */

#include "OpencvSource.moc"

Source::RTTI OpencvSource::type = Source::CAMERA_SOURCE;

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
			if (cvGrabFrame( cvs->capture )){
				cvs->frame = cvRetrieveFrame( cvs->capture );
				cvs->frameChanged = true;
			} else
				end = true;
			cvs->cond->wait(cvs->mutex);
		}
		cvs->mutex->unlock();

		if ( ++f == 100 ) { // hundred frames to average the frame rate {
			cvs->framerate = 100000.0 / (double) t.elapsed();
			t.restart();
			f = 0;
		}
	}
}

OpencvSource::OpencvSource(int opencvIndex, GLuint texture, double d) : Source(texture, d), framerate(0.0)
{

	opencvCameraIndex = opencvIndex;
	capture = cvCreateCameraCapture(opencvCameraIndex);
	if (!capture) {
		throw NoCameraIndexException();
	}

	width = (int) cvGetCaptureProperty(capture, CV_CAP_PROP_FRAME_WIDTH);
	height = (int) cvGetCaptureProperty(capture, CV_CAP_PROP_FRAME_HEIGHT);
	aspectratio = (float)width / (float)height;
	name.prepend("opencv");

	// fill in first frame
	glBindTexture(GL_TEXTURE_2D, textureIndex);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	frame = cvQueryFrame( capture );
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, frame->width, frame->height,0, GL_BGR, GL_UNSIGNED_BYTE, (unsigned char*) frame->imageData);

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

	play(false);
	delete cond;
	delete mutex;
	delete thread;

	// should delete
	if (capture)
		cvReleaseCapture(&capture);

	// free the OpenGL texture
	glDeleteTextures(1, &textureIndex);
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

	Source::update();

	if( frameChanged )
	{
    	// update the texture
		glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

		mutex->lock();
		frameChanged = false;
		glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, frame->width, frame->height, GL_BGR, GL_UNSIGNED_BYTE, (unsigned char*) frame->imageData);
		cond->wakeAll();
		mutex->unlock();
	}

}
