/*
 * OpencvDisplayWidget.h
 *
 *  Created on: Dec 19, 2009
 *      Author: bh
 */

#ifndef OPENCVDISPLAYWIDGET_H_
#define OPENCVDISPLAYWIDGET_H_

#include <glRenderWidget.h>
#include <QMutex>
#include <QWaitCondition>

#ifdef OPEN_CV

#include <highgui.h>


class OpencvThread;

class OpencvDisplayWidget: public glRenderWidget {

    friend class OpencvThread;

public:
	OpencvDisplayWidget(QWidget *parent = 0);
	virtual ~OpencvDisplayWidget();

    // OpenGL implementation
    virtual void initializeGL();
    virtual void paintGL();

    void setCamera(int camindex);

protected:
	CvCapture* capture;
    GLuint squareDisplayList;
    GLuint textureIndex;

    bool frameChanged;
    IplImage *frame;

    OpencvThread *thread;
    QMutex *mutex;
    QWaitCondition *cond;
};


#endif

#endif /* OPENCVDISPLAYWIDGET_H_ */
