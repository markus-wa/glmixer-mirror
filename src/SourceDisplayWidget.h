/*
 * SourceDisplayWidget.h
 *
 *  Created on: Jan 31, 2010
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

#ifndef SOURCEDISPLAYWIDGET_H_
#define SOURCEDISPLAYWIDGET_H_

#include "glRenderWidget.h"

class Source;

class SourceDisplayWidget: public glRenderWidget {

    Q_OBJECT

public:
	SourceDisplayWidget(QWidget *parent = 0);

    virtual void initializeGL();
    virtual void resizeGL(int w, int h);
    virtual void paintGL();
	void setSource(Source *sourceptr);
	void playSource(bool on);

	GLuint getNewTextureIndex();
	void useAspectRatio(bool on) { use_aspect_ratio = on; }

private:
    Source *s;
    bool use_aspect_ratio;
    GLuint _bgTexture;
};

#endif /* SOURCEDISPLAYWIDGET_H_ */
