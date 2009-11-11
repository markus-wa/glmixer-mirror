/*
 * MixerViewWidget.h
 *
 *  Created on: Nov 9, 2009
 *      Author: bh
 */

#ifndef MIXERVIEWWIDGET_H_
#define MIXERVIEWWIDGET_H_

#include <glRenderWidget.h>

class MixerViewWidget: public glRenderWidget {

	Q_OBJECT

public:
	MixerViewWidget(QWidget * parent, const QGLWidget * shareWidget);
	virtual ~MixerViewWidget();


    virtual void paintGL();

    static GLuint squareDisplayList;
};

#endif /* MIXERVIEWWIDGET_H_ */
