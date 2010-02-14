/*
 * OutputRenderWindow.h
 *
 *  Created on: Feb 10, 2010
 *      Author: bh
 */

#ifndef OUTPUTRENDERWINDOW_H_
#define OUTPUTRENDERWINDOW_H_

#include "glRenderWidget.h"

class OutputRenderWidget: public glRenderWidget {

public:
	OutputRenderWidget(QWidget *parent = 0, const QGLWidget * shareWidget = 0, Qt::WindowFlags f = 0);

    virtual void initializeGL();
    virtual void paintGL();

	float getAspectRatio();

protected:
	bool useAspectRatio;
};

class OutputRenderWindow : public OutputRenderWidget {

	Q_OBJECT

public:
	// get singleton instance
	static OutputRenderWindow *getInstance();

    virtual void initializeGL();

	// events handling
	virtual void keyPressEvent(QKeyEvent * event);
	virtual void mouseDoubleClickEvent(QMouseEvent * event);
	virtual void closeEvent(QCloseEvent * event);

public slots:
	void useRenderingAspectRatio(bool on);
	void setFullScreen(bool on);

signals:
	void windowClosed();

	/**
	 * singleton mechanism
	 */
private:
	OutputRenderWindow();
	static OutputRenderWindow *_instance;

};

#endif /* OUTPUTRENDERWINDOW_H_ */
