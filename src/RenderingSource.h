/*
 * RenderingSource.h
 *
 *  Created on: Feb 14, 2010
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
 *   Copyright 2009, 2012 Bruno Herbelin
 *
 */

#ifndef RENDERINGSOURCE_H_
#define RENDERINGSOURCE_H_

#include "Source.h"
#include "RenderingManager.h"

class RenderingSource: public Source {

	friend class RenderingManager;

public:

	static RTTI type;
	RTTI rtti() const { return type; }

    // only RenderingManager can create a source
protected:
	RenderingSource(GLuint texture, double d): Source(texture, d) {
		// configure texture display
		glBindTexture(GL_TEXTURE_2D, textureIndex);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		// increment the counter of rendering sources
		RenderingManager::getInstance()->countRenderingSource++;
		aspectratio = RenderingManager::getInstance()->getFrameBufferAspectRatio();
		name.prepend("render");
	}

	virtual ~RenderingSource() {
		// decrement the counter of rendering sources
		RenderingManager::getInstance()->countRenderingSource--;
	}

    int getFrameWidth() const {
        return RenderingManager::getInstance()->getFrameBufferWidth();
    }

    int getFrameHeight() const {
        return RenderingManager::getInstance()->getFrameBufferHeight();
    }
};

#endif /* RENDERINGSOURCE_H_ */
