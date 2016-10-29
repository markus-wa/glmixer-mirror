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

#include <QNetworkCookieJar>

#include "RenderingManager.h"
#include "WebSource.moc"
#ifdef FFGL
#include "FFGLPluginSource.h"
#endif

Source::RTTI WebSource::type = Source::WEB_SOURCE;
bool WebSource::playable = true;


WebSource::WebSource(QUrl web, GLuint texture, double d, int w, int h, int height, int scroll, int update):  Source(texture, d), _playing(true), _updateFrequency(update)
{

   // Web browser settings
   QWebSettings::setIconDatabasePath("");
   QWebSettings::setOfflineStoragePath("");
   QWebSettings::setMaximumPagesInCache(0);

    _webrenderer = new WebRenderer(web, w, h, height, scroll);
    _webrenderer->setUpdate(_updateFrequency);

    glBindTexture(GL_TEXTURE_2D, textureIndex);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

}

bool WebSource::isPlaying() const
{
    return (_playing);
}

void WebSource::play(bool on)
{
    if (!_webrenderer || isPlaying() == on )
        return;

    _playing = on;

    if (_playing)
        _webrenderer->setUpdate(_updateFrequency);
    else
        _webrenderer->setUpdate(0);

    Source::play(on);
}

void WebSource::adjust()
{
    // rescale to match updated dimensions
    scaley = scalex / getAspectRatio();

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
    if (_webrenderer)
    _webrenderer->setHeight(h);
}

void WebSource::setPageScroll(int s)
{
    if (_webrenderer)
    _webrenderer->setScroll(s);
}

void WebSource::setPageUpdate(int u)
{
    _updateFrequency = u;
}

void WebSource::update()
{
    // bind texture
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, textureIndex);

    if (!_webrenderer)
        return;

    // readjust the properties and plugins if required
    if ( _webrenderer->propertyChanged() ) {

        adjust();

        QImage i = _webrenderer->image();

#if QT_VERSION >= 0x040700
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA,  i.width(), i.height(), 0, GL_BGRA, GL_UNSIGNED_INT_8_8_8_8_REV, i.constBits() );
#else
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, i.width(), i.height(), 0, GL_BGRA, GL_UNSIGNED_INT_8_8_8_8_REV, i.bits() );
#endif
    }
    // update texture if image changed (might be because property change also affected image)
    else if ( _webrenderer->imageChanged() )
    {
        QImage i = _webrenderer->image();

#if QT_VERSION >= 0x040700

        glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, i.width(), i.height(), GL_BGRA, GL_UNSIGNED_INT_8_8_8_8_REV, i.constBits() );
#else
        glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0,  i.width(), i.height(), GL_BGRA, GL_UNSIGNED_INT_8_8_8_8_REV, i.bits() );
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
    if (_webrenderer)
    return _webrenderer->image().width();
    else
    return 0;
}

int WebSource::getFrameHeight() const
{
    if (_webrenderer)
    return _webrenderer->image().height();
    else
    return 0;
}

QUrl WebSource::getUrl() const
{
    if (_webrenderer)
    return _webrenderer->url();
    else
    return QUrl();
}

int WebSource::getPageHeight() const
{
    if (_webrenderer)
    return _webrenderer->height();
    else
    return 0;
}

int WebSource::getPageScroll() const
{
    if (_webrenderer)
    return _webrenderer->scroll();
    else
    return 0;
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

//        _imageChanged = true;
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


WebRenderer::WebRenderer(const QUrl &url, int w, int h, int height, int scroll) : _url(url), _propertyChanged(true)
{

    // init
    setHeight(height);
    setScroll(scroll);
    _updateTimerId = -1;
    _pagesize = QSize();

    // display loading screen
    _image = QImage(QString(":/glmixer/textures/loading.png"));

    _page.settings()->setAttribute( QWebSettings::AutoLoadImages,  true);

    // enable cookies
    _page.networkAccessManager()->setCookieJar( new QNetworkCookieJar(&_page) );
    _page.setPreferredContentsSize(QSize(w,h));

    // render page when loaded
    qDebug() << _url << QChar(124).toLatin1() << tr("Loading web page...");
    _page.mainFrame()->load(_url);
    connect(&_page, SIGNAL(loadFinished(bool)), this, SLOT(render(bool)));

    // time out 10 seconds
    _timer.setSingleShot(true);
    connect(&_timer, SIGNAL(timeout()), this, SLOT(timeout()));
    _timer.start(10000);

}

WebRenderer::~WebRenderer()
{
    disconnect(&_page, 0, 0, 0);
    disconnect(&_timer, 0, 0, 0);
    _timer.stop();
}

void WebRenderer::render(bool ok)
{
    // cancel time out
    disconnect(&_timer, 0, 0, 0);
    _timer.stop();

    // could load
    if (ok) {
        // remember page size
        _pagesize = _page.mainFrame()->contentsSize();

        _propertyChanged = true;
        _imageChanged = true;
    }
    else
        timeout();
}

void WebRenderer::update()
{
    // cancel update if render buffer not initialized
    if ( !_pagesize.isValid() )
        return;

    // setup viewport
    _page.setViewportSize(QSize(_pagesize.width(), _pagesize.height() * _height / 100));
    QImage render = QImage(_page.viewportSize(), QImage::Format_ARGB32_Premultiplied);

    // setup scroll
    _page.mainFrame()->setScrollPosition(QPoint(0, _pagesize.height() * _scroll / 100));

    // render the page into the render buffer
    QPainter pagePainter(&render);
    pagePainter.setRenderHint(QPainter::TextAntialiasing, true);
    _page.mainFrame()->render(&pagePainter);
    pagePainter.end();

    // fill image with section of render
    _image = render;
}

void WebRenderer::timeout()
{
    // cancel loading
    disconnect(&_page, 0, 0, 0);
    // display timeout
    _image = QImage(QString(":/glmixer/textures/timeout.png"));
    // inform of need to change
    _imageChanged = true;
    _propertyChanged = true;
}

