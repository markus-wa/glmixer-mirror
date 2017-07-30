#ifndef GLTESTWIDGET_H
#define GLTESTWIDGET_H

#include "glRenderWidget.h"

class glTestWidget : public glRenderWidget
{
    Q_OBJECT

public:
    glTestWidget(QWidget * parent = 0);

    virtual void initializeGL();
    virtual void paintGL();
    virtual void resizeGL(int w = 0, int h = 0);

public slots:
    void disableBlitFBO(bool off);
    void disablePBO(bool off);
    void disableFilters(bool off) { _use_filters = !off; }

private:
    bool _use_blit, _use_pbo, _use_filters;
    GLuint _texture;
    QGLFramebufferObject *_fbo;
    GLuint _pbo, _pbo_texture;
    float _img_ar;
};

#endif // GLTESTWIDGET_H
