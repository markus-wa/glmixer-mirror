/*
 * OpencvSource.h
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

#ifndef OPENCVSOURCE_H_
#define OPENCVSOURCE_H_


#ifdef OPEN_CV

#include "common.h"
#include "Source.h"

#include <cv.h>
#include <highgui.h>
#include <stdexcept>

#include <QMutex>
#include <QWaitCondition>

class CameraThread;

struct NoCameraIndexException : public std::runtime_error
{
	NoCameraIndexException() : std::runtime_error("OpenCV camera index unavailable.") {}
};

class OpencvSource: public QObject, public Source {

    Q_OBJECT

    friend class CameraDialog;
    friend class RenderingManager;
    friend class CameraThread;

public:

	static RTTI type;
	RTTI rtti() const { return type; }

    inline int getOpencvCameraIndex() const { return opencvCameraIndex; }
	inline double getFrameRate() const { return framerate; }
	int getFrameWidth() const { return width; }
	int getFrameHeight() const { return height; }
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
