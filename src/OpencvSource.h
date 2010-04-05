/*
 * OpencvSource.h
 *
 *  Created on: Dec 13, 2009
 *      Author: bh
 */

#ifndef OPENCVSOURCE_H_
#define OPENCVSOURCE_H_


#ifdef OPEN_CV

#include "common.h"
#include "Source.h"

#include <cv.h>
#include <highgui.h>

#include <QMutex>
#include <QWaitCondition>

class CameraThread;

class OpencvSource: public QObject, public Source {

    Q_OBJECT

    friend class CameraDialog;
    friend class RenderingManager;
    friend class CameraThread;

public:

	static RTTI type;
	RTTI rtti() const { return type; }

    inline int getOpencvCameraIndex() const { return opencvCameraIndex; }
	inline int getFrameWidth() const { return width; }
	inline int getFrameHeight() const { return height; }
	inline double getFrameRate() const { return framerate; }
	bool isRunning();

public Q_SLOTS:
	void play(bool on);

protected:
    // only MainRenderWidget can create a source (need its GL context)
	OpencvSource(int opencvIndex, GLuint texture, double d);
	virtual ~OpencvSource();

	void update();

	int opencvCameraIndex;
	CvCapture* capture;
	int width, height;
	double framerate;
    IplImage *frame;

    CameraThread *thread;
    QMutex *mutex;
    QWaitCondition *cond;
};

#endif

#endif /* OPENCVSOURCE_H_ */
