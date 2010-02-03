/*
 * MixerViewWidget.h
 *
 *  Created on: Nov 9, 2009
 *      Author: bh
 */

#ifndef MIXERVIEWWIDGET_H_
#define MIXERVIEWWIDGET_H_

#include "glRenderWidget.h"
#include "View.h"

class MixerViewWidget: public glRenderWidget, View {

	Q_OBJECT

public:

    typedef enum {NONE = 0, OVER, GRAB, SELECT } actionType;

	MixerViewWidget(QWidget * parent, const QGLWidget * shareWidget = 0);
	virtual ~MixerViewWidget();

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

    SourceSet::iterator  getSourceAtCoordinates(int mouseX, int mouseY);
    void grabSource(SourceSet::iterator s, int x, int y, int dx, int dy);

//    static GLuint circle;
//    GLuint buildCircleList();

    actionType currentAction;
    QPoint lastClicPos;

    void setAction(actionType a);

};

#endif /* MIXERVIEWWIDGET_H_ */
