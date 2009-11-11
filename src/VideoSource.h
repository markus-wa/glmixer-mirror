/*
 * VideoSource.h
 *
 *  Created on: 3 nov. 2009
 *      Author: herbelin
 */

#ifndef VIDEOSOURCE_H_
#define VIDEOSOURCE_H_

#include <QtOpenGL>
#include "VideoFile.h"

class VideoSource : public QObject {
    Q_OBJECT

    friend class MainRenderWidget;

public:

    GLuint getFboTexture(){
    	return fbo->texture();
    }

    static GLuint getDisplayList(){
    	return squareDisplayList;
    }


protected:
	VideoSource(VideoFile *f, QGLWidget *context);
	virtual ~VideoSource();
    void renderTexture();

public slots:
    void updateFrame (int i);

private:
    VideoFile *is;
    QGLWidget *glcontext;
    GLuint textureIndex;
    QGLFramebufferObject *fbo;
    bool bufferChanged;
    int bufferIndex;

    static GLuint squareDisplayList;
};

#endif /* VIDEOSOURCE_H_ */
