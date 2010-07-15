/*
 * Cursor class
 *
 *  Created on: Jul 13, 2010
 *      Author: bh
 */

#ifndef CURSOR_H_
#define CURSOR_H_

#include "common.h"
#include <QMouseEvent>

class Cursor {

public:
	Cursor() : event(0), updated(false), active(false) { }

	/**
	 * Provide the cursor with the original mouse event
	 */
	void update(QMouseEvent *e){
		if (e->type() == QEvent::MouseButtonPress){
			pressPos 	=  QPointF(e->pos());
			active = true;
		} else if (e->type() == QEvent::MouseButtonRelease){
			releasePos 	=  QPointF(e->pos());
			active = false;
		}

		mousePos =  QPointF(e->pos());
		b = e->button();
		bs = e->buttons();
		updated = true;
	}

	/**
	 *
	 * @return returns an artificial mouse event created by modifying the the original one
	 */
	QMouseEvent *getMouseMoveEvent(){
		if (event)
			delete event;
		event = new QMouseEvent(QEvent::MouseMove, QPoint(shadowPos.x(), shadowPos.y()), b, bs, Qt::NoModifier);
		return ( event );
	}

	/**
	 * Compute the coordinates of the artificial mouse event
	 * @return True if the shadow was changed
	 */
	virtual bool apply(double fpsaverage){
		if (active && updated) {
			shadowPos = mousePos;
			updated = false;
			return true;
		}
		return false;
	}

	/**
	 * Draws the shadow
	 *
	 */
	virtual void draw(GLint viewport[4]) {
		glDisable(GL_TEXTURE_2D);
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

		glMatrixMode(GL_PROJECTION);
		glPopMatrix();
		glMatrixMode(GL_MODELVIEW);
		glPopMatrix();
		glEnable(GL_TEXTURE_2D);
	}

	/**
	 *
	 * @return True if the event was processed and used.
	 */
	virtual bool wheelEvent(QWheelEvent * event) {
		return false;
	}

protected:
	QMouseEvent *event;
	QPointF pressPos, releasePos, mousePos, shadowPos;
	Qt::MouseButton b;
	Qt::MouseButtons bs;
	bool updated, active;
};

#endif /* CURSOR_H_ */
