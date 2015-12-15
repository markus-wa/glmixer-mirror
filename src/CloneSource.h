/*
 * CloneSource.h
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
 *   Copyright 2009, 2012 Bruno Herbelin
 *
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

	inline QString getOriginalName() { return original->getName(); }
	inline GLuint getOriginalId() { return original->getId(); }
	int getFrameWidth() const { return original->getFrameWidth(); }
	int getFrameHeight() const { return original->getFrameHeight(); }
    double getFrameRate() const { return original->getFrameRate(); }

    // only RenderingManager can create a source
protected:
    CloneSource(SourceSet::iterator sit,  double d): Source( (*sit)->getTextureIndex(), d), original(*sit) {
        // clone the properties
        importProperties(original, true);

        // when cloning a clone, get back to the original ;
		CloneSource *tmp = dynamic_cast<CloneSource *>(original);
        if (tmp)
			original = tmp->original;
        // add this clone to the list of clones into the original source
        std::pair<SourceList::iterator,bool> ret;
        ret = original->getClones()->insert((Source *) this);
        if (!ret.second)
            SourceConstructorException().raise();
	}

	~CloneSource() {
        // remove myself from the list of clones or my original
		original->getClones()->erase((Source*) this);
        // avoid deleting the texture of the original
        textureIndex = 0;
	}


private:
	Source *original;

};

#endif /* CLONESOURCE_H_ */

