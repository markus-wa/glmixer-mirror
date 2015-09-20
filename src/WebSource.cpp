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

#include "QNetworkCookieJar"

#include "RenderingManager.h"
#include "WebSource.moc"
#ifdef FFGL
#include "FFGLPluginSource.h"
#endif

Source::RTTI WebSource::type = Source::WEB_SOURCE;
bool WebSource::playable = true;


WebSource::WebSource(QUrl web, GLuint texture, double d, int height, int scroll, int update): QObject(), Source(texture, d), _playing(true), _updateFrequency(update)
{
    _webrenderer = new WebRenderer(web, height, scroll);
    _webrenderer->setUpdate(_updateFrequency);

    aspectratio = 1.0;

    glBindTexture(GL_TEXTURE_2D, textureIndex);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

}

bool WebSource::isPlaying() const
{
    return _playing;
}

void WebSource::play(bool on)
{
    Source::play(on);

    if ( isPlaying() == on )
        return;

    _playing = on;

    if (_playing)
        _webrenderer->setUpdate(_updateFrequency);
    else
        _webrenderer->setUpdate(0);

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

void WebSource::setPageUpdate(int u)
{
    _webrenderer->setUpdate(u);
    _updateFrequency = u;
}

void WebSource::update()
{
    // bind texture
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, textureIndex);

    // readjust the properties and plugins if required
    if ( _webrenderer->propertyChanged() )
        adjust();

    // update texture if image changed (might be because property change also affected image)
    if ( _webrenderer->imageChanged() )
    {
        QImage i = _webrenderer->image();

#if QT_VERSION >= 0x040700
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA,  i.width(), i.height(), 0, GL_BGRA, GL_UNSIGNED_INT_8_8_8_8_REV, i.constBits() );
#else
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, i.width(), i.height(), 0, GL_BGRA, GL_UNSIGNED_INT_8_8_8_8_REV, i.bits() );
#endif
    }

    // perform source update
    Source::update();
}

WebSource::~WebSource()
{
    delete _webrenderer;

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

int WebSource::getPageUpdate() const
{
    return _updateFrequency;
}

void WebRenderer::setHeight(int h)
{
    _height = qBound(10, h, 100);
    _propertyChanged = true;
}

void WebRenderer::setScroll(int s)
{
    _scroll = qBound(0, s, 90);
    _propertyChanged = true;
}


QImage WebRenderer::image() const {

    return _image;
}

bool WebRenderer::propertyChanged() {

    if (_propertyChanged) {

        update();

        _imageChanged = true;
        _propertyChanged = false;
        return true;
    }

    return false;
}

bool WebRenderer::imageChanged() {

    if (_imageChanged) {

        update();

        _imageChanged = false;
        return true;
    }
    return false;
}

void WebRenderer::timerEvent(QTimerEvent *event)
{
    // set image as changed every timer event (if initialized)
    if (!_render.isNull())
        _imageChanged = true;
}

void WebRenderer::setUpdate(int u)
{
    // kill previous update if exist
    if ( _updateTimerId > 0) {
        killTimer( _updateTimerId );
        _updateTimerId = -1;
    }

    // start updating if frequency above 0
    if ( u > 0 ) {
        _updateTimerId = startTimer( int ( 1000.0 / double(u) )  );
    }
}


WebRenderer::WebRenderer(const QUrl &url, int height, int scroll) : _url(url), _propertyChanged(true)
{
    // init
    setHeight(height);
    setScroll(scroll);
    _updateTimerId = -1;

    // display loading screen
    _image = QImage(QString(":/glmixer/textures/loading.png"));

    // enable cookies
    _page.networkAccessManager()->setCookieJar( new QNetworkCookieJar(&_page) );

    // render page when loaded
    _page.mainFrame()->load(_url);
    connect(&_page, SIGNAL(loadFinished(bool)), this, SLOT(render(bool)));

    // time out 10 seconds
    _timer.setSingleShot(true);
    connect(&_timer, SIGNAL(timeout()), this, SLOT(timeout()));
    _timer.start(10000);

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

    // could load
    if (ok) {
        // setup viewport and render buffer
        _page.setViewportSize(_page.mainFrame()->contentsSize());
        _render = QImage(_page.viewportSize(), QImage::Format_ARGB32_Premultiplied);
        _propertyChanged = true;
        _imageChanged = true;
    }
    else
        timeout();
}

void WebRenderer::update()
{
    // cancel update if render buffer not initialized
    if (_render.isNull())
        return;

    // render the page into the render buffer
    QPainter pagePainter(&_render);
    pagePainter.setRenderHint(QPainter::TextAntialiasing, true);
    _page.mainFrame()->render(&pagePainter);
    pagePainter.end();

    // fill image with section of render
    _image = _render.copy(0, _render.height() * _scroll / 100, _render.width(), _render.height() * _height / 100);

}

void WebRenderer::timeout()
{
    // cancel loading
    disconnect(&_page, SIGNAL(loadFinished(bool)), this, SLOT(render(bool)));
    // reset rendering
    _render = QImage();
    // display timeout
    _image = QImage(QString(":/glmixer/textures/timeout.png"));
    // inform of need to change
    _imageChanged = true;
    _propertyChanged = true;

}
