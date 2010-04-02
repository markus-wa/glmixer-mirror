/*
 * glRenderWidget.h
 *
 *  Created on: 3 nov. 2009
 *      Author: herbelin
 */

#ifndef GLRENDERWIDGET_H_
#define GLRENDERWIDGET_H_

#include "common.h"

class glRenderWidget  : public QGLWidget
{
    Q_OBJECT

public:
	glRenderWidget(QWidget *parent = 0, const QGLWidget * shareWidget = 0, Qt::WindowFlags f = 0);
	virtual ~glRenderWidget();

    // QGLWidget implementation
    virtual void initializeGL();
    virtual void resizeGL(int w, int h);
    virtual void paintGL();

    // Update events management
    virtual void timerEvent( QTimerEvent *) { update(); }
    virtual void showEvent ( QShowEvent * event );
    virtual void hideEvent ( QHideEvent * event );

    // cosmetics
    void setBackgroundColor(const QColor &c);

    // OpenGL informations
    static void showGlExtensionsInformationDialog(QString iconfile = "");

public slots:
    inline void setUpdatePeriod(int miliseconds) {
    	period = miliseconds;
		if (timer > 0)  { killTimer(timer); timer = startTimer(period); }
    }

protected:

	float aspectRatio;
    int timer, period;

};

#endif /* GLRENDERWIDGET_H_ */
