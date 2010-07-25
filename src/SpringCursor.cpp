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

#define euclidean(P1, P2)  sqrt( (P1.x()-P2.x()) * (P1.x()-P2.x()) +  (P1.y()-P2.y()) * (P1.y()-P2.y()) )

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
		duration = 0.0;
		// start at press position
		shadowPos = pressPos;
	}
}

bool SpringCursor::apply(double fpsaverage){

//	return Cursor::apply(fpsaverage);

	double dt = 1.0 / (fpsaverage < 1.0 ? 1.0 : fpsaverage);

//	if (!active) {
////		shadowPos = mousePos;
//		return false;
//	}
//
//	if (updated)
//		updated = false;

	// animate the shadow


	if (active) {

		duration += dt;

		// target is the current pos if not release button
		releasePos = mousePos;



	}

	t += dt;

//	QPointF delta =  mass * dt;

	double coef = t / duration;

	// interpolation
	shadowPos = (coef) * pressPos + (1.0 - coef) * releasePos;

	// interpolation finished
	return (coef - 1.0 < EPSILON);

	// if the shadow reached the real cursor position
	// then return false
	// else return true

//	qDebug("dist %f", euclidean(POS, _POS));
//	qDebug("mana %f", (POS - pos).manhattanLength());

//	return (shadowPos - releasePos).manhattanLength() > 1.0 ;

}


bool SpringCursor::wheelEvent(QWheelEvent * event){

	if (!active)
		return false;

	mass += ((float) event->delta() * mass * 1.0) / (120.0 * 10.0) ;
	mass = CLAMP(mass, 1.0, 10.0);

	return true;
}


void SpringCursor::draw(GLint viewport[4]) {
		glDisable(GL_TEXTURE_2D);
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

		glMatrixMode(GL_PROJECTION);
		glPopMatrix();
		glMatrixMode(GL_MODELVIEW);
		glPopMatrix();
		glEnable(GL_TEXTURE_2D);
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
