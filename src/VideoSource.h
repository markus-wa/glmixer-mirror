/*
 * VideoSource.h
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

#ifndef VIDEOSOURCE_H_
#define VIDEOSOURCE_H_

#include <QObject>

#include "Source.h"
#include "VideoFile.h"
#include "ViewRenderWidget.h"
//class VideoFile;

class VideoSource : public QObject, public Source {

    Q_OBJECT

    friend class RenderingManager;
    friend class SessionSwitcher;

    // only friends can create a source
protected:
	VideoSource(VideoFile *f, GLuint texture, double d);
	virtual ~VideoSource();
    void update();

public:

	RTTI rtti() const { return VideoSource::type; }
	bool isPlayable() const;
	bool isPlaying() const;
	bool isPaused() const;

    inline VideoFile *getVideoFile() const { return is; }

	int getFrameWidth() const { return is->getFrameWidth(); }
	int getFrameHeight() const { return is->getFrameHeight(); }
    double getFrameRate() const { return is->getFrameRate(); }

	double getStorageAspectRatio() const { return is->getStreamAspectRatio(); }

	// Adjust brightness factor
	void setBrightness(int b);
	// Adjust contrast factor
	void setContrast(int c) ;
	// Adjust saturation factor
	void setSaturation(int s) ;

public Q_SLOTS:
	void play(bool on);
	void pause(bool on);
    void updateFrame (int i);
    void applyFilter();

private:

	static RTTI type;

    VideoFile *is;
    VideoPicture copy;

    int bufferIndex;
};

#endif /* VIDEOSOURCE_H_ */
