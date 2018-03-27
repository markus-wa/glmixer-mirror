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
#include "VideoFile.h"
#include "CaptureSource.h"
#include "SvgSource.h"
#include "WebSource.h"
#include "VideoStreamSource.h"
#include "BasketSource.h"
#include "RenderingSource.h"
Source::RTTI RenderingSource::type = Source::RENDERING_SOURCE;
#include "CloneSource.h"
#include "ViewRenderWidget.h"
#include "CatalogView.h"
#include "RenderingEncoder.h"
#include "SourcePropertyBrowser.h"
#include "SessionSwitcher.h"
#include "SelectionManager.h"
#include "Tag.h"

#ifdef GLM_SHM
#include <QSharedMemory>
#include "SharedMemoryManager.h"
#include "SharedMemorySource.h"
#endif

#ifdef GLM_SPOUT
#include <Spout.h>
#include <SpoutSource.h>
#endif

#ifdef GLM_OPENCV
#include "OpencvSource.h"
#endif

#ifdef GLM_FFGL
#include "FFGLPluginSourceShadertoy.h"
#include "FFGLSource.h"
#include <FFGL.h>
#endif

#ifdef GLM_CUDA
#include "CUDAVideoFile.h"
#endif

#ifdef GLM_UNDO
#include "UndoManager.h"
#endif

#ifdef GLM_HISTORY
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

    if (glewIsSupported("GL_EXT_framebuffer_blit GL_EXT_framebuffer_multisample"))
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

    if (glewIsSupported("GL_ARB_pixel_buffer_object") )
        RenderingManager::pbo_extension = on;
    else {
        // if extension not supported but it is requested, show warning
        if (on) {
            qCritical()  << tr("OpenGL Pixel Buffer Object is requested but not supported (GL_ARB_pixel_buffer_object).\n\nDisabling Pixel Buffer Object.");
        }
        RenderingManager::pbo_extension = false;
    }

    qDebug() << "RenderingManager" << QChar(124).toLatin1() << tr("OpenGL Pixel Buffer Object (GL_ARB_pixel_buffer_object) ") << (RenderingManager::pbo_extension ? "ON" : "OFF");
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
    QObject(), _fbo(NULL), previousframe_fbo(NULL), pbo_index(0), pbo_nextIndex(0), output_frame_index(0), output_frame_period(1), previous_frame_index(0), previous_frame_period(1), clearWhite(false), renderingQuality(QUALITY_VGA), renderingAspectRatio(ASPECT_RATIO_4_3), _scalingMode(Source::SCALE_CROP), _elapsedTime(0), _playOnDrop(true), paused(false), needsUpdate(true), maxSourceCount(0)
{
    // idenfity for event
    setObjectName("RenderingManager");

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

#ifdef GLM_SHM
    _sharedMemory = NULL;
    _sharedMemoryGLFormat = GL_RGB;
    _sharedMemoryGLType = GL_UNSIGNED_SHORT_5_6_5;
#endif
#ifdef GLM_SPOUT
    _spoutEnabled = false;
    _spoutInitialized = false;
#endif

#ifdef GLM_UNDO
    UndoManager::getInstance()->connect(this, SIGNAL(methodCalled(QString)), SLOT(store(QString)));
#endif

    // elapsed clocks
    _displayTime.start();

}

RenderingManager::~RenderingManager() {
#ifdef GLM_SHM
    setFrameSharingEnabled(false);
#endif
    clearSourceSet();
    delete _defaultSource;

    if (_renderwidget)
        delete _renderwidget;

    if (_fbo)
        delete _fbo;

    if (previousframe_fbo)
        delete previousframe_fbo;

    if (pboIds[0] || pboIds[1])
        glDeleteBuffers(2, pboIds);

    if (_recorder) {
        _recorder->kill();
        delete _recorder;
    }

    if (_switcher)
        delete _switcher;


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

    // Check limits of the openGL frame buffer dimensions
    GLint maxwidth = glMaximumFramebufferWidth();;
    GLint maxheight = glMaximumFramebufferHeight();

    // init
    renderingSize = size;

    // Check limits based on openGL texture capabilities
    if (maxSourceCount == 0) {

        qDebug() << "RenderingManager" << QChar(124).toLatin1() << tr("OpenGL Maximum RGBA frame buffer resolution: ") << maxwidth << "x" << maxheight;

        // TODO : better texture atlas to avoid this limitation
        // setup the maximum texture count accordingly
        maxSourceCount = maxwidth / (CATALOG_TEXTURE_HEIGHT + 1);
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
    _fbo = new QGLFramebufferObject( qMin(size.width(), maxwidth), qMin(size.height(), maxheight));
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

#ifdef GLM_SHM
    // re-setup shared memory
    if(_sharedMemory) {
        setFrameSharingEnabled(false);
        setFrameSharingEnabled(true);
    }
#endif
#ifdef GLM_SPOUT
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

    if (_renderwidget->_catalogView->visible() ) {
        // finalize catalog view
        _renderwidget->_catalogView->reorganize();
    }

    if (!_fbo)
        return;

    // setup standard state for texturing (used bellow)
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    gluOrtho2D(-1.0, 1.0, -1.0, 1.0);

    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();

    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glBlendEquation(GL_FUNC_ADD);
    glEnable(GL_TEXTURE_2D);
    glActiveTexture(GL_TEXTURE0);


    // Draw recursive loop back (skip if no rendering source)
    if (_rendering_sources.size() > 0 && !paused)
    {
        // frame delay
        previous_frame_index++;
        if (!(previous_frame_index % previous_frame_period))
        {
            previous_frame_index = 0;
            // use the accelerated GL_EXT_framebuffer_blit if available
            if (RenderingManager::blit_fbo_extension)
            {
                glBindFramebuffer(GL_READ_FRAMEBUFFER, _fbo->handle());
                glBindFramebuffer(GL_DRAW_FRAMEBUFFER, previousframe_fbo->handle());

                glBlitFramebuffer(0, _fbo->height(), _fbo->width(), 0, 0, 0,
                                  previousframe_fbo->width(), previousframe_fbo->height(),
                                  GL_COLOR_BUFFER_BIT, GL_NEAREST);

                glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
            }
            // 	Draw quad with fbo texture in a more basic OpenGL way
            else if (previousframe_fbo->bind()) {
                glPushAttrib(GL_COLOR_BUFFER_BIT | GL_VIEWPORT_BIT);

                // render to the frame buffer object
                glViewport(0, 0, previousframe_fbo->width(), previousframe_fbo->height());
                glColor4f(1.f, 1.f, 1.f, 1.f);
                glBindTexture(GL_TEXTURE_2D, _fbo->texture());
                glCallList(ViewRenderWidget::quad_texured);

                glPopAttrib();
                previousframe_fbo->release();
            }
        }
    }

    // At last, draw the session switcher overlay on the frame buffer
    if ( (_switcher->alpha()>0 || _switcher->overlay()>0 ) && _fbo->bind())
    {
        glPushAttrib(GL_COLOR_BUFFER_BIT | GL_VIEWPORT_BIT);
        glViewport(0, 0, _renderwidget->_renderView->viewport[2], _renderwidget->_renderView->viewport[3]);

        // render the transition layer on _fbo
        _switcher->render();

        glPopAttrib();
        _fbo->release();
    }

    // save the frame to file or copy to SHM
    if ( _recorder->isRecording()
     #ifdef GLM_SHM
         || _sharedMemory != NULL
     #endif
         ) {


#ifdef RECORDING_READ_PIXEL
        // bind fbo context for ReadPixel
        glBindFramebuffer(GL_READ_FRAMEBUFFER, _fbo->handle());
#else
        // read texture from the framebuferobject and record this frame (the recorder knows if it is active or not)
        glBindTexture(GL_TEXTURE_2D, _fbo->texture());
#endif

        // use pixel buffer object if initialized
        if (pboIds[0] && pboIds[1]) {

            // bind a PBO for asynchronous get of buffer
            glBindBuffer(GL_PIXEL_PACK_BUFFER, pboIds[pbo_index]);

#ifdef RECORDING_READ_PIXEL
            // ReadPixel of _fbo
            glReadPixels(0, 0, _fbo->width(), _fbo->height(), GL_BGRA, GL_UNSIGNED_BYTE, nullptr);
#else
            // read pixels from texture
            glGetTexImage(GL_TEXTURE_2D, 0, GL_BGRA, GL_UNSIGNED_BYTE, 0);
#endif

            // map the other PBO to process its data by CPU
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


#ifdef GLM_SHM
        // share to memory if needed
        if (_sharedMemory != NULL) {

            _sharedMemory->lock();

            // read the pixels from the texture
            glGetTexImage(GL_TEXTURE_2D, 0, _sharedMemoryGLFormat, _sharedMemoryGLType, (GLvoid *) _sharedMemory->data());

            _sharedMemory->unlock();
        }
#endif // SHM

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

#ifdef GLM_SPOUT

    if ( _spoutInitialized ) {

        Spout::SendTexture( _fbo->texture(), GL_TEXTURE_2D, _fbo->width(), _fbo->height());
    }

#endif // SPOUT

    // restore state
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
    glPopMatrix();
    glDisable(GL_TEXTURE_2D);

    needsUpdate = false;
}

void RenderingManager::preRenderToFrameBuffer()
{
    // create frame buffer if not existing
    if (!_fbo)
        setFrameBufferResolution( sizeOfFrameBuffer[renderingAspectRatio][renderingQuality] );

    glPushAttrib( GL_COLOR_BUFFER_BIT );

    // clear frame buffer
    if (!paused)
    {
        // frame delay
        if (!(++output_frame_index % output_frame_period))
        {
            output_frame_index = 0;

            if (_fbo && _fbo->bind())
            {
                if (clearWhite)
                    glClearColor(1.f, 1.f, 1.f, 1.f);
                else
                    glClearColor(0.f, 0.f, 0.f, 1.f);
                glClear(GL_COLOR_BUFFER_BIT);

                _fbo->release();
            }

            needsUpdate = true;
        }
    }

    // clear catalog
    if (_renderwidget->_catalogView->visible() )
        _renderwidget->_catalogView->clear();

    glPopAttrib();
}

void RenderingManager::sourceRenderToFrameBuffer(Source *source) {

    if (!source)
        return;

    glPushAttrib(GL_COLOR_BUFFER_BIT | GL_VIEWPORT_BIT);

    glViewport(0, 0, _renderwidget->_renderView->viewport[2], _renderwidget->_renderView->viewport[3]);

    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadMatrixd(_renderwidget->_renderView->projection);

    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();

    if (needsUpdate)
    {
        // render to the frame buffer object
        if (_fbo && _fbo->bind())
        {
            //
            // 1. Draw into first texture attachment; the final output rendering
            //

            // draw the source only if not culled and alpha not null
            if (!source->isCulled() && source->getAlpha() > 0.0) {
                glTranslated(source->getX(), source->getY(), 0.0);
                glRotated(source->getRotationAngle(), 0.0, 0.0, 1.0);
                glScaled(source->getScaleX(), source->getScaleY(), 1.f);

                source->blend();
                source->draw();
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

        // Draw this source into the catalog
        _renderwidget->_catalogView->drawSource( source );

    }

    // pop the projection matrix and GL state back for rendering the current view
    // to the actual widget
    glPopAttrib();
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
    glPopMatrix();
}

Source *RenderingManager::newRenderingSource(bool recursive, double depth) {

#ifndef NDEBUG
    qDebug() << tr("RenderingManager::newRenderingSource ")<< depth << recursive;
#endif

    RenderingSource *s = 0;
    _renderwidget->makeCurrent();

    try {
        // create a source appropriate
        s = new RenderingSource(recursive, getAvailableDepthFrom(depth));
        s->setName( _defaultSource->getName() + "Loop");

    } catch (AllocationException &e){
        qWarning() << tr("Cannot create Loopback source; ") << e.message();
        // return an invalid pointer
        s = 0;
    }

    return ( (Source *) s );
}

QImage RenderingManager::captureFrameBuffer() {

    QImage img = _fbo ? _fbo->toImage() : QImage();

    return img;
}


Source *RenderingManager::newSvgSource(QByteArray content, double depth){

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
        s = new SvgSource(content, textureIndex, getAvailableDepthFrom(depth));
        s->setName(_defaultSource->getName() + "Svg");

        // connect to error
        QObject::connect(s, SIGNAL(failed()), this, SLOT(onSourceFailure()));

    } catch (AllocationException &e){
        qWarning() << tr("Cannot create SVG source; ") << e.message();
        // free the OpenGL texture
        glDeleteTextures(1, &textureIndex);
        // return an invalid pointer
        s = 0;
    }

    return ( (Source *) s );
}

Source *RenderingManager::newWebSource(QUrl web, int w, int h, int height, int scroll, int update, double depth){
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
        s = new WebSource(web, textureIndex, getAvailableDepthFrom(depth), w, h, height, scroll, update);
        s->setName(_defaultSource->getName() + "Web");

        // connect to error
        QObject::connect(s, SIGNAL(failed()), this, SLOT(onSourceFailure()));

    } catch (AllocationException &e){
        qWarning() << tr("Cannot create Web source; ") << e.message();
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
        s->setName(_defaultSource->getName() + "Pixmap");

        // connect to error
        QObject::connect(s, SIGNAL(failed()), this, SLOT(onSourceFailure()));

    } catch (AllocationException &e){
        qWarning() << tr("Cannot create Pixmap source; ") << e.message();
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
        s = new VideoSource( vf, textureIndex, getAvailableDepthFrom(depth) );
        s->setName(_defaultSource->getName() + QDir(vf->getFileName()).dirName().split(".").first());

        // connect to error
        QObject::connect(s, SIGNAL(failed()), this, SLOT(onSourceFailure()));

    } catch (AllocationException &e){
        qWarning() << tr("Cannot create Media source; ") << e.message();
        // free the OpenGL texture
        glDeleteTextures(1, &textureIndex);
        // return an invalid pointer
        s = 0;
    }

    return ( (Source *) s );
}

#ifdef GLM_OPENCV
Source *RenderingManager::newOpencvSource(int opencvIndex, int mode, double depth) {

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
        OpencvSource::CameraMode m = (OpencvSource::CameraMode) CLAMP(mode, OpencvSource::DEFAULT_MODE, OpencvSource::HIGHRES_MODE );
        s = new OpencvSource(opencvIndex, m, textureIndex, getAvailableDepthFrom(depth));
        s->setName(_defaultSource->getName() + QString("Camera%1").arg(opencvIndex) );

        // connect to error
        QObject::connect(s, SIGNAL(failed()), this, SLOT(onSourceFailure()));

    } catch (AllocationException &e){
        qWarning() << tr("Cannot create OpenCV source; ") << e.message();
        // free the OpenGL texture
        glDeleteTextures(1, &textureIndex);
        // return an invalid pointer
        s = 0;
    }

    return ( (Source *) s );
}
#endif

#ifdef GLM_FFGL

FFGLPluginSource *RenderingManager::newFreeframeGLPlugin(int width, int height, FFGLTextureStruct it, QString filename)
{
    FFGLPluginSource *ffgl_plugin = NULL;

    // enable current opengl context
    _renderwidget->makeCurrent();

    // Shadertoy (no filename given)
    if (filename.isEmpty()) {

        ffgl_plugin = (FFGLPluginSource *) new FFGLPluginSourceShadertoy(FF_EFFECT, width, height, it);
    }
    // FreeFrame (from file)
    else {

        // create new plugin with this file
        ffgl_plugin = new FFGLPluginSource(width, height, it);

        // load dll
        ffgl_plugin->load(filename);
    }

    return ffgl_plugin;
}

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
                        s->setName(_defaultSource->getName() + QString("Freeframe") );

                        // connect to error
                        QObject::connect(s, SIGNAL(failed()), this, SLOT(onSourceFailure()));

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
                    qCritical() << fileNameToOpen << QChar(124).toLatin1() << tr("Could no create GPU plugin source.");
                    // free the OpenGL texture
                    glDeleteTextures(1, &textureIndex);
                    // return an invalid pointer
                    s = 0;
                }

            }
            else
                qCritical() << fileNameToOpen << QChar(124).toLatin1() << tr("File does not exist.");

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
                    s->setName(_defaultSource->getName() + QString("Shadertoy") );

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
                qCritical() << "shadertoy" << QChar(124).toLatin1() << tr("Could no create GPU plugin source.");
                // free the OpenGL texture
                glDeleteTextures(1, &textureIndex);
                // return an invalid pointer
                s = 0;
            }

        }

    }

    if (s)
        s->update();

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
        s->setName(_defaultSource->getName() + tr("Algo"));

        // connect to error
        QObject::connect(s, SIGNAL(failed()), this, SLOT(onSourceFailure()));

    } catch (AllocationException &e){
        qWarning() << tr("Cannot create Algorithm source; ") << e.message();
        // free the OpenGL texture
        glDeleteTextures(1, &textureIndex);
        // return an invalid pointer
        s = 0;
    }

    return ( (Source *) s );
}

#ifdef GLM_SHM
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

        // connect to error
        QObject::connect(s, SIGNAL(failed()), this, SLOT(onSourceFailure()));

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

#ifdef GLM_SPOUT
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

        // connect to error
        QObject::connect(s, SIGNAL(failed()), this, SLOT(onSourceFailure()));

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

Source *RenderingManager::newStreamSource(VideoStream *vs, double depth) {

    VideoStreamSource *s = 0;
    // create the texture for this source
    GLuint textureIndex;
    _renderwidget->makeCurrent();
    glGenTextures(1, &textureIndex);
    GLclampf lowpriority = 0.1;
    glPrioritizeTextures(1, &textureIndex, &lowpriority);

    try {
        // create a source appropriate
        s = new VideoStreamSource(vs, textureIndex, getAvailableDepthFrom(depth) );
        s->setName(_defaultSource->getName() + vs->getUrl());

        // connect to error
        QObject::connect(s, SIGNAL(failed()), this, SLOT(onSourceFailure()));

    } catch (AllocationException &e){
        qWarning() << tr("Cannot create Network Stream source; ") << e.message();
        // free the OpenGL texture
        glDeleteTextures(1, &textureIndex);
        // return an invalid pointer
        s = 0;
    }

    return ( (Source *) s );
}


Source *RenderingManager::newBasketSource(QStringList files, int w, int h, int p, bool bidir, bool shuf, QString playlist, double depth){

    _renderwidget->makeCurrent();

    BasketSource *s = 0;
    try {
        // create a source appropriate
        s = new BasketSource(files, getAvailableDepthFrom(depth), w, h, (qint64) p);
        s->setName(_defaultSource->getName() + "Basket");

        // set playlist
        if (!playlist.isEmpty())
            s->setPlaylistString(playlist);
        s->setBidirectional(bidir);
        s->setShuffle(shuf);

    } catch (AllocationException &e){
        qWarning() << tr("Cannot create Basket source; ") << e.message();
        // return an invalid pointer
        s = 0;
    }

    return ( (Source *) s );
}


Source *RenderingManager::newCloneSource(SourceSet::iterator sit, double depth) {

    CloneSource *s = 0;
    try{
        // create a source
        s = new CloneSource(sit, getAvailableDepthFrom(depth));

        if ((*sit)->rtti() == Source::CLONE_SOURCE) {
            //  if its a clone already, do not add suffix clone
            s->setName((*sit)->getName());
        } else
            s->setName((*sit)->getName() + tr("Clone"));


    } catch (AllocationException &e){
        qWarning() << tr("Cannot clone source; ") << e.message();
        // return an invalid pointer
        s = 0;
    }

    return ( (Source *) s );
}


bool RenderingManager::insertSource(Source *s)
{
    if (!s) return false;

    // set Workspace
    s->setWorkspace( WorkspaceManager::getInstance()->current() );

    // inform undo manager
    emit methodCalled(QString("_insertSource(%1)").arg(s->getId()));

    return _insertSource(s);
}

bool RenderingManager::_insertSource(Source *s)
{
    if (s) {

        if (_front_sources.size() < maxSourceCount) {

            //insert the source to the list
            if (_front_sources.insert(s).second) {

//#ifdef GLM_HISTORY
//                // connect source to the history manager
//                HistoryManager::getInstance()->connect(s, SIGNAL(methodCalled(QString, QVariantPair, QVariantPair, QVariantPair, QVariantPair, QVariantPair)), SLOT(rememberEvent(QString, QVariantPair, QVariantPair, QVariantPair, QVariantPair, QVariantPair)));
//#endif

#ifdef GLM_UNDO
                // connect source to the undo manager
                UndoManager::getInstance()->connect(s, SIGNAL(methodCalled(QString, QVariantPair, QVariantPair, QVariantPair, QVariantPair, QVariantPair, QVariantPair, QVariantPair)), SLOT(store(QString)), Qt::UniqueConnection);
#endif
#ifdef GLM_TAG
                // set tag (get(s) returns default tag if none was set)
                Tag::get(s)->set(s);
#endif
                // inform of success
                return true;
            }
            else
                qWarning() << tr("No more space in depth stack to add this source (%1).").arg(_front_sources.size());

        }
        else
            qWarning() << tr("The maximum amount of source supported is reached (%1).").arg(maxSourceCount);
    }

    return false;
}

void RenderingManager::addSourceToBasket(Source *s)
{
    if (!s) return;

    // add the source into the basket
    dropBasket.insert(s);

    // apply default parameters
    s->importProperties(_defaultSource);
    // scale the source to match the preferences
    s->resetScale(_scalingMode);

    // select no source
    unsetCurrentSource();
}

void RenderingManager::clearBasket()
{
    qDebug() << tr("Canceled drop of %1 source(s).").arg(dropBasket.size());

    for (SourceList::iterator sit = dropBasket.begin(); sit != dropBasket.end(); sit = dropBasket.begin()) {
        dropBasket.erase(sit);
        delete (*sit);
    }
}

void RenderingManager::resetSource(SourceSet::iterator sit){

    // inform undo manager
    emit methodCalled(QString("_resetSource(%1)").arg((*sit)->getId()));

    // apply default parameters
    (*sit)->importProperties(_defaultSource);
    // scale the source to match the preferences
    (*sit)->resetScale(_scalingMode);
#ifdef GLM_FFGL
    // clear plugins
    (*sit)->clearFreeframeGLPlugin();
#endif
    // reset tag
    Tag::getDefault()->set(*sit);
    // inform GUI
    emit currentSourceChanged(sit);
}

bool RenderingManager::setWorkspaceCurrentSource(int w)
{
    if(isValid(_currentSource)) {

        // bound value
        w = qBound(0, w, WorkspaceManager::getInstance()->count() - 1);

        // ensure we move a Mixing group together
        _renderwidget->selectWholeGroup(*_currentSource);

        // move selection to workspace, if exists
        if (SelectionManager::getInstance()->isInSelection(*_currentSource))
        {
            for(SourceList::iterator  its = SelectionManager::getInstance()->selectionBegin(); its != SelectionManager::getInstance()->selectionEnd(); its++) {
                (*its)->setWorkspace(w);
            }
        }
        // move the source only, and cleanup selection
        else {
            (*_currentSource)->setWorkspace(w);
            SelectionManager::getInstance()->clearSelection();
        }

        return true;
    }

    return false;
}


void RenderingManager::setWorkspaceCount(int w)
{
    for (SourceSet::iterator its = _front_sources.begin(); its != _front_sources.end(); its++) {
        if ( (*its)->getWorkspace() > w - 1 )
            (*its)->setWorkspace(w - 1);
    }
}


void RenderingManager::setWorkspaceAllSources()
{
    int w = WorkspaceManager::getInstance()->current();

    for (SourceSet::iterator its = _front_sources.begin(); its != _front_sources.end(); its++) {
        if ( (*its)->getWorkspace() != w )
            (*its)->setWorkspace(w);
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

void RenderingManager::refreshCurrentSource(){

    if(notAtEnd(_currentSource)) {
        emit currentSourceChanged(_currentSource);
    }

    SelectionManager::getInstance()->updateSelectionSource();
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
            // inform of source drop
            emit sourceDropped(top);
            // inform of change
            emit countSourceChanged(_front_sources.size());
        }
        else
            delete top;
    }
}


void RenderingManager::dropReplaceSource(SourceSet::iterator itoldsource) {

    unsetCurrentSource();

    // something to drop ? something to replace ?
    if (!dropBasket.empty() && RenderingManager::getInstance()->isValid(itoldsource)) {

        // get the pointer to the source at the top of the drop basket
        Source *newsource = *dropBasket.begin();
        // remove from the basket
        dropBasket.erase(newsource);

        // prevent re-creation of clone source (which is useless
        // and provokes recursive self referencing)
        if (newsource->rtti() == Source::CLONE_SOURCE) {
            delete newsource;
            return;
        }

        // get the pointer the the source to remove
        Source *oldsource = *itoldsource;
        // inform undo manager
        emit methodCalled(QString("_dropReplaceSource(%1)").arg(oldsource->getId()));

        // CLONE ALL PROPERTIES AND OPTIONS
        //
        // apply all properties
        newsource->importProperties( oldsource );
#ifdef GLM_FFGL
        // copy the Freeframe plugin stack
        newsource->reproduceFreeframeGLPluginStack( oldsource );
#endif
        // change all clones of old source to clone the new source
        try{
            for (SourceList::iterator clone = oldsource->getClones()->begin(); clone != oldsource->getClones()->end(); clone = oldsource->getClones()->begin()) {
                CloneSource *tmp = dynamic_cast<CloneSource *>(*clone);
                if (tmp) {
                    // change original of clone (this removes it from list of old source clones)
                    tmp->setOriginal(newsource);
                    tmp->_setName(newsource->getName() + tr("Clone"));
                }
            }
        } catch (AllocationException &e){
            qWarning() << tr("Cannot clone source; ") << e.message();
        }

        //
        // DONE CLONE PROPERTIES

        // set the depth already; will be used when inserting the source
        newsource->setDepth(oldsource->getDepth());
        // delete old source (and eventual plugins)
        QString name_oldsource = oldsource->getName();
        bool play = oldsource->isPlaying();
        _removeSource(itoldsource);
        // insert the source (will use the depth of the source)
        if ( _insertSource(newsource) ) {
            // start playing (according to previous state)
            newsource->play(play);
            // make it current
            setCurrentSource(newsource->getId());
            // log
            qDebug() << name_oldsource  << QChar(124).toLatin1() << tr("Source replaced by %1").arg(newsource->getName());
        }
        else
            delete newsource;
    }
}


void RenderingManager::replaceSource(GLuint oldsource, GLuint newsource) {

    SourceSet::iterator it_oldsource = getById(oldsource);
    SourceSet::iterator it_newsource = getById(newsource);

    if ( notAtEnd(it_oldsource) && notAtEnd(it_newsource)) {

        // inform undo manager
        emit methodCalled(QString("_replaceSource(%1,%2)").arg(oldsource).arg(newsource));

#ifdef GLM_UNDO
        // suspend
        UndoManager::getInstance()->suspend(true);
#endif

        double depth_oldsource = (*it_oldsource)->getDepth();
        QString name_oldsource = (*it_oldsource)->getName();

        // apply former parameters
        (*it_newsource)->importProperties(*it_oldsource);

        // change all clones of old source to clone the new source
        try {
            for (SourceList::iterator clone = (*it_oldsource)->getClones()->begin(); clone != (*it_oldsource)->getClones()->end(); clone = (*it_oldsource)->getClones()->begin()) {
                CloneSource *tmp = dynamic_cast<CloneSource *>(*clone);
                if (tmp) {
                    // change original of clone (this removes it from list of old source clones)
                    tmp->setOriginal(it_newsource);
                }
            }
        } catch (AllocationException &e){
            qWarning() << tr("Cannot clone source; ") << e.message();
        }

#ifdef GLM_FFGL
        // copy the Freeframe plugin stack
        (*it_newsource)->reproduceFreeframeGLPluginStack( (*it_oldsource) );
#endif

        // delete old source (and eventual plugins)
        removeSource(it_oldsource);

        // restore former depth
        changeDepth(it_newsource, depth_oldsource);

        // log
        qDebug() << name_oldsource  << QChar(124).toLatin1() << tr("Source replaced by %1").arg((*it_newsource)->getName());

#ifdef GLM_UNDO
        // unsuspend
        UndoManager::getInstance()->suspend(false);
#endif
    }

}


int RenderingManager::removeSource(SourceSet::iterator itsource)
{
    // inform undo manager
    emit methodCalled(QString("_removeSource(%1)").arg((*itsource)->getId()));

    // remove source
    int ret = _removeSource(itsource);

    // inform of change
    emit countSourceChanged(_front_sources.size());

    return ret;
}

int RenderingManager::_removeSource(const GLuint idsource){

    return _removeSource(getById(idsource));
}

int RenderingManager::_removeSource(SourceSet::iterator itsource) {

    if (!isValid(itsource)) {
        qWarning() << tr("Invalid Source cannot be deleted.");
        return 0;
    }

    // remove from selection and group
    _renderwidget->removeFromSelections(*itsource);

    // if we are removing the current source, ensure it is not the current one anymore
    if (itsource == _currentSource) {
        (*_currentSource)->disconnect();
        _currentSource = _front_sources.end();
        emit currentSourceChanged(_currentSource);
    }

    int num_sources_deleted = 0;
    if (itsource != _front_sources.end()) {
        Source *s = *itsource;
        // remove tag
        Tag::remove(s);
        // if this is not a clone
        if (s->rtti() != Source::CLONE_SOURCE)
            // remove every clone of the source to be removed
            for (SourceList::iterator clone = s->getClones()->begin(); clone != s->getClones()->end(); clone = s->getClones()->begin()) {
                num_sources_deleted += _removeSource((*clone)->getId());
            }
        // then remove the source itself
        qDebug() << s->getName() << QChar(124).toLatin1() << tr("Delete source.");
        _front_sources.erase(itsource);

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
            num_sources_deleted += _removeSource(its);

        qDebug() << "RenderingManager" << QChar(124).toLatin1() << tr("All sources cleared (%1/%2)").arg(num_sources_deleted).arg(total);
    }

    // cleanup VideoPicture memory map
    VideoPicture::clearPictureMaps();

#ifdef GLM_UNDO
    // cleanup & reactivate Undo Manager
    UndoManager::getInstance()->clear();
#endif

    // clear tag
    Tag::remove();

    // restore default Workspaces
    WorkspaceManager::getInstance()->setCount();
    WorkspaceManager::getInstance()->setCurrent(0);

    // restart elapsed clock
    _displayTime.start();

    // inform of change
    emit countSourceChanged(_front_sources.size());
}

bool RenderingManager::notAtEnd(SourceSet::const_iterator itsource)  const{

    return (itsource != _front_sources.end());
}

bool RenderingManager::isValid(SourceSet::const_iterator itsource)  const {

    // basic check
    if (notAtEnd(itsource) && (*itsource) != NULL)
        // a source is valid if it is in the _front_sources list
        // (there are other sources and other lists of source)
       return (_front_sources.find(*itsource) != _front_sources.end());
    else
        return false;
}

QStringList RenderingManager::getSourceNameList() const {
    QStringList list;
    for(SourceList::iterator  its = getBegin(); its != getEnd(); its++)
        list << (*its)->getName();
    return list;
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
        // disconnect current source from play actions
        if ( isValid(_currentSource) )
            (*_currentSource)->disconnect(SIGNAL(playing(bool)));

        // set current source
        _currentSource = si;

        // switch to workspace of current source
        if (_currentSource != _front_sources.end()) {

            // change only if workspace is different
            int ws = (*_currentSource)->getWorkspace();
            if ( ws != WorkspaceManager::getInstance()->current() ) {
                WorkspaceManager::getInstance()->setCurrent(ws);

                // if changing workspace without current source, clear selection
                if ( !SelectionManager::getInstance()->isInSelection(*_currentSource))
                    SelectionManager::getInstance()->clearSelection();
            }
        }

        emit currentSourceChanged(_currentSource);
    }
}

void RenderingManager::setCurrentSource(GLuint id)
{

    setCurrentSource(getById(id));
}

void RenderingManager::updateCurrentSource()
{
    if ( isValid(_currentSource) )
        emit currentSourceChanged(_currentSource);
}


void RenderingManager::setCurrentNext(){

    if (_front_sources.empty() )
        return;

    SourceSet::iterator next = std::next(_currentSource, 1);
    // if at the end, go to the begining
    if (next == _front_sources.end())
        next = _front_sources.begin();

    setCurrentSource(next);
}

void RenderingManager::setCurrentPrevious(){

    if (_front_sources.empty() )
        return;

    SourceSet::iterator previous;

    // if at the begining, go to the end
    if (_currentSource == _front_sources.begin())
        previous = std::prev(_front_sources.end(), 1);
    else
        previous = std::prev(_currentSource, 1);

    setCurrentSource(previous);
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

    double foundDepth = depth;

    // place it at the front if no depth is provided (default argument = -1)
    if (foundDepth < 0.0) {
        foundDepth  = (_front_sources.empty()) ? 0.0 : (*_front_sources.rbegin())->getDepth() + DEPTH_DEFAULT_SPACING;

        foundDepth += dropBasket.size();
    }
    // a depth is given, try to place the source at this location
    else {

        // try to find a source at this depth in the list; it is not ok if it exists
        bool isok = false;
        while (!isok) {
            if ( isValid( std::find_if(_front_sources.begin(), _front_sources.end(), isCloseTo(foundDepth)) ) ){
                foundDepth += DEPTH_EPSILON;
            } else
                isok = true;
        }
    }

    // make some room if we hit the maximum depth
    if (foundDepth > MAX_DEPTH_LAYER) {

        // compute the depth between sources to uniformly distribute them
        double d_inc = MAX_DEPTH_LAYER / (double)(_front_sources.size() + dropBasket.size() + 1 );

        // if this is not a problem for distinguishing depth
        if (d_inc > DEPTH_EPSILON)
        {
            // uniformly space all depths of sources
            double d = 0;
            for (SourceSet::iterator it = _front_sources.begin(); it != _front_sources.end(); it++, d+=d_inc)
                (*it)->setDepth(d);

            for (SourceList::iterator it = dropBasket.begin(); it != dropBasket.end(); it++, d+=d_inc)
                (*it)->setDepth(d);

            foundDepth = d;
        }
        else
        // else the foundDepth will be invalid
            AllocationException().raise();
    }

    // eventually the tentative depth is ok
    return foundDepth;
}


SourceSet::iterator RenderingManager::changeDepth(SourceSet::iterator itsource,
                                                  double newdepth) {

    // ignore invalid iterator
    if (itsource != _front_sources.end()) {

        // clamp values
        newdepth = CLAMP( newdepth, MIN_DEPTH_LAYER, MAX_DEPTH_LAYER);

        // ignore if no change required
        if ( ABS(newdepth - (*itsource)->getDepth()) < EPSILON)
            return itsource;

        // inform undo
        emit methodCalled(QString("_changeDepth(%1,double)").arg((*itsource)->getId()));

        // verify that the depth value is not already taken, or too close to, and adjust in case.
        SourceSet::iterator sb, se;
        double depthinc = 0.0;
        if (newdepth < (*itsource)->getDepth()) {
            sb = _front_sources.begin();
            se = itsource;
            depthinc = -DEPTH_EPSILON * 2.0;
        } else {
            sb = itsource;
            sb++;
            se = _front_sources.end();
            depthinc = DEPTH_EPSILON * 2.0;
        }
        while (std::find_if(sb, se, isCloseTo(newdepth)) != se) {
            newdepth += depthinc;
        }

        // if the action requires to push the source to ZERO
        if (newdepth < 0) {
            // ignore and return it
            return itsource;
        }

        // General case
        // Lookup for the source just before and source just after
        SourceSet::iterator before = _front_sources.begin();
        SourceSet::iterator after = _front_sources.begin();
        SourceSet::iterator i = _front_sources.begin();
        while (i != _front_sources.end()) {
            if ( i == itsource ) {
                after++;
                break;
            }
            if (i != _front_sources.begin())
                before++;
            after++;
            i++;
        }

        // Test if the order with the source before and after would change with the new depth
        bool needreordering = false;
        // if there is a source before, and the new depth would be lower
        if (before != _front_sources.end() && before != itsource && newdepth < (*before)->getDepth()) {
            // have to swap the source order
            needreordering = true;
        }
        // if there is a source after, and the new depth would be higher
        if(after != _front_sources.end() && newdepth > (*after)->getDepth()) {
            // have to swap the source order
            needreordering = true;
        }

        // We need to reorder the set by depth when changing the depth of the source
        // this is done by removing the element and adding it again after changing its depth
        if (needreordering) {

            // make sure we do not keep the _current_cource iterator to an invalid value.
            unsetCurrentSource();

            // remember pointer to the source
            Source *tmp = (*itsource);
            // remove the element
            _front_sources.erase(itsource);
            // change the source internal depth value
            tmp->setDepth(newdepth);
            // re-insert the source into the sorted list ; it will be placed according to its new depth
            std::pair<SourceSet::iterator, bool> ret;
            ret = _front_sources.insert(tmp);
            // inform of success
            if (ret.second)
                return (ret.first);
            else
                return (_front_sources.end()); // should never happen
        }

        // no reordering necessary, just change the source internal depth value
        (*itsource)->setDepth(newdepth);
        // and return it
        return itsource;
    }

    // error
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

SourceSet::const_iterator RenderingManager::getById(const GLuint id) const {

    return std::find_if(_front_sources.begin(), _front_sources.end(), hasId(id));
}

SourceSet::iterator RenderingManager::getByName(const QString name) {

    return std::find_if(_front_sources.begin(), _front_sources.end(), hasName(name));
}

SourceSet::const_iterator RenderingManager::getByName(const QString name) const {

    return std::find_if(_front_sources.begin(), _front_sources.end(), hasName(name));
}

/**
 * save and load configuration
 */

QDomElement RenderingManager::getSourceConfiguration(SourceSet::iterator its, QDomDocument &doc, QDir current) {

    QDomElement sourceElem;

    // type specific settings
    if ((*its)->rtti() == Source::VIDEO_SOURCE) {
        VideoSource *vs = dynamic_cast<VideoSource *> (*its);
        sourceElem = vs->getConfiguration(doc, current);
    }
    else if ((*its)->rtti() == Source::ALGORITHM_SOURCE) {
        AlgorithmSource *as = dynamic_cast<AlgorithmSource *> (*its);
        sourceElem = as->getConfiguration(doc, current);
    }
    else if ((*its)->rtti() == Source::CAPTURE_SOURCE) {
        CaptureSource *cs = dynamic_cast<CaptureSource *> (*its);
        sourceElem = cs->getConfiguration(doc, current);
    }
    else if ((*its)->rtti() == Source::CLONE_SOURCE) {
        CloneSource *cs = dynamic_cast<CloneSource *> (*its);
        sourceElem = cs->getConfiguration(doc, current);
    }
    else if ((*its)->rtti() == Source::SVG_SOURCE) {
        SvgSource *svgs = dynamic_cast<SvgSource *> (*its);
        sourceElem = svgs->getConfiguration(doc, current);
    }
    else if ((*its)->rtti() == Source::WEB_SOURCE) {
        WebSource *ws = dynamic_cast<WebSource *> (*its);
        sourceElem = ws->getConfiguration(doc, current);
    }
#ifdef GLM_OPENCV
    else if ((*its)->rtti() == Source::CAMERA_SOURCE) {
        OpencvSource *cs = dynamic_cast<OpencvSource *> (*its);
        sourceElem = cs->getConfiguration(doc, current);
    }
#endif
#ifdef GLM_SHM
    else if ((*its)->rtti() == Source::SHM_SOURCE) {
        SharedMemorySource *shms = dynamic_cast<SharedMemorySource *> (*its);
        sourceElem = shms->getConfiguration(doc, current);
    }
#endif
#ifdef GLM_SPOUT
    else if ((*its)->rtti() == Source::SPOUT_SOURCE) {
        SpoutSource *spouts = dynamic_cast<SpoutSource *> (*its);
        sourceElem = spouts->getConfiguration(doc, current);
    }
#endif
#ifdef GLM_FFGL
    else if ((*its)->rtti() == Source::FFGL_SOURCE) {
        FFGLSource *ffs = dynamic_cast<FFGLSource *> (*its);
        sourceElem = ffs->getConfiguration(doc, current);
    }
#endif
    else if ((*its)->rtti() == Source::STREAM_SOURCE) {
        VideoStreamSource *sts = dynamic_cast<VideoStreamSource *> (*its);
        sourceElem = sts->getConfiguration(doc, current);
    }
    else if ((*its)->rtti() == Source::RENDERING_SOURCE) {
        RenderingSource *rs = dynamic_cast<RenderingSource *> (*its);
        sourceElem = rs->getConfiguration(doc, current);
    }
    else if ((*its)->rtti() == Source::BASKET_SOURCE) {
        BasketSource *bs = dynamic_cast<BasketSource *> (*its);
        sourceElem = bs->getConfiguration(doc, current);
    }
    else
        sourceElem = (*its)->getConfiguration(doc, current);

    return sourceElem;
}

QDomElement RenderingManager::getConfiguration(QDomDocument &doc, QDir current) {

    QDomElement config = doc.createElement("SourceList");

    for (SourceSet::iterator its = _front_sources.begin(); its != _front_sources.end(); its++) {
        config.appendChild( getSourceConfiguration(its, doc, current));
    }

    return config;
}


int RenderingManager::addSourceConfiguration(QDomElement child, QDir current, QString version)
{
    // counter of errors
    int errors = 0;
    // pointer for new source
    Source *newsource = 0;

    // read the depth where the source should be created
    double depth = child.firstChildElement("Depth").attribute("Z", "0").toDouble();

    // get the type of the source to create
    QDomElement t = child.firstChildElement("TypeSpecific");
    Source::RTTI type = (Source::RTTI) t.attribute("type").toInt();

    if (type == Source::VIDEO_SOURCE )
    {
        // read the tags specific for a video source
        QDomElement Filename = t.firstChildElement("Filename");
        QDomElement marks = t.firstChildElement("Marks");

        // first reads with the absolute file name
        QString fileNameToOpen = Filename.text();
        // if there is no such file, try generate a file name from the relative file name
        if (!QFileInfo(fileNameToOpen).exists())
            fileNameToOpen = current.absoluteFilePath( Filename.attribute("Relative", "") );
        // if there is such a file
        if (QFileInfo(fileNameToOpen).exists()) {

            // create the video file
            VideoFile *newSourceVideoFile = NULL;

            // generate texture of size in power of two if required
            bool power2 = false;
            if ( Filename.attribute("PowerOfTwo","0").toInt() > 0
                 || !( glewIsSupported("GL_EXT_texture_non_power_of_two")
                       || glewIsSupported("GL_ARB_texture_non_power_of_two") ) ) {
                power2 = true;
            }

#ifdef GLM_CUDA
            // trying to use CUDA for decoding
            try {
                newSourceVideoFile = new CUDAVideoFile(this, power2, convert);
                qDebug() << child.attribute("name") << QChar(124).toLatin1() << "Using GPU accelerated CUDA Decoding.";
            }
            catch (AllocationException &e){
                qDebug() << child.attribute("name") << QChar(124).toLatin1() <<"CANNOT use GPU accelerated CUDA Decoding.";
                newSourceVideoFile = 0;
            }
#endif
            if (!newSourceVideoFile) {
                if (power2)
                    newSourceVideoFile = new VideoFile(this, true, RenderingManager::getInstance()->getFrameBufferWidth(), RenderingManager::getInstance()->getFrameBufferHeight());
                else
                    newSourceVideoFile = new VideoFile(this);
            }

            // if the video file was created successfully
            if (newSourceVideoFile){

                double markin = -1.0, markout = -1.0;
                // old version used different system for marking : ignore the values for now
                if ( !( version.toDouble() < 0.7) ) {
                    markin = marks.attribute("In").toDouble();
                    markout = marks.attribute("Out").toDouble();
                }

                // can we open this existing file ?
                // try to open until success (or maximum 3 tentatives)
                int tentative = 0;
                bool success = false;
                while (!success && tentative < 3) {
                    success = newSourceVideoFile->open( fileNameToOpen, markin, markout, Filename.attribute("IgnoreAlpha").toInt() ) ;
                    tentative++;
                }
                // inform if there was a problem
                if (tentative > 1)
                    qWarning() << child.attribute("name") << QChar(124).toLatin1() << tr("There might be a problem with this video. %1 attempts were required to open it.").arg(tentative);

                // somehow managed to open the file
                if ( success ) {

                    // fix old version marking : compute marks correctly
                    if ( version.toDouble() < 0.7) {
                        newSourceVideoFile->setMarkIn( (double) marks.attribute("In").toInt() / newSourceVideoFile->getFrameRate() );
                        newSourceVideoFile->setMarkOut( (double) marks.attribute("Out").toInt() / newSourceVideoFile->getFrameRate() );
                        qWarning() << child.attribute("name") << QChar(124).toLatin1()
                                   << tr("Converted marks from old version file: begin = %1 (%2)  end = %3 (%4)").arg(newSourceVideoFile->getMarkIn()).arg(marks.attribute("In").toInt()).arg(newSourceVideoFile->getMarkOut()).arg(marks.attribute("Out").toInt());
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
                        newSourceVideoFile->setOptionRestartToMarkIn(options.attribute("RestartToMarkIn","0").toInt());
                        newSourceVideoFile->setOptionRevertToBlackWhenStop(options.attribute("RevertToBlackWhenStop","0").toInt());

                        if ( version.toDouble() < 0.9 &&  options.hasAttribute("AllowDirtySeek"))
                            qDebug() << child.attribute("name") << QChar(124).toLatin1()
                                     << tr("Option Allow Seek Dirty Frames is deprecated: ignoring!");


                        qDebug() << child.attribute("name") << QChar(124).toLatin1()
                                 << tr("Media source created with ")
                                 << QFileInfo(fileNameToOpen).fileName()
                                 << " ("<<newSourceVideoFile->getFrameWidth()
                                 <<"x"<<newSourceVideoFile->getFrameHeight()<<").";
                    }
                    else {
                        qWarning() << child.attribute("name") << QChar(124).toLatin1()
                                   << tr("Could not be created.");
                        errors++;
                    }
                }
                else {
                    qWarning() << child.attribute("name") << QChar(124).toLatin1()
                               << tr("Could not load ")<< fileNameToOpen;
                    errors++;
                }
            }
            else {
                qWarning() << child.attribute("name") << QChar(124).toLatin1()
                           << tr("Could not allocate memory.");
                errors++;
            }

            // if creating the source failed, remove the video file object from memory
            if (newSourceVideoFile && !newsource)
                delete newSourceVideoFile;

        }
        else {
            qWarning() << child.attribute("name") << QChar(124).toLatin1()
                       << tr("No file named %1 or %2.").arg(Filename.text()).arg(fileNameToOpen);
            errors++;
        }

    }
    else if ( type == Source::ALGORITHM_SOURCE)
    {
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
            qWarning() << child.attribute("name") << QChar(124).toLatin1()
                       << tr("Could not create algorithm source.");
            errors++;
        } else
            qDebug() << child.attribute("name") << QChar(124).toLatin1()
                     << tr("Algorithm source created (")
                     << AlgorithmSource::getAlgorithmDescription(Algorithm.text().toInt())
                     << ", "<<newsource->getFrameWidth()
                     <<"x"<<newsource->getFrameHeight() << ").";
    }
    else if ( type == Source::RENDERING_SOURCE)
    {
        // tags specific for a rendering source
        int r = 1;
        QDomElement Recursive = t.firstChildElement("Recursive");
        if (!Recursive.isNull())
            r = Recursive.text().toInt();
        newsource = RenderingManager::_instance->newRenderingSource(r > 0, depth);
        if (!newsource) {
            qWarning() << child.attribute("name") << QChar(124).toLatin1()
                       << tr("Could not create rendering loop-back source.");
            errors++;
        } else
            qDebug() << child.attribute("name") << QChar(124).toLatin1()
                     <<  tr("Rendering loop-back source created.");
    }
    else if ( type == Source::CAPTURE_SOURCE)
    {
        QDomElement img = t.firstChildElement("Image");
        bool imageloaded = false;
        QImage image;

        // try to create image from text
        QByteArray data = QByteArray::fromBase64( img.text().toLatin1() );
        if ( image.loadFromData( reinterpret_cast<const uchar *>(data.data()), data.size()) )
            imageloaded = true;
        // compatibility with old format : try to read without Base64
        else {
            data = img.text().toLatin1();
            if ( image.loadFromData( reinterpret_cast<const uchar *>(data.data()), data.size()) )
                imageloaded = true;
        }
        // try to create the capture source
        if (imageloaded)
            newsource = RenderingManager::_instance->newCaptureSource(image, depth);

        if (!newsource) {
            qWarning() << child.attribute("name") << QChar(124).toLatin1()
                       << tr("Could not create Pixmap source; invalid picture in session file.");
            errors++;
        } else
            qDebug() << child.attribute("name") << QChar(124).toLatin1()
                     << tr("Pixmap source created (") <<newsource->getFrameWidth()
                     <<"x"<<newsource->getFrameHeight() << ").";
    }
    else if ( type == Source::SVG_SOURCE)
    {
        QDomElement svg = t.firstChildElement("Svg");
        QByteArray data =  svg.text().toLatin1();

        if ( !data.isEmpty() )
            newsource = RenderingManager::_instance->newSvgSource(data, depth);

        if (!newsource) {
            qWarning() << child.attribute("name")<< QChar(124).toLatin1()
                       << tr("Could not create vector graphics source.");
            errors++;
        } else
            qDebug() << child.attribute("name")<< QChar(124).toLatin1()
                     << tr("Vector graphics source created (")
                     <<newsource->getFrameWidth()<<"x"<<newsource->getFrameHeight() << ").";
    }
    else if ( type == Source::WEB_SOURCE)
    {
        QDomElement web = t.firstChildElement("Web");
        QUrl url =  QUrl(web.text());
        QDomElement Frame = t.firstChildElement("Frame");

        if ( Frame.hasAttributes() ) {
            newsource = RenderingManager::_instance->newWebSource(url, Frame.attribute("Width", "1024").toInt(), Frame.attribute("Height", "768").toInt(), web.attribute("Height", "100").toInt(), web.attribute("Scroll", "0").toInt(), web.attribute("Update", "0").toInt(), depth);
        }
        else {
            newsource = RenderingManager::_instance->newWebSource(url, 1024, 768, web.attribute("Height", "100").toInt(), web.attribute("Scroll", "0").toInt(), web.attribute("Update", "0").toInt(), depth);
            qDebug() << child.attribute("name") << QChar(124).toLatin1()
                     << tr("No resolution goven for web rendering; using 1024x768.");
        }

        if (!newsource) {
            qWarning() << child.attribute("name")<< QChar(124).toLatin1()
                       << tr("Could not create web source.");
            errors++;
        } else
            qDebug() << child.attribute("name")<< QChar(124).toLatin1()
                     << tr("Web source created (")<<web.text() << ").";
    }
    else if ( type == Source::SHM_SOURCE)
    {
#ifdef GLM_SHM
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
        qWarning() << child.attribute("name") << QChar(124).toLatin1()
                   << tr("Could not create source: type Shared Memory not supported.");
        errors++;
#endif
    }
    else if ( type == Source::SPOUT_SOURCE)
    {
#ifdef GLM_SPOUT
        // read the tags specific for an algorithm source
        QDomElement spout = t.firstChildElement("Spout");

        newsource = RenderingManager::_instance->newSpoutSource(spout.text(), depth);

        if (!newsource) {
            qWarning() << child.attribute("name") << QChar(124).toLatin1()<< tr("Could not connect to the SPOUT sender %1.").arg(spout.text());
            errors++;
        } else
            qDebug() << child.attribute("name")<< QChar(124).toLatin1() << tr("SPOUT source created (")<< spout.text() << ").";
#else
        qWarning() << child.attribute("name") << QChar(124).toLatin1()
                   << tr("Could not create source: type SPOUT not supported.");
        errors++;
#endif
    }
    else if (type == Source::FFGL_SOURCE )
    {
#ifdef GLM_FFGL
        QDomElement Frame = t.firstChildElement("Frame");
        QDomElement ffgl = t.firstChildElement("FreeFramePlugin");

        if ( ffgl.isNull() )
            ffgl = t.firstChildElement("FreeFramePlugin");

        newsource = RenderingManager::_instance->newFreeframeGLSource(ffgl, Frame.attribute("Width", "64").toInt(),  Frame.attribute("Height", "64").toInt(), depth);

        if (!newsource) {
            qWarning() << child.attribute("name") << QChar(124).toLatin1()
                       << tr("Could not create FreeframeGL source.");
            errors++;
        } else
            qDebug() << child.attribute("name") << QChar(124).toLatin1()
                     << tr("FreeframeGL source created (") <<newsource->getFrameWidth()
                     <<"x"<<newsource->getFrameHeight() << ").";;
#else
        qWarning() << child.attribute("name") << QChar(124).toLatin1()<< tr("Could not create source: type FreeframeGL not supported.");
        errors++;
#endif
    }
    else if ( type == Source::CAMERA_SOURCE )
    {
#ifdef GLM_OPENCV
        QDomElement camera = t.firstChildElement("CameraIndex");

        newsource = RenderingManager::_instance->newOpencvSource( camera.text().toInt(),  camera.attribute("Mode", "0").toInt(), depth);
        if (!newsource) {
            qWarning() << child.attribute("name") << QChar(124).toLatin1()
                       <<  tr("Could not open OpenCV device index %2.").arg(camera.text());
            errors ++;
        } else
            qDebug() << child.attribute("name") << QChar(124).toLatin1()
                     <<  tr("OpenCV source created (device index %2, ").arg(camera.text())
                      << newsource->getFrameWidth()<<"x"<<newsource->getFrameHeight() << " ).";
#else
        qWarning() << child.attribute("name") << QChar(124).toLatin1() << tr("Could not create source: type OpenCV not supported.");
        errors++;
#endif
    }
    else if ( type == Source::STREAM_SOURCE)
    {
        QDomElement str = t.firstChildElement("Url");

        VideoStream *vs = new VideoStream();
        vs->open(str.text());

        newsource = RenderingManager::_instance->newStreamSource(vs, depth);

        if (!newsource) {
            qWarning() << child.attribute("name")<< QChar(124).toLatin1()
                       << tr("Could not create video stream source.");
            errors++;
        } else
            qDebug() << child.attribute("name")<< QChar(124).toLatin1()
                     << tr("Video Stream source created (") << str.text() << ").";
    }
    else if ( type == Source::BASKET_SOURCE)
    {
        QDomElement basket = t.firstChildElement("Images");

        QStringList fileNames;
        QDomElement Filename = basket.firstChildElement("Filename");
        while (!Filename.isNull()) {

            // first reads with the absolute file name
            QString fileNameToOpen = Filename.text();
            // if there is no such file, try generate a file name from the relative file name
            if (!QFileInfo(fileNameToOpen).exists())
                fileNameToOpen = current.absoluteFilePath( Filename.attribute("Relative", "") );
            // if there is such a file
            if (QFileInfo(fileNameToOpen).exists() && QFileInfo(fileNameToOpen).isFile())
                fileNames.append(fileNameToOpen);
            else
                qWarning() << child.attribute("name") << QChar(124).toLatin1()
                           << tr("No file named %1 or %2.").arg(Filename.text()).arg(fileNameToOpen);

            Filename = Filename.nextSiblingElement("Filename");
        }

        QDomElement Frame = t.firstChildElement("Frame");
        QDomElement Update = t.firstChildElement("Update");
        qint64 period = Update.attribute("Periodicity", "0").toInt();

        QDomElement Playlist = t.firstChildElement("Playlist");
        bool bidir = Playlist.attribute("Bidirectional", "0").toInt() > 0;
        bool shuf = Playlist.attribute("Shuffle", "0").toInt() > 0;
        QString pl = Playlist.text();

        newsource = RenderingManager::_instance->newBasketSource(fileNames, Frame.attribute("Width", "1024").toInt(), Frame.attribute("Height", "768").toInt(), period, bidir, shuf, pl, depth);


    }
    else if ( type == Source::CLONE_SOURCE)
    {
        QDomElement f = t.firstChildElement("CloneOf");

        // find the source which name is f.text()
        SourceSet::iterator cloneof =  getByName(f.text());
        if (isValid(cloneof)) {
            newsource = RenderingManager::_instance->newCloneSource(cloneof, depth);
            if (!newsource) {
                qWarning() << child.attribute("name") << QChar(124).toLatin1()
                           << tr("Could not create clone source.");
                errors++;
            }
            else
                qDebug() << child.attribute("name") << QChar(124).toLatin1()
                         << tr("Clone of source %1 created.").arg(f.text());
        }
        else {
            qWarning() << child.attribute("name") << QChar(124).toLatin1()
                       << tr("Cannot clone %2 ; no such source.").arg(f.text());
            errors++;
        }

    }

    // if a new source was created
    if (newsource) {
        // Apply parameters to the created source
        if ( !newsource->setConfiguration(child, current) )  {
            qWarning() << child.attribute("name") << QChar(124).toLatin1()
                       << tr("Could apply all configuration.");
            errors++;
        }

        // Insert the source in the scene
        if ( _insertSource(newsource) )  {
            // ok ! source is configured, we can start it !

            // Play the source if playing attributes says so
            // NB: if no attribute, then play by default.
            newsource->setStandbyMode( (Source::StandbyMode) child.attribute("stanbyMode", "0").toInt() );
            if (!newsource->isStandby())
                newsource->play( child.attribute("playing", "1").toInt() );

        }
        else {
            qCritical() << child.attribute("name") << QChar(124).toLatin1()
                       << tr("Could not insert source.");
            errors++;
            delete newsource;
        }

    }

    return errors;
}

int RenderingManager::addConfiguration(QDomElement xmlconfig, QDir current, QString version) {

    QList<QDomElement> clones;
    int errors = 0;
    int count = _front_sources.size();

    // start loop of sources to create
    QDomElement child = xmlconfig.firstChildElement("Source");
    while (!child.isNull()) {

        // get the type of the source to create
        QDomElement t = child.firstChildElement("TypeSpecific");
        if (!t.isNull())
        {
            // create the source according to its specific type
            Source::RTTI type = (Source::RTTI) t.attribute("type").toInt();
            // error if invalid type
            if (type == Source::SIMPLE_SOURCE)
                errors++;
            // remember the node of the sources to clone
            else if ( type == Source::CLONE_SOURCE)
                clones.push_back(child);
            // create the source of known type
            else
                errors += addSourceConfiguration(child, current, version);
        }

        child = child.nextSiblingElement("Source");
    }
    // end loop on sources to create

    // Process the list of clones names ;
    // now that every source exist, we can be sure they can be cloned
    QListIterator<QDomElement> it(clones);
    while (it.hasNext()) {
        QDomElement c = it.next();

        errors += addSourceConfiguration(c, current, version);
    }

    // set current source to none (end of list)
    unsetCurrentSource();

    // log
    count = _front_sources.size() - count;
    qDebug() << "RenderingManager" << QChar(124).toLatin1() << tr("%1 sources loaded (%1/%2).").arg(count).arg(xmlconfig.childNodes().count());


#ifdef GLM_UNDO
    // cleanup & reactivate Undo Manager
    UndoManager::getInstance()->clear();
#endif

    // inform of change
    emit countSourceChanged(_front_sources.size());

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

void RenderingManager::pause(bool on)
{
    // setup status
    paused = on;

    if (paused)
        _elapsedTime = _displayTime.elapsed();
    else {
        int e = _displayTime.elapsed();
        _displayTime = _displayTime.addMSecs( e - _elapsedTime );
    }
}


double RenderingManager::getElapsedTime()
{
    if (!paused)
        _elapsedTime = _displayTime.elapsed();
    return (double) _elapsedTime / 1000.0;
}

void RenderingManager::onSourceFailure() {

    Source *s = qobject_cast<Source *>(sender());

    if (s) {
        QString name = s->getName();
        qCritical() << name << QChar(124).toLatin1() << tr("This source failed and is removed.");

        // try to remove the source from the manager
        if ( isValid( getByName(name) ) )
            _removeSource(s->getId());
        // not in the manager, maybe in the drop basket
        else {
            for (SourceList::iterator sit = dropBasket.begin(); sit != dropBasket.end(); sit++ ) {
                if ( (*sit)  == s ) {
                    dropBasket.erase(sit);
                    delete (*sit);
                    break;
                }
            }
        }
    }
}


#ifdef GLM_SHM
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

#ifdef GLM_SPOUT

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


//QString RenderingManager::renameSource(Source *s, const QString name){

//    s->setName( getAvailableNameFrom(name) );

//    return s->getName();
//}


