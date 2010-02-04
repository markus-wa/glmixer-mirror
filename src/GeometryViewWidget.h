/*
 * GeometryViewWidget.h
 *
 *  Created on: Jan 31, 2010
 *      Author: bh
 */

#ifndef GEOMETRYVIEWWIDGET_H_
#define GEOMETRYVIEWWIDGET_H_

#include "glRenderWidget.h"
#include "View.h"

class GeometryViewWidget: public glRenderWidget, public View {

	Q_OBJECT

public:

    typedef enum {NONE = 0, MOVE, SCALE, ROTATE } actionType;

	GeometryViewWidget(QWidget * parent, const QGLWidget * shareWidget = 0);
	virtual ~GeometryViewWidget();

    virtual void paintGL();
    virtual void initializeGL();
    virtual void resizeGL(int w, int h);
    void mousePressEvent(QMouseEvent *event);
    void mouseMoveEvent(QMouseEvent *event);
    void mouseReleaseEvent ( QMouseEvent * event );
    void wheelEvent ( QWheelEvent * event );
    void keyPressEvent ( QKeyEvent * event );

    // TODO void tabletEvent ( QTabletEvent * event ); // handling of tablet features like pressure and rotation

public slots:
	void zoomIn();
	void zoomOut();
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
    QPoint lastClicPos;
};

#endif /* GEOMETRYVIEWWIDGET_H_ */
