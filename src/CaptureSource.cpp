/*
 * CaptureSource.cpp
 *
 *  Created on: Jun 18, 2011
 *      Author: bh
 */

#include "CaptureSource.h"

Source::RTTI CaptureSource::type = Source::CAPTURE_SOURCE;

CaptureSource::CaptureSource(QImage capture, GLuint texture, double d): Source(texture, d), _capture(capture) {

	if (_capture.isNull())
		SourceConstructorException().raise();

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, textureIndex);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

#if QT_VERSION >= 0x040700
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8,  _capture.width(), _capture. height(),
				  0, GL_BGRA, GL_UNSIGNED_INT_8_8_8_8_REV, _capture.constBits() );
#else
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8,  _capture.width(), _capture. height(),
				  0, GL_BGRA, GL_UNSIGNED_INT_8_8_8_8_REV, _capture.bits() );
#endif

}

CaptureSource::~CaptureSource() {


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
