/*
 * GeometryView.h
 *
 *  Created on: Jan 31, 2010
 *      Author: bh
 */

#ifndef GEOMETRYVIEWWIDGET_H_
#define GEOMETRYVIEWWIDGET_H_

#include "View.h"

class GeometryView:  public View {


public:

    typedef enum {NONE = 0, MOVE, SCALE, ROTATE } actionType;

	GeometryView();

    virtual void paint();
    virtual void reset();
    virtual void resize(int w, int h);
    void mousePressEvent(QMouseEvent *event);
    void mouseMoveEvent(QMouseEvent *event);
    void mouseReleaseEvent ( QMouseEvent * event );
    void mouseDoubleClickEvent ( QMouseEvent * event );
    void wheelEvent ( QWheelEvent * event );
    // TODO void tabletEvent ( QTabletEvent * event ); // handling of tablet features like pressure and rotation

	void zoomReset();
	void zoomBestFit();

private:

    bool getSourcesAtCoordinates(int mouseX, int mouseY);
    char getSourceQuadrant(SourceSet::iterator s, int mouseX, int mouseY);
    void grabSource(SourceSet::iterator s, int x, int y, int dx, int dy);
    void scaleSource(SourceSet::iterator s, int x, int y, int dx, int dy);
    void panningBy(int x, int y, int dx, int dy);

    char quadrant;
    actionType currentAction;
};

#endif /* GEOMETRYVIEWWIDGET_H_ */
