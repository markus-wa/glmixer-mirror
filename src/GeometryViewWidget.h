/*
 * GeometryViewWidget.h
 *
 *  Created on: Jan 31, 2010
 *      Author: bh
 */

#ifndef GEOMETRYVIEWWIDGET_H_
#define GEOMETRYVIEWWIDGET_H_

#include "glRenderWidget.h"
#include "View.h"

class GeometryViewWidget: public glRenderWidget, public View {
public:
	GeometryViewWidget(QWidget * parent, const QGLWidget * shareWidget = 0);
	virtual ~GeometryViewWidget();
};

#endif /* GEOMETRYVIEWWIDGET_H_ */
