/*
 * OpencvSource.cpp
 *
 *  Created on: Dec 13, 2009
 *      Author: bh
 */

#include <OpencvSource.h>

OpencvSource::OpencvSource(int opencvIndex, QGLWidget *context) : Source(context)
{

	capture = cvCreateCameraCapture(opencvIndex);
//	cvGrabFrame(capture);
	if (!capture) {
		// TODO Through exception
	}
}

OpencvSource::~OpencvSource() {

	cvReleaseCapture(&capture);
}

void OpencvSource::update(){


	IplImage *frame = cvQueryFrame( capture );
	if( frame ){
		if (aspectratio == 1.0)
			aspectratio = (float)frame->width / (float) frame->height;

    	glcontext->makeCurrent();

    	// update the texture
        glBindTexture(GL_TEXTURE_2D, textureIndex);
		glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

		glTexImage2D(GL_TEXTURE_2D, 0, 4, frame->width, frame->height,0, GL_BGR, GL_UNSIGNED_BYTE, (unsigned char*) frame->imageData);
	}

}
