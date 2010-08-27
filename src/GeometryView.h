/*
 * GeometryView.h
 *
 *  Created on: Jan 31, 2010
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

#ifndef GEOMETRYVIEWWIDGET_H_
#define GEOMETRYVIEWWIDGET_H_

#include "View.h"

class GeometryView:  public View {


public:

    typedef enum {MOVE, SCALE, ROTATE, CROP} toolType;

	GeometryView();

    void paint();
    void setModelview();
    void resize(int w = -1, int h = -1);
    bool mousePressEvent(QMouseEvent *event);
    bool mouseMoveEvent(QMouseEvent *event);
    bool mouseReleaseEvent ( QMouseEvent * event );
    bool mouseDoubleClickEvent ( QMouseEvent * event );
    bool wheelEvent ( QWheelEvent * event );
    bool keyPressEvent ( QKeyEvent * event );

    // TODO void tabletEvent ( QTabletEvent * event ); // handling of tablet features like pressure and rotation

	void zoomReset();
	void zoomBestFit();

	void coordinatesFromMouse(int mouseX, int mouseY, double *X, double *Y);

	void setTool(toolType t);
	toolType getTool() { return currentTool; }

private:

    bool getSourcesAtCoordinates(int mouseX, int mouseY);
    char getSourceQuadrant(SourceSet::iterator s, int mouseX, int mouseY);
    void grabSource(SourceSet::iterator s, int x, int y, int dx, int dy);
    void scaleSource(SourceSet::iterator s, int x, int y, int dx, int dy, bool option = 0);
    void rotateSource(SourceSet::iterator s, int x, int y, int dx, int dy, bool option = 0);
    void cropSource(SourceSet::iterator s, int x, int y, int dx, int dy, bool option = 0);
    void panningBy(int x, int y, int dx, int dy);
    void setAction(actionType a);

    char quadrant;
    GLuint borderType;
    toolType currentTool;

};

#endif /* GEOMETRYVIEWWIDGET_H_ */
