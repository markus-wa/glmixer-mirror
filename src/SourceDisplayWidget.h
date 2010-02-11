/*
 * SourceDisplayWidget.h
 *
 *  Created on: Jan 31, 2010
 *      Author: bh
 */

#ifndef SOURCEDISPLAYWIDGET_H_
#define SOURCEDISPLAYWIDGET_H_

#include "glRenderWidget.h"
#include "Source.h"

class SourceDisplayWidget: public glRenderWidget {

public:
	SourceDisplayWidget(QWidget *parent = 0);

    virtual void initializeGL();
    virtual void paintGL();
	inline void setSource(Source *sourceptr) { s = sourceptr;}

private:
    Source *s;
};

#endif /* SOURCEDISPLAYWIDGET_H_ */
