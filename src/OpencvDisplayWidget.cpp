/*
 * OpencvDisplayWidget.cpp
 *
 *  Created on: Dec 19, 2009
 *      Author: bh
 */

#include "common.h"
#include "OpencvDisplayWidget.h"

#include <QThread>

class OpencvThread: public QThread {
public:
	OpencvThread(OpencvDisplayWidget *source) :
        QThread(), cvs(source), end(true) {
    }
    ~OpencvThread() {
    }

    void run();

    OpencvDisplayWidget* cvs;
    bool end;

};

void OpencvThread::run(){

	while (!end) {
		cvs->mutex->lock();
	    Q_CHECK_PTR(cvs->capture);
		if (!cvs->frameChanged) {
			cvs->frame = cvQueryFrame( cvs->capture );
			cvs->frameChanged = true;
			cvs->cond->wait(cvs->mutex);
		}
		cvs->mutex->unlock();
	}
}

OpencvDisplayWidget::OpencvDisplayWidget(QWidget *parent)
		  : glRenderWidget(parent), capture(NULL), frameChanged(false)
{
	// create thread
	mutex = new QMutex;
    Q_CHECK_PTR(mutex);
    cond = new QWaitCondition;
    Q_CHECK_PTR(cond);
	thread = new OpencvThread(this);
    Q_CHECK_PTR(thread);

}

void OpencvDisplayWidget::setCamera(int camindex)
{
	if (camindex < 0) {  // stop
		thread->end = true;
		mutex->lock();
		cond->wakeAll();
	    frameChanged = false;
		mutex->unlock();
	    thread->wait(500);
		if (capture) {
			cvReleaseCapture(&capture);
			capture = NULL;
		}

	} else { // start

		if (capture)
			cvReleaseCapture(&capture);
		capture  = cvCreateCameraCapture(camindex);
	    Q_CHECK_PTR(capture);
		thread->end = false;
		thread->start();
	}
}

OpencvDisplayWidget::~OpencvDisplayWidget() {

	thread->end = true;
	mutex->lock();
	cond->wakeAll();
	mutex->unlock();
    thread->wait(500);
	delete thread;
	delete cond;
	delete mutex;

	// release former capture if we had one (changing camera index)
	if (capture)
		cvReleaseCapture(&capture);

    if (squareDisplayList){
        makeCurrent();
        glDeleteLists(squareDisplayList, 1);
    	glDeleteTextures(1, &textureIndex);
    }
}


void OpencvDisplayWidget::resizeGL(int w, int h)
{
    glViewport(0, 0, w, h);

    // Setup specific projection and view for this window
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluOrtho2D(-1, 1, -1, 1);

    glMatrixMode(GL_MODELVIEW);
}

void OpencvDisplayWidget::initializeGL()
{
	glRenderWidget::initializeGL();

    glEnable(GL_TEXTURE_2D);
    glGenTextures(1, &textureIndex);

    glBindTexture(GL_TEXTURE_2D, textureIndex);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

    squareDisplayList = glGenLists(1);
    glNewList(squareDisplayList, GL_COMPILE);
    {
        glBegin(GL_QUADS); // begin drawing a square

        // Front Face (note that the texture's corners have to match the quad's corners)
        glNormal3f(0.0f, 0.0f, 1.0f); // front face points out of the screen on z.
        glTexCoord2f(0.0f, 1.0f);
        glVertex3f(-1.0f, -1.0f, 0.0f); // Bottom Left
        glTexCoord2f(1.0f, 1.0f);
        glVertex3f(1.0f, -1.0f, 0.0f); // Bottom Right
        glTexCoord2f(1.0f, 0.0f);
        glVertex3f(1.0f, 1.0f, 0.0f); // Top Right
        glTexCoord2f(0.0f, 0.0f);
        glVertex3f(-1.0f, 1.0f, 0.0f); // Top Left

        glEnd();
    }
    glEndList();

}

void OpencvDisplayWidget::paintGL()
{
	glRenderWidget::paintGL();

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

		glCallList(squareDisplayList);
	}

}

