/*
 * CaptureSource.h
 *
 *  Created on: Feb 27, 2010
 *      Author: bh
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

#ifndef CAPTURESOURCE_H_
#define CAPTURESOURCE_H_

#include "Source.h"

class CaptureSource: public Source {

	friend class RenderingManager;
    friend class OutputRenderWidget;

public:

	static RTTI type;
	RTTI rtti() const { return type; }

    // only friends can create a source
protected:
	CaptureSource(QImage capture, GLuint texture, double d): Source(texture, d), _capture(capture) {

		if (!_capture.isNull()) {
			glActiveTexture(GL_TEXTURE0);
	    	glBindTexture(GL_TEXTURE_2D, textureIndex);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA,  _capture.width(), _capture. height(),
						  0, GL_BGRA, GL_UNSIGNED_BYTE, _capture.bits() );

			aspectratio = double(_capture.width()) / double(_capture.height());
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

	int getFrameWidth() const { return _capture.width(); }
	int getFrameHeight() const { return _capture.height(); }

private:
	QImage _capture;

};

#endif /* CAPTURESOURCE_H_ */

