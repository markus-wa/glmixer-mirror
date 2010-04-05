/*
 * VideoSource.h
 *
 *  Created on: 3 nov. 2009
 *      Author: herbelin
 */

#ifndef VIDEOSOURCE_H_
#define VIDEOSOURCE_H_

#include <QObject>

#include "common.h"
#include "Source.h"
#include "VideoFile.h"
//class VideoFile;

class VideoSource : public QObject, public Source {

    Q_OBJECT

    friend class RenderingManager;

    // only RenderingManager can create a source
protected:
	VideoSource(VideoFile *f, GLuint texture, double d);
	virtual ~VideoSource();
    void update();

public Q_SLOTS:
    void updateFrame (int i);
    void applyFilter();

public:
	static RTTI type;
	RTTI rtti() const { return type; }

    inline VideoFile *getVideoFile() const { return is; }

private:
    VideoFile *is;
    VideoPicture copy;

    bool copyChanged;
    int bufferIndex;

};

#endif /* VIDEOSOURCE_H_ */
