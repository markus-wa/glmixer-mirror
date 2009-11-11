/*
 * MixRenderWidget.h
 *
 *  Created on: 3 nov. 2009
 *      Author: herbelin
 */

#ifndef MAINRENDERWIDGET_H_
#define MAINRENDERWIDGET_H_

#define DEFAULT_ASPECT_RATIO 1.333

#include "glRenderWidget.h"
#include "VideoSource.h"

class MainRenderWidget: public glRenderWidget {

	Q_OBJECT

public:
    // get singleton instance
    static const QGLWidget *getQGLWidget();
    static MainRenderWidget *getInstance();
    static void deleteInstance();

    QGLFramebufferObject *fbo;

    // QGLWidget rendering
    void paintGL();

    // Management of the video sources
    void createSource(VideoFile *vf);
    VideoSource *getSource() { return _s;};

    void setAspectRatio(float ratio) { _aspectRatio = ratio;}
    float getAspectRatio() {return _aspectRatio;}

public slots:
    void useAspectRatio(bool on) {_useAspectRatio = on;}

private:
	MainRenderWidget(QWidget *parent = 0);
	virtual ~MainRenderWidget();
    // singleton instance
    static MainRenderWidget *_instance;

private:
    // temporary hack
	VideoSource *_s;
	float _aspectRatio;
	bool _useAspectRatio;

};

#endif /* MAINRENDERWIDGET_H_ */
