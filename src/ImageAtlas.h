/*
 * ImageAtlas.h
 *
 *  Created on: Sept. 7 2017
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

#ifndef IMAGEATLAS_H
#define IMAGEATLAS_H

#include <QObject>
#include <QList>
#include <QRect>

class QGLFramebufferObject;

class ImageAtlasPage
{
public:
    ImageAtlasPage(QSize imagesize, int numimages);
    ~ImageAtlasPage();

    int count() const { return  _array.height() * _array.width(); }
    QSize array() const { return _array; }
    QGLFramebufferObject *fbo() const { return _fbo;}

    QRectF texturecoordinates(QRect rect) const;
    int texturesize() const;

private:
    QSize _array;
    QGLFramebufferObject *_fbo;
};

class ImageAtlasElement
{
public:
    ImageAtlasElement(QString filename);

    QString fileName() const { return _fileName; }

    QRect coordinates() const { return _coordinates; }
    void setCoordinates(QRect r) { _coordinates = r; }

    ImageAtlasPage *page() const { return _page; }
    void setPage(ImageAtlasPage * i) { _page = i; }

private:
    QString _fileName;
    QRect _coordinates;
    ImageAtlasPage *_page;
};


class ImageAtlas : public QObject
{
    Q_OBJECT

public:
    explicit ImageAtlas() : _initialized(false) {}
    ~ImageAtlas();

    // size of images bufer
    void setSize(int w, int h);
    QSize size() const { return _elementsSize; }

    // append several images
    bool appendImages(QStringList files);

    // access image #i
    ImageAtlasElement operator[] (int i) const { return _atlasElements[i]; }

    // number of images
    int count() const { return _atlasElements.count(); }

    // for GUI
    QStringList getImageList() const;
    int texturesize() const;

signals:

public slots:

private:

    // management of atlas elements
    QSize _elementsSize;
    QList<ImageAtlasElement> _atlasElements;

    // management of Frame buffer objects
    QList<ImageAtlasPage *> _atlasPages;

    bool _initialized;
};

#endif // IMAGEATLAS_H
