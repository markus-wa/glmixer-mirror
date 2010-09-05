/*
 * DelayCursor.h
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

#ifndef DelayCursor_H_
#define DelayCursor_H_

#include <QObject>

#include "Cursor.h"
#define MIN_SPEED 30
#define MAX_SPEED 250
#define MIN_WAIT 0.2
#define MAX_WAIT 3.0

class DelayCursor: public QObject, public Cursor
{
    Q_OBJECT

public:
	DelayCursor();

	void update(QMouseEvent *e);
	bool apply(double fpsaverage);
	bool wheelEvent(QWheelEvent * event);
	void draw(GLint viewport[4]);

	inline int getSpeed() const { return (int) speed; }
	inline double getWaitTime() const { return waitTime; }

public Q_SLOTS:
	inline void setSpeed(int s) { speed = (double) CLAMP(s, MIN_SPEED, MAX_SPEED); }
	inline void setWaitTime(double t) { waitTime = CLAMP(t, MIN_WAIT, MAX_WAIT); }

Q_SIGNALS:
	void speedChanged(int s);

private:

	double speed;
	double waitTime;

	// timing
	double t, duration;
};

#endif /* DelayCursor_H_ */
