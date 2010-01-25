/*
 * OpencvDisplayWidget.cpp
 *
 *  Created on: Dec 19, 2009
 *      Author: bh
 */

#include <OpencvDisplayWidget.h>

OpencvDisplayWidget::OpencvDisplayWidget(QWidget *parent)
		  : glRenderWidget(parent), capture(0)
{
}

void OpencvDisplayWidget::setCamera(int camindex)
{
	// release former capture if we had one (changing camera index)
	if (capture)
		cvReleaseCapture(&capture);

	capture  = cvCreateCameraCapture(camindex);
	startTimer(20);
}

OpencvDisplayWidget::~OpencvDisplayWidget() {

	cvReleaseCapture(&capture);

    if (squareDisplayList){
        makeCurrent();
        glDeleteLists(squareDisplayList, 1);
    }
}


void OpencvDisplayWidget::timerEvent( QTimerEvent * event )
{
	updateGL();
}

void OpencvDisplayWidget::initializeGL()
{
	glRenderWidget::initializeGL();

    glClearColor(0.0, 0.0, 0.0, 1.0f);

    glEnable(GL_TEXTURE_2D);
    glGenTextures(1, &textureIndex);

    glBindTexture(GL_TEXTURE_2D, textureIndex);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
//    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
//    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);


    squareDisplayList = glGenLists(1);
    glNewList(squareDisplayList, GL_COMPILE);
    {
        qglColor(QColor::fromRgb(10, 10, 10));
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

	if (capture) {
		IplImage *frame = cvQueryFrame( capture );
		glBindTexture(GL_TEXTURE_2D, textureIndex);
		if (frame) {
//			qDebug("OpencvDisplayWidget::paintGL()");

			glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
			glTexImage2D(GL_TEXTURE_2D, 0, 4, frame->width, frame->height,0, GL_BGR, GL_UNSIGNED_BYTE, (unsigned char*) frame->imageData);
		}

	} else {
		// TODO : apply texture with message "no camera detected"
	}

	glCallList(squareDisplayList);
}

