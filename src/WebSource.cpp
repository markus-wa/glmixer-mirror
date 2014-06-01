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
#ifdef FFGL
#include "FFGLPluginSource.h"
#endif

Source::RTTI WebSource::type = Source::WEB_SOURCE;
bool WebSource::playable = true;


WebSource::WebSource(QUrl web, GLuint texture, double d, int height, int scroll): QObject(), Source(texture, d), _playing(true)
{

    _webrenderer = new WebRenderer(web, height, scroll);
    connect(_webrenderer, SIGNAL(changed()), this, SLOT(adjust()));

    aspectratio = 1.0;

    glBindTexture(GL_TEXTURE_2D, textureIndex);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

//#if QT_VERSION >= 0x040700
//    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA,  loadingPixmap.width(), loadingPixmap.height(),
//                  0, GL_BGRA, GL_UNSIGNED_INT_8_8_8_8_REV, loadingPixmap.constBits() );
//#else
//    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA,  loadingPixmap.width(), loadingPixmap. height(),
//                  0, GL_BGRA, GL_UNSIGNED_INT_8_8_8_8_REV, loadingPixmap.bits() );
//#endif

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

void WebSource::adjust()
{
    // rescale to match updated dimensions
    aspectratio = double(getFrameWidth()) / double(getFrameHeight());
    scaley = scalex / aspectratio;

// freeframe gl plugin
#ifdef FFGL
    // resize all plugins
    for (FFGLPluginSourceStack::iterator it=_ffgl_plugins.begin(); it != _ffgl_plugins.end(); ++it){
        (*it)->resize(getFrameWidth(), getFrameHeight());
    }
#endif
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

    if ( _webrenderer->imageUpdate() ) {

        QImage i = _webrenderer->image();

    #if QT_VERSION >= 0x040700
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA,  i.width(), i.height(), 0, GL_BGRA, GL_UNSIGNED_INT_8_8_8_8_REV, i.constBits() );
    #else
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, i.width(), i.height(), 0, GL_BGRA, GL_UNSIGNED_INT_8_8_8_8_REV, i.bits() );
    #endif

        aspectratio = double(getFrameWidth()) / double(getFrameHeight());

    }

    Source::update();
}

WebSource::~WebSource()
{
    delete _webrenderer;

    // free the OpenGL texture
    glDeleteTextures(1, &textureIndex);

}


int WebSource::getFrameWidth() const
{
    return _webrenderer->image().width();
}

int WebSource::getFrameHeight() const
{
    return _webrenderer->image().height();
}

QUrl WebSource::getUrl() const
{
    return _webrenderer->url();

}

int WebSource::getPageHeight() const
{
        return _webrenderer->height();

}

int WebSource::getPageScroll() const
{
        return _webrenderer->scroll();

}

void WebRenderer::setHeight(int h)
{
    _height = h;
    _changed = true;
}

void WebRenderer::setScroll(int s)
{
    _scroll = s;
//    _scroll = qBound(0, s, 100 - _height);
    _changed = true;
}

QImage WebRenderer::image() const {

    return _image;
}

bool WebRenderer::imageUpdate() {

    if (_changed) {

        if (!_render.isNull())
            _image = _render.copy(0, _render.height() * _scroll / 100, _render.width(), _render.height() * _height / 100);

        _changed = false;

        emit changed();
        return true;
    }

    return false;
}

WebRenderer::WebRenderer(const QUrl &url, int height, int scroll) : _url(url), _height(height), _scroll(scroll), _changed(true)
{
     // display loading screen
     _image = QImage(QString(":/glmixer/textures/loading.png"));

     // render page when loaded
     _page.mainFrame()->load(_url);
     connect(&_page, SIGNAL(loadFinished(bool)), this, SLOT(render(bool)));

     // time out 10 seconds
     _timer.setSingleShot(true);
     connect(&_timer, SIGNAL(timeout()), this, SLOT(timeout()));
     _timer.start(10000);

     qDebug() << "WebRenderer" << _height << _scroll;
}

WebRenderer::~WebRenderer()
{
    disconnect(&_page, SIGNAL(loadFinished(bool)), this, SLOT(render(bool)));
    disconnect(&_timer, SIGNAL(timeout()), this, SLOT(timeout()));
    _timer.stop();
}

void WebRenderer::render(bool ok)
{
    // cancel time out
    disconnect(&_timer, SIGNAL(timeout()), this, SLOT(timeout()));
    _timer.stop();

    if (ok) {
        _page.setViewportSize(_page.mainFrame()->contentsSize());
        _render = QImage(_page.viewportSize(), QImage::Format_ARGB32_Premultiplied);
        QPainter pagePainter(&_render);
        pagePainter.setRenderHint(QPainter::HighQualityAntialiasing, true);
        pagePainter.setRenderHint(QPainter::SmoothPixmapTransform, true);
        pagePainter.setRenderHint(QPainter::TextAntialiasing, true);
        _page.mainFrame()->render(&pagePainter);
        pagePainter.end();
        _changed = true;
    }
    else
        timeout();
}

void WebRenderer::timeout()
{
    // cancel loading
    disconnect(&_page, SIGNAL(loadFinished(bool)), this, SLOT(render(bool)));
    _image = QImage(QString(":/glmixer/textures/timeout.png"));
    _changed = true;
}
