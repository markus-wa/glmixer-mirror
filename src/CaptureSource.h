/*
 * CaptureSource.h
 *
 *  Created on: Feb 27, 2010
 *      Author: bh
 */

#ifndef CAPTURESOURCE_H_
#define CAPTURESOURCE_H_

#include "Source.h"

class CaptureSource: public Source {

	friend class RenderingManager;

public:

	static RTTI type;
	RTTI rtti() const { return type; }

    // only RenderingManager can create a source
protected:
	CaptureSource(QImage capture, GLuint texture, double d): Source(texture, d), _capture(capture) {

		if (!_capture.isNull()) {
	    	glBindTexture(GL_TEXTURE_2D, textureIndex);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA,  _capture.width(), _capture. height(),
						  0, GL_BGRA, GL_UNSIGNED_BYTE, _capture.bits() );

			aspectratio = double(_capture.width()) / double(_capture.height());
			name.prepend("capture");
		}
	}

	~CaptureSource() {
		// free the OpenGL texture
		glDeleteTextures(1, &textureIndex);
	}

public:
	void update(){
		Source::update();
		if (frameChanged) {
        	glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0,  _capture.width(),
                     _capture.height(), GL_BGRA, GL_UNSIGNED_BYTE,
                     _capture.bits() );
        	frameChanged = false;
		}
	}

	inline int getFrameWidth() const { return _capture.width(); }
	inline int getFrameHeight() const { return _capture.height(); }

private:
	QImage _capture;

};

#endif /* CAPTURESOURCE_H_ */

