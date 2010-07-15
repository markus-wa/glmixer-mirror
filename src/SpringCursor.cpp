/*
 * springCursor.cpp
 *
 *  Created on: Jul 13, 2010
 *      Author: bh
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
}



bool SpringCursor::apply(double fpsaverage){

	double dt = 1.0 / (fpsaverage < 1.0 ? 1.0 : fpsaverage);

	if (!active) {
		shadowPos = mousePos;
		return false;
	}

	if (updated)
		updated = false;

	// animate the shadow

	// keep old position
	_shadowPos = shadowPos;

	// new position
	shadowPos = _shadowPos + dt * mass *(mousePos - shadowPos) ;

	// if the shadow reached the real cursor position
	// then return false
	// else return true

//	qDebug("dist %f", euclidean(POS, _POS));
//	qDebug("mana %f", (POS - pos).manhattanLength());

	return (shadowPos - mousePos).manhattanLength() > 1.0 ;

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
