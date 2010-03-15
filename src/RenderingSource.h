/*
 * RenderingSource.h
 *
 *  Created on: Feb 14, 2010
 *      Author: bh
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
		// increment the counter of rendering sources
		RenderingManager::getInstance()->countRenderingSource++;
		aspectratio = RenderingManager::getInstance()->getFrameBufferAspectRatio();
	}

	virtual ~RenderingSource() {
		// decrement the counter of rendering sources
		RenderingManager::getInstance()->countRenderingSource--;
	}

};

#endif /* RENDERINGSOURCE_H_ */
