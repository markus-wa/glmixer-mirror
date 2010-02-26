/*
 * ViewRenderWidget.h
 *
 *  Created on: Feb 13, 2010
 *      Author: bh
 */

#ifndef VIEWRENDERWIDGET_H_
#define VIEWRENDERWIDGET_H_

#include "common.h"
#include "glRenderWidget.h"

class View;
class MixerView;
class GeometryView;
class LayersView;

class ViewRenderWidget: public glRenderWidget {

	Q_OBJECT

	friend class RenderingManager;
	friend class Source;
	friend class MixerView;
	friend class GeometryView;
	friend class LayersView;
	friend class OutputRenderWidget;

public:
	ViewRenderWidget();
	virtual ~ViewRenderWidget();

	/**
	 * QGLWidget implementation
	 */
	void paintGL();
	void initializeGL();
	void resizeGL(int w, int h);
    void mousePressEvent(QMouseEvent *event);
    void mouseMoveEvent(QMouseEvent *event);
    void mouseReleaseEvent ( QMouseEvent * event );
    void mouseDoubleClickEvent ( QMouseEvent * event );
    void wheelEvent ( QWheelEvent * event );
    void keyPressEvent ( QKeyEvent * event );
    void hideEvent ( QHideEvent * event ) { QGLWidget::hideEvent(event); }  // keep updating even if hidden

	/**
	 * management of the manipulation views
	 */
	typedef enum {NONE = 0, MIXING=1, GEOMETRY=2, LAYER=3 } viewMode;
	void setViewMode(viewMode mode);
	QPixmap getViewIcon();


public slots:
	void zoomIn();
	void zoomOut();
	void zoomReset();
	void zoomBestFit();

	void showMessage(QString s);
	void hideMessage() { displayMessage = false; }

	void setStipplingMode(int m) { quad_half_textured = quad_stipped_textured[CLAMP(m, 0, 3)]; }

protected:

	// all the display lists
	static GLuint border_thin_shadow, border_large_shadow;
	static GLuint border_thin, border_large, border_scale;
	static GLuint frame_selection, frame_screen;
	static GLuint quad_texured, quad_half_textured, quad_black;
	static GLuint circle_mixing;
	static GLuint quad_stipped_textured[4];

	// utility to build the display lists
    GLuint buildHalfList_fine();
    GLuint buildHalfList_gross();
    GLuint buildHalfList_checkerboard();
    GLuint buildHalfList_triangle();
    GLuint buildSelectList();
    GLuint buildLineList();
    GLuint buildQuadList();
    GLuint buildCircleList();
    GLuint buildFrameList();
    GLuint buildBlackList();
    GLuint buildBordersList();

private:
    // V i e w s
	View *currentManipulationView, *noView;
	MixerView *mixingManipulationView;
	GeometryView *geometryManipulationView;
	LayersView *layersManipulationView;

	// M e s s a g e s
	QString message;
	bool displayMessage;
	QTimer messageTimer;

};

#endif /* VIEWRENDERWIDGET_H_ */
