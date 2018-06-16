/*
 * glRenderWidget.h
 *
 *  Created on: 3 nov. 2009
 *      Author: herbelin
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
 *   Copyright 2009, 2012 Bruno Herbelin
 *
 */

#ifndef GLRENDERWIDGET_H_
#define GLRENDERWIDGET_H_

#include "common.h"

class glRenderTimer: public QObject {

    Q_OBJECT

    glRenderTimer();
    static glRenderTimer *_instance;

public:
    static glRenderTimer *getInstance();

    void setInterval(int ms);
    inline const int interval() { return _interval; }

    void setActiveTimingMode(bool on);
    inline const bool isActiveTimingMode() { return _activeTiming; }
    void beginActiveTiming();
    void endActiveTiming();

signals:
    void timeout();

private:
    void timerEvent(QTimerEvent * event);
    void restartTimer(bool active);
    int _interval, _updater;
    bool _activeTiming;
    class QElapsedTimer *_elapsedTimer;
    class QTimer *_timer;
};

class glRenderWidget  : public QGLWidget
{
    Q_OBJECT

public:
    glRenderWidget(QWidget *parent = 0, const QGLWidget * shareWidget = 0, Qt::WindowFlags f = 0);

    // QGLWidget implementation
    virtual void initializeGL();
    virtual void resizeGL(int w, int h);
    virtual void paintGL();

    // cosmetics
    void setBackgroundColor(const QColor &c);
    inline bool antiAliasing() { return antialiasing; }
    void setAntiAliasing(bool on);

    // OpenGL informations
    static void showGlExtensionsInformationDialog(QString iconfile = "");

protected:
    bool needUpdate();
    float aspectRatio;
    bool antialiasing;
};

#endif /* GLRENDERWIDGET_H_ */
