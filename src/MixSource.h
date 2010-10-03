/*
 * MixSource.h
 *
 *  Created on: Jul 13, 2010
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

#ifndef MIXSOURCE_H_
#define MIXSOURCE_H_

#include "Source.h"
#include "RenderingManager.h"

class MixSource: public Source
{
	friend class RenderingManager;
    friend class OutputRenderWidget;

public:

	static RTTI type;
	RTTI rtti() const { return type; }

	int getFrameWidth() const { return 0; }
	int getFrameHeight() const { return 0; }

    // only friends can create a source
protected:

	MixSource(GLuint texture, double d);
	virtual ~MixSource();
	void update();

private:
	SourceSet _sources;

};

#endif /* MIXSOURCE_H_ */
