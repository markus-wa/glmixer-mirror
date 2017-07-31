#include "glTestWidget.moc"
#include "RenderingManager.h"

#include <QImage>

glTestWidget::glTestWidget(QWidget *parent) :  glRenderWidget(parent), _use_blit(true), _use_pbo(true), _use_filters(true), _texture(0), _fbo(NULL), _pbo(0)
{
}


void glTestWidget::disableBlitFBO(bool off) {
    _use_blit = !off && glewIsSupported("GL_EXT_framebuffer_blit GL_EXT_framebuffer_multisample");
}

void glTestWidget::disablePBO(bool off) {
    _use_pbo = !off && glewIsSupported("GL_ARB_pixel_buffer_object");
}

void glTestWidget::initializeGL() {

    glRenderWidget::initializeGL();

#ifdef Q_OS_MAC
    setBackgroundColor( palette().color(QPalette::Window).darker(105) );
#else
    setBackgroundColor(palette().color(QPalette::Window));
#endif

    // test image
//    _image = QImage(":/glmixer/images/glmixer_splash.png");
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
    // generic widget resize (also computes aspectRatio)
    glRenderWidget::resizeGL(w, h);

    // rescale fbo
    if (_fbo)
        delete _fbo;
    _fbo = new QGLFramebufferObject( w, h);

}


void glTestWidget::paintGL()
{
    glRenderWidget::paintGL();

    if (!isEnabled() || !_fbo)
        return;


    // render image in FBO
    if (_fbo->bind())
    {
        glClear(GL_COLOR_BUFFER_BIT);
        glLoadIdentity();

        glBegin(GL_POINTS);
        glColor4ub(_use_blit ? 0 :250, _use_blit ? 250 : 0,0,250);
        glVertex2f(0.85, 0.3);
        glColor4ub(_use_pbo ? 0 :250, _use_pbo ? 250 : 0,0,250);
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
        glLoadIdentity();
        drawTexture(QRectF(-1.0, 1.0, 2.0, -2.0), _fbo->texture() );
    }



}
