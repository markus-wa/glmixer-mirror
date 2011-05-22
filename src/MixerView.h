/*
 * MixerView.h
 *
 *  Created on: Nov 9, 2009
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

#ifndef MIXERVIEWWIDGET_H_
#define MIXERVIEWWIDGET_H_

#include "View.h"

class MixerView: public View {

public:

	MixerView();

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

	void clear();
	void zoomReset();
	void zoomBestFit( bool onlyClickedSource = false );

	void alphaCoordinatesFromMouse(int mouseX, int mouseY, double *alphaX, double *alphaY);
    bool isInAGroup(Source *);
    void removeFromGroup(Source *s);

	QDomElement getConfiguration(QDomDocument &doc);
	void setConfiguration(QDomElement xmlconfig);

private:

    bool getSourcesAtCoordinates(int mouseX, int mouseY, bool clic = true);
    void grabSource(Source *s, int x, int y, int dx, int dy);
    void panningBy(int x, int y, int dx, int dy) ;

    void setAction(actionType a);

    // creation of groups from set of selection
	SourceListArray groupSources;
	QMap<SourceListArray::iterator, QColor> groupColor;
	GLdouble rectangleStart[2], rectangleEnd[2];
};

#endif /* MIXERVIEWWIDGET_H_ */
