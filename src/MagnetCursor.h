/*
 * MagnetCursor.h
 *
 *  Created on: Oct 9, 2010
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

#ifndef MAGNETCURSOR_H_
#define MAGNETCURSOR_H_

#include "Cursor.h"

class MagnetCursor: public QObject, public Cursor
{
    Q_OBJECT

public:
    MagnetCursor();

	void update(QMouseEvent *e);
	bool apply(double fpsaverage);
	bool wheelEvent(QWheelEvent * event);
	void draw(GLint viewport[4]);

	inline int getSpeed() const { return (int) speed; }
	inline double getWaitTime() const { return waitTime; }

public Q_SLOTS:
	inline void setSpeed(int s) { speed = (double) s; }

Q_SIGNALS:
	void speedChanged(int s);

private:

	double speed;
	double waitTime;

	// timing
	double t, duration;
};

#endif /* MAGNETCURSOR_H_ */
