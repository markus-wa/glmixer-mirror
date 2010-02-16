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
		is(f),  bufferChanged(true)
{

    if (is) {
        QObject::connect(is, SIGNAL(frameReady(int)), this, SLOT(updateFrame(int)));

        aspectratio = is->getStreamAspectRatio();

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
    // TODO : else through exception

    resetScale();
}

VideoSource::~VideoSource() {

    if (is)
    	delete is;
}



// only Rendering Manager can call this
void VideoSource::update(){

	Source::update();

    // update texture
    if (is && bufferChanged) {

        const VideoPicture *vp = is->getPictureAtIndex(bufferIndex);
        if (vp && vp->isAllocated()) {

            // Animated textures
            // If you want to use an animated texture, perhaps live video textures,
            // don't use glTexImage2D to repeatedly change the texture.
            // Use glTexSubImage2D or glTexCopyTexSubImage2D.
            if ( vp->getFormat() == PIX_FMT_RGBA)
            	glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0,  vp->getWidth(),
                         vp->getHeight(), GL_RGBA, GL_UNSIGNED_BYTE,
                         vp->getBuffer() );
            else
            	glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0,  vp->getWidth(),
                         vp->getHeight(), GL_RGB, GL_UNSIGNED_BYTE,
                         vp->getBuffer() );


        }
        bufferChanged = false;
    }
}

void VideoSource::updateFrame (int i)
{

	bufferChanged = true;
	bufferIndex = i;

}

