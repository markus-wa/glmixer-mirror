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
		if (!cvs->frameChanged) {
		    Q_CHECK_PTR(cvs->capture);
			if (cvGrabFrame( cvs->capture )){
				cvs->frame = cvRetrieveFrame( cvs->capture );
				cvs->frameChanged = true;
			}
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
	if (!thread->end) {
		thread->end = true;
		mutex->lock();
		cond->wakeAll();
		frameChanged = false;
		mutex->unlock();
		thread->wait(500);
	}

	if (capture) {
		cvReleaseCapture(&capture);
		capture = NULL;
	}

	if (camindex >= 0){
		capture  = cvCreateCameraCapture(camindex);
	    Q_CHECK_PTR(capture);
		frameChanged = false;
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
	if (capture){
		// TODO ; NOT releasing capture for the case when a source with the same capture is already running ;
		// releasing here will also release the source's capture and stop it (we don't want that).
		// As not releasing a capture does not seem to hurt, i just don't do it. Its bad, i know..
//		cvReleaseCapture(&capture);
	}
    if (squareDisplayList){
        makeCurrent();
        glDeleteLists(squareDisplayList, 1);
    	glDeleteTextures(1, &textureIndex);
    }
}


void OpencvDisplayWidget::initializeGL()
{
	glRenderWidget::initializeGL();

    glEnable(GL_TEXTURE_2D);
    glGenTextures(1, &textureIndex);

    glBindTexture(GL_TEXTURE_2D, textureIndex);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

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

    glDisable(GL_BLEND);
	setBackgroundColor(palette().color(QPalette::Base));
}

void OpencvDisplayWidget::paintGL()
{
	if( frameChanged )
	{
	    glRenderWidget::paintGL();

		mutex->lock();
		frameChanged = false;
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, frame->width, frame->height,0, GL_BGR, GL_UNSIGNED_BYTE, (unsigned char*) frame->imageData);
		cond->wakeAll();
		mutex->unlock();

		float renderingAspectRatio = float(frame->width) / float(frame->height);
		if (aspectRatio < renderingAspectRatio)
			glScalef(1.f, aspectRatio / renderingAspectRatio, 1.f);
		else
			glScalef(renderingAspectRatio / aspectRatio, 1.f, 1.f);

		glCallList(squareDisplayList);
	}
}

