/*
 * CloneSource.h
 *
 *  Created on: Feb 27, 2010
 *      Author: bh
 */

#ifndef CLONESOURCE_H_
#define CLONESOURCE_H_

#include <algorithm>

#include "SourceSet.h"
#include "RenderingManager.h"

class CloneSource: public Source {

	friend class RenderingManager;

public:

	static RTTI type;
	RTTI rtti() const { return type; }

	QString getOriginalName() { return original->getName(); }
	GLuint getOriginalId() { return original->getId(); }

    // only RenderingManager can create a source
protected:
	CloneSource(SourceSet::iterator sit, double d): Source((*sit)->getTextureIndex(), d), original(*sit) {
		// when cloning a clone, get back to the original ;
		CloneSource *tmp = dynamic_cast<CloneSource *>(original);
		if (tmp)
			original = tmp->original;
		// add this clone to the list of clones into the original source
		original->getClones()->push_back((Source*) this);
		aspectratio = original->getAspectRatio();
	}

	~CloneSource() {
		SourceList::iterator exist = std::find( original->getClones()->begin(), original->getClones()->end(), (Source*) this);
		if (exist != original->getClones()->end())
			original->getClones()->erase(exist);
	}


private:
	Source *original;

};

#endif /* CLONESOURCE_H_ */

