/*
 * OutputRenderWindow.h
 *
 *  Created on: Feb 10, 2010
 *      Author: bh
 */

#ifndef OUTPUTRENDERWINDOW_H_
#define OUTPUTRENDERWINDOW_H_

#include "glRenderWidget.h"

class OutputRenderWindow : public glRenderWidget {

	Q_OBJECT

public:
	// get singleton instance
	static OutputRenderWindow *getInstance();

	// QGLWidget rendering
	void paintGL();
	void initializeGL();
	void resizeGL(int w, int h);

	// events handling
	virtual void keyPressEvent(QKeyEvent * event);
	virtual void mouseDoubleClickEvent(QMouseEvent * event);
	virtual void closeEvent(QCloseEvent * event);

	float getAspectRatio();

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
	virtual ~OutputRenderWindow();
	static OutputRenderWindow *_instance;

	float _aspectRatio;
	bool _useAspectRatio;
	GLuint quad_texured;
	

};

#endif /* OUTPUTRENDERWINDOW_H_ */
