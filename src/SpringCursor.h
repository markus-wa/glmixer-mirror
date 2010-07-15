/*
 * springCursor.h
 *
 *  Created on: Jul 13, 2010
 *      Author: bh
 */

#ifndef SPRINGCursor_H_
#define SPRINGCursor_H_

#include "Cursor.h"

class SpringCursor: public Cursor
{
public:
	SpringCursor();

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
};

#endif /* SPRINGCursor_H_ */
