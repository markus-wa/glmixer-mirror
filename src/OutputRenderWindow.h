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

	Q_OBJECT

public:
	OutputRenderWidget(QWidget *parent = 0, const QGLWidget * shareWidget = 0, Qt::WindowFlags f = 0);

    virtual void initializeGL();
    virtual void paintGL();
    virtual void resizeGL(int w, int h);

	float getAspectRatio() const;
	inline bool freeAspectRatio() const { return !useAspectRatio; }

public Q_SLOTS:
	void useFreeAspectRatio(bool on);

protected:
	bool useAspectRatio, useWindowAspectRatio;
	int rx, ry, rw, rh;
};

class OutputRenderWindow : public OutputRenderWidget {

	Q_OBJECT

public:
	// get singleton instance
	static OutputRenderWindow *getInstance();

    void initializeGL();
    void resizeGL(int w = 0, int h = 0);

	// events handling
	void keyPressEvent(QKeyEvent * event);
	void mouseDoubleClickEvent(QMouseEvent * event);
//	void closeEvent(QCloseEvent * event);

public Q_SLOTS:
	void setFullScreen(bool on);

Q_SIGNALS:
//	void windowClosed();
	void resized(bool);

	/**
	 * singleton mechanism
	 */
private:
	OutputRenderWindow();
	static OutputRenderWindow *_instance;

};

#endif /* OUTPUTRENDERWINDOW_H_ */
