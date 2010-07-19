/*
 * VideoSource.cpp
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
 *   Copyright 2009, 2010 Bruno Herbelin
 *
 */

#include "VideoSource.moc"

#include "VideoFile.h"
#include "RenderingManager.h"

GLuint videoSourceIconIndex = 0;
Source::RTTI VideoSource::type = Source::VIDEO_SOURCE;

VideoSource::VideoSource(VideoFile *f, GLuint texture, double d) : QObject(), Source(texture, d),
		is(f), copyChanged(false), bufferIndex(-1)
{

    if (videoSourceIconIndex == 0) {

    	glGenTextures(1, &videoSourceIconIndex);
    	glBindTexture(GL_TEXTURE_2D, videoSourceIconIndex);
    	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    	QImage p( ":/glmixer/icons/video.png" );
    	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA,  p.width(), p. height(),
    	    		  0, GL_RGBA, GL_UNSIGNED_BYTE, p.bits() );
    }

    iconIndex = videoSourceIconIndex;

    if (is) {
        QObject::connect(is, SIGNAL(frameReady(int)), this, SLOT(updateFrame(int)));

        aspectratio = is->getStreamAspectRatio();

		QObject::connect(is, SIGNAL(prefilteringChanged()), this, SLOT(applyFilter()));

    	glBindTexture(GL_TEXTURE_2D, textureIndex);
    	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

    	// fills in the first frame
        const VideoPicture *vp = is->getPictureAtIndex(-1);
		copy = *vp;
        if (vp && vp->isAllocated()) {

        	// fill in the texture
    		if ( vp->getFormat() == PIX_FMT_RGBA)
    			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA,  vp->getWidth(),
    					 vp->getHeight(), 0, GL_RGBA, GL_UNSIGNED_BYTE,
    					 vp->getBuffer() );
    		else
    			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB,  vp->getWidth(),
    					 vp->getHeight(), 0, GL_RGB, GL_UNSIGNED_BYTE,
    					 vp->getBuffer() );
        }
        is->play(true);
    }
    else
    	qWarning("** WARNING **\nThe media source could not be created properly. Remove it and retry.");

}

VideoSource::~VideoSource() {

    if (is)
    	delete is;

	// free the OpenGL texture
	glDeleteTextures(1, &textureIndex);
}



// only Rendering Manager can call this
void VideoSource::update(){

	Source::update();

    // update texture
    if (is && frameChanged) {

        const VideoPicture *vp = is->getPictureAtIndex(bufferIndex);

        // if we decided that the copy has changed and should be displayed
        // then make a copy of the bufferIndex (this applies the pre-filters) and use it
		if (copyChanged) {
			copy = *vp;
			vp = &copy;
		}
		// is the picture good ?
        if (vp && vp->isAllocated()) {
        	// use it for OpenGL
            if ( vp->getFormat() == PIX_FMT_RGBA)
            	glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0,  vp->getWidth(),
                         vp->getHeight(), GL_RGBA, GL_UNSIGNED_BYTE,
                         vp->getBuffer() );
            else
            	glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0,  vp->getWidth(),
                         vp->getHeight(), GL_RGB, GL_UNSIGNED_BYTE,
                         vp->getBuffer() );
        }
        // shouldn't update next time unless requested
        frameChanged = false;
    }
}

void VideoSource::updateFrame (int i)
{
	frameChanged = true;
	bufferIndex = i;
    copyChanged = false;
}

// TODO : this implementation is not exact. The problem is that it applies the copy filtering on top of the
// already filtered frame produced in the VideoFile; so the visual effect is not correct when applying the effect
// on a video file when paused. To change this, one should keep a copy of the original frame WITHOUT filtering, which
// would require to make it EXTRA (double conversion) and take much more CPU time.
// So, this 'not working' compromise seems reasonnable comparatively to the important loss of performance
// But maybe a smarter solution can be found...

void VideoSource::applyFilter(){

	// modify the filtering for the copy
	copy.setCopyFiltering(is->getBrightness(), is->getContrast(), is->getSaturation());

	// if the source is still on the original frame or the video file is paused
	if (bufferIndex == -1 || !is->isRunning() || is->isPaused()) {
		// request to change the buffer from the new copy
		frameChanged = copyChanged = true;
	}
	else {
		// else do nothing special; wait for next frame to apply filter
		copyChanged = false;
	}
}

