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

#define BASKET_DEFAULT_FRAME_WIDTH 1024
#define BASKET_DEFAULT_FRAME_HEIGHT 768
#define BASKET_DEFAULT_PERIOD 25
#define BASKET_DEFAULT_BIDIRECTION false
#define BASKET_DEFAULT_SHUFFLE false


#include "Source.h"
#include "RenderingManager.h"
#include "ImageAtlas.h"

class FboRenderingException : public SourceConstructorException {
public:
    virtual QString message() { return "Error binding Frame Buffer Object."; }
    void raise() const { throw *this; }
    Exception *clone() const { return new FboRenderingException(*this); }
};


class BasketSource : public Source
{
    Q_OBJECT

    friend class RenderingManager;
    friend class OutputRenderWidget;
    friend class BasketSelectionDialog;

public:

    RTTI rtti() const { return BasketSource::type; }
    bool isPlayable() const { return BasketSource::playable; }
    bool isPlaying() const;

    GLuint getTextureIndex() const ;
    int getFrameWidth() const ;
    int getFrameHeight() const ;
    double getFrameRate() const ;
    void update();

    bool isBidirectional() const;
    bool isShuffle() const;
    qint64 getPeriod() const;
    QStringList getImageFileList() const;
    QList<int> getPlaylist() const;
    QString getPlaylistString() const;

    QDomElement getConfiguration(QDomDocument &doc, QDir current);

public slots:

    void play(bool on);

    void setBidirectional(bool on);
    void setShuffle(bool on);
    void setPeriod(qint64 p);
    void appendImages(QStringList files);
    void setPlaylist(QList<int> playlist);
    void setPlaylistString(QString playlist);

protected:

    BasketSource(QStringList files, double d,
                 int w = BASKET_DEFAULT_FRAME_WIDTH,
                 int h = BASKET_DEFAULT_FRAME_HEIGHT,
                 qint64 p = BASKET_DEFAULT_PERIOD);
    ~BasketSource();

    static RTTI type;
    static bool playable;

private:

    void generateExecutionPlaylist();

    // source info
    qint64 period;
    bool bidirectional;
    bool shuffle;

    // timer
    QElapsedTimer _timer;
    bool _pause;

    // Frame buffer objets for rendering
    class QGLFramebufferObject *_renderFBO;

    // atlas of images
    ImageAtlas _atlas;

    // playing order
    QList<int> _playlist;
    QList<int> _executionList;

};

#endif // BASKETSOURCE_H
