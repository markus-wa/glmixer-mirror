/*
 * AlgorithmSource.cpp
 *
 *  Created on: Feb 27, 2010
 *      Author: bh
 */

#include "AlgorithmSource.moc"

#include <limits>
#include <iostream>
//#include <ctime>

#include <QMutex>
#include <QWaitCondition>
#include <QThread>
#include <QTime>

class AlgorithmThread: public QThread {
public:
	AlgorithmThread(AlgorithmSource *source) :
        QThread(), as(source), end(false) {
    }
    ~AlgorithmThread() {
    }

    void run();

    AlgorithmSource* as;
    bool end;

};

void AlgorithmThread::run(){

	QTime t;
	int f = 0;

	t.start();
	while (!end) {

		as->mutex->lock();
		if (!as->frameChanged) {
			// compute new frame

			// Immediately discard the FLAT 'algo' ; it is the "do nothing" algorithm :)
			if( as->algotype != AlgorithmSource::FLAT )
			{
				srand( t.elapsed() );
				if ( as->algotype == AlgorithmSource::BW_NOISE ){
					for (int i = 0; i < (as->width * as->height); ++i) {
						as->buffer[i * 4 + 0] = (unsigned char) ( rand() % std::numeric_limits<unsigned char>::max());
						as->buffer[i * 4 + 1] = as->buffer[i * 4];
						as->buffer[i * 4 + 2] = as->buffer[i * 4];
						as->buffer[i * 4 + 3] = as->buffer[i * 4];
					}

				} else
				if ( as->algotype == AlgorithmSource::COLOR_NOISE ){
					for (int i = 0; i < (as->width * as->height * 4); ++i)
		//				buffer[i] = (unsigned char) (  rand() % std::numeric_limits<unsigned char>::max() );
						as->buffer[i] = (unsigned char) ((  rand() % 2 ) * std::numeric_limits<unsigned char>::max());

				}

			}
			as->frameChanged = true;
			as->cond->wait(as->mutex);
		}
		as->mutex->unlock();

		if (as->period > 0){
			// wait for the period duration before updating next frame
			usleep(as->period);
		}

		if ( ++f == 100 ) { // hundred frames to average the frame rate {
			as->framerate = 100000.0 / (double) t.elapsed();
			t.restart();
			f = 0;
		}
	}
}


AlgorithmSource::AlgorithmSource(int type, GLuint texture, double d, int w, int h, unsigned long  p) : Source(texture, d), width(w), height(h), period(p), framerate(0) {

	algotype = CLAMP(AlgorithmSource::algorithmType(type), AlgorithmSource::FLAT, AlgorithmSource::WATER);
	// allocate and initialize the buffer
	buffer = new unsigned char [width * height * 4];
	initBuffer();

	aspectratio = (float)width / (float)height;

	// apply the texture
	glBindTexture(GL_TEXTURE_2D, textureIndex);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height,0, GL_BGRA, GL_UNSIGNED_BYTE, (unsigned char*) buffer);

	// create thread
	mutex = new QMutex;
    Q_CHECK_PTR(mutex);
    cond = new QWaitCondition;
    Q_CHECK_PTR(cond);
	thread = new AlgorithmThread(this);
    Q_CHECK_PTR(thread);
	thread->start();
}

AlgorithmSource::~AlgorithmSource() {

	thread->end = true;
	mutex->lock();
	cond->wakeAll();
	mutex->unlock();
    thread->wait(500);
	delete thread;
	delete cond;
	delete mutex;

	delete [] buffer;

	// free the OpenGL texture
	glDeleteTextures(1, &textureIndex);
}


void AlgorithmSource::play(bool on){

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


bool AlgorithmSource::isRunning(){

	return !thread->end;

}

void AlgorithmSource::initBuffer(){

	if (algotype == AlgorithmSource::FLAT) {
		// CLEAR the buffer to white
		for (int i = 0; i < (width * height * 4); ++i)
			buffer[i] = std::numeric_limits<unsigned char>::max();
	}
}


void AlgorithmSource::update(){

	Source::update();

	if( frameChanged )
	{
		mutex->lock();
		frameChanged = false;
		glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, width, height, GL_BGRA, GL_UNSIGNED_BYTE, (unsigned char*) buffer);
		cond->wakeAll();
		mutex->unlock();
	}

}


QString AlgorithmSource::getAlgorithmDescription(algorithmType t) {

	QString description;
	switch (t) {
	case FLAT:
		description = QString("Flat color");
		break;
	case BW_NOISE:
		description = QString("Black and white noise");
		break;
	case COLOR_NOISE:
		description = QString("Color noise");
		break;
	case PERLIN_BW_NOISE:
		description = QString("Black and white Perlin noise");
		break;
	case PERLIN_COLOR_NOISE:
		description = QString("Color Perlin noise");
		break;
	case WATER:
		description = QString("Water effect");
		break;
	}

	return description;
}

