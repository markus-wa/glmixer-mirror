/*
 * CaptureSource.cpp
 *
 *  Created on: Jun 18, 2011
 *      Author: bh
 */

#include "CaptureSource.h"

CaptureSource::CaptureSource(QImage capture, GLuint texture, double d): Source(texture, d), _capture(capture) {

	if (!_capture.isNull()) {
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, textureIndex);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA,  _capture.width(), _capture. height(),
					  0, GL_BGRA, GL_UNSIGNED_BYTE, _capture.constBits() );

		aspectratio = double(_capture.width()) / double(_capture.height());
	}
}

CaptureSource::~CaptureSource() {
	// free the OpenGL texture
	glDeleteTextures(1, &textureIndex);
}




//	void update(){
//		Source::update();
//		if (frameChanged) {
//        	glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0,  _capture.width(),
//                     _capture.height(), GL_BGRA, GL_UNSIGNED_BYTE,
//                     _capture.bits() );
//        	frameChanged = false;
//		}
//	}
