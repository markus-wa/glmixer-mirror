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
    void displayFPS(Qt::GlobalColor);
	float getFPS() { return f_p_s_; }
    void setBackgroundColor(const QColor &c);

    // OpenGL informations
	static void showFramerate(bool on) { showFps_ = on; }
    static void showGlExtensionsInformationDialog(QString iconfile = "");

public slots:
    inline void setUpdatePeriod(int miliseconds) {
    	period = miliseconds;
		if (timer > 0)  { killTimer(timer); timer = startTimer(period); }
    }

protected:

	float aspectRatio;
    int timer, period;

	// F P S    d i s p l a y
	QTime fpsTime_;
	unsigned int fpsCounter_;
	QString fpsString_;
	float f_p_s_;
	static bool showFps_;
};

#endif /* GLRENDERWIDGET_H_ */
