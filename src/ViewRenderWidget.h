/*
 * ViewRenderWidget.h
 *
 *  Created on: Feb 13, 2010
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

#ifndef VIEWRENDERWIDGET_H_
#define VIEWRENDERWIDGET_H_

#include <QDomElement>

#include "common.h"
#include "glRenderWidget.h"
#include "SourceSet.h"

class View;
class MixerView;
class GeometryView;
class LayersView;
class CatalogView;

class Cursor;
class SpringCursor;
class DelayCursor;
class AxisCursor;
class LineCursor;
class FuzzyCursor;

class ViewRenderWidget: public glRenderWidget {

	Q_OBJECT

	friend class RenderingManager;
	friend class Source;
	friend class MixerView;
	friend class GeometryView;
	friend class LayersView;
	friend class CatalogView;
	friend class OutputRenderWidget;
	friend class SessionSwitcher;
	friend class SourceDisplayWidget;

public:
	ViewRenderWidget();
	virtual ~ViewRenderWidget();

	/**
	 * QGLWidget implementation
	 */
	void paintGL();
	void initializeGL();
	void resizeGL(int w = 0, int h = 0);
    void mousePressEvent(QMouseEvent *event);
    void mouseMoveEvent(QMouseEvent *event);
    void mouseReleaseEvent ( QMouseEvent * event );
    void mouseDoubleClickEvent ( QMouseEvent * event );
    void wheelEvent ( QWheelEvent * event );
    void keyPressEvent ( QKeyEvent * event );
    bool eventFilter(QObject *object, QEvent *event);
    void hideEvent ( QHideEvent * event ) { QGLWidget::hideEvent(event); }  // keep updating even if hidden
    void leaveEvent ( QEvent * event );

    /**
     * Specific methods
     */
    void displayFramerate();
	float getFramerate() { return f_p_s_; }
	void showFramerate(bool on);
	void setViewContextMenu(QMenu *m) { viewMenu = m; }
	void setCatalogContextMenu(QMenu *m) { catalogMenu = m; }
	void setLabels(QLabel *label, QLabel *labelFPS) { messageLabel = label; fpsLabel = labelFPS; }

	/**
	 * management of the manipulation views
	 */
	typedef enum {NONE = 0, MIXING=1, GEOMETRY=2, LAYER=3 } viewMode;
	void setViewMode(viewMode mode);
	View *getView() {return _currentView;}

	typedef enum {MOUSE_ARROW = 0, MOUSE_HAND_OPEN, MOUSE_HAND_CLOSED, MOUSE_SCALE_F, MOUSE_SCALE_B, MOUSE_ROT_TOP_RIGHT, MOUSE_ROT_TOP_LEFT, MOUSE_ROT_BOTTOM_RIGHT, MOUSE_ROT_BOTTOM_LEFT, MOUSE_QUESTION, MOUSE_SIZEALL, MOUSE_HAND_INDEX} mouseCursor;
	void setMouseCursor(mouseCursor c);

	typedef enum {TOOL_GRAB=0, TOOL_SCALE, TOOL_ROTATE, TOOL_CUT } toolMode;
	void setToolMode(toolMode m);
	toolMode getToolMode();

	typedef enum {CURSOR_NORMAL=0, CURSOR_SPRING, CURSOR_DELAY, CURSOR_AXIS, CURSOR_LINE, CURSOR_FUZZY} cursorMode;
	void setCursorMode(cursorMode m);
	cursorMode getCursorMode();

	Cursor *getCursor(cursorMode m = CURSOR_NORMAL);
//	inline SpringCursor *getSpringCursor() const { return _springCursor; }
//	inline DelayCursor *getDelayCursor() const { return _delayCursor; }
//	inline LineCursor *getLineCursor() const { return _lineCursor; }

	/**
	 * save and load configuration
	 */
	QDomElement getConfiguration(QDomDocument &doc);
	void setConfiguration(QDomElement xmlconfig);

	static inline unsigned int getStipplingMode() { return stipplingMode; }
	static inline void setStipplingMode(unsigned int m) { stipplingMode = CLAMP(m, 0, 3); }

	static inline bool filteringEnabled() { return !disableFiltering; }
	void setFilteringEnabled(bool on);

	void removeFromSelections(Source *s);

Q_SIGNALS:
	void sourceMixingModified();
	void sourceGeometryModified();
	void sourceLayerModified();

	void sourceMixingDrop(double, double);
	void sourceGeometryDrop(double, double);
	void sourceLayerDrop(double);

public Q_SLOTS:

	void clearViews();
	void zoomIn();
	void zoomOut();
	void zoomReset();
	void zoomBestFit();
	void zoomCurrentSource();
	void refresh();
	void showMessage(QString s);
	void hideMessage();
	void contextMenu(const QPoint &);
	void setCatalogVisible(bool on = false);
	void setCatalogSizeSmall();
	void setCatalogSizeMedium();
	void setCatalogSizeLarge();
	void setFaded(bool on) { faded = on; }
	void setCursorEnabled(bool on) { enableCursor = on; }

protected:

	// Shading
    static GLfloat coords[12];
    static GLfloat texc[8];
    static GLfloat maskc[8];
    static QGLShaderProgram *program;
    static bool disableFiltering;

	// all the display lists
	static GLuint border_thin_shadow, border_large_shadow;
	static GLuint border_thin, border_large, border_scale;
	static GLuint frame_selection, frame_screen, frame_screen_thin;
	static GLuint quad_texured, quad_window[2];
	static GLuint circle_mixing, layerbg;
	static GLuint mask_textures[Source::CUSTOM_MASK];
	static GLuint fading;
	static GLuint stipplingMode;
	static GLubyte stippling[];

    static void setSourceDrawingMode(bool on);

private:
    // V i e w s
	View *_currentView, *_renderView;
	MixerView *_mixingView;
	GeometryView *_geometryView;
	LayersView *_layersView;
	CatalogView *_catalogView;
	bool faded;

	// C u r s o r s
	Cursor *_currentCursor, *_normalCursor;
	SpringCursor *_springCursor;
	DelayCursor *_delayCursor;
	AxisCursor *_axisCursor;
	LineCursor *_lineCursor;
	FuzzyCursor *_fuzzyCursor;
	bool enableCursor;

	// M e s s a g e s
	QLabel *messageLabel, *fpsLabel;
	QTimer messageTimer;
	QMenu *viewMenu, *catalogMenu;

	// F P S    d i s p l a y
	QTime fpsTime_;
	unsigned int fpsCounter_;
	float f_p_s_;
	bool showFps_;

	// utility to build the display lists
    GLuint buildSelectList();
    GLuint buildLineList();
    GLuint buildTexturedQuadList();
    GLuint buildCircleList();
    GLuint buildLayerbgList();
    GLuint buildFrameList();
    GLuint buildWindowList(GLubyte r, GLubyte g, GLubyte b);
    GLuint buildBordersList();
    GLuint buildFadingList();

};

#endif /* VIEWRENDERWIDGET_H_ */
