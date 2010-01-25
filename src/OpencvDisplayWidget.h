/*
 * OpencvDisplayWidget.h
 *
 *  Created on: Dec 19, 2009
 *      Author: bh
 */

#ifndef OPENCVDISPLAYWIDGET_H_
#define OPENCVDISPLAYWIDGET_H_

#include <glRenderWidget.h>

#ifdef OPEN_CV

#include <highgui.h>

class OpencvDisplayWidget: public glRenderWidget {

public:
	OpencvDisplayWidget(QWidget *parent = 0);
	virtual ~OpencvDisplayWidget();

    // OpenGL implementation
    virtual void initializeGL();
    virtual void paintGL();

    void setCamera(int camindex);
    void timerEvent( QTimerEvent * event );

private:
	CvCapture* capture;
    GLuint squareDisplayList;
    GLuint textureIndex;
};


#endif

#endif /* OPENCVDISPLAYWIDGET_H_ */
