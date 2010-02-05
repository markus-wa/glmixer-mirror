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
#include "Source.h"

class VideoSource : public QObject, public Source {

    Q_OBJECT

    friend class MainRenderWidget;

    // only MainRenderWidget can create a source (need its GL context)
protected:
	VideoSource(VideoFile *f, QGLWidget *context, double d);
	virtual ~VideoSource();
    void update();

public slots:
    void updateFrame (int i);

public:
    inline VideoFile *getVideoFile() { return is; }

private:
    VideoFile *is;

    bool bufferChanged;
    int bufferIndex;

};

#endif /* VIDEOSOURCE_H_ */
