/*
 * VideoSource.cpp
 *
 *  Created on: 3 nov. 2009
 *      Author: herbelin
 */

#include "VideoSource.moc"

#include "MainRenderWidget.h"


VideoSource::VideoSource(VideoFile *f, QGLWidget *context, double d) : QObject(), Source(context, d),
		is(f),  bufferChanged(true)
{

    if (is) {
        QObject::connect(is, SIGNAL(frameReady(int)), this, SLOT(updateFrame(int)));

        aspectratio = is->getStreamAspectRatio();
    }
    // TODO : else through exception

}

VideoSource::~VideoSource() {

    if (is)
    	delete is;
}



// only MixRenderWidget can call this
void VideoSource::update(){

    // update texture
    if (is && bufferChanged) {

        const VideoPicture *vp = is->getPictureAtIndex(bufferIndex);
        if (vp && vp->isAllocated()) {

//        	glcontext->makeCurrent();

        	// update the texture
            glBindTexture(GL_TEXTURE_2D, textureIndex);
            if ( vp->getFormat() == PIX_FMT_RGBA)
                glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA,  vp->getWidth(),
                         vp->getHeight(), 0, GL_RGBA, GL_UNSIGNED_BYTE,
                         vp->getBuffer() );
            else
                glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB,  vp->getWidth(),
                         vp->getHeight(), 0, GL_RGB, GL_UNSIGNED_BYTE,
                         vp->getBuffer() );

            // prepare rendering to fbo : straight projection
//            glPushMatrix(); //GL_MODELVIEW
//            glLoadIdentity();
//            glPushAttrib(GL_VIEWPORT_BIT);
//            glViewport(0, 0, fbo->size().width(), fbo->size().height());
//            glMatrixMode(GL_PROJECTION);
//            glPushMatrix();
//            glLoadIdentity();
//            gluOrtho2D(-UNIT, UNIT, -UNIT, UNIT);
//
//            // now we render to fbo
//            fbo->bind();
//            glClear(GL_COLOR_BUFFER_BIT);
//    		  glCallList(squareDisplayList);
//            fbo->release();
//
//            // retrieve context back
//            glPopMatrix();
//            glPopAttrib();
//            glMatrixMode(GL_MODELVIEW);
//            glPopMatrix();


        }
        bufferChanged = false;
    }

}

void VideoSource::updateFrame (int i)
{

	bufferChanged = true;
	bufferIndex = i;

}

