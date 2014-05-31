/*
 * WebSource.cpp
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

#include "RenderingManager.h"
#include "WebSource.moc"

Source::RTTI WebSource::type = Source::WEB_SOURCE;
bool WebSource::playable = true;


WebSource::WebSource(QUrl web, GLuint texture, double d, int height, int scroll): QObject(), Source(texture, d),  _initialized(false)
{

    _webrenderer = new WebRenderer(web, height, scroll);


    // get aspect ratio from orifinal box
//    QRect vb = _webpage->mainFrame()->geometry();
//    aspectratio = double(vb.width()) / double(vb.height());
//    _webpage->setViewportSize(_webpage->mainFrame()->contentsSize());

//    aspectratio = double(_webpage->viewportSize().width()) / double(_webpage->viewportSize().height()) ;

    // setup renderer resolution to match to rendering manager preference
//    int w = RenderingManager::getInstance()->getFrameBufferWidth();
//    _rendered = QImage(_webpage->viewportSize(), QImage::Format_ARGB32_Premultiplied);

    // render an image from the svg
//    QPainter imagePainter(&_rendered);
//    imagePainter.setRenderHint(QPainter::HighQualityAntialiasing, true);
//    imagePainter.setRenderHint(QPainter::SmoothPixmapTransform, true);
//    imagePainter.setRenderHint(QPainter::TextAntialiasing, true);
//    _webpage->mainFrame()->render(&imagePainter);
//    imagePainter.end();

//    if (_rendered.isNull())
//        SourceConstructorException().raise();


    QImage loadingPixmap(QString(":/glmixer/textures/loading.png"));
    aspectratio = 1.0;

    glBindTexture(GL_TEXTURE_2D, textureIndex);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

#if QT_VERSION >= 0x040700
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA,  loadingPixmap.width(), loadingPixmap.height(),
                  0, GL_BGRA, GL_UNSIGNED_INT_8_8_8_8_REV, loadingPixmap.constBits() );
#else
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA,  loadingPixmap.width(), loadingPixmap. height(),
                  0, GL_BGRA, GL_UNSIGNED_INT_8_8_8_8_REV, loadingPixmap.bits() );
#endif

}

bool WebSource::isPlaying() const
{
    return _playing;
}

void WebSource::play(bool on)
{
    _playing = on;

    Source::play(_playing);
}


void WebSource::setPageHeight(int h)
{
    _webrenderer->setHeight(h);
}

void WebSource::setPageScroll(int s)
{
    _webrenderer->setScroll(s);
}

void WebSource::update()
{
    // generate a texture from the rendered image
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, textureIndex);

    if ( _webrenderer->imageChanged() && _webrenderer->ready()) {

        QImage i = _webrenderer->image();

    #if QT_VERSION >= 0x040700
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA,  i.width(), i.height(), 0, GL_BGRA, GL_UNSIGNED_INT_8_8_8_8_REV, i.constBits() );
    #else
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, i.width(), i.height(), 0, GL_BGRA, GL_UNSIGNED_INT_8_8_8_8_REV, i.bits() );
    #endif

        aspectratio = double(getFrameWidth()) / double(getFrameHeight());

        _webrenderer->setChanged(false);
    }

    Source::update();
}

WebSource::~WebSource()
{
    // free the OpenGL texture
    glDeleteTextures(1, &textureIndex);

}


int WebSource::getFrameWidth() const
{
    if (_webrenderer->ready())
        return _webrenderer->image().width();
    else
        return 1;
}

int WebSource::getFrameHeight() const
{
    if (_webrenderer->ready())
        return _webrenderer->image().height();
    else
        return 1;
}

QUrl WebSource::getUrl() const
{
    if (_webrenderer)
        return _webrenderer->url();

    return QUrl();
}

QByteArray WebSource::getDescription(){

    QBuffer dev;



    return dev.buffer();
}


void WebRenderer::setHeight(int h)
{
    _height = h;
    setChanged(true);
}

void WebRenderer::setScroll(int s)
{
    _scroll = s;
    setChanged(true);
}

QImage WebRenderer::image() const {

    return _render.copy(0, _page.viewportSize().height() * _scroll / 100, _page.viewportSize().width(), _page.viewportSize().height() * _height / 100);

}

WebRenderer::WebRenderer(const QUrl &url, int height, int scroll) : _url(url), _height(height), _scroll(scroll), _changed(true)
{
     _render = QImage(QString(":/glmixer/textures/loading.png"));

     _page.mainFrame()->load(_url);
     connect(&_page, SIGNAL(loadFinished(bool)), this, SLOT(render(bool)));

     _timer.setSingleShot(true);
     connect(&_timer, SIGNAL(timeout()), this, SLOT(timeout()));
     _timer.start(7000);
}

void WebRenderer::render(bool ok)
{
    disconnect(&_timer, SIGNAL(timeout()), this, SLOT(timeout()));
    setChanged(true);

    if (ok) {
        _page.setViewportSize(_page.mainFrame()->contentsSize());
        _render = QImage(_page.viewportSize(), QImage::Format_ARGB32_Premultiplied);
        QPainter pagePainter(&_render);
        pagePainter.setRenderHint(QPainter::HighQualityAntialiasing, true);
        pagePainter.setRenderHint(QPainter::SmoothPixmapTransform, true);
        pagePainter.setRenderHint(QPainter::TextAntialiasing, true);
        _page.mainFrame()->render(&pagePainter);
        pagePainter.end();
    }
    else
        timeout();
}

void WebRenderer::timeout()
{
    disconnect(&_page, SIGNAL(loadFinished(bool)), this, SLOT(render(bool)));
    _render = QImage(QString(":/glmixer/textures/timeout.png"));

    setChanged(true);
}
