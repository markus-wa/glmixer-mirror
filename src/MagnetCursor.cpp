/*
 * MagnetCursor.cpp
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

#include <cmath>

#include "MagnetCursor.moc"

MagnetCursor::MagnetCursor() : Cursor(),
    radius(150.0),
    duration(0.0)
{
    targetPos = QPointF(0,0);
}

void MagnetCursor::update(QMouseEvent *e){

    Cursor::update(e);

    if (e->type() == QEvent::MouseButtonPress){
        // reset time
    }
}

bool MagnetCursor::apply(double fpsaverage){

    double dt = 1.0 / (fpsaverage < 1.0 ? 1.0 : fpsaverage);

    // animate the shadow
    if (active) {

        // where is the target ?
        float d = EUCLIDEAN(pressPos, mousePos);
        if (d > radius)
            targetPos = mousePos;
        else
            targetPos = pressPos;

        shadowPos = targetPos;

        return true;
    }

    return false;
}


bool MagnetCursor::wheelEvent(QWheelEvent * event){

    if (!active)
        return false;

    radius += (float) event->delta() / 12.0 ;
    radius = CLAMP(radius, MIN_DISTANCE, MAX_DISTANCE);

    emit radiusChanged((int)radius);

    return true;
}

void MagnetCursor::setParameter(float percent){

    radius = MIN_DISTANCE + (MAX_DISTANCE - MIN_DISTANCE) * (percent-0.1);

    emit radiusChanged((int)radius);
}

void MagnetCursor::draw(GLint viewport[4]) {

    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    gluOrtho2D(viewport[0], viewport[2], viewport[1], viewport[3]);
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();

    glColor4ub(COLOR_CURSOR, 255);

    glPointSize(10);
    glBegin(GL_POINTS);
    glVertex2d(pressPos.x(), viewport[3] - pressPos.y());
    glEnd();

    glPointSize(15);
    glBegin(GL_POINTS);
    glVertex2d(shadowPos.x(), viewport[3] - shadowPos.y());
    glEnd();

    glEnable(GL_LINE_STIPPLE);
    glLineStipple( 1, 0x9999);

    glTranslatef(mousePos.x(), viewport[3] - mousePos.y(), 0.0);
    glLineWidth(1);
    glBegin(GL_LINE_LOOP);
    for (float i = 0; i < 2.0 * M_PI; i += 0.2)
            glVertex2d( radius * cos(i), radius * sin(i));
    glEnd();

    glDisable(GL_LINE_STIPPLE);

    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
    glPopMatrix();
}
