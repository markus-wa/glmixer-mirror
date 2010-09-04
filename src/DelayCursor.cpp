/*
 * DelayCursor.cpp
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

#define euclidean(P1, P2)  sqrt( (P1.x()-P2.x()) * (P1.x()-P2.x()) +  (P1.y()-P2.y()) * (P1.y()-P2.y()) )

#include <DelayCursor.h>

DelayCursor::DelayCursor() : Cursor(), t(0.0), duration(0.0)
{

}


void DelayCursor::update(QMouseEvent *e){

	Cursor::update(e);

	if (e->type() == QEvent::MouseButtonPress){
		// reset time
		t = 0.0;
		duration = 0.0;
	}
}

bool DelayCursor::apply(double fpsaverage){

	double dt = 1.0 / (fpsaverage < 1.0 ? 1.0 : fpsaverage);

	// animate the shadow
	if (active) {

		releasePos = mousePos;

		duration += dt;


		if (duration > 1.0) {

//		duration += 2.0 * dt;

		t += dt;

//		double coef = 0.0;
//
//		if ((shadowPos - releasePos).manhattanLength() > 1)
//			coef += t *(double)(pressPos - shadowPos).manhattanLength() / (double)(shadowPos - releasePos).manhattanLength();

		// interpolation
		shadowPos = pressPos + 100.0 * t * (releasePos - pressPos ) / euclidean(releasePos, pressPos) ;
//		shadowPos += dt * coef * (releasePos - shadowPos);

//		qDebug("%f", euclidean(releasePos, pressPos));

		// interpolation finished?
		if ( euclidean(releasePos, shadowPos) < 1.0)
			active = false;


		}
		return true;
	}

	return false;
}


bool DelayCursor::wheelEvent(QWheelEvent * event){

	if (!active)
		return false;


	return true;
}


void DelayCursor::draw(GLint viewport[4]) {
//	glDisable(GL_TEXTURE_2D);
	glMatrixMode(GL_PROJECTION);
	glPushMatrix();
	glLoadIdentity();
	gluOrtho2D(viewport[0], viewport[2], viewport[1], viewport[3]);
	glMatrixMode(GL_MODELVIEW);
	glPushMatrix();
	glLoadIdentity();

	glPointSize(10);
	glColor4ub(13, 148, 224, 255);

	glBegin(GL_POINTS);
	glVertex2d(shadowPos.x(), viewport[3] - shadowPos.y());
	glEnd();

	glLineWidth(1);
	glBegin(GL_LINES);
	glVertex2d(pressPos.x(), viewport[3] - pressPos.y());
	glVertex2d(releasePos.x(), viewport[3] - releasePos.y());
	glEnd();

	glMatrixMode(GL_PROJECTION);
	glPopMatrix();
	glMatrixMode(GL_MODELVIEW);
	glPopMatrix();
//	glEnable(GL_TEXTURE_2D);
}

