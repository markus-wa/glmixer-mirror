/*
 * BasketSource.h
 *
 *  Created on: Aug. 2 2017
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
 *   Copyright 2009, 2017 Bruno Herbelin
 *
 */

#ifndef BASKETSOURCE_H
#define BASKETSOURCE_H

#include "Source.h"
#include "RenderingManager.h"

class FboRenderingException : public SourceConstructorException {
public:
    virtual QString message() { return "Error binding Frame Buffer Object."; }
    void raise() const { throw *this; }
    Exception *clone() const { return new FboRenderingException(*this); }
};

class BasketImage {
public:
    BasketImage(QString f);

    QString fileName() const { return _fileName; }
    QRect coordinates() const { return _coordinates; }
    void setCoordinates(QRect r) { _coordinates = r; }

private:
    QString _fileName;
    QRect _coordinates;
    bool _filled;
};

class BasketSource : public Source
{
    Q_OBJECT

    friend class RenderingManager;
    friend class OutputRenderWidget;

public:

    RTTI rtti() const { return BasketSource::type; }
    bool isPlayable() const { return BasketSource::playable; }
    bool isPlaying() const;

    GLuint getTextureIndex() const ;
    int getFrameWidth() const ;
    int getFrameHeight() const ;
    double getFrameRate() const ;
    void update();

    QDomElement getConfiguration(QDomDocument &doc, QDir current);

public slots:
    void play(bool on);

protected:

    BasketSource(QStringList files, double d, int w = 1024, int h = 768,  qint64 p = 25);
    ~BasketSource();

    void appendImages(QStringList files);

    static RTTI type;
    static bool playable;

private:

    QSize allocateAtlas(int n);

    // source info
    int width, height;
    qint64 period;

    // timer
    QElapsedTimer _timer;
//    qint64 _elapsedtime;
    bool _pause;

    // Frame buffer objetsB
    class QGLFramebufferObject *_renderFBO;
    class QGLFramebufferObject *_atlasFBO;
    bool _atlasInitialized;
    QList<BasketImage> _atlasImages;

    // playing order
    QList<int> _playlist;
    int _indexPlaylist;

};

#endif // BASKETSOURCE_H
