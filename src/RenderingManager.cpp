/*
 * RenderingManager.cpp
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

#include "RenderingManager.moc"

#include "common.h"

#include "AlgorithmSource.h"
#include "VideoFile.h"
#include "VideoSource.h"
#include "CaptureSource.h"
#include "SvgSource.h"
#include "WebSource.h"
#include "RenderingSource.h"
Source::RTTI RenderingSource::type = Source::RENDERING_SOURCE;
#include "CloneSource.h"
Source::RTTI CloneSource::type = Source::CLONE_SOURCE;

#ifdef SHM
#include <QSharedMemory>
#include "SharedMemoryManager.h"
#include "SharedMemorySource.h"
#endif

#ifdef SPOUT
#include <Spout.h>
#include <SpoutSource.h>
#endif

#ifdef OPEN_CV
#include "OpencvSource.h"
#endif

#ifdef FFGL
#include "FFGLPluginSource.h"
#include "FFGLPluginSourceShadertoy.h"
#include "FFGLSource.h"
#endif

#include "ViewRenderWidget.h"
#include "CatalogView.h"
#include "RenderingEncoder.h"
#include "SourcePropertyBrowser.h"
#include "SessionSwitcher.h"

#ifdef HISTORY_MANAGEMENT
#include "HistoryManager.h"
#endif

#include <map>
#include <algorithm>
#include <QGLFramebufferObject>
#include <QElapsedTimer>


// static members
RenderingManager *RenderingManager::_instance = 0;
bool RenderingManager::blit_fbo_extension = true;
bool RenderingManager::pbo_extension = true;

QSize RenderingManager::sizeOfFrameBuffer[ASPECT_RATIO_FREE][QUALITY_UNSUPPORTED] = { { QSize(640,480), QSize(768,576), QSize(800,600), QSize(1024,768), QSize(1600,1200), QSize(2048,1536) },
                                                                           { QSize(720,480), QSize(864,576), QSize(900,600), QSize(1152,768), QSize(1440,960), QSize(1920,1280) },
                                                                           { QSize(800,480), QSize(912,570), QSize(960,600), QSize(1280,800), QSize(1920,1200), QSize(2048,1280) },
                                                                           { QSize(848,480), QSize(1024,576), QSize(1088,612), QSize(1280,720), QSize(1920,1080), QSize(2048,1152) }};

ViewRenderWidget *RenderingManager::getRenderingWidget() {

    return getInstance()->_renderwidget;
}

SourcePropertyBrowser *RenderingManager::getPropertyBrowserWidget() {

    return getInstance()->_propertyBrowser;
}

RenderingEncoder *RenderingManager::getRecorder() {

    return getInstance()->_recorder;
}

SessionSwitcher *RenderingManager::getSessionSwitcher() {

    return getInstance()->_switcher;
}

void RenderingManager::setUseFboBlitExtension(bool on){

    if (glewIsSupported("GL_EXT_framebuffer_blit"))
        RenderingManager::blit_fbo_extension = on;
    else {
        // if extension not supported but it is requested, show warning
        if (on) {
            qCritical()  << tr("OpenGL Framebuffer Blit operation is requested but not supported (GL_EXT_framebuffer_blit).\n\nDisabling Framebuffer Blit.");
        }
        RenderingManager::blit_fbo_extension = false;
    }

    qDebug() << "RenderingManager" << QChar(124).toLatin1() << tr("OpenGL Framebuffer Blit  (GL_EXT_framebuffer_blit) ") << (RenderingManager::blit_fbo_extension ? "ON" : "OFF");
}


void RenderingManager::setUsePboExtension(bool on){

    if (glewIsSupported("GL_EXT_pixel_buffer_object") || glewIsSupported("GL_ARB_pixel_buffer_object") )
        RenderingManager::pbo_extension = on;
    else {
        // if extension not supported but it is requested, show warning
        if (on) {
            qCritical()  << tr("OpenGL Pixel Buffer Object is requested but not supported (GL_EXT_pixel_buffer_object).\n\nDisabling Pixel Buffer Object.");
        }
        RenderingManager::pbo_extension = false;
    }

    qDebug() << "RenderingManager" << QChar(124).toLatin1() << tr("OpenGL Pixel Buffer Object (GL_EXT_pixel_buffer_object) ") << (RenderingManager::pbo_extension ? "ON" : "OFF");
}

RenderingManager *RenderingManager::getInstance() {

    if (_instance == 0) {

        if (!QGLFramebufferObject::hasOpenGLFramebufferObjects())
            qFatal( "%s", qPrintable( tr("OpenGL Frame Buffer Objects are not supported (GL_EXT_framebuffer_object).") ));

        if (!glewIsSupported("GL_ARB_vertex_program") || !glewIsSupported("GL_ARB_fragment_program"))
            qFatal( "%s", qPrintable( tr("OpenGL GLSL programming is not supported (GL_ARB_vertex_program, GL_ARB_fragment_program).")));

        // ok to instanciate rendering manager
        _instance = new RenderingManager;
        Q_CHECK_PTR(_instance);
    }

    return _instance;
}

void RenderingManager::deleteInstance() {
    if (_instance != 0)
        delete _instance;
    _instance = 0;
}

RenderingManager::RenderingManager() :
    QObject(), _fbo(NULL), previousframe_fbo(NULL), pbo_index(0), pbo_nextIndex(0), previousframe_index(0), previousframe_delay(1), clearWhite(false), maxtexturewidth(TEXTURE_REQUIRED_MAXIMUM), maxtextureheight(TEXTURE_REQUIRED_MAXIMUM), renderingQuality(QUALITY_VGA), renderingAspectRatio(ASPECT_RATIO_4_3), _scalingMode(Source::SCALE_CROP), _playOnDrop(true), paused(false), maxSourceCount(0), countRenderingSource(0)
{
    // 1. Create the view rendering widget
    _renderwidget = new ViewRenderWidget;
    Q_CHECK_PTR(_renderwidget);

    _propertyBrowser = new SourcePropertyBrowser;
    Q_CHECK_PTR(_propertyBrowser);

    // no pixel buffer objects by default
    pboIds[0] = 0;
    pboIds[1] = 0;

    // create recorder and session switcher
    _recorder = new RenderingEncoder(this);
    _switcher = new SessionSwitcher(this);

#ifdef HISTORY_MANAGEMENT
    // create the history manager used for undo
    _undoHistory = new HistoryManager(this);
#endif

    // 2. Connect the above view holders to events
    QObject::connect(this, SIGNAL(currentSourceChanged(SourceSet::iterator)), _propertyBrowser, SLOT(showProperties(SourceSet::iterator) ) );

    QObject::connect(_renderwidget, SIGNAL(sourceMixingModified()), _propertyBrowser, SLOT(updateMixingProperties() ) );
    QObject::connect(_renderwidget, SIGNAL(sourceGeometryModified()), _propertyBrowser, SLOT(updateGeometryProperties() ) );
    QObject::connect(_renderwidget, SIGNAL(sourceLayerModified()), _propertyBrowser, SLOT(updateLayerProperties() ) );

    QObject::connect(_renderwidget, SIGNAL(sourceMixingDrop(double,double)), this, SLOT(dropSourceWithAlpha(double, double) ) );
    QObject::connect(_renderwidget, SIGNAL(sourceGeometryDrop(double,double)), this, SLOT(dropSourceWithCoordinates(double, double)) );
    QObject::connect(_renderwidget, SIGNAL(sourceLayerDrop(double)), this, SLOT(dropSourceWithDepth(double)) );

    QObject::connect(this, SIGNAL(frameBufferChanged()), _renderwidget, SLOT(refresh()));

    // 3. Setup the default default values ! :)
    _defaultSource = new Source();
    _currentSource = getEnd();

#ifdef SHM
    _sharedMemory = NULL;
    _sharedMemoryGLFormat = GL_RGB;
    _sharedMemoryGLType = GL_UNSIGNED_SHORT_5_6_5;
#endif
#ifdef SPOUT
    _spoutEnabled = false;
    _spoutInitialized = false;
#endif
}

RenderingManager::~RenderingManager() {
#ifdef SHM
    setFrameSharingEnabled(false);
#endif
    clearSourceSet();
    delete _defaultSource;

    if (_fbo)
        delete _fbo;

    if (previousframe_fbo)
        delete previousframe_fbo;

    if (pboIds[0] || pboIds[1])
        glDeleteBuffers(2, pboIds);

    if (_recorder)
        delete _recorder;

    if (_switcher)
        delete _switcher;

#ifdef HISTORY_MANAGEMENT
    if (_undoHistory)
        delete _undoHistory;
#endif

    qDebug() << "RenderingManager" << QChar(124).toLatin1() << "All clear.";
}

void RenderingManager::resetFrameBuffer()
{
    // delete fbo to force update function to re-initialize it
    if (_fbo)
        delete _fbo;

    _fbo = NULL;
}

void RenderingManager::setRenderingQuality(frameBufferQuality q)
{
    // by default, revert to lower resolution
    if ( q == QUALITY_UNSUPPORTED )
        q = QUALITY_VGA;

    // request update of frame buffer only if changed
    if (q != renderingQuality)
        resetFrameBuffer();

    // quality changed
    renderingQuality = q;
}

void RenderingManager::setRenderingAspectRatio(standardAspectRatio ar)
{
    // by default, free windows are rendered with a 4:3 aspect ratio frame bufer
    if (ar == ASPECT_RATIO_FREE)
        ar = ASPECT_RATIO_4_3;

    // request update of frame buffer only if changed
    if (ar != renderingAspectRatio)
        resetFrameBuffer();

    // aspect ratio changed
    renderingAspectRatio = ar;
}

void RenderingManager::setFrameBufferResolution(QSize size) {

    // Check limits based on openGL texture capabilities
    if (maxSourceCount == 0) {
        if (glewIsSupported("GL_ARB_internalformat_query2")) {
            glGetInternalformativ(GL_TEXTURE_2D, GL_RGBA8, GL_MAX_WIDTH, 1, &maxtexturewidth);
            glGetInternalformativ(GL_TEXTURE_2D, GL_RGBA8, GL_MAX_HEIGHT, 1, &maxtextureheight);
        } else {
            glGetIntegerv(GL_MAX_TEXTURE_SIZE, &maxtexturewidth);
            maxtextureheight = maxtexturewidth;
        }

        maxtexturewidth = qMin(maxtexturewidth, GL_MAX_FRAMEBUFFER_WIDTH);
        maxtextureheight = qMin(maxtextureheight, GL_MAX_FRAMEBUFFER_WIDTH);
        qDebug() << "RenderingManager" << QChar(124).toLatin1() << tr("OpenGL Maximum RGBA texture dimension: ") << maxtexturewidth << "x" << maxtextureheight;

        // setup the maximum texture count accordingly
        maxSourceCount = maxtexturewidth / CATALOG_TEXTURE_HEIGHT;
        qDebug() << "RenderingManager" << QChar(124).toLatin1() << tr("Maximum number of sources: ") << maxSourceCount;
    }

    // cleanup
    if (_fbo)
        delete _fbo;
    if (previousframe_fbo)
        delete previousframe_fbo;
    if (pboIds[0] || pboIds[1])
        glDeleteBuffers(2, pboIds);

    // create an fbo (with internal automatic first texture attachment)
    _fbo = new QGLFramebufferObject( qMin(size.width(), maxtexturewidth), qMin(size.height(), maxtextureheight));
    Q_CHECK_PTR(_fbo);

    if (_fbo->bind()) {
        // initial clear to black
        glPushAttrib(GL_COLOR_BUFFER_BIT);
        glClearColor(0.f, 0.f, 0.f, 1.f);
        glClear(GL_COLOR_BUFFER_BIT);
        glPopAttrib();

        _fbo->release();
    }
    else
        qFatal( "%s", qPrintable( tr("OpenGL Frame Buffer Objects is not accessible (RenderingManager FBO %1x%2 bind failed).").arg(_fbo->width()).arg(_fbo->height())));

    // create the previous frame (frame buffer object) if needed
    previousframe_fbo = new QGLFramebufferObject( _fbo->width(), _fbo->height());
    // initial clear to black
    if (previousframe_fbo->bind())  {
        // initial clear to black
        glPushAttrib(GL_COLOR_BUFFER_BIT);
        glClearColor(0.f, 0.f, 0.f, 1.f);
        glClear(GL_COLOR_BUFFER_BIT);
        glPopAttrib();

        previousframe_fbo->release();
    }
    else
        qFatal( "%s", qPrintable( tr("OpenGL Frame Buffer Objects is not accessible (RenderingManager background FBO %1x%2 bind failed).").arg(_fbo->width()).arg(_fbo->height())));

    // configure texture display
    glBindTexture(GL_TEXTURE_2D, previousframe_fbo->texture());
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glBindTexture(GL_TEXTURE_2D, 0);

    // store viewport info
    _renderwidget->_renderView->viewport[0] = 0;
    _renderwidget->_renderView->viewport[1] = 0;
    _renderwidget->_renderView->viewport[2] = _fbo->width();
    _renderwidget->_renderView->viewport[3] = _fbo->height();

    // allocate PBOs
    if (RenderingManager::pbo_extension) {
        glGenBuffers(2, pboIds);
        glBindBuffer(GL_PIXEL_PACK_BUFFER, pboIds[0]);
        glBufferData(GL_PIXEL_PACK_BUFFER, _fbo->width() * _fbo->height() * 4, 0, GL_STREAM_READ);
        glBindBuffer(GL_PIXEL_PACK_BUFFER, pboIds[1]);
        glBufferData(GL_PIXEL_PACK_BUFFER, _fbo->width() * _fbo->height() * 4, 0, GL_STREAM_READ);
        glBindBuffer(GL_PIXEL_PACK_BUFFER, 0);

        qDebug() << "RenderingManager" << QChar(124).toLatin1() << tr("Pixel Buffer Objects initialized: RGBA (") << _fbo->width() << "x" << _fbo->height() <<").";
    }
    else {
        // no PBO
        pboIds[0] = 0;
        pboIds[1] = 0;
    }

    // setup recorder frames size
    _recorder->setFrameSize(_fbo->size());

#ifdef SHM
    // re-setup shared memory
    if(_sharedMemory) {
        setFrameSharingEnabled(false);
        setFrameSharingEnabled(true);
    }
#endif
#ifdef SPOUT
    if(_spoutEnabled) {
        setSpoutSharingEnabled(false);
        setSpoutSharingEnabled(true);
    }
#endif

    emit frameBufferChanged();

    qDebug() << "RenderingManager" << QChar(124).toLatin1() << tr("Frame Buffer Objects initialized: RGBA (") << size.width() << "x" << size.height() <<").";
}


double RenderingManager::getFrameBufferAspectRatio() const{

    if (_fbo)
        return ((double) _fbo->width() / (double) _fbo->height());
    else
        return 1.0;
}

void RenderingManager::postRenderToFrameBuffer() {

    if (!_fbo)
        return;

    // skip loop back if no rendering source
    if (countRenderingSource > 0 && !paused)
    {
        // frame delay
        previousframe_index++;
        if (!(previousframe_index % previousframe_delay)) {

            previousframe_index = 0;

            if (RenderingManager::blit_fbo_extension)
            // use the accelerated GL_EXT_framebuffer_blit if available
            {
                glBindFramebuffer(GL_READ_FRAMEBUFFER, _fbo->handle());
                // TODO : Can we draw in different texture buffer so we can keep an history of
                // several frames, and each loopback source could use a different one
                glBindFramebuffer(GL_DRAW_FRAMEBUFFER, previousframe_fbo->handle());

                glBlitFramebuffer(0, _fbo->height(), _fbo->width(), 0, 0, 0,
                        previousframe_fbo->width(), previousframe_fbo->height(),
                        GL_COLOR_BUFFER_BIT, GL_NEAREST);

                glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
            }
            // 	Draw quad with fbo texture in a more basic OpenGL way
            else {
                glPushAttrib(GL_COLOR_BUFFER_BIT | GL_VIEWPORT_BIT);

                glViewport(0, 0, previousframe_fbo->width(), previousframe_fbo->height());

                glMatrixMode(GL_PROJECTION);
                glPushMatrix();
                glLoadIdentity();
                gluOrtho2D(-1.0, 1.0, -1.0, 1.0);

                glMatrixMode(GL_MODELVIEW);
                glPushMatrix();
                glLoadIdentity();

                glColor4f(1.0, 1.0, 1.0, 1.0);
                glEnable(GL_TEXTURE_2D);

                // render to the frame buffer object
                if (previousframe_fbo->bind())
                {
                    glBindTexture(GL_TEXTURE_2D, _fbo->texture());
                    glCallList(ViewRenderWidget::quad_texured);

                    previousframe_fbo->release();
                }

                // pop the projection matrix and GL state back for rendering the current view
                // to the actual widget
                glDisable(GL_TEXTURE_2D);
                glPopAttrib();
                glMatrixMode(GL_PROJECTION);
                glPopMatrix();
                glMatrixMode(GL_MODELVIEW);
                glPopMatrix();
            }
        }
    }

    // save the frame to file or copy to SHM
    if ( _recorder->isRecording()
#ifdef SHM
            || _sharedMemory != NULL
#endif
        ) {


        glEnable(GL_TEXTURE_2D);
        glActiveTexture(GL_TEXTURE0);
        // read texture from the framebuferobject and record this frame (the recorder knows if it is active or not)
        glBindTexture(GL_TEXTURE_2D, _fbo->texture());

        // use pixel buffer object if initialized
        if (pboIds[0] && pboIds[1]) {

            glBindBuffer(GL_PIXEL_PACK_BUFFER, pboIds[pbo_index]);
            // read pixels from texture
            glGetTexImage(GL_TEXTURE_2D, 0, GL_BGRA, GL_UNSIGNED_BYTE, 0);

            // map the PBO to process its data by CPU
            glBindBuffer(GL_PIXEL_PACK_BUFFER, pboIds[pbo_nextIndex]);
            unsigned char* ptr = (unsigned char*) glMapBuffer(GL_PIXEL_PACK_BUFFER, GL_READ_ONLY);
            if(ptr)  {
                _recorder->addFrame(ptr);
                glUnmapBuffer(GL_PIXEL_PACK_BUFFER);
            }
        }
        // just get current texture if not using pixel buffer object
        else
            _recorder->addFrame();


#ifdef SHM
        // share to memory if needed
        if (_sharedMemory != NULL) {

            _sharedMemory->lock();

            // read the pixels from the texture
            glGetTexImage(GL_TEXTURE_2D, 0, _sharedMemoryGLFormat, _sharedMemoryGLType, (GLvoid *) _sharedMemory->data());

            _sharedMemory->unlock();
        }


#endif // SHM

        glDisable(GL_TEXTURE_2D);

        // restore state if using PBO
        if (pboIds[0] && pboIds[1]) {
            // back to conventional pixel operation
            glBindBuffer(GL_PIXEL_PACK_BUFFER, 0);
            glBindFramebuffer(GL_FRAMEBUFFER, 0);

            // "index" is used to read pixels from framebuffer to a PBO
            // "nextIndex" is used to update pixels in the other PBO
            pbo_index = (pbo_index + 1) % 2;
            pbo_nextIndex = (pbo_index + 1) % 2;
        }

    }
    // end of recording : ensure PBO double buffer mechanism is reset
    else {
        pbo_index = pbo_nextIndex = 0;
    }

#ifdef SPOUT

    if ( _spoutInitialized ) {

        Spout::SendTexture( _fbo->texture(), GL_TEXTURE_2D, _fbo->width(), _fbo->height());
    }

#endif // SPOUT

}


void RenderingManager::renderToFrameBuffer(Source *source, bool first, bool last) {

    if (!_fbo)
        setFrameBufferResolution( sizeOfFrameBuffer[renderingAspectRatio][renderingQuality] );

    glPushAttrib(GL_COLOR_BUFFER_BIT | GL_VIEWPORT_BIT);

    glViewport(0, 0, _renderwidget->_renderView->viewport[2], _renderwidget->_renderView->viewport[3]);

    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadMatrixd(_renderwidget->_renderView->projection);

    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();

    if (!paused)
    {
        // render to the frame buffer object
        if (_fbo->bind())
        {
            //
            // 1. Draw into first texture attachment; the final output rendering
            //
            if (first) {
                if (clearWhite)
                    glClearColor(1.f, 1.f, 1.f, 1.f);
                else
                    glClearColor(0.f, 0.f, 0.f, 1.f);
                glClear(GL_COLOR_BUFFER_BIT);
            }

            if (source) {
                // draw the source only if not culled and alpha not null
                if (!source->isCulled() && source->getAlpha() > 0.0) {
                    glTranslated(source->getX(), source->getY(), 0.0);
                    glRotated(source->getRotationAngle(), 0.0, 0.0, 1.0);
                    glScaled(source->getScaleX(), source->getScaleY(), 1.f);

                    source->blend();
                    source->draw();
                }

            }

            // render the transition layer on top after the last frame
            if (last) {
                _switcher->render();
            }

            _fbo->release();
        }
        else
            qFatal( "%s", qPrintable( tr("OpenGL Frame Buffer Objects is not accessible "
                                         "(RenderingManager %1x%2 bind failed).").arg(_fbo->width()).arg(_fbo->height())));
    }


    //
    // 2. Draw sources into second texture  attachment ; the catalog (if visible)
    //
    if (_renderwidget->_catalogView->visible() ) {

        if (first)
            // Clear Catalog view
            _renderwidget->_catalogView->clear();

        if (source)
            // Draw this source into the catalog
            _renderwidget->_catalogView->drawSource( source );

        if (last)
            _renderwidget->_catalogView->reorganize();

    }


    // pop the projection matrix and GL state back for rendering the current view
    // to the actual widget
    glPopAttrib();
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
    glPopMatrix();
}

Source *RenderingManager::newRenderingSource(double depth) {

#ifndef NDEBUG
    qDebug() << tr("RenderingManager::newRenderingSource ")<< depth;
#endif

    RenderingSource *s = 0;
    _renderwidget->makeCurrent();

    try {
        // create a source appropriate
        s = new RenderingSource(getAvailableDepthFrom(depth));
        renameSource( s, _defaultSource->getName() + "Render");

    } catch (AllocationException &e){
        qWarning() << "Cannot create Rendering source; " << e.message();
        // return an invalid pointer
        s = 0;
    }

    return ( (Source *) s );
}

QImage RenderingManager::captureFrameBuffer(QImage::Format format) {

    QImage img = _fbo ? _fbo->toImage() : QImage();

    if (format != QImage::Format_RGB888)
        img = img.convertToFormat(format);

    return img;
}


Source *RenderingManager::newSvgSource(QSvgRenderer *svg, double depth){

#ifndef NDEBUG
    qDebug() << tr("RenderingManager::newSvgSource ")<< depth;
#endif

    SvgSource *s = 0;
    // create the texture for this source
    GLuint textureIndex;
    _renderwidget->makeCurrent();
    glGenTextures(1, &textureIndex);
    // high priority means low variability
    GLclampf highpriority = 1.0;
    glPrioritizeTextures(1, &textureIndex, &highpriority);

    try {
        // create a source appropriate
        s = new SvgSource(svg, textureIndex, getAvailableDepthFrom(depth));
        renameSource( s, _defaultSource->getName() + "Svg");

    } catch (AllocationException &e){
        qWarning() << "Cannot create SVG source; " << e.message();
        // free the OpenGL texture
        glDeleteTextures(1, &textureIndex);
        // return an invalid pointer
        s = 0;
    }

    return ( (Source *) s );
}

Source *RenderingManager::newWebSource(QUrl web, int height, int scroll, int update, double depth){
    WebSource *s = 0;

#ifndef NDEBUG
    qDebug() << tr("RenderingManager::newWebSource ")<< depth;
#endif

    // create the texture for this source
    GLuint textureIndex;
    _renderwidget->makeCurrent();
    glGenTextures(1, &textureIndex);

    // high priority means low variability
    GLclampf highpriority = 1.0;
    glPrioritizeTextures(1, &textureIndex, &highpriority);

    try {
        // create a source appropriate
        s = new WebSource(web, textureIndex, getAvailableDepthFrom(depth), height, scroll, update);
        renameSource( s, _defaultSource->getName() + "Web");

    } catch (AllocationException &e){
        qWarning() << "Cannot create Web source; " << e.message();
        // free the OpenGL texture
        glDeleteTextures(1, &textureIndex);
        // return an invalid pointer
        s = 0;
    }

    return ( (Source *) s );
}

Source *RenderingManager::newCaptureSource(QImage img, double depth) {

#ifndef NDEBUG
    qDebug() << tr("RenderingManager::newCaptureSource ")<< depth;
#endif

    CaptureSource *s = 0;
    // create the texture for this source
    GLuint textureIndex;
    _renderwidget->makeCurrent();
    glGenTextures(1, &textureIndex);
    // high priority means low variability
    GLclampf highpriority = 1.0;
    glPrioritizeTextures(1, &textureIndex, &highpriority);

    try {
        // create a source appropriate
        s = new CaptureSource(img, textureIndex, getAvailableDepthFrom(depth));
        renameSource( s, _defaultSource->getName() + "Capture");

    } catch (AllocationException &e){
        qWarning() << "Cannot create Capture source; " << e.message();
        // free the OpenGL texture
        glDeleteTextures(1, &textureIndex);
        // return an invalid pointer
        s = 0;
    }

    return ( (Source *) s );
}

Source *RenderingManager::newMediaSource(VideoFile *vf, double depth) {

#ifndef NDEBUG
    qDebug() << tr("RenderingManager::newMediaSource ")<< depth << vf->getFileName() ;
#endif

    VideoSource *s = 0;
    // create the texture for this source
    GLuint textureIndex;
    _renderwidget->makeCurrent();
    glGenTextures(1, &textureIndex);
    // low priority means high variability
    GLclampf lowpriority = 0.1;
    glPrioritizeTextures(1, &textureIndex, &lowpriority);

    try {
        // create a source appropriate for this videofile
        s = new VideoSource(vf, textureIndex, getAvailableDepthFrom(depth) );
        renameSource( s, _defaultSource->getName() + QDir(vf->getFileName()).dirName().split(".").first());

        QObject::connect(s, SIGNAL(failed()), this, SLOT(onSourceFailure()));

    } catch (AllocationException &e){
        qWarning() << "Cannot create Media source; " << e.message();
        // free the OpenGL texture
        glDeleteTextures(1, &textureIndex);
        // return an invalid pointer
        s = 0;
    }

    return ( (Source *) s );
}

#ifdef OPEN_CV
Source *RenderingManager::newOpencvSource(int opencvIndex, double depth) {

    GLuint textureIndex;
    OpencvSource *s = 0;

    s = OpencvSource::getExistingSourceForCameraIndex(opencvIndex);
    if ( s ) {
        return newCloneSource(getById(s->getId()), depth);
    }

    // try to create the OpenCV source
    try {
        // create the texture for this source
        _renderwidget->makeCurrent();
        glGenTextures(1, &textureIndex);
        GLclampf lowpriority = 0.1;

        glPrioritizeTextures(1, &textureIndex, &lowpriority);

        // try to create the opencv source
        s = new OpencvSource(opencvIndex, textureIndex, getAvailableDepthFrom(depth));
        renameSource( s, _defaultSource->getName() + QString("Camera%1").arg(opencvIndex) );

    } catch (AllocationException &e){
        qWarning() << "Cannot create OpenCV source; " << e.message();
        // free the OpenGL texture
        glDeleteTextures(1, &textureIndex);
        // return an invalid pointer
        s = 0;
    }

    return ( (Source *) s );
}
#endif

#ifdef FFGL
Source *RenderingManager::newFreeframeGLSource(QDomElement configuration, int w, int h, double depth) {

    FFGLSource *s = 0;

    // for Freeframe plugin sources
    if (configuration.tagName() == QString("FreeFramePlugin")) {

        QDomElement Filename = configuration.firstChildElement("Filename");
        if (!Filename.isNull()) {

            // first reads with the absolute file name
            QString fileNameToOpen = Filename.text();
            // if there is no such file, try generate a file name from the relative file name
            if (!QFileInfo(fileNameToOpen).exists())
                fileNameToOpen = Filename.attribute("Relative", "");
            // if there is no such file, try generate a file name from the generic basename
            if (!QFileInfo(fileNameToOpen).exists() && Filename.hasAttribute("Basename"))
                fileNameToOpen =  FFGLPluginSource::libraryFileName( Filename.attribute("Basename", ""));
            // if there is such a file
            if ( QFileInfo(fileNameToOpen).exists()) {

                GLuint textureIndex;
                // try to create the FFGL source
                try {
                    try {
                        // create the texture for this source
                        _renderwidget->makeCurrent();
                        glGenTextures(1, &textureIndex);
                        GLclampf lowpriority = 0.1;

                        glPrioritizeTextures(1, &textureIndex, &lowpriority);

                        // try to create the opencv source
                        s = new FFGLSource(fileNameToOpen, textureIndex, getAvailableDepthFrom(depth), w, h);

                        // all good, set parameters
                        s->freeframeGLPlugin()->setConfiguration( configuration );

                        // give it a name
                        renameSource( s, _defaultSource->getName() + QString("Freeframe") );

                    } catch (AllocationException &e){
                        qCritical() << tr("Allocation Exception; ") << e.message();
                        throw;
                    }
                    catch (FFGLPluginException &e)  {
                        qCritical() << tr("Freeframe error; ") << e.message();
                        throw;
                    }
                }
                catch (...)  {
                    qCritical() << fileNameToOpen << QChar(124).toLatin1() << tr("Could no create plugin source.");
                    // free the OpenGL texture
                    glDeleteTextures(1, &textureIndex);
                    // return an invalid pointer
                    s = 0;
                }

            }
            else
                qCritical() << fileNameToOpen << QChar(124).toLatin1() << tr("File does not exist.");

        } else
            qCritical() << tr("No file name provided to create Freeframe source.");

        }
    // not FreeFramePlugin : must be ShadertoyPlugin
    else {

        GLuint textureIndex;
        // try to create the Shadertoy source
        try {
            try {
                // create the texture for this source
                _renderwidget->makeCurrent();
                glGenTextures(1, &textureIndex);
                GLclampf lowpriority = 0.1;

                glPrioritizeTextures(1, &textureIndex, &lowpriority);

                // try to create the opencv source
                s = new FFGLSource(textureIndex, getAvailableDepthFrom(depth), w, h);

                // all good, set parameters
                s->freeframeGLPlugin()->setConfiguration( configuration );

                // give it a name
                renameSource( s, _defaultSource->getName() + QString("Shadertoy") );

            } catch (AllocationException &e){
                qCritical() << tr("Allocation Exception; ") << e.message();
                throw;
            }
            catch (FFGLPluginException &e)  {
                qCritical() << tr("Shadertoy error; ") << e.message();
                throw;
            }
        }
        catch (...)  {
            qCritical() << "shadertoy" << QChar(124).toLatin1() << tr("Could no create plugin source.");
            // free the OpenGL texture
            glDeleteTextures(1, &textureIndex);
            // return an invalid pointer
            s = 0;
        }

    }


    return ( (Source *) s );
}
#endif

Source *RenderingManager::newAlgorithmSource(int type, int w, int h, double v,
                                             int p, bool ia, double depth) {

#ifndef NDEBUG
    qDebug() << tr("RenderingManager::newAlgorithmSource ")<< depth << type;
#endif
    AlgorithmSource *s = 0;
    // create the texture for this source
    GLuint textureIndex;
    _renderwidget->makeCurrent();
    glGenTextures(1, &textureIndex);
    GLclampf lowpriority = 0.1;
    glPrioritizeTextures(1, &textureIndex, &lowpriority);

    try {
        // create a source appropriate
        s = new AlgorithmSource(type, textureIndex, getAvailableDepthFrom(depth), w, h, v, p, ia);
        renameSource( s, _defaultSource->getName() + tr("Algo"));

    } catch (AllocationException &e){
        qWarning() << "Cannot create Algorithm source; " << e.message();
        // free the OpenGL texture
        glDeleteTextures(1, &textureIndex);
        // return an invalid pointer
        s = 0;
    }

    return ( (Source *) s );
}

#ifdef SHM
Source *RenderingManager::newSharedMemorySource(qint64 shmid, double depth) {

    SharedMemorySource *s = 0;
    // create the texture for this source
    GLuint textureIndex;
    _renderwidget->makeCurrent();
    glGenTextures(1, &textureIndex);
    GLclampf lowpriority = 0.1;
    glPrioritizeTextures(1, &textureIndex, &lowpriority);

    try {
        // create a source appropriate
        s = new SharedMemorySource(textureIndex, getAvailableDepthFrom(depth), shmid);
        renameSource( s, _defaultSource->getName() + s->getKey());

    } catch (AllocationException &e){
        qWarning() << "Cannot create Shared Memory source; " << e.message();
        // free the OpenGL texture
        glDeleteTextures(1, &textureIndex);
        // return an invalid pointer
        s = 0;
    }

    return ( (Source *) s );
}
#endif

#ifdef SPOUT
Source *RenderingManager::newSpoutSource(QString senderName, double depth) {

    SpoutSource *s = 0;
    // create the texture for this source
    GLuint textureIndex;
    _renderwidget->makeCurrent();
    glGenTextures(1, &textureIndex);
    GLclampf lowpriority = 0.1;
    glPrioritizeTextures(1, &textureIndex, &lowpriority);

    try {
        // create a source appropriate
        s = new SpoutSource(textureIndex, getAvailableDepthFrom(depth), senderName);
        renameSource( s, _defaultSource->getName() + senderName );

    } catch (AllocationException &e){
        qWarning() << "Cannot create SPOUT source; " << e.message();
        // free the OpenGL texture
        glDeleteTextures(1, &textureIndex);
        // return an invalid pointer
        s = 0;
    }

    return ( (Source *) s );
}
#endif

Source *RenderingManager::newCloneSource(SourceSet::iterator sit, double depth) {

#ifndef NDEBUG
    qDebug() << tr("RenderingManager::newCloneSource ")<< depth;
#endif

    CloneSource *s = 0;
    try{
        // create a source appropriate for this videofile
        s = new CloneSource(sit, getAvailableDepthFrom(depth));

        if ((*sit)->rtti() == Source::CLONE_SOURCE) {
            CloneSource *o = dynamic_cast<CloneSource *>(*sit);
            renameSource( s, o->getOriginalName() + tr("Clone"));
        } else
            renameSource( s, (*sit)->getName() + tr("Clone"));


    } catch (AllocationException &e){
        qWarning() << "Cannot clone source; " << e.message();
        // return an invalid pointer
        s = 0;
    }

    return ( (Source *) s );
}

bool RenderingManager::insertSource(Source *s)
{
    if (s) {
        // replace the source name by another available one based on the original name
        s->setName( getAvailableNameFrom(s->getName()) );

        if (_front_sources.size() < maxSourceCount) {
            //insert the source to the list
            if (_front_sources.insert(s).second) {

#ifdef HISTORY_MANAGEMENT
                // connect source to the history manager
                _undoHistory->connect(s, SIGNAL(methodCalled(QString, QVariantPair, QVariantPair, QVariantPair, QVariantPair, QVariantPair)), SLOT(rememberEvent(QString, QVariantPair, QVariantPair, QVariantPair, QVariantPair, QVariantPair)));
#endif

                // inform of success
                return true;
            } else
                qCritical() << tr("Not enough space to insert the source into the stack (%1).").arg(_front_sources.size());
        }
        else
            qCritical() << tr("You have reached the maximum amount of source supported (%1).").arg(maxSourceCount);
    }

    return false;
}

void RenderingManager::addSourceToBasket(Source *s)
{
    // add the source into the basket
    dropBasket.insert(s);

    if (s->rtti() != Source::CLONE_SOURCE) {
        // apply default parameters
        s->importProperties(_defaultSource);
        // scale the source to match the preferences
        s->resetScale(_scalingMode);
    }

    // select no source
    unsetCurrentSource();
}

void RenderingManager::clearBasket()
{
    for (SourceList::iterator sit = dropBasket.begin(); sit != dropBasket.end(); sit = dropBasket.begin()) {
        dropBasket.erase(sit);
        delete (*sit);
    }
}

void RenderingManager::resetSource(SourceSet::iterator sit){

    // apply default parameters
    (*sit)->importProperties(_defaultSource);
    // scale the source to match the preferences
    (*sit)->resetScale(_scalingMode);
#ifdef FFGL
    // clear plugins
    (*sit)->clearFreeframeGLPlugin();
#endif
    // inform GUI
    emit currentSourceChanged(sit);
}


void RenderingManager::toggleUnchangeableCurrentSource(bool on){

    if(isValid(_currentSource)) {
        (*_currentSource)->setModifiable( ! on );
        emit currentSourceChanged(_currentSource);
    }
}


void RenderingManager::toggleFixAspectRatioCurrentSource(bool on){

    if(isValid(_currentSource)) {
        (*_currentSource)->setFixedAspectRatio( on );
        emit currentSourceChanged(_currentSource);
    }
}

void RenderingManager::setOriginalAspectRatioCurrentSource(){

    if(isValid(_currentSource)) {
        double scale = (*_currentSource)->getScaleX();
        (*_currentSource)->resetScale(Source::SCALE_FIT);
        scale /= (*_currentSource)->getScaleX();
        (*_currentSource)->scaleBy( scale, scale );
        emit currentSourceChanged(_currentSource);
    }
}

void RenderingManager::setRenderingAspectRatioCurrentSource(){

    if(isValid(_currentSource)) {
        double scale = (*_currentSource)->getScaleX();
        (*_currentSource)->resetScale(Source::SCALE_DEFORM);
        scale /= (*_currentSource)->getScaleX();
        (*_currentSource)->scaleBy( scale, scale );
        emit currentSourceChanged(_currentSource);
    }
}

void RenderingManager::resetCurrentSource(){

    if(isValid(_currentSource)) {
        resetSource(_currentSource);
        emit currentSourceChanged(_currentSource);
    }
}


int RenderingManager::getSourceBasketSize() const{

    return (int) (dropBasket.size());
}

Source *RenderingManager::getSourceBasketTop() const{

    if (dropBasket.empty())
        return 0;
    else
        return (*dropBasket.begin());
}

void RenderingManager::dropSourceWithAlpha(double alphax, double alphay){

    dropSource();
    if (isValid(_currentSource))
        // apply the modifications
        (*_currentSource)->setAlphaCoordinates(alphax, alphay);

}

void RenderingManager::dropSourceWithCoordinates(double x, double y){

    dropSource();
    if (isValid(_currentSource)){
        // apply the modifications
        (*_currentSource)->setX(x);
        (*_currentSource)->setY(y);
    }
}

void RenderingManager::dropSourceWithDepth(double depth){

    dropSource();
    if (isValid(_currentSource))
        // apply the modifications
        changeDepth(_currentSource, depth);
}

void RenderingManager::dropSource(){

    unsetCurrentSource();

    // something to drop ?
    if (!dropBasket.empty()) {
        // get the pointer to the source at the top of the list
        Source *top = *dropBasket.begin();
        // remove from the basket
        dropBasket.erase(top);
        // insert the source
        if ( insertSource(top) ) {
            // make it current
            setCurrentSource(top->getId());
            // start playing (according to preference)
            top->play(_playOnDrop);
        } else
            delete top;
    }
}

void RenderingManager::replaceSource(GLuint oldsource, GLuint newsource) {

    SourceSet::iterator it_oldsource = getById(oldsource);
    SourceSet::iterator it_newsource = getById(newsource);

    if ( isValid(it_oldsource) && isValid(it_newsource)) {

        double depth_oldsource = (*it_oldsource)->getDepth();
        QString name_oldsource = (*it_oldsource)->getName();

        // apply former parameters
        (*it_newsource)->importProperties(*it_oldsource);

        // change all clones of old source to clone the new source
        for (SourceList::iterator clone = (*it_oldsource)->getClones()->begin(); clone != (*it_oldsource)->getClones()->end(); clone = (*it_oldsource)->getClones()->begin()) {
            CloneSource *tmp = dynamic_cast<CloneSource *>(*clone);
            if (tmp)
                tmp->setOriginal(it_newsource);
        }

#ifdef FFGL
        // copy the Freeframe plugin stack
        for (FFGLPluginSourceStack::const_iterator it = (*it_oldsource)->getFreeframeGLPluginStack()->begin(); it != (*it_oldsource)->getFreeframeGLPluginStack()->end(); ++it) {

            FFGLPluginSource *plugin = (*it_newsource)->addFreeframeGLPlugin( (*it)->fileName() );
            // set configuration
            if (plugin)
                plugin->setConfiguration( (*it)->getConfiguration() );
        }
#endif

        // delete old source
        removeSource(it_oldsource);

        // restore former depth
        changeDepth(it_newsource, depth_oldsource);

        // log
        qDebug() << name_oldsource  << QChar(124).toLatin1() << tr("Source replaced by %1").arg((*it_newsource)->getName());
    }

}

int RenderingManager::removeSource(const GLuint idsource){

    return removeSource(getById(idsource));
}

int RenderingManager::removeSource(SourceSet::iterator itsource) {

    if (!isValid(itsource)) {
        qWarning() << tr("Invalid Source cannot be deleted.");
        return 0;
    }

    // remove from selection and group
    _renderwidget->removeFromSelections(*itsource);

    // if we are removing the current source, ensure it is not the current one anymore
    if (itsource == _currentSource) {
        _currentSource = _front_sources.end();
        emit currentSourceChanged(_currentSource);
    }

    int num_sources_deleted = 0;
    if (itsource != _front_sources.end()) {
        Source *s = *itsource;
        // if this is not a clone
        if (s->rtti() != Source::CLONE_SOURCE)
            // remove every clone of the source to be removed
            for (SourceList::iterator clone = s->getClones()->begin(); clone != s->getClones()->end(); clone = s->getClones()->begin()) {
                num_sources_deleted += removeSource((*clone)->getId());
            }
        // then remove the source itself
        qDebug() << s->getName() << QChar(124).toLatin1() << tr("Delete source.");
        _front_sources.erase(itsource);

#ifdef HISTORY_MANAGEMENT
        // disconnect this source from the history manager
        _undoHistory->disconnect(s);
#endif

        delete s;
        num_sources_deleted++;
    }

    return num_sources_deleted;
}

void RenderingManager::clearSourceSet() {

    // how many sources to remove ?
    int total = _front_sources.size();
    int num_sources_deleted = 0;

    // remove all sources in the stack
    if (total > 0) {

        // clear the list of sources
        for (SourceSet::iterator its = _front_sources.begin(); its != _front_sources.end(); its = _front_sources.begin())
            num_sources_deleted += removeSource(its);

        // reset the id counter
        Source::lastid = 1;

        qDebug() << "RenderingManager" << QChar(124).toLatin1() << tr("All sources cleared (%1/%2)").arg(num_sources_deleted).arg(total);
    }

#ifdef VIDEOFILE_DEBUG
    VideoPicture::VideoPictureCountLock.lock();
    qDebug() << "Pending video Picture :" << VideoPicture::VideoPictureCount;
    VideoPicture::VideoPictureCountLock.unlock();

    VideoFile::PacketCountLock.lock();
    qDebug() << "Pending video packets :" << VideoFile::PacketCount;
    VideoFile::PacketCountLock.unlock();

    VideoFile::PacketListElementCountLock.lock();
    qDebug() << "Pending packets list elements :" << VideoFile::PacketListElementCount;
    VideoFile::PacketListElementCountLock.unlock();
#endif
}

bool RenderingManager::notAtEnd(SourceSet::const_iterator itsource)  const{

    return (itsource != _front_sources.end());
}

bool RenderingManager::isValid(SourceSet::const_iterator itsource)  const{

    if (notAtEnd(itsource))
        return (_front_sources.find(*itsource) != _front_sources.end());
    else
        return false;
}


bool RenderingManager::isCurrentSource(const Source *s){

    if (_currentSource != _front_sources.end())
        return ( s == *_currentSource );
    else
        return false;
}

bool RenderingManager::isCurrentSource(SourceSet::iterator si){

    return ( si == _currentSource );

}

void RenderingManager::setCurrentSource(SourceSet::iterator si) {

    if (si != _currentSource){
        _currentSource = si;
        emit currentSourceChanged(_currentSource);
    }
}

void RenderingManager::setCurrentSource(GLuint id) {

    setCurrentSource(getById(id));
}


bool RenderingManager::setCurrentNext(){

    if (_front_sources.empty() )
        return false;

    if (_currentSource != _front_sources.end()) {
        // increment to next source
        _currentSource++;
        // loop to begin if at end
        if (_currentSource == _front_sources.end())
            _currentSource = _front_sources.begin();
    } else
        _currentSource = _front_sources.begin();

    emit currentSourceChanged(_currentSource);
    return true;
}

bool RenderingManager::setCurrentPrevious(){

    if (_front_sources.empty() )
        return false;

    if (_currentSource != _front_sources.end()) {

        // if at the beginning, go to the end
        if (_currentSource == _front_sources.begin())
            _currentSource = _front_sources.end();
    }

    // decrement to previous source
    _currentSource--;
    emit currentSourceChanged(_currentSource);
    return true;
}


QString RenderingManager::getAvailableNameFrom(QString name) const{

    // start with a tentative name and assume it is NOT ok
    QString tentativeName = name;
    bool isok = false;
    int countbad = 2;
    // try to find the name in the list; it is still not ok if it exists
    while (!isok) {
        if ( isValid( getByName(tentativeName) ) ){
            // modify the tentative name and keep trying
            tentativeName = name + QString("-%1").arg(countbad++);
        } else
            isok = true;
    }
    // finally the tentative name is ok
    return tentativeName;
}

double RenderingManager::getAvailableDepthFrom(double depth) const {

    double tentativeDepth = depth;

    // place it at the front if no depth is provided (default argument = -1)
    if (tentativeDepth < 0)
        tentativeDepth  = (_front_sources.empty()) ? 0.0 : (*_front_sources.rbegin())->getDepth() + 0.4;

    tentativeDepth += dropBasket.size();

    // try to find a source at this depth in the list; it is not ok if it exists
    bool isok = false;
    while (!isok) {
        if ( isValid( std::find_if(_front_sources.begin(), _front_sources.end(), isCloseTo(tentativeDepth)) ) ){
            tentativeDepth += DEPTH_EPSILON;
        } else
            isok = true;
    }
    // finally the tentative depth is ok
    return tentativeDepth;
}

SourceSet::iterator RenderingManager::changeDepth(SourceSet::iterator itsource,
        double newdepth) {

    newdepth = CLAMP( newdepth, MIN_DEPTH_LAYER, MAX_DEPTH_LAYER);

    if (itsource != _front_sources.end()) {
        // verify that the depth value is not already taken, or too close to, and adjust in case.
        SourceSet::iterator sb, se;
        double depthinc = 0.0;
        if (newdepth < (*itsource)->getDepth()) {
            sb = _front_sources.begin();
            se = itsource;
            depthinc = -DEPTH_EPSILON;
        } else {
            sb = itsource;
            sb++;
            se = _front_sources.end();
            depthinc = DEPTH_EPSILON;
        }
        while (std::find_if(sb, se, isCloseTo(newdepth)) != se) {
            newdepth += depthinc;
        }

        // remember pointer to the source
        Source *tmp = (*itsource);
        // sort again the set by depth: this is done by removing the element and adding it again after changing its depth
        _front_sources.erase(itsource);
        // change the source internal depth value
        tmp->setDepth(newdepth);

        if (newdepth < 0) {
            // if request to place the source in a negative depth, shift all sources forward
            for (SourceSet::iterator it = _front_sources.begin(); it
                    != _front_sources.end(); it++)
                (*it)->setDepth((*it)->getDepth() - newdepth);
        }

        // re-insert the source into the sorted list ; it will be placed according to its new depth
        std::pair<SourceSet::iterator, bool> ret;
        ret = _front_sources.insert(tmp);
        if (ret.second)
            return (ret.first);
        else
            return (_front_sources.end());
    }

    return _front_sources.end();
}

SourceSet::iterator RenderingManager::getBegin() {
    return _front_sources.begin();
}

SourceSet::iterator RenderingManager::getEnd() {
    return _front_sources.end();
}

SourceSet::const_iterator RenderingManager::getBegin() const{
    return _front_sources.begin();
}

SourceSet::const_iterator RenderingManager::getEnd() const{
    return _front_sources.end();
}

SourceSet::iterator RenderingManager::getById(const GLuint id) {

    return std::find_if(_front_sources.begin(), _front_sources.end(), hasId(id));
}

SourceSet::iterator RenderingManager::getByName(const QString name){

    return std::find_if(_front_sources.begin(), _front_sources.end(), hasName(name));
}

SourceSet::const_iterator RenderingManager::getByName(const QString name) const {

    return std::find_if(_front_sources.begin(), _front_sources.end(), hasName(name));
}
/**
 * save and load configuration
 */
QDomElement RenderingManager::getConfiguration(QDomDocument &doc, QDir current) {

    QDomElement config = doc.createElement("SourceList");

    for (SourceSet::iterator its = _front_sources.begin(); its != _front_sources.end(); its++) {

        QDomElement sourceElem = doc.createElement("Source");
        sourceElem.setAttribute("name", (*its)->getName());
        sourceElem.setAttribute("playing", (*its)->isPlaying());
        sourceElem.setAttribute("stanbyMode", (int) (*its)->getStandbyMode());
        sourceElem.setAttribute("modifiable", (*its)->isModifiable());
        sourceElem.setAttribute("fixedAR", (*its)->isFixedAspectRatio());

        QDomElement pos = doc.createElement("Position");
        pos.setAttribute("X", QString::number((*its)->getX(),'f',PROPERTY_DECIMALS)  );
        pos.setAttribute("Y", QString::number((*its)->getY(),'f',PROPERTY_DECIMALS) );
        sourceElem.appendChild(pos);

        QDomElement rot = doc.createElement("Center");
        rot.setAttribute("X", QString::number((*its)->getRotationCenterX(),'f',PROPERTY_DECIMALS) );
        rot.setAttribute("Y", QString::number((*its)->getRotationCenterY(),'f',PROPERTY_DECIMALS) );
        sourceElem.appendChild(rot);

        QDomElement a = doc.createElement("Angle");
        a.setAttribute("A", QString::number((*its)->getRotationAngle(),'f',PROPERTY_DECIMALS) );
        sourceElem.appendChild(a);

        QDomElement scale = doc.createElement("Scale");
        scale.setAttribute("X", QString::number((*its)->getScaleX(),'f',PROPERTY_DECIMALS) );
        scale.setAttribute("Y", QString::number((*its)->getScaleY(),'f',PROPERTY_DECIMALS) );
        sourceElem.appendChild(scale);

        QDomElement crop = doc.createElement("Crop");
        crop.setAttribute("X", QString::number((*its)->getTextureCoordinates().x(),'f',PROPERTY_DECIMALS) );
        crop.setAttribute("Y", QString::number((*its)->getTextureCoordinates().y(),'f',PROPERTY_DECIMALS) );
        crop.setAttribute("W", QString::number((*its)->getTextureCoordinates().width(),'f',PROPERTY_DECIMALS) );
        crop.setAttribute("H", QString::number((*its)->getTextureCoordinates().height(),'f',PROPERTY_DECIMALS) );
        sourceElem.appendChild(crop);

        QDomElement d = doc.createElement("Depth");
        d.setAttribute("Z", (*its)->getDepth());
        sourceElem.appendChild(d);

        QDomElement alpha = doc.createElement("Alpha");
        alpha.setAttribute("X", QString::number((*its)->getAlphaX(),'f',PROPERTY_DECIMALS) );
        alpha.setAttribute("Y", QString::number((*its)->getAlphaY(),'f',PROPERTY_DECIMALS) );
        sourceElem.appendChild(alpha);

        QDomElement color = doc.createElement("Color");
        color.setAttribute("R", (*its)->getColor().red());
        color.setAttribute("G", (*its)->getColor().green());
        color.setAttribute("B", (*its)->getColor().blue());
        sourceElem.appendChild(color);

        QDomElement blend = doc.createElement("Blending");
        blend.setAttribute("Function", (*its)->getBlendFuncDestination());
        blend.setAttribute("Equation", (*its)->getBlendEquation());
        blend.setAttribute("Mask", (*its)->getMask());
        sourceElem.appendChild(blend);

        QDomElement filter = doc.createElement("Filter");
        filter.setAttribute("Pixelated", (*its)->isPixelated());
        filter.setAttribute("InvertMode", (*its)->getInvertMode());
        filter.setAttribute("Filter", (*its)->getFilter());
        sourceElem.appendChild(filter);

        QDomElement Coloring = doc.createElement("Coloring");
        Coloring.setAttribute("Brightness", (*its)->getBrightness());
        Coloring.setAttribute("Contrast", (*its)->getContrast());
        Coloring.setAttribute("Saturation", (*its)->getSaturation());
        Coloring.setAttribute("Hueshift", (*its)->getHueShift());
        Coloring.setAttribute("luminanceThreshold", (*its)->getLuminanceThreshold());
        Coloring.setAttribute("numberOfColors", (*its)->getNumberOfColors());
        sourceElem.appendChild(Coloring);

        QDomElement Chromakey = doc.createElement("Chromakey");
        Chromakey.setAttribute("on", (*its)->getChromaKey());
        Chromakey.setAttribute("R", (*its)->getChromaKeyColor().red());
        Chromakey.setAttribute("G", (*its)->getChromaKeyColor().green());
        Chromakey.setAttribute("B", (*its)->getChromaKeyColor().blue());
        Chromakey.setAttribute("Tolerance", (*its)->getChromaKeyTolerance());
        sourceElem.appendChild(Chromakey);

        QDomElement Gamma = doc.createElement("Gamma");
        Gamma.setAttribute("value", (*its)->getGamma());
        Gamma.setAttribute("minInput", (*its)->getGammaMinInput());
        Gamma.setAttribute("maxInput", (*its)->getGammaMaxInput());
        Gamma.setAttribute("minOutput", (*its)->getGammaMinOuput());
        Gamma.setAttribute("maxOutput", (*its)->getGammaMaxOutput());
        sourceElem.appendChild(Gamma);

// freeframe gl plugin
#ifdef FFGL
        // list of plugins
        FFGLPluginSourceStack *plugins = (*its)->getFreeframeGLPluginStack();
        for (FFGLPluginSourceStack::iterator it = plugins->begin(); it != plugins->end(); ++it ) {

            sourceElem.appendChild( (*it)->getConfiguration(current) );
        }
#endif

        // type specific settings
        QDomElement specific = doc.createElement("TypeSpecific");
        specific.setAttribute("type", (*its)->rtti());

        if ((*its)->rtti() == Source::VIDEO_SOURCE) {
            VideoSource *vs = dynamic_cast<VideoSource *> (*its);
            VideoFile *vf = vs->getVideoFile();
            // Necessary information for re-creating this video source:
            // filename, marks, saturation
            QDomElement f = doc.createElement("Filename");
            f.setAttribute("PowerOfTwo", (int) vf->getPowerOfTwoConversion());
            f.setAttribute("IgnoreAlpha", (int) vf->ignoresAlphaChannel());
            f.setAttribute("Relative", current.relativeFilePath(vf->getFileName()) );
            QDomText filename = doc.createTextNode( current.absoluteFilePath( vf->getFileName() ));
            f.appendChild(filename);
            specific.appendChild(f);

            QDomElement m = doc.createElement("Marks");
            m.setAttribute("In", QString::number(vf->getMarkIn(),'f',PROPERTY_DECIMALS) );
            m.setAttribute("Out",QString::number(vf->getMarkOut(),'f',PROPERTY_DECIMALS));
            specific.appendChild(m);

            QDomElement p = doc.createElement("Play");
            p.setAttribute("Speed", QString::number(vf->getPlaySpeed(),'f',PROPERTY_DECIMALS));
            p.setAttribute("Loop", vf->isLoop());
            specific.appendChild(p);

            QDomElement o = doc.createElement("Options");
            o.setAttribute("AllowDirtySeek", vf->getOptionAllowDirtySeek());
            o.setAttribute("RestartToMarkIn", vf->getOptionRestartToMarkIn());
            o.setAttribute("RevertToBlackWhenStop", vf->getOptionRevertToBlackWhenStop());
            specific.appendChild(o);
        }
        else if ((*its)->rtti() == Source::ALGORITHM_SOURCE) {
            AlgorithmSource *as = dynamic_cast<AlgorithmSource *> (*its);

            QDomElement f = doc.createElement("Algorithm");
            QDomText algo = doc.createTextNode(QString::number(as->getAlgorithmType()));
            f.appendChild(algo);
            f.setAttribute("IgnoreAlpha", as->getIgnoreAlpha());
            specific.appendChild(f);

            // get size
            QDomElement s = doc.createElement("Frame");
            s.setAttribute("Width", as->getFrameWidth());
            s.setAttribute("Height", as->getFrameHeight());
            specific.appendChild(s);

            QDomElement x = doc.createElement("Update");
            x.setAttribute("Periodicity", QString::number(as->getPeriodicity()) );
            x.setAttribute("Variability", as->getVariability());
            specific.appendChild(x);
        }
        else if ((*its)->rtti() == Source::CAPTURE_SOURCE) {
            CaptureSource *cs = dynamic_cast<CaptureSource *> (*its);

            QByteArray ba;
            QBuffer buffer(&ba);
            buffer.open(QIODevice::WriteOnly);

            if (!QImageWriter::supportedImageFormats().count("jpeg")){
                qWarning() << cs->getName() << QChar(124).toLatin1() << tr("Qt JPEG plugin not found; using XPM format (slower).") << QImageWriter::supportedImageFormats();
                if (!cs->image().save(&buffer, "xpm") )
                    qWarning() << cs->getName() << QChar(124).toLatin1() << tr("Could not save captured source (XPM format).");
            } else
                if (!cs->image().save(&buffer, "jpeg") )
                    qWarning() << cs->getName()  << QChar(124).toLatin1() << tr("Could not save captured source (JPG format).");

            buffer.close();

            QDomElement f = doc.createElement("Image");
            QDomText img = doc.createTextNode( QString::fromLatin1(ba.constData(), ba.size()) );

            f.appendChild(img);
            specific.appendChild(f);
        }
        else if ((*its)->rtti() == Source::CLONE_SOURCE) {
            CloneSource *cs = dynamic_cast<CloneSource *> (*its);

            QDomElement f = doc.createElement("CloneOf");
            QDomText name = doc.createTextNode(cs->getOriginalName());
            f.appendChild(name);
            specific.appendChild(f);
        }
        else if ((*its)->rtti() == Source::SVG_SOURCE) {
            SvgSource *svgs = dynamic_cast<SvgSource *> (*its);
            QByteArray ba = svgs->getDescription();

            QDomElement f = doc.createElement("Svg");
            QDomText name = doc.createTextNode( QString::fromLatin1(ba.constData(), ba.size()) );
            f.appendChild(name);
            specific.appendChild(f);
        }
        else if ((*its)->rtti() == Source::WEB_SOURCE) {
            WebSource *ws = dynamic_cast<WebSource *> (*its);

            QDomElement f = doc.createElement("Web");
            f.setAttribute("Scroll", ws->getPageScroll());
            f.setAttribute("Height", ws->getPageHeight());
            f.setAttribute("Update", ws->getPageUpdate());
            QDomText name = doc.createTextNode( ws->getUrl().toString() );
            f.appendChild(name);
            specific.appendChild(f);
        }
#ifdef OPEN_CV
        else if ((*its)->rtti() == Source::CAMERA_SOURCE) {
            OpencvSource *cs = dynamic_cast<OpencvSource *> (*its);

            QDomElement f = doc.createElement("CameraIndex");
            QDomText id = doc.createTextNode(QString::number(cs->getOpencvCameraIndex()));
            f.appendChild(id);
            specific.appendChild(f);
        }
#endif
#ifdef SHM
        else if ((*its)->rtti() == Source::SHM_SOURCE) {
            SharedMemorySource *shms = dynamic_cast<SharedMemorySource *> (*its);

            QDomElement f = doc.createElement("SharedMemory");
            f.setAttribute("Info", shms->getInfo());
            QDomText key = doc.createTextNode(shms->getProgram());
            f.appendChild(key);
            specific.appendChild(f);

        }
#endif
#ifdef SPOUT
        else if ((*its)->rtti() == Source::SPOUT_SOURCE) {
            SpoutSource *spouts = dynamic_cast<SpoutSource *> (*its);

            QDomElement f = doc.createElement("Spout");
            QDomText name = doc.createTextNode(spouts->getSenderName());
            f.appendChild(name);
            specific.appendChild(f);

        }
#endif
#ifdef FFGL
        else if ((*its)->rtti() == Source::FFGL_SOURCE) {
            FFGLSource *ffs = dynamic_cast<FFGLSource *> (*its);

            // get size
            QDomElement s = doc.createElement("Frame");
            s.setAttribute("Width", ffs->getFrameWidth());
            s.setAttribute("Height", ffs->getFrameHeight());
            specific.appendChild(s);

            // get FFGL plugin config
            specific.appendChild(ffs->freeframeGLPlugin()->getConfiguration());

        }
#endif

        sourceElem.appendChild(specific);
        config.appendChild(sourceElem);
    }

    return config;
}

int applySourceConfig(Source *newsource, QDomElement child, QDir current) {

    int errors = 0;
    QDomElement tmp;

    newsource->_setModifiable( child.attribute("modifiable", "1").toInt() );
    newsource->_setFixedAspectRatio( child.attribute("fixedAR", "0").toInt() );

    newsource->_setX( child.firstChildElement("Position").attribute("X", "0").toDouble() );
    newsource->_setY( child.firstChildElement("Position").attribute("Y", "0").toDouble() );
    newsource->_setRotationCenterX( child.firstChildElement("Center").attribute("X", "0").toDouble() );
    newsource->_setRotationCenterY( child.firstChildElement("Center").attribute("Y", "0").toDouble() );
    newsource->_setRotationAngle( child.firstChildElement("Angle").attribute("A", "0").toDouble() );
    newsource->_setScaleX( child.firstChildElement("Scale").attribute("X", "1").toDouble() );
    newsource->_setScaleY( child.firstChildElement("Scale").attribute("Y", "1").toDouble() );

    tmp = child.firstChildElement("Alpha");
    newsource->_setAlphaCoordinates( tmp.attribute("X", "0").toDouble(), tmp.attribute("Y", "0").toDouble() );

    tmp = child.firstChildElement("Color");
    newsource->_setColor( QColor( tmp.attribute("R", "255").toInt(),tmp.attribute("G", "255").toInt(), tmp.attribute("B", "255").toInt() ) );

    tmp = child.firstChildElement("Crop");
    newsource->_setTextureCoordinates( QRectF( tmp.attribute("X", "0").toDouble(), tmp.attribute("Y", "0").toDouble(),tmp.attribute("W", "1").toDouble(),tmp.attribute("H", "1").toDouble() ) );

    tmp = child.firstChildElement("Blending");
    newsource->_setBlendEquation( (uint) tmp.attribute("Equation", "32774").toInt()  );
    newsource->_setBlendFunc( GL_SRC_ALPHA, (uint) tmp.attribute("Function", "1").toInt() );
    newsource->_setMask( tmp.attribute("Mask", "0").toInt() );

    tmp = child.firstChildElement("Filter");
    newsource->_setPixelated( tmp.attribute("Pixelated", "0").toInt() );
    newsource->_setInvertMode( (Source::invertModeType) tmp.attribute("InvertMode", "0").toInt() );
    newsource->_setFilter( (Source::filterType) tmp.attribute("Filter", "0").toInt() );

    tmp = child.firstChildElement("Coloring");
    newsource->_setBrightness( tmp.attribute("Brightness", "0").toInt() );
    newsource->_setContrast( tmp.attribute("Contrast", "0").toInt() );
    newsource->_setSaturation( tmp.attribute("Saturation", "0").toInt() );
    newsource->_setHueShift( tmp.attribute("Hueshift", "0").toInt() );
    newsource->_setLuminanceThreshold( tmp.attribute("luminanceThreshold", "0").toInt() );
    newsource->_setNumberOfColors( tmp.attribute("numberOfColors", "0").toInt() );

    tmp = child.firstChildElement("Chromakey");
    newsource->_setChromaKey( tmp.attribute("on", "0").toInt() );
    newsource->_setChromaKeyColor( QColor( tmp.attribute("R", "255").toInt(),tmp.attribute("G", "0").toInt(), tmp.attribute("B", "0").toInt() ) );
    newsource->_setChromaKeyTolerance( tmp.attribute("Tolerance", "7").toInt() );

    tmp = child.firstChildElement("Gamma");
    newsource->_setGamma( tmp.attribute("value", "1").toFloat(),
            tmp.attribute("minInput", "0").toFloat(),
            tmp.attribute("maxInput", "1").toFloat(),
            tmp.attribute("minOutput", "0").toFloat(),
            tmp.attribute("maxOutput", "1").toFloat());

    // apply FreeFrame plugins
    // start loop of plugins to load
    QDomElement p = child.firstChildElement("FreeFramePlugin");
    while (!p.isNull()) {
#ifdef FFGL
        QDomElement Filename = p.firstChildElement("Filename");
        // first reads with the absolute file name
        QString fileNameToOpen = Filename.text();
        // if there is no such file, try generate a file name from the relative file name
        if (!QFileInfo(fileNameToOpen).exists())
            fileNameToOpen = current.absoluteFilePath( Filename.attribute("Relative", "") );
        // if there is no such file, try generate a file name from the generic basename
        if (!QFileInfo(fileNameToOpen).exists() && Filename.hasAttribute("Basename"))
            fileNameToOpen =  FFGLPluginSource::libraryFileName( Filename.attribute("Basename", ""));
        // if there is such a file
        if (QFileInfo(fileNameToOpen).exists()) {
            // create and push the plugin to the source
            FFGLPluginSource *plugin = newsource->addFreeframeGLPlugin( fileNameToOpen );
            // apply the configuration
            if (plugin) {
                plugin->setConfiguration(p);
                qDebug() << child.attribute("name") << QChar(124).toLatin1() << QObject::tr("FreeFrame plugin %1 added.").arg(fileNameToOpen);

            }else {
                errors++;
                qWarning() << child.attribute("name") << QChar(124).toLatin1() << QObject::tr("FreeFrame plugin %1 failed.").arg(fileNameToOpen);
            }
        }
        else {
            errors++;
            qWarning() << child.attribute("name") << QChar(124).toLatin1() << QObject::tr("No FreeFrame plugin file named %1 or %2.").arg(Filename.text()).arg(fileNameToOpen);
        }
#else
        qWarning() << child.attribute("name") << QChar(124).toLatin1() << QObject::tr("FreeframeGL plugin not supported.");
        errors++;
#endif
        p = p.nextSiblingElement("FreeFramePlugin");
    }

    // apply Shadertoy plugins
    // start loop of plugins to load
    p = child.firstChildElement("ShadertoyPlugin");
    while (!p.isNull()) {
#ifdef FFGL

        // create and push the plugin to the source
        FFGLPluginSource *plugin = newsource->addFreeframeGLPlugin();
        // apply the code
        if (plugin && plugin->rtti() == FFGLPluginSource::SHADERTOY_PLUGIN) {

            FFGLPluginSourceShadertoy *stp = qobject_cast<FFGLPluginSourceShadertoy *>(plugin);

            if (stp) {
                stp->setCode(p.firstChildElement("Code").text());
                stp->setName(p.firstChildElement("Name").text());
                stp->setAbout(p.firstChildElement("About").text());
                stp->setDescription(p.firstChildElement("Description").text());

                qDebug() << child.attribute("name") << QChar(124).toLatin1() << QObject::tr("Shadertoy plugin %1 added.").arg(p.firstChildElement("Name").text());
            }
            else {
                errors++;
                qWarning() << child.attribute("name") << QChar(124).toLatin1() << QObject::tr("Failed to create Shadertoy plugin.");
            }

        }
#else
        qWarning() << child.attribute("name") << QChar(124).toLatin1() << QObject::tr("Shadertoy plugin not supported.");
        errors++;
#endif
        p = p.nextSiblingElement("ShadertoyPlugin");
    }

    // ok source is configured, can start it
    // play the source if attribute says so
    // and if no attribute, then play by default.
    newsource->setStandbyMode( (Source::StandbyMode) child.attribute("stanbyMode", "0").toInt() );
    newsource->play( child.attribute("playing", "1").toInt() );

    return errors;
}

int RenderingManager::addConfiguration(QDomElement xmlconfig, QDir current, QString version) {

    QList<QDomElement> clones;
    int errors = 0;
    int count = 0;

    // busy


    // start loop of sources to create
    QDomElement child = xmlconfig.firstChildElement("Source");
    while (!child.isNull()) {

        // pointer for new source
        Source *newsource = 0;
        // read the depth where the source should be created
        double depth = child.firstChildElement("Depth").attribute("Z", "0").toDouble();

        // get the type of the source to create
        QDomElement t = child.firstChildElement("TypeSpecific");
        Source::RTTI type = (Source::RTTI) t.attribute("type").toInt();

        // create the source according to its specific type information
        if (type == Source::VIDEO_SOURCE ){
            // read the tags specific for a video source
            QDomElement Filename = t.firstChildElement("Filename");
            QDomElement marks = t.firstChildElement("Marks");

            // create the video file
            VideoFile *newSourceVideoFile = NULL;
            if ( !Filename.attribute("PowerOfTwo","0").toInt() && (glewIsSupported("GL_EXT_texture_non_power_of_two") || glewIsSupported("GL_ARB_texture_non_power_of_two") ) )
                newSourceVideoFile = new VideoFile(this);
            else
                newSourceVideoFile = new VideoFile(this, true, SWS_FAST_BILINEAR);
            // if the video file was created successfully
            if (newSourceVideoFile){
                // first reads with the absolute file name
                QString fileNameToOpen = Filename.text();
                // if there is no such file, try generate a file name from the relative file name
                if (!QFileInfo(fileNameToOpen).exists())
                    fileNameToOpen = current.absoluteFilePath( Filename.attribute("Relative", "") );
                // if there is such a file
                if (QFileInfo(fileNameToOpen).exists()) {

                    double markin = -1.0, markout = -1.0;
                    // old version used different system for marking : ignore the values for now
                    if ( !( version.toDouble() < 0.7) ) {
                        markin = marks.attribute("In").toDouble();
                        markout = marks.attribute("Out").toDouble();
                    }

                    // can we open this existing file ?
                    if ( newSourceVideoFile->open( fileNameToOpen, markin, markout, Filename.attribute("IgnoreAlpha").toInt() ) ) {

                        // fix old version marking : compute marks correctly
                        if ( version.toDouble() < 0.7) {
                            newSourceVideoFile->setMarkIn(newSourceVideoFile->getTimefromFrame((int64_t)marks.attribute("In").toInt()));
                            newSourceVideoFile->setMarkOut(newSourceVideoFile->getTimefromFrame((int64_t)marks.attribute("Out").toInt()));
                            qWarning() << child.attribute("name") << QChar(124).toLatin1()<< tr("Converted marks from old version file: begin = %1 (%2)  end = %3 (%4)").arg(newSourceVideoFile->getMarkIn()).arg(marks.attribute("In").toInt()).arg(newSourceVideoFile->getMarkOut()).arg(marks.attribute("Out").toInt());
                        }

                        // create the source as it is a valid video file (this also set it to be the current source)
                        newsource = RenderingManager::_instance->newMediaSource(newSourceVideoFile, depth);
                        if (newsource){
                            // all is good ! we can apply specific parameters to the video file
                            QDomElement play = t.firstChildElement("Play");

                            // fix old version speed : compute speed correctly
                            double play_speed = 1.0;
                            if ( version.toDouble() < 0.7 ) {
                                    switch (play.attribute("Speed","3").toInt())
                                    {
                                    case 0:
                                        play_speed = 0.25;
                                        break;
                                    case 1:
                                        play_speed = 0.333;
                                        break;
                                    case 2:
                                        play_speed = 0.5;
                                        break;
                                    case 4:
                                        play_speed = 2.0;
                                        break;
                                    case 5:
                                        play_speed = 3.0;
                                        break;
                                    case 3:
                                    default:
                                        play_speed = 1.0;
                                        break;
                                    }
                            }
                            // new format speed
                            else
                                play_speed = play.attribute("Speed","1.0").toDouble();
                            newSourceVideoFile->setPlaySpeed(play_speed);

                            newSourceVideoFile->setLoop(play.attribute("Loop","1").toInt());
                            QDomElement options = t.firstChildElement("Options");
                            newSourceVideoFile->setOptionAllowDirtySeek(options.attribute("AllowDirtySeek","0").toInt());
                            newSourceVideoFile->setOptionRestartToMarkIn(options.attribute("RestartToMarkIn","0").toInt());
                            newSourceVideoFile->setOptionRevertToBlackWhenStop(options.attribute("RevertToBlackWhenStop","0").toInt());

                            qDebug() << child.attribute("name") << QChar(124).toLatin1()<< tr("Media source created with ") << QFileInfo(fileNameToOpen).fileName() << " ("<<newSourceVideoFile->getFrameWidth()<<"x"<<newSourceVideoFile->getFrameHeight()<<").";
                        }
                        else {
                            qWarning() << child.attribute("name") << QChar(124).toLatin1()<< tr("Could not be created.");
                            errors++;
                        }
                    }
                    else {
                        qWarning() << child.attribute("name") << QChar(124).toLatin1()<< tr("Could not load ")<< fileNameToOpen;
                        errors++;
                    }
                }
                else {
                    qWarning() << child.attribute("name") << QChar(124).toLatin1()<< tr("No file named %1 or %2.").arg(Filename.text()).arg(fileNameToOpen);
                    errors++;
                }

                // if one of the above failed, remove the video file object from memory
                if (!newsource)
                    delete newSourceVideoFile;

            }
            else {
                qWarning() << child.attribute("name") << QChar(124).toLatin1()<< tr("Could not allocate memory.");
                errors++;
            }

        }
        else if ( type == Source::ALGORITHM_SOURCE) {
            // read the tags specific for an algorithm source
            QDomElement Algorithm = t.firstChildElement("Algorithm");
            QDomElement Frame = t.firstChildElement("Frame");
            QDomElement Update = t.firstChildElement("Update");

            int type = Algorithm.text().toInt();
            double v = Update.attribute("Variability").toDouble();
            // fix old version
            if ( version.toDouble() < 0.8 && type == 0)
                v = 0.0;

            newsource = RenderingManager::_instance->newAlgorithmSource(type,  Frame.attribute("Width", "64").toInt(), Frame.attribute("Height", "64").toInt(), v, Update.attribute("Periodicity").toInt(), Algorithm.attribute("IgnoreAlpha", "0").toInt(), depth);
            if (!newsource) {
                qWarning() << child.attribute("name") << QChar(124).toLatin1()<< tr("Could not create algorithm source.");
                errors++;
            } else
                qDebug() << child.attribute("name") << QChar(124).toLatin1() << tr("Algorithm source created (")<< AlgorithmSource::getAlgorithmDescription(Algorithm.text().toInt()) << ", "<<newsource->getFrameWidth()<<"x"<<newsource->getFrameHeight() << ").";
        }
        else if ( type == Source::RENDERING_SOURCE) {
            // no tags specific for a rendering source
            newsource = RenderingManager::_instance->newRenderingSource(depth);
            if (!newsource) {
                qWarning() << child.attribute("name") << QChar(124).toLatin1() << tr("Could not create rendering loop-back source.");
                errors++;
            } else
                qDebug() << child.attribute("name") << QChar(124).toLatin1() <<  tr("Rendering loop-back source created.");
        }
        else if ( type == Source::CAPTURE_SOURCE) {

            QDomElement img = t.firstChildElement("Image");
            QImage image;
            QByteArray data =  img.text().toLatin1();

            if ( image.loadFromData( reinterpret_cast<const uchar *>(data.data()), data.size()) )
                newsource = RenderingManager::_instance->newCaptureSource(image, depth);

            if (!newsource) {
                qWarning() << child.attribute("name") << QChar(124).toLatin1()<< tr("Could not create capture source; invalid picture in session file.");
                errors++;
            } else
                qDebug() << child.attribute("name") << QChar(124).toLatin1()<< tr("Capture source created (") <<newsource->getFrameWidth()<<"x"<<newsource->getFrameHeight() << ").";
        }
        else if ( type == Source::SVG_SOURCE) {

            QDomElement svg = t.firstChildElement("Svg");
            QByteArray data =  svg.text().toLatin1();

            QSvgRenderer *rendersvg = new QSvgRenderer(data);
            if ( rendersvg )
                newsource = RenderingManager::_instance->newSvgSource(rendersvg, depth);

            if (!newsource) {
                qWarning() << child.attribute("name")<< QChar(124).toLatin1() << tr("Could not create vector graphics source.");
                errors++;
            } else
                qDebug() << child.attribute("name")<< QChar(124).toLatin1() << tr("Vector graphics source created (")<<newsource->getFrameWidth()<<"x"<<newsource->getFrameHeight() << ").";
        }
        else if ( type == Source::WEB_SOURCE) {

            QDomElement web = t.firstChildElement("Web");
            QUrl url =  QUrl(web.text());

            newsource = RenderingManager::_instance->newWebSource(url, web.attribute("Height", "100").toInt(), web.attribute("Scroll", "0").toInt(), web.attribute("Update", "0").toInt(), depth);

            if (!newsource) {
                qWarning() << child.attribute("name")<< QChar(124).toLatin1() << tr("Could not create web source.");
                errors++;
            } else
                qDebug() << child.attribute("name")<< QChar(124).toLatin1() << tr("Web source created (")<<newsource->getFrameWidth()<<"x"<<newsource->getFrameHeight() << ").";
        }
        else if ( type == Source::CLONE_SOURCE) {
            // remember the node of the sources to clone
            clones.push_back(child);
        }
        else if ( type == Source::SHM_SOURCE) {
#ifdef SHM
            // read the tags specific for an algorithm source
            QDomElement SharedMemory = t.firstChildElement("SharedMemory");

            qint64 shmid = SharedMemoryManager::_instance->findProgramSharedMap(SharedMemory.text());
            if (shmid != 0)
                newsource = RenderingManager::_instance->newSharedMemorySource(shmid, depth);
            if (!newsource) {
                qWarning() << child.attribute("name") << QChar(124).toLatin1()<< tr("Could not connect to the program %1.").arg(SharedMemory.text());
                errors++;
            } else
                qDebug() << child.attribute("name")<< QChar(124).toLatin1() << tr("Shared memory source created (")<< SharedMemory.text() << ").";
#else
            qWarning() << child.attribute("name") << QChar(124).toLatin1()<< tr("Could not create source: type Shared Memory not supported.");
            errors++;
#endif
        }
        else if ( type == Source::SPOUT_SOURCE) {
#ifdef SPOUT
            // read the tags specific for an algorithm source
            QDomElement spout = t.firstChildElement("Spout");

            newsource = RenderingManager::_instance->newSpoutSource(spout.text(), depth);

            if (!newsource) {
                qWarning() << child.attribute("name") << QChar(124).toLatin1()<< tr("Could not connect to the SPOUT sender %1.").arg(spout.text());
                errors++;
            } else
                qDebug() << child.attribute("name")<< QChar(124).toLatin1() << tr("SPOUT source created (")<< spout.text() << ").";
#else
            qWarning() << child.attribute("name") << QChar(124).toLatin1()<< tr("Could not create source: type SPOUT not supported.");
            errors++;
#endif
        }
        else if (type == Source::FFGL_SOURCE ){
#ifdef FFGL
            QDomElement Frame = t.firstChildElement("Frame");
            QDomElement ffgl = t.firstChildElement("FreeFramePlugin");

            if ( ffgl.isNull() )
                ffgl = t.firstChildElement("ShadertoyPlugin");

            newsource = RenderingManager::_instance->newFreeframeGLSource(ffgl, Frame.attribute("Width", "64").toInt(),  Frame.attribute("Height", "64").toInt());

            if (!newsource) {
                qWarning() << child.attribute("name") << QChar(124).toLatin1()<< tr("Could not create FreeframeGL source.");
                errors++;
            } else
                qDebug() << child.attribute("name") << QChar(124).toLatin1()<< tr("FreeframeGL source created (") <<newsource->getFrameWidth()<<"x"<<newsource->getFrameHeight() << ").";;
#else
            qWarning() << child.attribute("name") << QChar(124).toLatin1()<< tr("Could not create source: type FreeframeGL not supported.");
            errors++;
#endif
        }
        else if ( type == Source::CAMERA_SOURCE ) {
#ifdef OPEN_CV
            QDomElement camera = t.firstChildElement("CameraIndex");

            newsource = RenderingManager::_instance->newOpencvSource( camera.text().toInt(), depth);
            if (!newsource) {
                qWarning() << child.attribute("name") << QChar(124).toLatin1()<<  tr("Could not open OpenCV device index %2.").arg(camera.text());
                errors ++;
            } else
                qDebug() << child.attribute("name") << QChar(124).toLatin1()<<  tr("OpenCV source created (device index %2, ").arg(camera.text()) <<newsource->getFrameWidth()<<"x"<<newsource->getFrameHeight() << " ).";
#else
            qWarning() << child.attribute("name") << QChar(124).toLatin1() << tr("Could not create source: type OpenCV not supported.");
            errors++;
#endif
        }

        if (newsource) {
            renameSource( newsource, child.attribute("name") );
            // insert the source in the scene
            if ( insertSource(newsource) ) {
                // increment counter
                ++count;
                // Apply parameters to the created source
                errors += applySourceConfig(newsource, child, current);
            }
            else {
                qWarning() << child.attribute("name") << QChar(124).toLatin1() << tr("Could not insert source.");
                errors++;
                delete newsource;
            }
        }

        child = child.nextSiblingElement("Source");
    }

    // end loop on sources to create

    // Process the list of clones names ; now that every source exist, we can be sure they can be cloned
    QListIterator<QDomElement> it(clones);
    while (it.hasNext()) {

        Source *clonesource = 0;

        QDomElement c = it.next();
        double depth = c.firstChildElement("Depth").attribute("Z").toDouble();
        QDomElement t = c.firstChildElement("TypeSpecific");
        QDomElement f = t.firstChildElement("CloneOf");
        // find the source which name is f.text()
        SourceSet::iterator cloneof =  getByName(f.text());
        if (isValid(cloneof)) {
            clonesource = RenderingManager::_instance->newCloneSource(cloneof, depth);
            if (clonesource) {
                renameSource( clonesource, c.attribute("name") );
                // Apply parameters to the created source
                errors += applySourceConfig(clonesource, c, current);
                // insert the source in the scene
                if ( insertSource(clonesource) )  {
                    // increment counter
                    ++count;
                    // log
                    qDebug() << c.attribute("name") << QChar(124).toLatin1() << tr("Clone of source %1 created.").arg(f.text());
                }
                else {
                    errors++;
                    delete clonesource;
                }
            }
            else {
                qWarning() << c.attribute("name") << QChar(124).toLatin1() << tr("Could not create clone source.");
                errors++;
            }
        }
        else {
            qWarning() << c.attribute("name") << QChar(124).toLatin1() << tr("Cannot clone %2 ; no such source.").arg(f.text());
            errors++;
        }
    }

    // un-busy


    // set current source to none (end of list)
    unsetCurrentSource();

    // log
    qDebug() << "RenderingManager" << QChar(124).toLatin1() << tr("%1 sources created (%1/%2).").arg(count).arg(xmlconfig.childNodes().count());

    return errors;
}

standardAspectRatio doubleToAspectRatio(double ar)
{
    if ( ABS(ar - (4.0 / 3.0) ) < EPSILON )
        return ASPECT_RATIO_4_3;
    else if ( ABS(ar - (16.0 / 9.0) ) < EPSILON )
        return ASPECT_RATIO_16_9;
    else  if ( ABS(ar - (3.0 / 2.0) ) < EPSILON )
        return ASPECT_RATIO_3_2;
    else if ( ABS(ar - (16.0 / 10.0) ) < EPSILON )
        return ASPECT_RATIO_16_10;
    else
        return ASPECT_RATIO_FREE;
}

void RenderingManager::pause(bool on){

    // setup status
    paused = on;


    qDebug() << "RenderingManager" << QChar(124).toLatin1() << (on ? tr("Rendering paused.") : tr("Rendering un-paused.") );
}

void RenderingManager::onSourceFailure() {

    Source *s = qobject_cast<Source *>(sender());

    if (s) {

        QString name = s->getName();
        removeSource(s->getId());

//    qDebug() << "RenderingManager" << QChar(124).toLatin1() << "deleting source " << s->getId();


    qCritical() << name << QChar(124).toLatin1() << tr("Libav codec decoding failed. The video was closed and removed.");

    }
}


#ifdef SHM
void RenderingManager::setFrameSharingEnabled(bool on){

    if ( on == (_sharedMemory != NULL))
        return;

    // delete shared memory object
    if (_sharedMemory != NULL) {
        _sharedMemory->unlock();
        _sharedMemory->detach();
        delete _sharedMemory;
        _sharedMemory = NULL;
        qDebug() << tr("Sharing output to RAM stopped.");

    }


    if (on) {
        //
        // create shared memory descriptor
        //

        // TODO  : manage id globally to avoid using the same pid
        qint64 id = QCoreApplication::applicationPid();

        QVariantMap processInformation;
        processInformation["program"] = "GLMixer";
        processInformation["size"] = _fbo->size();
        processInformation["opengl"] = true;
        processInformation["info"] = QString("Process id %1").arg(id);
        QVariant variant = QPixmap(QString::fromUtf8(":/glmixer/icons/glmixer.png"));
        processInformation["icon"] = variant;

        QString m_sharedMemoryKey = QString("glmixer%1").arg(id);
        processInformation["key"] = m_sharedMemoryKey;

        //
        // setup format and size according to color depth
        //
        //			 glReadPixels((GLint)0, (GLint)0, (GLint) _fbo->width(), (GLint) _fbo->height(), GL_RGB, GL_UNSIGNED_BYTE, (GLvoid *) m_sharedMemory->data());
        //			 glReadPixels((GLint)0, (GLint)0, (GLint) _fbo->width(), (GLint) _fbo->height(), GL_RGB, GL_UNSIGNED_SHORT_5_6_5, (GLvoid *) m_sharedMemory->data());
        //			 glReadPixels((GLint)0, (GLint)0, (GLint) _fbo->width(), (GLint) _fbo->height(), GL_BGRA, GL_UNSIGNED_INT_8_8_8_8_REV, (GLvoid *) _sharedMemory->data());
        int shmbytecount = 0;
        switch (_sharedMemoryGLType) {
        case GL_UNSIGNED_BYTE:
            _sharedMemoryGLFormat = GL_RGB;
            processInformation["format"] = (int) QImage::Format_RGB888;
            shmbytecount = sizeof(unsigned char) * 3 * _fbo->width() * _fbo->height();
            break;
        case GL_UNSIGNED_SHORT_5_6_5:
            _sharedMemoryGLFormat = GL_RGB;
            processInformation["format"] = (int) QImage::Format_RGB16;
            shmbytecount = sizeof(unsigned short) * _fbo->width() * _fbo->height();
            break;
        case GL_UNSIGNED_INT_8_8_8_8_REV:
            _sharedMemoryGLFormat = GL_BGRA;
            processInformation["format"] = (int) QImage::Format_ARGB32;
            shmbytecount = sizeof(unsigned int) * _fbo->width() * _fbo->height();
            break;
        default:
            qWarning() << tr("Invalid format for shared memory.");
            return;
        }

        //
        // Create the shared memory
        //
        _sharedMemory = new QSharedMemory(m_sharedMemoryKey);
        if (!_sharedMemory->create( shmbytecount ) ) {
            qWarning() << tr("Unable to create shared memory:") << _sharedMemory->errorString();
            if ( !_sharedMemory->attach()) {
                 qWarning() << tr("Unable to attach shared memory:") << _sharedMemory->errorString();
                return;
            }
        }

        qDebug() << tr("Sharing output to memory enabled (%1x%2, %3 bytes).").arg(_fbo->width()).arg(_fbo->height()).arg(_sharedMemory->size());

        SharedMemoryManager::getInstance()->addItemSharedMap(id, processInformation);

    } else {

        qDebug() << tr("Sharing output to RAM disabled.");
        SharedMemoryManager::getInstance()->removeItemSharedMap(QCoreApplication::applicationPid());
    }

}


uint RenderingManager::getSharedMemoryColorDepth(){
    switch (_sharedMemoryGLType) {
    case GL_UNSIGNED_BYTE:
        return 1;
    case GL_UNSIGNED_INT_8_8_8_8_REV:
        return 2;
    default:
    case GL_UNSIGNED_SHORT_5_6_5:
        return 0;
    }
}

void RenderingManager::setSharedMemoryColorDepth(uint mode){

    switch (mode) {
    case 1:
        _sharedMemoryGLType =  GL_UNSIGNED_BYTE;
        break;
    case 2:
        _sharedMemoryGLType =  GL_UNSIGNED_INT_8_8_8_8_REV;
        break;
    default:
    case 0:
        _sharedMemoryGLType =  GL_UNSIGNED_SHORT_5_6_5;
        break;
    }

    // re-setup shared memory
    if(_sharedMemory) {
        setFrameSharingEnabled(false);
        setFrameSharingEnabled(true);
    }
}

#endif

#ifdef SPOUT

void RenderingManager::setSpoutSharingEnabled(bool on){

    _spoutEnabled = on;

    // reset if already initialized
    if (_spoutInitialized)
    {
        if ( !Spout::ReleaseSender() )
            qWarning() << tr("Could not release SPOUT sender");

        _spoutInitialized = false;
        qDebug() << tr("Sharing output to SPOUT disabled.");
    }


    if (_spoutEnabled) {

        char SenderName[256];
//        strcpy(SenderName, "GLMixer"  );
        strcpy(SenderName, QString("GLMixer%1").arg(QCoreApplication::applicationPid()).toUtf8().constData()  );
        bool spoutTextureShare = false;

        _spoutInitialized = Spout::InitSender(SenderName, _fbo->width(), _fbo->height(), spoutTextureShare, true);

        // log status
        if (_spoutInitialized)
            qDebug() << tr("Sharing output to SPOUT enabled (%1x%2, sender name '%3')").arg(_fbo->width()).arg(_fbo->height()).arg(SenderName);
        else {

            QMessageBox::warning(NULL, tr("SPOUT GLMixer"),
                                       tr("Unfortunately, SPOUT can be enabled only ONCE per execution of GLMixer.\n\n"
                                          "The SPOUT forum says 'This initial release of Spout is very basic and the concept is one thing at a time.'  and we are waiting for a more robust implementation. \n"
                                          "\nFor now, you should restart GLMixer every time you get this message.\n"));

            qCritical() << tr("Could not enable SPOUT (%1x%2, sender name '%3')").arg(_fbo->width()).arg(_fbo->height()).arg(SenderName);
            Spout::ReleaseSender();
            _spoutEnabled = false;
            emit spoutSharingEnabled(false);
        }
    }
}
#endif


QString RenderingManager::renameSource(Source *s, const QString name){

    s->setName( getAvailableNameFrom(name) );

    return s->getName();
}

#ifdef HISTORY_MANAGEMENT

HistoryManager *RenderingManager::getUndoHistory() {

    return getInstance()->_undoHistory;
}

void RenderingManager::undo() {

    // go backward in history
    _undoHistory->setCursorNextKey(HistoryManager::BACKWARD);

    // update current source displays
    emit currentSourceChanged(_currentSource);
}

void RenderingManager::redo() {

    // go forward in history
    _undoHistory->setCursorNextKey(HistoryManager::FORWARD);

    // update current source displays
    emit currentSourceChanged(_currentSource);
}

#endif

