/*
 * VideoSource.cpp
 *
 *  Created on: 3 nov. 2009
 *      Author: herbelin
 */

#include "VideoSource.moc"

#include "VideoFile.h"
#include "RenderingManager.h"


VideoSource::VideoSource(VideoFile *f, GLuint texture, double d) : QObject(), Source(texture, d),
		is(f),  bufferChanged(false), copyChanged(false), bufferIndex(-1)
{

    if (is) {
        QObject::connect(is, SIGNAL(frameReady(int)), this, SLOT(updateFrame(int)));

        aspectratio = is->getStreamAspectRatio();
		QObject::connect(is, SIGNAL(prefilteringChanged()), this, SLOT(applyFilter()));

    	glBindTexture(GL_TEXTURE_2D, textureIndex);
    	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

    	// fills in the first frame
        const VideoPicture *vp = is->getPictureAtIndex(-1);
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
    }
    else
    	qWarning("** WARNING **\nThe media source could not be created properly. Remove it and retry.");


    resetScale();
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
    if (is && bufferChanged) {

        const VideoPicture *vp = is->getPictureAtIndex(bufferIndex);

        // if we decided that the copy has changed and should be displayed
        // then make a copy of the bufferIndex (this applies the pre-filters) and use it
		if (copyChanged) {
			copy = *vp;
			vp = &copy;
	        copyChanged = false;
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
        bufferChanged = false;
    }
}

void VideoSource::updateFrame (int i)
{
	bufferChanged = true;
	bufferIndex = i;
}

void VideoSource::applyFilter(){

	// modify the filtering for the copy
	copy.setCopyFiltering(is->getBrightness(), is->getContrast(), is->getSaturation());

	// if the source is still on the original frame
	if (bufferIndex == -1 || !is->isRunning() || is->isPaused()) {
		// request to change the buffer from the new copy
		bufferChanged = copyChanged = true;
	}
//	else {
		// else do nothing special; wait for next frame
		// except if the video file is paused

//	}
}

