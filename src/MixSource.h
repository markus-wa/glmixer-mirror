/*
 * MixSource.h
 *
 *  Created on: Jul 13, 2010
 *      Author: bh
 */

#ifndef MIXSOURCE_H_
#define MIXSOURCE_H_

#include "Source.h"
#include "RenderingManager.h"

class MixSource: public Source
{
	friend class RenderingManager;

public:

	static RTTI type;
	RTTI rtti() const { return type; }

    // only RenderingManager can create a source
protected:

	MixSource(GLuint texture, double d);
	virtual ~MixSource();
	void update();

private:
	SourceSet _sources;

};

#endif /* MIXSOURCE_H_ */
