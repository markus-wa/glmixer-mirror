/*
 * springCursor.cpp
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

#include <cmath>

#include <SpringCursor.h>


SpringCursor::SpringCursor() : Cursor()
{
	// init physics vars
	mass = 1.0;
	lenght = 0.0;
	stiffness = 0.1;
	damping = 1.0;
	viscousness = 0.01;

	t = 0.0;
}


void SpringCursor::update(QMouseEvent *e){

	Cursor::update(e);

	if (e->type() == QEvent::MouseButtonPress){
		// reset time
		t = 0.0;
	}
}

bool SpringCursor::apply(double fpsaverage){

	double dt = 1.0 / (fpsaverage < 1.0 ? 1.0 : fpsaverage);

	// animate the shadow
	if (active) {

		releasePos = mousePos;

		t += dt;

		double coef = 0.5;

		if ((shadowPos - releasePos).manhattanLength() > 1.0)
			coef += 1.0 / mass * (pressPos - shadowPos).manhattanLength() / (shadowPos - releasePos).manhattanLength();

		// interpolation
		shadowPos += dt * coef * (releasePos - shadowPos);

		// interpolation finished?
		return ((shadowPos - releasePos).manhattanLength() > 3.0);
	}

	return false;
}


bool SpringCursor::wheelEvent(QWheelEvent * event){

	if (!active)
		return false;

	mass += ((float) event->delta() * mass * 1.0) / (120.0 * 10.0) ;
	mass = CLAMP(mass, 1.0, 10.0);

	return true;
}


void SpringCursor::draw(GLint viewport[4]) {

	glMatrixMode(GL_PROJECTION);
	glPushMatrix();
	glLoadIdentity();
	gluOrtho2D(viewport[0], viewport[2], viewport[1], viewport[3]);
	glMatrixMode(GL_MODELVIEW);
	glPushMatrix();
	glLoadIdentity();

	glPointSize(10 + (10 - mass));
	glColor4ub(13, 148, 224, 255);

	glBegin(GL_POINTS);
	glVertex2d(shadowPos.x(), viewport[3] - shadowPos.y());
	glEnd();

	glLineWidth(1);
	glBegin(GL_LINES);
	glVertex2d(shadowPos.x(), viewport[3] - shadowPos.y());
	glVertex2d(releasePos.x(), viewport[3] - releasePos.y());
	glEnd();

	glMatrixMode(GL_PROJECTION);
	glPopMatrix();
	glMatrixMode(GL_MODELVIEW);
	glPopMatrix();
}

//	return Cursor::update(fpsaverage);

//	// 0. speeds
//	double dt = 1 / (fpsaverage < 0 ? 1.0 : fpsaverage);
//	vx = (event->x() - _x) * dt;
//	vy = (event->y() - _y) * dt;
////	vX = (X - _X) * dt;
////	vY = (Y - _Y) * dt;
//
//	// 1. apply viscosity to shadow speed
//	fX -= viscousness * vX;
//	fY -= viscousness * vY;
//
//	// 2. spring interaction
//	static double dx = 0.0, dy = 0.0, len = 0.0, f = 0.0;
//	dx = event->x() - X;
//	dy = event->x() - Y;
//	len = sqrt(dx*dx + dy*dy);
//
//	f = stiffness * (len - lenght);
////	f += damping * ( vx - vX ) * dx / len;
//	f *= -dx / len;
//	fX += f;
//
//	f = stiffness * (len - lenght);
////	f += damping * ( vx - vX ) * dy / len;
//	f *= -dy / len;
//	fY += f;
//
//	vX += fX * dt / mass;
//	vY += fY * dt / mass;
//	// TODO clamp velocity
//
//	X += (int)(fX * dt);
//	Y +=  (int)(fY * dt);
//	qDebug("X %d  Y %d", X, Y );

//	return false;
//}
