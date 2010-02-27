/*
 * CloneSource.h
 *
 *  Created on: Feb 27, 2010
 *      Author: bh
 */

#ifndef CLONESOURCE_H_
#define CLONESOURCE_H_

#include "Source.h"
#include "RenderingManager.h"

class CloneSource: public Source {

	friend class RenderingManager;

    // only RenderingManager can create a source
protected:
	CloneSource(Source *s, double d): Source(s->getTextureIndex(), d) {
		aspectratio = s->getAspectRatio();
	}
};

#endif /* CLONESOURCE_H_ */
