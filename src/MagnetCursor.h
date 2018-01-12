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
 *   Copyright 2009, 2012 Bruno Herbelin
 *
 */

#ifndef MAGNETCURSOR_H_
#define MAGNETCURSOR_H_

#include <QObject>

#include "defines.h"
#include "Cursor.h"
#define MIN_DISTANCE 30
#define MAX_DISTANCE 300
#define MIN_STRENGTH 0.1
#define MAX_STRENGTH 1.0

class MagnetCursor: public QObject, public Cursor
{
    Q_OBJECT

public:
    MagnetCursor();

        void update(QMouseEvent *e);
        bool apply(double fpsaverage);
        bool wheelEvent(QWheelEvent * event);
        void draw(GLint viewport[4]);
        void setParameter(float percent);

        inline int getRadius() const { return radius; }
        inline double getStrength() const { return strength; }

public Q_SLOTS:
        inline void setRadius(int r) { radius = CLAMP(r, MIN_DISTANCE, MAX_DISTANCE); }
        inline void setStrength(double s) { strength = CLAMP(s, MIN_STRENGTH, MAX_STRENGTH); }

signals:
        void radiusChanged(int r);

private:

        int radius;
        double strength;
        QPointF targetPos;
        bool targethit, targetmode;

};

#endif /* MAGNETCURSOR_H_ */
