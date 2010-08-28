/*
 * OutputRenderWindow.h
 *
 *  Created on: Feb 10, 2010
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

#ifndef OUTPUTRENDERWINDOW_H_
#define OUTPUTRENDERWINDOW_H_

#include "glRenderWidget.h"

class OutputRenderWidget: public glRenderWidget {

	Q_OBJECT

public:
	OutputRenderWidget(QWidget *parent = 0, const QGLWidget * shareWidget = 0, Qt::WindowFlags f = 0);

    virtual void initializeGL();
    virtual void paintGL();
    virtual void resizeGL(int w = 0, int h = 0);

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
