/*
 * OpencvSource.cpp
 *
 *  Created on: Dec 13, 2009
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
 *   Copyright 2009, 2010 Bruno Herbelin
 *
 */

#include "OpencvSource.moc"

Source::RTTI OpencvSource::type = Source::CAMERA_SOURCE;
bool OpencvSource::playable = true;

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

	if ( isPlaying() == on )
		return;

	if ( on ) { // start play
		thread->end = false;
		thread->start();
	} else { // stop play
		thread->end = true;
		mutex->lock();
		cond->wakeAll();
		frameChanged = false;
		mutex->unlock();
		thread->wait(500);
	}
}

bool OpencvSource::isPlaying() const{

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
