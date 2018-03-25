#include "glTestWidget.moc"
#include "RenderingManager.h"
#include <QTimer>

#include <QImage>

glTestWidget::glTestWidget(QWidget *parent) : QGLWidget(glRenderWidgetFormat(), parent), _use_blit(true), _use_pbo(true), _use_filters(true), _texture(0), _fbo(NULL), _pbo(0), aspectRatio(1.0)
{
    f_p_s_ = 50.0;
    fpsTime_.invalidate();
    connect(&timer, SIGNAL(timeout()), this, SLOT(updateGL()));
    timer.setInterval(20);
}


void glTestWidget::disableBlitFBO(bool off) {
    _use_blit = !off && glewIsSupported("GL_EXT_framebuffer_blit GL_EXT_framebuffer_multisample");
}

void glTestWidget::disablePBO(bool off) {
    _use_pbo = !off && glewIsSupported("GL_ARB_pixel_buffer_object");
}

void glTestWidget::setUpdatePeriod(int miliseconds) {

    if (miliseconds > 10)
        timer.start(miliseconds);
    else {
        timer.stop();
        fpsTime_.invalidate();
    }
}

int glTestWidget::updatePeriod() {
    return timer.interval();
}

void glTestWidget::initializeGL()
{
    // Set flat color shading without dithering
    glShadeModel(GL_FLAT);
    glDisable(GL_DITHER);
    glDisable(GL_POLYGON_SMOOTH);

    // disable depth and lighting by default
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);
    glDepthMask(GL_FALSE);
    glDisable(GL_LIGHTING);
    glDisable(GL_NORMALIZE);

    // Enables texturing
    glActiveTexture(GL_TEXTURE0);
    glEnable(GL_TEXTURE_2D);

    // default texture parameters
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_BASE_LEVEL, 0);//We add these two lines
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 0); //because we don't have mipmaps.

    // ensure alpha channel is modulated ; otherwise the source is not mixed by its alpha channel
    glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_COMBINE);
    glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_ALPHA, GL_MODULATE);
    glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_RGB, GL_MODULATE);

    // Turn blending on
    glEnable(GL_BLEND);

    // Blending Function For transparency Based On Source Alpha Value
    glBlendEquation(GL_FUNC_ADD);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // This hint can improve the speed of texturing when perspective-correct texture coordinate interpolation isn't needed
    glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_FASTEST);
    // This hint can improve the speed of shading when dFdx dFdy aren't needed in GLSL
    glHint(GL_FRAGMENT_SHADER_DERIVATIVE_HINT, GL_FASTEST);

#ifdef Q_OS_MAC
    qglClearColor( palette().color(QPalette::Window).darker(105) );
#else
    qglClearColor(palette().color(QPalette::Window));
#endif

    // test image
    _image = QImage(":/glmixer/icons/glmixer_256x256.png");
    _img_ar = (float) _image.width() / (float)_image.height();

    // standard texture
    _texture = bindTexture(_image, GL_TEXTURE_2D, GL_RGBA,  QGLContext::LinearFilteringBindOption);

    // PBO
    if ( glewIsSupported("GL_ARB_pixel_buffer_object") ) {

        // create pbo texture
        _pbo_texture = bindTexture(_image, GL_TEXTURE_2D, GL_RGBA,  QGLContext::LinearFilteringBindOption);

        // create 2 pixel buffer objects,
        glGenBuffers(1, &_pbo);
        // create first PBO
        glBindBuffer(GL_PIXEL_UNPACK_BUFFER, _pbo);
        // glBufferDataARB with NULL pointer reserves only memory space.
        glBufferData(GL_PIXEL_UNPACK_BUFFER, _image.byteCount(), 0, GL_STREAM_DRAW);
        // fill in with reset picture
        GLubyte* ptr = (GLubyte*) glMapBuffer(GL_PIXEL_UNPACK_BUFFER, GL_WRITE_ONLY);
        if (ptr)  {
            // update data directly on the mapped buffer
            memmove(ptr, _image.constBits(), _image.byteCount());
            // release pointer to mapping buffer
            glUnmapBuffer(GL_PIXEL_UNPACK_BUFFER);
        }
        // copy pixels from PBO to texture object
        glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, _image.width(), _image.height(), GL_BGRA, GL_UNSIGNED_BYTE, 0);
        // done
        glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);

    }

    glPointSize(10);

}

void glTestWidget::resizeGL(int w, int h)
{
    glViewport(0, 0, w, h);
    aspectRatio = (float) w / (float) h;

    // Setup specific projection and view for this window
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluOrtho2D(-1.0, 1.0, -1.0, 1.0);
    glMatrixMode(GL_MODELVIEW);

    // rescale fbo
    if (_fbo)
        delete _fbo;
    _fbo = new QGLFramebufferObject( w, h);

}


void glTestWidget::paintGL()
{
    glClear(GL_COLOR_BUFFER_BIT);

    if (!isEnabled() || !_fbo)
        return;

    if (!fpsTime_.isValid())
        fpsTime_.start();

    // render image in FBO
    if (_fbo->bind())
    {
        glClear(GL_COLOR_BUFFER_BIT);
        glLoadIdentity();

        glBegin(GL_POINTS);
        glColor4ub(_use_blit ? 0 :255, _use_blit ? 255 : 0, 0, 255);
        glVertex2f(0.85, 0.3);
        glColor4ub(_use_pbo ? 0 :255, _use_pbo ? 255 : 0, 0, 255);
        glVertex2f(0.85, 0.8);
        glEnd();

        // respect aspect ratio
        glScaled( 0.93 / aspectRatio, 0.93, 1.0);

        // animate logo
        _angle = fmod(_angle + 3601.0, 360.0);
        glRotatef( _angle, 0.0, 0.0, 1.0);

        // draw in white
        glColor4ub(255, 255, 255, 255);

        if (_use_pbo) {
            // stress test of PBO by replacing texture each frame
            glBindBuffer(GL_PIXEL_UNPACK_BUFFER, _pbo);
            glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, _image.width(), _image.height(), GL_BGRA, GL_UNSIGNED_BYTE, 0);
            glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);

            drawTexture(QRectF(-_img_ar, -1.0, 2.0 * _img_ar, 2.0), _pbo_texture );
        }
        else
            drawTexture(QRectF(-_img_ar, -1.0, 2.0 * _img_ar, 2.0), _texture );

        _fbo->release();
    }

    // render fbo
    if (_use_blit) {
        // select fbo texture read target
        glBindFramebuffer(GL_READ_FRAMEBUFFER, _fbo->handle());
        // select screen target
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
        // blit
        glBlitFramebuffer(0, 0, _fbo->width(), _fbo->height(), 0, 0, width(), height(), GL_COLOR_BUFFER_BIT, GL_LINEAR);
    }
    else {
        // standard opengl draw
        glColor4ub(255, 255, 255, 255);
        glLoadIdentity();
        drawTexture(QRectF(-1.0, 1.0, 2.0, -2.0), _fbo->texture() );
    }


    // compute fps
    f_p_s_ = qRound( 0.8f * f_p_s_ + 0.2f * ( 1000.f / (float) qBound(10, (int) fpsTime_.restart(), 100) ) );

    if (f_p_s_ < 800.f / (float) updatePeriod())
        // show warning on slow FPS if bellow 80% of requested rendering fps
        emit slowFps(true);
    else
        emit slowFps(false);

    glColor4ub(60, 60, 60, 200);
    renderText(10, height() - 10, QString("%1 fps").arg(f_p_s_) );
    qDebug()<<".";
}
