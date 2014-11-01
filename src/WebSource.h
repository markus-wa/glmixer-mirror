/*
 * WebSource.h
 *
 *  Created on: May 26, 2014
 *      Author: Bruno Herbelin
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
 *   Copyright 2009, 2014 Bruno Herbelin
 *
 */

#ifndef WEBSOURCE_H
#define WEBSOURCE_H

#include "Source.h"
#include <QWebPage>
#include <QWebFrame>

class WebRenderer : public QObject
{
    Q_OBJECT

public:
    WebRenderer(const QUrl &url, int height, int scroll);
    ~WebRenderer();

    QImage image() const;
    bool imageChanged();
    bool propertyChanged();

    QUrl url() const { return _url; }
    int height() const { return _height; }
    int scroll() const { return _scroll; }

    void setHeight(int);
    void setScroll(int);
    void setUpdate(int);

private slots:
    void render(bool);
    void timeout();
    void update();

protected:
     void timerEvent(QTimerEvent *event);

private:
    QUrl _url;
    QWebPage _page;
    QImage _render, _image;
    int _height, _scroll;
    QTimer _timer;
    int _updateTimerId;
    bool _propertyChanged;
    bool _imageChanged;
};

class WebSource : public QObject, public Source
{
    Q_OBJECT

    friend class RenderingManager;
    friend class OutputRenderWidget;
    friend class WebSourceCreationDialog;

public:

    QUrl getUrl() const;
    int getPageHeight() const;
    int getPageScroll() const;
    int getPageUpdate() const;

    RTTI rtti() const { return WebSource::type; }
    bool isPlayable() const { return WebSource::playable; }
    bool isPlaying() const;

    int getFrameWidth() const;
    int getFrameHeight() const;
    inline double getFrameRate() const { return double(_updateFrequency); }

public Q_SLOTS:

    void play(bool on);
    void setPageHeight(int);
    void setPageScroll(int);
    void setPageUpdate(int);
    void adjust();

protected:

    // only friends can create a source
    WebSource(const QUrl url, GLuint texture, double d, int height = 100, int scroll = 0, int update = 0);
    virtual ~WebSource();
    void update();

private:

    static RTTI type;
    static bool playable;

    WebRenderer *_webrenderer;
    bool _playing;
    int _updateFrequency;
};

#endif // WEBSOURCE_H