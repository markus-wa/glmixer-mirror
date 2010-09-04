/*
 * springCursor.h
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

#ifndef SPRINGCursor_H_
#define SPRINGCursor_H_

#include "Cursor.h"

class SpringCursor: public Cursor
{
public:
	SpringCursor();

	void update(QMouseEvent *e);
	bool apply(double fpsaverage);
	bool wheelEvent(QWheelEvent * event);
	void draw(GLint viewport[4]);

private:

	// parameters of the physics
	double mass, lenght, stiffness, damping, viscousness;
	// previous coordinates to compute speed
	QPointF _mousePos, _shadowPos;
	// speeds of real cursor (v) and shadow (V)
	QPointF v, V;
	// force computed
	QPointF f;
	// timing
	double t;
};

#endif /* SPRINGCursor_H_ */
