/****************************************************************************
**

**
****************************************************************************/

#ifndef GLWIDGET_H_
#define GLWIDGET_H_

#include "glRenderWidget.h"
#include "VideoFile.h"


class VideoFileDisplayWidget : public glRenderWidget
{
    Q_OBJECT

public:
    VideoFileDisplayWidget(QWidget *parent = 0);
    ~VideoFileDisplayWidget();

    // OpenGL implementation
    void initializeGL();
    void paintGL();
    virtual void showEvent ( QShowEvent * event ) { QGLWidget::showEvent(event);}


public Q_SLOTS:
    void updateFrame (int i);
    void setVideo(VideoFile* f);
    void setVideoAspectRatio(bool usevideoratio);

protected:

    VideoFile *is;
    GLuint squareDisplayList;
    GLuint textureIndex;
    bool useVideoAspectRatio;

};

#endif

