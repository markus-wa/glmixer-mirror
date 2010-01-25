/*
 * OpencvSource.h
 *
 *  Created on: Dec 13, 2009
 *      Author: bh
 */

#ifndef OPENCVSOURCE_H_
#define OPENCVSOURCE_H_


#ifdef OPEN_CV

#include <Source.h>

#include <cv.h>
#include <highgui.h>


class OpencvSource: public Source {

    friend class MainRenderWidget;

protected:
    // only MainRenderWidget can create a source (need its GL context)
	OpencvSource(int opencvIndex, QGLWidget *context);
	virtual ~OpencvSource();

	void update();

private:
	CvCapture* capture;
};

#endif

#endif /* OPENCVSOURCE_H_ */
