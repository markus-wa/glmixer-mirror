/*
 * VideoSource.cpp
 *
 *  Created on: 3 nov. 2009
 *      Author: herbelin
 */

#include "VideoSource.moc"

#include "MainRenderWidget.h"

GLuint VideoSource::squareDisplayList = 0;

VideoSource::VideoSource(VideoFile *f, QGLWidget *context) : is(f),  glcontext(context), fbo(NULL), bufferChanged(true){

	if (!QGLFramebufferObject::hasOpenGLFramebufferObjects ())
	  qWarning("Frame Buffer Objects not supported on this graphics hardware");

	if (!context)
		return;
	glcontext->makeCurrent();

    if (is) {

        fbo = new QGLFramebufferObject(512, 512);
//    	fbo = new QGLFramebufferObject(is->getStreamFrameWidth(), is->getStreamFrameHeight());
        QObject::connect(is, SIGNAL(frameReady(int)), this, SLOT(updateFrame(int)));
    }

	glGenTextures(1, &textureIndex);
    glBindTexture(GL_TEXTURE_2D, textureIndex);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

	// create display list if never created
	if (!squareDisplayList){
		squareDisplayList = glGenLists(1);
		glNewList(squareDisplayList, GL_COMPILE);
		{
			//qglColor(QColor::fromRgb(100, 100, 100).lighter());
			glBegin(GL_QUADS); // begin drawing a square

			// Front Face (note that the texture's corners have to match the quad's corners)
			glNormal3f(0.0f, 0.0f, 1.0f); // front face points out of the screen on z.
			glTexCoord2f(0.0f, 1.0f);
			glVertex3f(-1.0f, -1.0f, 0.0f); // Bottom Left
			glTexCoord2f(1.0f, 1.0f);
			glVertex3f(1.0f, -1.0f, 0.0f); // Bottom Right
			glTexCoord2f(1.0f, 0.0f);
			glVertex3f(1.0f, 1.0f, 0.0f); // Top Right
			glTexCoord2f(0.0f, 0.0f);
			glVertex3f(-1.0f, 1.0f, 0.0f); // Top Left

			glEnd();
		}
		glEndList();
	}
}

VideoSource::~VideoSource() {
    if (fbo)
    	delete fbo;
    if (is)
    	delete is;
}



// only MixRenderWidget can call this
void VideoSource::renderTexture(){

    // update texture
    if (is && bufferChanged) {

    	glcontext->makeCurrent();

        const VideoPicture *vp = is->getPictureAtIndex(bufferIndex);
        if (vp && vp->isAllocated()) {

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

            // now we render to fbo
//            fbo->bind();
////            glClear(GL_COLOR_BUFFER_BIT);
//            // TODO ; do i need to re-bind the texture ?
//            glBindTexture(GL_TEXTURE_2D, textureIndex);
//            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
//            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
//
//    		glCallList(squareDisplayList);
//            fbo->release();

        }
        bufferChanged = false;
    }

}

void VideoSource::updateFrame (int i)
{

	bufferChanged = true;
	bufferIndex = i;

}

