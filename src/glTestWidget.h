#ifndef GLTESTWIDGET_H
#define GLTESTWIDGET_H

#include "common.h"

#include <QElapsedTimer>
#include <QTimer>

class glTestWidget : public QGLWidget
{
    Q_OBJECT

public:
    glTestWidget(QWidget * parent = 0);

    void initializeGL();
    void paintGL();
    void resizeGL(int w = 0, int h = 0);
    int updatePeriod();

public slots:
    void disableBlitFBO(bool off);
    void disablePBO(bool off);
    void disableFilters(bool off) { _use_filters = !off; }
    void setUpdatePeriod(int miliseconds);

private:
    bool _use_blit, _use_pbo, _use_filters;
    GLuint _texture;
    QGLFramebufferObject *_fbo;
    GLuint _pbo, _pbo_texture;
    float _img_ar, _angle;
    QImage _image;
    QElapsedTimer fpsTime_;
    float f_p_s_;
    QTimer timer;
    float aspectRatio;
};

#endif // GLTESTWIDGET_H
