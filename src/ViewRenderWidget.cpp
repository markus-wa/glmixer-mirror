/*
 * ViewRenderWidget.cpp
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

#include "ViewRenderWidget.moc"

#include "View.h"
#include "MixerView.h"
#include "GeometryView.h"
#include "LayersView.h"
#include "RenderingManager.h"
#include "OutputRenderWindow.h"
#include "CatalogView.h"
#include "Cursor.h"
#include "SpringCursor.h"
#include "DelayCursor.h"

#define PROGRAM_VERTEX_ATTRIBUTE 0
#define PROGRAM_TEXCOORD_ATTRIBUTE 1
#define PROGRAM_MASKCOORD_ATTRIBUTE 2

GLuint ViewRenderWidget::border_thin_shadow = 0,
		ViewRenderWidget::border_large_shadow = 0;
GLuint ViewRenderWidget::border_thin = 0, ViewRenderWidget::border_large = 0;
GLuint ViewRenderWidget::border_scale = 0;
GLuint ViewRenderWidget::quad_texured = 0, ViewRenderWidget::quad_window[] = {0, 0};
GLuint ViewRenderWidget::frame_selection = 0, ViewRenderWidget::frame_screen = 0, ViewRenderWidget::frame_screen_thin = 0;
GLuint ViewRenderWidget::circle_mixing = 0, ViewRenderWidget::layerbg = 0,
		ViewRenderWidget::catalogbg = 0;
GLuint ViewRenderWidget::mask_textures[] = { 0, 0, 0, 0, 0, 0, 0, 0 };
GLuint ViewRenderWidget::fading = 0;

GLuint ViewRenderWidget::stipplingMode = 0;

GLubyte ViewRenderWidget::stippling[] = {
		// stippling fine
		0xAA, 0xAA, 0xAA, 0xAA, 0x55, 0x55, 0x55, 0x55, 0xAA, 0xAA, 0xAA, 0xAA,
		0x55, 0x55, 0x55, 0x55, 0xAA, 0xAA, 0xAA, 0xAA, 0x55, 0x55, 0x55,
		0x55, 0xAA, 0xAA, 0xAA, 0xAA, 0x55, 0x55, 0x55, 0x55, 0xAA, 0xAA,
		0xAA, 0xAA, 0x55, 0x55, 0x55, 0x55, 0xAA, 0xAA, 0xAA, 0xAA, 0x55,
		0x55, 0x55, 0x55, 0xAA, 0xAA, 0xAA, 0xAA, 0x55, 0x55, 0x55, 0x55,
		0xAA, 0xAA, 0xAA, 0xAA, 0x55, 0x55, 0x55, 0x55, 0xAA, 0xAA, 0xAA,
		0xAA, 0x55, 0x55, 0x55, 0x55, 0xAA, 0xAA, 0xAA, 0xAA, 0x55, 0x55,
		0x55, 0x55, 0xAA, 0xAA, 0xAA, 0xAA, 0x55, 0x55, 0x55, 0x55, 0xAA,
		0xAA, 0xAA, 0xAA, 0x55, 0x55, 0x55, 0x55, 0xAA, 0xAA, 0xAA, 0xAA,
		0x55, 0x55, 0x55, 0x55, 0xAA, 0xAA, 0xAA, 0xAA, 0x55, 0x55, 0x55,
		0x55, 0xAA, 0xAA, 0xAA, 0xAA, 0x55, 0x55, 0x55, 0x55, 0xAA, 0xAA,
		0xAA, 0xAA, 0x55, 0x55, 0x55, 0x55 ,
		// stippling gross
		0xCC, 0xCC, 0xCC, 0xCC, 0x33, 0x33, 0x33, 0x33, 0x33, 0x33, 0x33, 0x33,
		0xCC, 0xCC, 0xCC, 0xCC, 0xCC, 0xCC, 0xCC, 0xCC, 0x33, 0x33, 0x33,
		0x33, 0x33, 0x33, 0x33, 0x33, 0xCC, 0xCC, 0xCC, 0xCC, 0xCC, 0xCC,
		0xCC, 0xCC, 0x33, 0x33, 0x33, 0x33, 0x33, 0x33, 0x33, 0x33, 0xCC,
		0xCC, 0xCC, 0xCC, 0xCC, 0xCC, 0xCC, 0xCC, 0x33, 0x33, 0x33, 0x33,
		0x33, 0x33, 0x33, 0x33, 0xCC, 0xCC, 0xCC, 0xCC, 0xCC, 0xCC, 0xCC,
		0xCC, 0x33, 0x33, 0x33, 0x33, 0x33, 0x33, 0x33, 0x33, 0xCC, 0xCC,
		0xCC, 0xCC, 0xCC, 0xCC, 0xCC, 0xCC, 0x33, 0x33, 0x33, 0x33, 0x33,
		0x33, 0x33, 0x33, 0xCC, 0xCC, 0xCC, 0xCC, 0xCC, 0xCC, 0xCC, 0xCC,
		0x33, 0x33, 0x33, 0x33, 0x33, 0x33, 0x33, 0x33, 0xCC, 0xCC, 0xCC,
		0xCC, 0xCC, 0xCC, 0xCC, 0xCC, 0x33, 0x33, 0x33, 0x33, 0x33, 0x33,
		0x33, 0x33, 0xCC, 0xCC, 0xCC, 0xCC ,
		// stippling lines
	    0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA,
	    0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA,
	    0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA,
	    0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA,
	    0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA,
	    0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA,
	    0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA,
	    0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA,
	    0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA,
	    0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA,
	    0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA,
	    0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA,
	    0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA,
	    0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA,
	    0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA,
	    0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA,
		// stippling GLM
	    0x7B, 0xA2, 0x7B, 0xA2, 0x7B, 0xA2, 0x7B, 0xA2,
	    0x4A, 0x22, 0x4A, 0x22, 0x4A, 0x22, 0x4A, 0x22,
	    0x4A, 0x22, 0x4A, 0x22, 0x4A, 0x22, 0x4A, 0x22,
	    0x5A, 0x2A, 0x5A, 0x2A, 0x5A, 0x2A, 0x5A, 0x2A,
	    0x42, 0x36, 0x42, 0x36, 0x42, 0x36, 0x42, 0x36,
	    0x42, 0x36, 0x42, 0x36, 0x42, 0x36, 0x42, 0x36,
	    0x7A, 0x22, 0x7A, 0x22, 0x7A, 0x22, 0x7A, 0x22,
	    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	    0x7B, 0xA2, 0x7B, 0xA2, 0x7B, 0xA2, 0x7B, 0xA2,
	    0x4A, 0x22, 0x4A, 0x22, 0x4A, 0x22, 0x4A, 0x22,
	    0x4A, 0x22, 0x4A, 0x22, 0x4A, 0x22, 0x4A, 0x22,
	    0x5A, 0x2A, 0x5A, 0x2A, 0x5A, 0x2A, 0x5A, 0x2A,
	    0x42, 0x36, 0x42, 0x36, 0x42, 0x36, 0x42, 0x36,
	    0x42, 0x36, 0x42, 0x36, 0x42, 0x36, 0x42, 0x36,
	    0x7A, 0x22, 0x7A, 0x22, 0x7A, 0x22, 0x7A, 0x22,
	    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

GLfloat ViewRenderWidget::coords[12];
GLfloat ViewRenderWidget::texc[8];
GLfloat ViewRenderWidget::maskc[8];
QGLShaderProgram *ViewRenderWidget::program = 0;

ViewRenderWidget::ViewRenderWidget() :
	glRenderWidget(), messageLabel(0), fpsLabel(0), faded(false), viewMenu(0), catalogMenu(0), showFps_(0)
{

	setMouseTracking(true);
	setFocusPolicy(Qt::ClickFocus);
	setMouseCursor(MOUSE_ARROW);

	// create the main views
	_renderView = new View;
	Q_CHECK_PTR(_renderView);
	_mixingView = new MixerView;
	Q_CHECK_PTR(_mixingView);
	_geometryView = new GeometryView;
	Q_CHECK_PTR(_geometryView);
	_layersView = new LayersView;
	Q_CHECK_PTR(_layersView);
	// sets the current view
	_currentView = _renderView;

	// create the catalog view
	_catalogView = new CatalogView;
	Q_CHECK_PTR(_catalogView);

	// create the cursors
	_normalCursor = new Cursor;
	Q_CHECK_PTR(_normalCursor);
	_springCursor = new SpringCursor;
	Q_CHECK_PTR(_springCursor);
	_delayCursor = new DelayCursor;
	Q_CHECK_PTR(_delayCursor);
	// sets the current cursor
	_currentCursor = _normalCursor;

	// opengl HID display
	connect(&messageTimer, SIGNAL(timeout()), SLOT(hideMessage()));
	messageTimer.setSingleShot(true);
	fpsTime_.start();
	fpsCounter_ = 0;
	f_p_s_ = 1000.0 / period;

	// qt context menu
	setContextMenuPolicy(Qt::CustomContextMenu);
	connect(this, SIGNAL(customContextMenuRequested(const QPoint &)), this,
			SLOT(contextMenu(const QPoint &)));

	installEventFilter(this);
}

ViewRenderWidget::~ViewRenderWidget()
{
	if (_renderView)
		delete _renderView;
	if (_mixingView)
		delete _mixingView;
	if (_geometryView)
		delete _geometryView;
	if (_layersView)
		delete _layersView;
	if (_catalogView)
		delete _catalogView;
}


void ViewRenderWidget::initializeGL()
{
	glRenderWidget::initializeGL();
	buildShader();

	setBackgroundColor(QColor(52, 52, 52));

	border_thin_shadow = buildLineList();
	border_large_shadow = border_thin_shadow + 1;
	quad_texured = buildTexturedQuadList();
	frame_selection = buildSelectList();
	circle_mixing = buildCircleList();
	layerbg = buildLayerbgList();
	catalogbg = buildCatalogbgList();
	quad_window[0] = buildWindowList(0, 0, 0);
	quad_window[1] = buildWindowList(255, 255, 255);
	frame_screen = buildFrameList();
	frame_screen_thin = frame_screen + 1;
	border_thin = buildBordersList();
	border_large = border_thin + 1;
	border_scale = border_thin + 2;
	fading = buildFadingList();

	if (!mask_textures[0])
	{
		glActiveTexture(GL_TEXTURE1);
		glEnable(GL_TEXTURE_2D);
		const char * const black_xpm[] = { "2 2 1 1", ". c #000000", "..", ".."};
		mask_textures[Source::NO_MASK] = bindTexture(QPixmap(black_xpm), GL_TEXTURE_2D);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_ADD);
		mask_textures[Source::ROUNDCORNER_MASK] = bindTexture(QPixmap(QString::fromUtf8(
				":/glmixer/textures/mask_roundcorner.png")), GL_TEXTURE_2D);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_COMBINE);
		glTexEnvf(GL_TEXTURE_ENV, GL_COMBINE_ALPHA, GL_MODULATE);
		glTexEnvf(GL_TEXTURE_ENV, GL_COMBINE_RGB, GL_ADD);
		mask_textures[Source::CIRCLE_MASK] = bindTexture(QPixmap(QString::fromUtf8(
				":/glmixer/textures/mask_circle.png")), GL_TEXTURE_2D);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_COMBINE);
		glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_ALPHA, GL_MODULATE);
		glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_RGB, GL_ADD);
		mask_textures[Source::GRADIENT_CIRCLE_MASK] = bindTexture(QPixmap(QString::fromUtf8(
				":/glmixer/textures/mask_linear_circle.png")), GL_TEXTURE_2D);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_COMBINE);
		glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_ALPHA, GL_MODULATE);
		glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_RGB, GL_ADD);
		mask_textures[Source::GRADIENT_SQUARE_MASK] = bindTexture(QPixmap(QString::fromUtf8(
				":/glmixer/textures/mask_linear_square.png")), GL_TEXTURE_2D);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_COMBINE);
		glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_ALPHA, GL_MODULATE);
		glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_RGB, GL_ADD);
		mask_textures[Source::GRADIENT_LEFT_MASK] = bindTexture(QPixmap(QString::fromUtf8(
				":/glmixer/textures/mask_linear_left.png")), GL_TEXTURE_2D);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_COMBINE);
		glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_ALPHA, GL_MODULATE);
		glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_RGB, GL_ADD);
		mask_textures[Source::GRADIENT_RIGHT_MASK] = bindTexture(QPixmap(QString::fromUtf8(
				":/glmixer/textures/mask_linear_right.png")), GL_TEXTURE_2D);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_COMBINE);
		glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_ALPHA, GL_MODULATE);
		glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_RGB, GL_ADD);
		mask_textures[Source::GRADIENT_TOP_MASK] = bindTexture(QPixmap(QString::fromUtf8(
				":/glmixer/textures/mask_linear_top.png")), GL_TEXTURE_2D);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_COMBINE);
		glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_ALPHA, GL_MODULATE);
		glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_RGB, GL_ADD);
		mask_textures[Source::GRADIENT_BOTTOM_MASK] = bindTexture(QPixmap(QString::fromUtf8(
				":/glmixer/textures/mask_linear_bottom.png")), GL_TEXTURE_2D);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_COMBINE);
		glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_ALPHA, GL_MODULATE);
		glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_RGB, GL_ADD);

		glDisable(GL_TEXTURE_2D);
		glActiveTexture(GL_TEXTURE0);
	}

	// store render View matrices
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	gluOrtho2D(-SOURCE_UNIT * OutputRenderWindow::getInstance()->getAspectRatio(), SOURCE_UNIT * OutputRenderWindow::getInstance()->getAspectRatio(), -SOURCE_UNIT, SOURCE_UNIT);
	glGetDoublev(GL_PROJECTION_MATRIX, _renderView->projection);
	glGetDoublev(GL_PROJECTION_MATRIX, _catalogView->projection);

//	glMatrixMode(GL_MODELVIEW);
//	glLoadIdentity();
//	glGetDoublev(GL_MODELVIEW_MATRIX, _renderView->modelview);
//	glGetDoublev(GL_MODELVIEW_MATRIX, _catalogView->modelview);

}

void ViewRenderWidget::setViewMode(viewMode mode)
{
	switch (mode)
	{
	case ViewRenderWidget::MIXING:
		_currentView = (View *) _mixingView;
		break;
	case ViewRenderWidget::GEOMETRY:
		_currentView = (View *) _geometryView;
		break;
	case ViewRenderWidget::LAYER:
		_currentView = (View *) _layersView;
		break;
	case ViewRenderWidget::NONE:
	default:
		_currentView = _renderView;
	}

	// update view to match with the changes in modelview and projection matrices (e.g. resized widget)
	makeCurrent();
	refresh();
}

void ViewRenderWidget::setCatalogVisible(bool on)
{
	_catalogView->setVisible(on);
}

void ViewRenderWidget::setCatalogSizeSmall()
{
	_catalogView->setSize(CatalogView::SMALL);
}

void ViewRenderWidget::setCatalogSizeMedium()
{
	_catalogView->setSize(CatalogView::MEDIUM);
}

void ViewRenderWidget::setCatalogSizeLarge()
{
	_catalogView->setSize(CatalogView::LARGE);
}

void ViewRenderWidget::contextMenu(const QPoint &pos)
{

	if (_catalogView->isInside(pos) && catalogMenu)
	{
		catalogMenu->exec(mapToGlobal(pos));
	}
	else if (viewMenu && _currentView->noSourceClicked())
	{
		viewMenu->exec(mapToGlobal(pos));
	}
}

void ViewRenderWidget::setToolMode(toolMode m){

	if (_currentView == (View *) _geometryView) {
		_geometryView->setTool( (GeometryView::toolType) m );
	}

}

ViewRenderWidget::toolMode ViewRenderWidget::getToolMode(){

	if (_currentView == (View *) _geometryView) {
		return (ViewRenderWidget::toolMode) _geometryView->getTool();
	}
	else
		return ViewRenderWidget::TOOL_GRAB;
}


void ViewRenderWidget::setCursorMode(cursorMode m){

	switch(m) {
	case ViewRenderWidget::CURSOR_DELAY:
		_currentCursor = _delayCursor;
		break;
	case ViewRenderWidget::CURSOR_SPRING:
		_currentCursor = _springCursor;
		break;
	default:
	case ViewRenderWidget::CURSOR_NORMAL:
		_currentCursor = _normalCursor;
	}

}

ViewRenderWidget::cursorMode ViewRenderWidget::getCursorMode(){

	if (_currentCursor == _springCursor)
		return ViewRenderWidget::CURSOR_SPRING;

	return ViewRenderWidget::CURSOR_NORMAL;
}

/**
 *  REDIRECT every calls to the current view implementation
 */

void ViewRenderWidget::resizeGL(int w, int h)
{
	// modify catalog view
	_catalogView->resize(w, h);

	// resize the view taking
	_currentView->resize(w, h);
}

void ViewRenderWidget::refresh()
{
	makeCurrent();

	// store render View matrices ; output render window may have been resized, and the ViewRenderWidget is told so if necessary
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	gluOrtho2D(-SOURCE_UNIT * OutputRenderWindow::getInstance()->getAspectRatio(), SOURCE_UNIT * OutputRenderWindow::getInstance()->getAspectRatio(), -SOURCE_UNIT, SOURCE_UNIT);
	glGetDoublev(GL_PROJECTION_MATRIX, _renderView->projection);
	glGetDoublev(GL_PROJECTION_MATRIX, _catalogView->projection);

	// default resize ; will refresh everything
	_currentView->resize(width(), height());
}

void ViewRenderWidget::paintGL()
{
	//
	// 1. The view
	//
	// background clear
	glRenderWidget::paintGL();

	// apply modelview transformations from zoom and panning only when requested
	if (_currentView->isModified()) {
		_currentView->setModelview();
		// update modelview-projection matrix of the shader
//		program->setUniformValue("ModelViewProjectionMatrix",QMatrix4x4 (_currentView->projection) * QMatrix4x4 (_currentView->modelview) );
	}
	// draw the view
	_currentView->paint();

	// draw a semi-transparent overlay if view should be faded
	if (faded)
	{
		glCallList(ViewRenderWidget::fading);
		setMouseCursor(MOUSE_ARROW);
	}

	//
	// 2. the shadow of the cursor
	//
	if (_currentCursor->apply(f_p_s_) ) {

		_currentCursor->draw(_currentView->viewport);

		if (_currentView->mouseMoveEvent( _currentCursor->getMouseMoveEvent() ))
		{
			// the view 'mouseMoveEvent' returns true ; there was something changed!
			if (_currentView == _mixingView)
				emit sourceMixingModified();
			else if (_currentView == _geometryView)
				emit sourceGeometryModified();
			else if (_currentView == _layersView)
				emit sourceLayerModified();
		}
	}

	//
	// 3. The catalog view with transparency
	//
	if (_catalogView->visible())
		_catalogView->paint();

	//
	// 4. The extra information
	//
	// FPS computation
	if (++fpsCounter_ == 10)
	{
		f_p_s_ = 1000.0 * 10.0 / fpsTime_.restart();
		fpsCounter_ = 0;

		if (fpsLabel && showFps_) {
			fpsLabel->setText(QString("%1 fps").arg(f_p_s_, 0, 'f', ((f_p_s_ < 10.0) ? 1 : 0)) );
		}
	}
	// HUD display of framerate (on request or if FPS is dangerously slow)
	if (showFps_ || ( f_p_s_ < 25 && f_p_s_ > 0) )
		displayFramerate();
}

void ViewRenderWidget::displayFramerate()
{

	glMatrixMode(GL_PROJECTION);
	glPushMatrix();
	glLoadIdentity();
	gluOrtho2D(0.0, width(), 0.0, height());

	glMatrixMode(GL_MODELVIEW);
	glPushMatrix();
	glLoadIdentity();

	qglColor(Qt::lightGray);
	glRecti(width() - 71, height() - 1, width() - 9, height() - 11);
	qglColor(f_p_s_ > 40.f ? Qt::darkGreen : (f_p_s_ > 20.f ? Qt::yellow : Qt::red));
	// Draw a filled rectangle with current color
	glRecti(width() - 70, height() - 2, width() - 70 + (int)f_p_s_, height() - 10);

	glMatrixMode(GL_PROJECTION);
	glPopMatrix();
	glMatrixMode(GL_MODELVIEW);
	glPopMatrix();


}


void ViewRenderWidget::showFramerate(bool on)
{
	showFps_ = on;

	if (fpsLabel)
		fpsLabel->clear();

}

void ViewRenderWidget::mousePressEvent(QMouseEvent *event)
{
	makeCurrent();

	_currentCursor->update(event);

	if (_catalogView->mousePressEvent(event))
		return;

	// inform the view of the mouse press event
	if (!_currentView->mousePressEvent(event))
	{

		// if there is something to drop, inform the rendering manager that it can drop the source at the clic coordinates
		if (RenderingManager::getInstance()->getSourceBasketTop())
		{

			// depending on the view, ask the rendering manager to drop the source with the user parameters
			if (_currentView == _mixingView)
			{
				double ax = 0.0, ay = 0.0;
				_mixingView->alphaCoordinatesFromMouse(event->x(),
						event->y(), &ax, &ay);
				emit sourceMixingDrop(ax, ay);
			}
			else if (_currentView == _geometryView)
			{
				double x = 0.0, y = 0.0;
				_geometryView->coordinatesFromMouse(event->x(),
						event->y(), &x, &y);
				emit sourceGeometryDrop(x, y);
			}
			else if (_currentView == _layersView)
			{
				double d = 0.0, dumm;
				_layersView->unProjectDepth(event->x(), event->y(),
						0.0, 0.0, &d, &dumm);
				emit sourceLayerDrop(d);
			}
		}
		else
			// the mouse press was not treated ; forward it
			QWidget::mousePressEvent(event);
	}

}

void ViewRenderWidget::mouseMoveEvent(QMouseEvent *event)
{
	makeCurrent();


	if (event->buttons() == Qt::NoButton && _catalogView->mouseMoveEvent(event))
	{
		setFaded(true);
		return;
	}
	else
		setFaded(false);

//	// if there is a source to drop, direct cursor
//	if ( RenderingManager::getInstance()->getSourceBasketTop() )
//		_currentView->mouseMoveEvent(event);
//	// else, indirect cursor
//	else


	if (_currentCursor->isActive())
		_currentCursor->update(event);
	else
		_currentView->mouseMoveEvent(event);

//	else
//	{   // the view 'mouseMoveEvent' returns true ; there was something changed!
//		if (_currentView == _mixingView)
//			emit sourceMixingModified();
//		else if (_currentView == _geometryView)
//			emit sourceGeometryModified();
//		else if (_currentView == _layersView)
//			emit sourceLayerModified();
//	}

}

void ViewRenderWidget::mouseReleaseEvent(QMouseEvent * event)
{
	makeCurrent();

	_currentCursor->update(event);

	if (_catalogView->mouseReleaseEvent(event))
		return;

	if (!_currentView->mouseReleaseEvent(event))
		QWidget::mouseReleaseEvent(event);
}

void ViewRenderWidget::mouseDoubleClickEvent(QMouseEvent * event)
{
	makeCurrent();

	if (_catalogView->mouseDoubleClickEvent(event))
		return;

	if (!_currentView->mouseDoubleClickEvent(event))
		QWidget::mouseDoubleClickEvent(event);
	else
	{   // special case ; double clic changes geometry
		if (_currentView == _geometryView)
			emit sourceGeometryModified();
	}
}

void ViewRenderWidget::wheelEvent(QWheelEvent * event)
{
	makeCurrent();

	if (_catalogView->wheelEvent(event))
		return;

	if (_currentCursor->wheelEvent(event))
		return;

	if (!_currentView->wheelEvent(event))
		QWidget::wheelEvent(event);

	showMessage(QString("%1 \%").arg(_currentView->getZoomPercent(), 0, 'f', 1));
}

void ViewRenderWidget::keyPressEvent(QKeyEvent * event)
{
	makeCurrent();

	if (!_currentView->keyPressEvent(event))
		QWidget::keyPressEvent(event);
	else
	{   // the view 'keyPressEvent' returns true ; there was something changed!
		if (_currentView == _mixingView)
			emit sourceMixingModified();
		else if (_currentView == _geometryView)
			emit sourceGeometryModified();
		else if (_currentView == _layersView)
			emit sourceLayerModified();
	}
}

/**
 * Tab key switches to the next source, CTRl-Tab the previous.
 *
 * NB: I wanted SHIFT-Tab for the previous, but this event is captured
 * by the main application.
 */
bool ViewRenderWidget::eventFilter(QObject *object, QEvent *event)
{
	if (object == (QObject *) (this) && event->type() == QEvent::KeyPress)
	{
		QKeyEvent *keyEvent = static_cast<QKeyEvent *> (event);
		if (keyEvent->key() == Qt::Key_Tab)
		{
			if (keyEvent->modifiers() & Qt::ControlModifier)
				RenderingManager::getInstance()->setCurrentPrevious();
			else
				RenderingManager::getInstance()->setCurrentNext();
			return true;
		}
	}

	// standard event processing
	 return QWidget::eventFilter(object, event);
}

void ViewRenderWidget::leaveEvent ( QEvent * event ){

	setFaded(false);
	_catalogView->setTransparent(true);

	QWidget::leaveEvent(event);
}

void ViewRenderWidget::zoomIn()
{
	makeCurrent();
	_currentView->zoomIn();

	showMessage(QString("%1 \%").arg(_currentView->getZoomPercent(), 0, 'f', 1));
}

void ViewRenderWidget::zoomOut()
{
	makeCurrent();
	_currentView->zoomOut();

	showMessage(QString("%1 \%").arg(_currentView->getZoomPercent(), 0, 'f', 1));
}

void ViewRenderWidget::zoomReset()
{
	makeCurrent();
	_currentView->zoomReset();

	showMessage(QString("%1 \%").arg(_currentView->getZoomPercent(), 0, 'f', 1));
}

void ViewRenderWidget::zoomBestFit()
{
	makeCurrent();
	_currentView->zoomBestFit();

	showMessage(QString("%1 \%").arg(_currentView->getZoomPercent(), 0, 'f', 1));
}

void ViewRenderWidget::clearViews()
{
	// clear all views
	_mixingView->clear();
	_geometryView->clear();
	_layersView->clear();

	refresh();
}

void ViewRenderWidget::showMessage(QString s)
{
	messageTimer.stop();
	if (messageLabel)
		messageLabel->setText(s);
	messageTimer.start(1000);
}

void ViewRenderWidget::hideMessage()
{
	messageLabel->clear();
}
/**
 * save and load configuration
 */
QDomElement ViewRenderWidget::getConfiguration(QDomDocument &doc)
{
	QDomElement config = doc.createElement("Views");

	if (_currentView == _mixingView)
		config.setAttribute("current", ViewRenderWidget::MIXING);
	else if (_currentView == _geometryView)
		config.setAttribute("current", ViewRenderWidget::GEOMETRY);
	else if (_currentView == _layersView)
		config.setAttribute("current", ViewRenderWidget::LAYER);

	QDomElement mix = doc.createElement("View");
	mix.setAttribute("name", "Mixing");
	config.appendChild(mix);
	{
		QDomElement z = doc.createElement("Zoom");
		z.setAttribute("value", _mixingView->getZoom());
		mix.appendChild(z);

		QDomElement pos = doc.createElement("Panning");
		pos.setAttribute("X", _mixingView->getPanningX());
		pos.setAttribute("Y", _mixingView->getPanningY());
		mix.appendChild(pos);
	}

	QDomElement geom = doc.createElement("View");
	geom.setAttribute("name", "Geometry");
	config.appendChild(geom);
	{
		QDomElement z = doc.createElement("Zoom");
		z.setAttribute("value", _geometryView->getZoom());
		geom.appendChild(z);

		QDomElement pos = doc.createElement("Panning");
		pos.setAttribute("X", _geometryView->getPanningX());
		pos.setAttribute("Y", _geometryView->getPanningY());
		geom.appendChild(pos);
	}

	QDomElement depth = doc.createElement("View");
	depth.setAttribute("name", "Depth");
	config.appendChild(depth);
	{
		QDomElement z = doc.createElement("Zoom");
		z.setAttribute("value", _layersView->getZoom());
		depth.appendChild(z);

		QDomElement pos = doc.createElement("Panning");
		pos.setAttribute("X", _layersView->getPanningX());
		pos.setAttribute("Y", _layersView->getPanningY());
		depth.appendChild(pos);
	}

	QDomElement catalog = doc.createElement("Catalog");
	config.appendChild(catalog);
	catalog.setAttribute("visible", _catalogView->visible());
	QDomElement s = doc.createElement("Parameters");
	s.setAttribute("catalogSize", _catalogView->getSize());
	catalog.appendChild(s);

	return config;
}

void ViewRenderWidget::setConfiguration(QDomElement xmlconfig)
{

	QDomElement child = xmlconfig.firstChildElement("View");
	while (!child.isNull()) {
		if (child.attribute("name") == "Mixing") {
			_mixingView->setZoom(child.firstChildElement("Zoom").attribute("value").toFloat());
			_mixingView->setPanningX(child.firstChildElement("Panning").attribute("X").toFloat());
			_mixingView->setPanningY(child.firstChildElement("Panning").attribute("Y").toFloat());
		}
		if (child.attribute("name") == "Geometry") {
			_geometryView->setZoom(child.firstChildElement("Zoom").attribute("value").toFloat());
			_geometryView->setPanningX(child.firstChildElement("Panning").attribute("X").toFloat());
			_geometryView->setPanningY(child.firstChildElement("Panning").attribute("Y").toFloat());
		}
		if (child.attribute("name") == "Depth") {
			_layersView->setZoom(child.firstChildElement("Zoom").attribute("value").toFloat());
			_layersView->setPanningX(child.firstChildElement("Panning").attribute("X").toFloat());
			_layersView->setPanningY(child.firstChildElement("Panning").attribute("Y").toFloat());
		}
		// TODO xlm of catalog view
		child = child.nextSiblingElement();
	}

}



void ViewRenderWidget::buildShader(){

	coords[0] = -1;
	coords[1] = 1;
	coords[2] = 0;
	coords[3] = 1;
	coords[4] = 1;
	coords[5] = 0;
	coords[6] = 1;
	coords[7] = -1;
	coords[8] = 0;
	coords[9] = -1;
	coords[10] = -1;
	coords[11] = 0;
	texc[0] = 0.f;
	texc[1] = 0.f;
	texc[2] = 1.f;
	texc[3] = 0.f;
	texc[4] = 1.f;
	texc[5] = 1.f;
	texc[6] = 0.f;
	texc[7] = 1.f;
	maskc[0] = 0.f;
	maskc[1] = 1.f;
	maskc[2] = 1.f;
	maskc[3] = 1.f;
	maskc[4] = 1.f;
	maskc[5] = 0.f;
	maskc[6] = 0.f;
	maskc[7] = 0.f;

	program = new QGLShaderProgram(this);
	program->addShaderFromSourceFile(QGLShader::Vertex, ":/glmixer/shaders/imageProcessing_vertex.glsl");
	program->addShaderFromSourceFile(QGLShader::Fragment, ":/glmixer/shaders/imageProcessing_fragment.glsl");
	program->bindAttributeLocation("vertex", PROGRAM_VERTEX_ATTRIBUTE);
	program->bindAttributeLocation("texCoord", PROGRAM_TEXCOORD_ATTRIBUTE);
	program->bindAttributeLocation("maskCoord", PROGRAM_MASKCOORD_ATTRIBUTE);

	program->link();

	program->bind();
	program->setUniformValue("sourceTexture", 0);
	program->setUniformValue("maskTexture", 1);
	program->setUniformValue("utilityTexture", 2);

	program->setUniformValue("sourceDrawing", false);
	program->setUniformValue("contrast", 1.f);
	program->setUniformValue("saturation", 1.f);
	program->setUniformValue("brightness", 0.f);
	program->setUniformValue("gamma", 1.f);
	//             gamma levels : minInput, maxInput, minOutput, maxOutput:
	program->setUniformValue("levels", 0.f, 1.f, 0.f, 1.f);
	program->setUniformValue("hueshift", 0.f);
	program->setUniformValue("chromakey", 0.0, 0.0, 0.0 );
	program->setUniformValue("chromadelta", 0.1f);
	program->setUniformValue("threshold", 0.0f);
	program->setUniformValue("nbColors", -1);
	program->setUniformValue("invertMode", 0);
	program->setUniformValue("filter", 0);

	program->setUniformValue("ModelViewProjectionMatrix", QMatrix4x4 ());

	program->enableAttributeArray(PROGRAM_VERTEX_ATTRIBUTE);
	program->enableAttributeArray(PROGRAM_TEXCOORD_ATTRIBUTE);
	program->enableAttributeArray(PROGRAM_MASKCOORD_ATTRIBUTE);

	program->setAttributeArray (PROGRAM_VERTEX_ATTRIBUTE, coords, 3, 0);
	program->setAttributeArray (PROGRAM_TEXCOORD_ATTRIBUTE, texc, 2, 0);
	program->setAttributeArray (PROGRAM_MASKCOORD_ATTRIBUTE, maskc, 2, 0);

	program->release();

}


void ViewRenderWidget::setSourceDrawingMode(bool on)
{
	program->setUniformValue("sourceDrawing", on);

	if (on)
		glActiveTexture(GL_TEXTURE0);
	else {
		glActiveTexture(GL_TEXTURE2);
		glBindTexture(GL_TEXTURE_2D,ViewRenderWidget::mask_textures[Source::NO_MASK]);
	}

}

/**
 * Build a display lists for the line borders and returns its id
 **/
GLuint ViewRenderWidget::buildSelectList()
{
	GLuint base = glGenLists(1);

	// selected
	glNewList(base, GL_COMPILE);

	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glBlendEquation(GL_FUNC_ADD);

	glLineWidth(2.0);
	glColor4f(0.2, 0.80, 0.2, 1.0);

	glLineStipple(1, 0x9999);
	glEnable(GL_LINE_STIPPLE);

	glBegin(GL_LINE_LOOP); // begin drawing a square
	glVertex3d(-1.1, -1.1, 0.0); // Bottom Left
	glVertex3d(1.1, -1.1, 0.0); // Bottom Right
	glVertex3d(1.1, 1.1, 0.0); // Top Right
	glVertex3d(-1.1, 1.1, 0.0); // Top Left
	glEnd();

	glDisable(GL_LINE_STIPPLE);

	glEndList();

	return base;
}

/**
 * Build a display list of a textured QUAD and returns its id
 *
 * This is used only for the source drawing in GL_SELECT mode
 *
 **/
GLuint ViewRenderWidget::buildTexturedQuadList()
{
	GLuint id = glGenLists(1);
	glNewList(id, GL_COMPILE);

	glBegin(GL_QUADS); // begin drawing a square

	glTexCoord2f(0.f, 1.f);
	glVertex3f(-1.0f, -1.0f, 0.0f); // Bottom Left
	glTexCoord2f(1.f, 1.f);
	glVertex3f(1.0f, -1.0f, 0.0f); // Bottom Right
	glTexCoord2f(1.f, 0.f);
	glVertex3f(1.0f, 1.0f, 0.0f); // Top Right
	glTexCoord2f(0.f, 0.f);
	glVertex3f(-1.0f, 1.0f, 0.0f); // Top Left

	glEnd();

	glEndList();
	return id;
}

/**
 * Build 2 display lists for the line borders and shadows
 **/
GLuint ViewRenderWidget::buildLineList()
{
	glActiveTexture(GL_TEXTURE2);
	GLuint texid = bindTexture(QPixmap(QString::fromUtf8(
			":/glmixer/textures/shadow_corner.png")), GL_TEXTURE_2D);
	GLuint texid2 = bindTexture(QPixmap(QString::fromUtf8(
			":/glmixer/textures/shadow_corner_selected.png")), GL_TEXTURE_2D);

	GLclampf highpriority = 1.0;
	glPrioritizeTextures(1, &texid, &highpriority);

	GLuint base = glGenLists(2);
	glListBase(base);

	// default thin border
	glNewList(base, GL_COMPILE);

	glBindTexture(GL_TEXTURE_2D, texid); // 2d texture (x and y size)

	glPushMatrix();
	glScalef(1.23, 1.23, 1.0);

	glColor4f(0.0, 0.0, 0.0, 0.0);
    glDrawArrays(GL_QUADS, 0, 4);

	glPopMatrix();

	glLineWidth(1.0);
	glColor4f(0.9, 0.9, 0.0, 0.7);

	glBegin(GL_LINE_LOOP); // begin drawing a square
	glVertex3f(-1.05f, -1.05f, 0.0f); // Bottom Left
	glVertex3f(1.05f, -1.05f, 0.0f); // Bottom Right
	glVertex3f(1.05f, 1.05f, 0.0f); // Top Right
	glVertex3f(-1.05f, 1.05f, 0.0f); // Top Left
	glEnd();


	glEndList();

	// over
	glNewList(base + 1, GL_COMPILE);

	glBindTexture(GL_TEXTURE_2D, texid2); // 2d texture (x and y size)

	glPushMatrix();
	glScalef(1.23, 1.23, 1.0);

	glColor4f(0.0, 0.0, 0.0, 0.0);
    glDrawArrays(GL_QUADS, 0, 4);

	glPopMatrix();

	glLineWidth(3.0);
	glColor4f(0.9, 0.9, 0.0, 0.7);

	glBegin(GL_LINE_LOOP); // begin drawing a square
	glVertex3f(-1.05f, -1.05f, 0.0f); // Bottom Left
	glVertex3f(1.05f, -1.05f, 0.0f); // Bottom Right
	glVertex3f(1.05f, 1.05f, 0.0f); // Top Right
	glVertex3f(-1.05f, 1.05f, 0.0f); // Top Left
	glEnd();

	glEndList();

	return base;
}

GLuint ViewRenderWidget::buildCircleList()
{
	GLuint id = glGenLists(1);
	GLUquadricObj *quadObj = gluNewQuadric();

	glActiveTexture(GL_TEXTURE0);
	GLuint texid = 0; //bindTexture(QPixmap(QString::fromUtf8(":/glmixer/textures/circle.png")), GL_TEXTURE_2D);
	glGenTextures(1, &texid);
	glBindTexture(GL_TEXTURE_2D, texid);
	QImage p(":/glmixer/textures/circle.png");
	gluBuild2DMipmaps(GL_TEXTURE_2D, GL_COMPRESSED_RGBA, p.width(),
			p. height(), GL_RGBA, GL_UNSIGNED_BYTE, p.bits());
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER,
			GL_LINEAR_MIPMAP_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER,
			GL_LINEAR_MIPMAP_LINEAR);
	GLclampf highpriority = 1.0;
	glPrioritizeTextures(1, &texid, &highpriority);

	glNewList(id, GL_COMPILE);

	glPushMatrix();
	glTranslatef(0.0, 0.0, -1.0);

	glDisable(GL_BLEND);

	glActiveTexture(GL_TEXTURE0);
	glEnable(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, texid); // 2d texture (x and y size)

	glColor4f(1.0, 1.0, 1.0, 1.0);
	gluQuadricTexture(quadObj, GL_TRUE);
	gluDisk(quadObj, 0.0, CIRCLE_SIZE * SOURCE_UNIT, 50, 3);

	glDisable(GL_TEXTURE_2D);

	// blended antialiasing
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glBlendEquation(GL_FUNC_ADD);

	glColor4f(0.6, 0.6, 0.6, 1.0);
	glLineWidth(5.0);

	glBegin(GL_LINE_LOOP);
	for (float i = 0; i < 2.0 * M_PI; i += 0.07)
		glVertex3f(CIRCLE_SIZE * SOURCE_UNIT * cos(i), CIRCLE_SIZE
				* SOURCE_UNIT * sin(i), 0);
	glEnd();

	//limbo
	glColor4f(0.1, 0.1, 0.1, 0.8);
	gluDisk(quadObj, CIRCLE_SIZE * SOURCE_UNIT * 3.0, CIRCLE_SIZE * SOURCE_UNIT * 10.0, 50, 3);


	glPopMatrix();
	glEndList();

	return id;
}

GLuint ViewRenderWidget::buildLayerbgList()
{
	GLuint id = glGenLists(1);

	glNewList(id, GL_COMPILE);

	glColor4f(0.6, 0.6, 0.6, 1.0);
	glLineWidth(0.7);
	glBegin(GL_LINES);
	for (float i = -4.0; i < 6.0; i += CLAMP( ABS(i)/2.f , 0.01, 5.0))
	{
		glVertex3f(i - 1.3, -1.1 + exp(-10 * (i + 0.2)), 0.0);
		glVertex3f(i - 1.3, -1.1 + exp(-10 * (i + 0.2)), 31.0);
	}
	glEnd();

	glEndList();

	return id;
}

GLuint ViewRenderWidget::buildCatalogbgList()
{
	GLuint id = glGenLists(1);

//	GLuint texid = bindTexture(QPixmap(QString::fromUtf8(
//			":/glmixer/textures/catalog_bg.png")), GL_TEXTURE_2D);

	glNewList(id, GL_COMPILE);

	glBlendFunc(GL_SRC_ALPHA, GL_ONE);
	glBlendEquation(GL_FUNC_ADD);

//	glBindTexture(GL_TEXTURE_2D, texid); // 2d texture
	glColor3f(1.0, 1.0, 1.0);
	glBegin(GL_QUADS); // begin drawing a square
	glTexCoord2f(0.0f, 0.0f);
	glVertex2d(-SOURCE_UNIT, -SOURCE_UNIT); // Bottom Left
	glTexCoord2f(1.0f, 0.0f);
	glVertex2d(0.0, -SOURCE_UNIT); // Bottom Right
	glTexCoord2f(1.0f, 1.0f);
	glVertex2d(0.0, SOURCE_UNIT); // Top Right
	glTexCoord2f(0.0f, 1.0f);
	glVertex2d(-SOURCE_UNIT, SOURCE_UNIT); // Top Left
	glEnd();

	glEndList();

	return id;
}

/**
 * Build a display list of a black QUAD and returns its id
 **/
GLuint ViewRenderWidget::buildWindowList(GLubyte r, GLubyte g, GLubyte b)
{
	static GLuint texid = 0; // bindTexture(QPixmap(QString::fromUtf8(":/glmixer/textures/shadow.png")), GL_TEXTURE_2D);

	if (texid == 0) {
		// generate the texture with optimal performance ;
		glGenTextures(1, &texid);
		glBindTexture(GL_TEXTURE_2D, texid);
		QImage p(":/glmixer/textures/shadow.png");
		gluBuild2DMipmaps(GL_TEXTURE_2D, GL_COMPRESSED_RGBA, p.width(), p. height(), GL_RGBA, GL_UNSIGNED_BYTE, p.bits());
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR_MIPMAP_LINEAR);
		GLclampf highpriority = 1.0;
		glPrioritizeTextures(1, &texid, &highpriority);
	}

	GLuint id = glGenLists(1);
	glNewList(id, GL_COMPILE);

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glBlendEquation(GL_FUNC_ADD);
	glActiveTexture(GL_TEXTURE0);
	glEnable(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, texid); // 2d texture (x and y size)
	glColor4ub(0, 0, 0, 200);

	glPushMatrix();
		glTranslatef(0.02 * SOURCE_UNIT, -0.1 * SOURCE_UNIT, 0.1);
		glScalef(1.5 * SOURCE_UNIT, 1.5 * SOURCE_UNIT, 1.0);
		glBegin(GL_QUADS); // begin drawing a square
			glTexCoord2f(0.0f, 1.0f);
			glVertex3f(-1.f, -1.f, 0.0f); // Bottom Left
			glTexCoord2f(1.0f, 1.0f);
			glVertex3f(1.f, -1.f, 0.0f); // Bottom Right
			glTexCoord2f(1.0f, 0.0f);
			glVertex3f(1.f, 1.f, 0.0f); // Top Right
			glTexCoord2f(0.0f, 0.0f);
			glVertex3f(-1.f, 1.f, 0.0f); // Top Left
		glEnd();
	glPopMatrix();

	glDisable(GL_TEXTURE_2D);
	glColor4ub(r, g, b, 255);
	glBegin(GL_QUADS); // begin drawing a square
		// Front Face
		glNormal3f(0.0f, 0.0f, 1.0f); // front face points out of the screen on z.
		glVertex3f(-1.0f * SOURCE_UNIT, -1.0f * SOURCE_UNIT, 0.0f); // Bottom Left
		glVertex3f(1.0f * SOURCE_UNIT, -1.0f * SOURCE_UNIT, 0.0f); // Bottom Right
		glVertex3f(1.0f * SOURCE_UNIT, 1.0f * SOURCE_UNIT, 0.0f); // Top Right
		glVertex3f(-1.0f * SOURCE_UNIT, 1.0f * SOURCE_UNIT, 0.0f); // Top Left
	glEnd();


	glEndList();
	return id;
}

/**
 * Build a display list of the front line border of the render area and returns its id
 **/
GLuint ViewRenderWidget::buildFrameList()
{
	GLuint base = glGenLists(2);
	glListBase(base);

	// default thik
	glNewList(base, GL_COMPILE);

	// blended antialiasing
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glBlendEquation(GL_FUNC_ADD);

	glLineWidth(5.0);
	glColor4f(0.85, 0.15, 0.85, 1.0);

	glBegin(GL_LINE_LOOP); // begin drawing the frame (with marks on axis)
		glVertex3f(-1.01f * SOURCE_UNIT, -1.01f * SOURCE_UNIT, 0.0f); // Bottom Left
		glVertex3f(0.0f, -1.01f * SOURCE_UNIT, 0.0f);
		glVertex3f(0.0f, -1.07f * SOURCE_UNIT, 0.0f);
		glVertex3f(0.0f, -1.01f * SOURCE_UNIT, 0.0f);
		glVertex3f(1.01f * SOURCE_UNIT, -1.01f * SOURCE_UNIT, 0.0f); // Bottom Right
		glVertex3f(1.01f * SOURCE_UNIT, 0.0f, 0.0f);
		glVertex3f(1.05f * SOURCE_UNIT, 0.0f, 0.0f);
		glVertex3f(1.01f * SOURCE_UNIT, 0.0f, 0.0f);
		glVertex3f(1.01f * SOURCE_UNIT, 1.01f * SOURCE_UNIT, 0.0f); // Top Right
		glVertex3f(0.0f, 1.01f * SOURCE_UNIT, 0.0f);
		glVertex3f(0.0f, 1.07f * SOURCE_UNIT, 0.0f);
		glVertex3f(0.0f, 1.01f * SOURCE_UNIT, 0.0f);
		glVertex3f(-1.01f * SOURCE_UNIT, 1.01f * SOURCE_UNIT, 0.0f); // Top Left
		glVertex3f(-1.01f * SOURCE_UNIT, 0.0f, 0.0f);
		glVertex3f(-1.05f * SOURCE_UNIT, 0.0f, 0.0f);
		glVertex3f(-1.01f * SOURCE_UNIT, 0.0f, 0.0f);
	glEnd();

	glEndList();


	// thin
	glNewList(base + 1, GL_COMPILE);

	// blended antialiasing
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glBlendEquation(GL_FUNC_ADD);

	glLineWidth(1.0);
	glColor4f(0.85, 0.15, 0.85, 1.0);

	glBegin(GL_LINE_LOOP); // begin drawing the frame (with marks on axis)
		glVertex3f(-1.01f * SOURCE_UNIT, -1.01f * SOURCE_UNIT, 0.0f); // Bottom Left
		glVertex3f(0.0f, -1.01f * SOURCE_UNIT, 0.0f);
		glVertex3f(0.0f, -1.05f * SOURCE_UNIT, 0.0f);
		glVertex3f(0.0f, -1.01f * SOURCE_UNIT, 0.0f);
		glVertex3f(1.01f * SOURCE_UNIT, -1.01f * SOURCE_UNIT, 0.0f); // Bottom Right
		glVertex3f(1.01f * SOURCE_UNIT, 0.0f, 0.0f);
		glVertex3f(1.05f * SOURCE_UNIT, 0.0f, 0.0f);
		glVertex3f(1.01f * SOURCE_UNIT, 0.0f, 0.0f);
		glVertex3f(1.01f * SOURCE_UNIT, 1.01f * SOURCE_UNIT, 0.0f); // Top Right
		glVertex3f(0.0f, 1.01f * SOURCE_UNIT, 0.0f);
		glVertex3f(0.0f, 1.05f * SOURCE_UNIT, 0.0f);
		glVertex3f(0.0f, 1.01f * SOURCE_UNIT, 0.0f);
		glVertex3f(-1.01f * SOURCE_UNIT, 1.01f * SOURCE_UNIT, 0.0f); // Top Left
		glVertex3f(-1.01f * SOURCE_UNIT, 0.0f, 0.0f);
		glVertex3f(-1.05f * SOURCE_UNIT, 0.0f, 0.0f);
		glVertex3f(-1.01f * SOURCE_UNIT, 0.0f, 0.0f);
	glEnd();

	glEndList();
	return base;
}

/**
 * Build 3 display lists for the line borders of sources and returns the base id
 **/
GLuint ViewRenderWidget::buildBordersList()
{
	GLuint base = glGenLists(4);
	glListBase(base);

	// default thin border
	glNewList(base, GL_COMPILE);

	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glBlendEquation(GL_FUNC_ADD);

	glLineWidth(1.0);
	glColor4f(0.9, 0.9, 0.0, 0.7);

	glBegin(GL_LINE_LOOP); // begin drawing a square
	glVertex3f(-1.0f, -1.0f, 0.0f); // Bottom Left
	glVertex3f(1.0f, -1.0f, 0.0f); // Bottom Right
	glVertex3f(1.0f, 1.0f, 0.0f); // Top Right
	glVertex3f(-1.0f, 1.0f, 0.0f); // Top Left
	glEnd();

	glEndList();

	// selected large border (no action)
	glNewList(base + 1, GL_COMPILE);

	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glBlendEquation(GL_FUNC_ADD);

	glColor4f(0.9, 0.9, 0.0, 0.8);

	// draw the bold border
	glLineWidth(3.0);
	glBegin(GL_LINE_LOOP); // begin drawing a square
	glVertex3f(-1.0f, -1.0f, 0.0f); // Bottom Left
	glVertex3f(1.0f, -1.0f, 0.0f); // Bottom Right
	glVertex3f(1.0f, 1.0f, 0.0f); // Top Right
	glVertex3f(-1.0f, 1.0f, 0.0f); // Top Left
	glEnd();

	glEnd();

	glEndList();

	// selected for TOOL
	glNewList(base + 2, GL_COMPILE);

	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glBlendEquation(GL_FUNC_ADD);

	glColor4f(0.9, 0.9, 0.0, 0.9);

	// draw the bold border
	glLineWidth(3.0);
	glBegin(GL_LINE_LOOP); // begin drawing a square
	glVertex3f(-1.0f, -1.0f, 0.0f); // Bottom Left
	glVertex3f(1.0f, -1.0f, 0.0f); // Bottom Right
	glVertex3f(1.0f, 1.0f, 0.0f); // Top Right
	glVertex3f(-1.0f, 1.0f, 0.0f); // Top Left
	glEnd();

	glLineWidth(1.0);
	glBegin(GL_LINES); // begin drawing a square
	//    glVertex3f(-1.0f, -1.0f, 0.0f); // Bottom Left
	glVertex3f(-BORDER_SIZE, -1.0f, 0.0f);
	glVertex3f(-BORDER_SIZE, -BORDER_SIZE, 0.0f);
	glVertex3f(-BORDER_SIZE, -BORDER_SIZE, 0.0f);
	glVertex3f(-1.0f, -BORDER_SIZE, 0.0f);

	//    glVertex3f(1.0f, -1.0f, 0.0f); // Bottom Right
	glVertex3f(1.0f, -BORDER_SIZE, 0.0f);
	glVertex3f(BORDER_SIZE, -BORDER_SIZE, 0.0f);
	glVertex3f(BORDER_SIZE, -BORDER_SIZE, 0.0f);
	glVertex3f(BORDER_SIZE, -1.0f, 0.0f);

	//    glVertex3f(1.0f, 1.0f, 0.0f); // Top Right
	glVertex3f(BORDER_SIZE, 1.0f, 0.0f);
	glVertex3f(BORDER_SIZE, BORDER_SIZE, 0.0f);
	glVertex3f(BORDER_SIZE, BORDER_SIZE, 0.0f);
	glVertex3f(1.0f, BORDER_SIZE, 0.0f);

	//    glVertex3f(-1.0f, 1.0f, 0.0f); // Top Left
	glVertex3f(-BORDER_SIZE, 1.0f, 0.0f);
	glVertex3f(-BORDER_SIZE, BORDER_SIZE, 0.0f);
	glVertex3f(-BORDER_SIZE, BORDER_SIZE, 0.0f);
	glVertex3f(-1.0f, BORDER_SIZE, 0.0f);

	glEnd();

	glEndList();

	return base;
}

GLuint ViewRenderWidget::buildFadingList()
{

	GLuint id = glGenLists(1);

	glNewList(id, GL_COMPILE);

	glMatrixMode(GL_PROJECTION);
	glPushMatrix();
	glLoadIdentity();
	gluOrtho2D(-1, 1, -1, 1);
	glMatrixMode(GL_MODELVIEW);
	glPushMatrix();
	glLoadIdentity();

	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glBlendEquation(GL_FUNC_ADD);
	glColor4f(0.1, 0.1, 0.1, 0.5);
	glRectf(-1, -1, 1, 1);

	glMatrixMode(GL_PROJECTION);
	glPopMatrix();
	glMatrixMode(GL_MODELVIEW);
	glPopMatrix();

	glEndList();

	return id;
}

static char * rotate_top_right[] =
{ "25 25 3 1", " 	c None", ".	c #EEEEEE", "+	c #000000",
		"                         ",
		"                         ",
		"           +             ",
		"          +.+            ",
		"          +..+           ",
		"        +++...+  ++      ",
		"      ++.......++..+     ",
		"     +.........++..+     ",
		"    +.....+...+  ++      ",
		"    +...+++..+   ++      ",
		"   +...+  +.+   +..+     ",
		"   +...+   +   +....+    ",
		"  +...+       +......+   ",
		"  +...+      +........+  ",
		"  +...+       +++..+++   ",
		"   +...+       +...+     ",
		"   +...+       +...+     ",
		"    +...++   ++...+      ",
		"    +.....+++.....+      ",
		"     +...........+       ",
		"      ++.......++        ",
		"        ++...++          ",
		"          +++            ",
		"                         ",
		"                         "};


static char * rotate_top_left[] =
{ "25 25 3 1", " 	c None", ".	c #EEEEEE", "+	c #000000",
		"                         ",
		"                         ",
		"             +           ",
		"            +.+          ",
		"           +..+          ",
		"      ++  +...+++        ",
		"     +..++.......++      ",
		"     +..++.........+     ",
		"      ++  +...+.....+    ",
		"      ++   +..+++...+    ",
		"     +..+   +.+  +...+   ",
		"    +....+   +   +...+   ",
		"   +......+       +...+  ",
		"  +........+      +...+  ",
		"   +++..+++       +...+  ",
		"     +...+       +...+   ",
		"     +...+       +...+   ",
		"      +...++   ++...+    ",
		"      +.....+++.....+    ",
		"       +...........+     ",
		"        ++.......++      ",
		"          ++...++        ",
		"            +++          ",
		"                         ",
		"                         "};

static char * rotate_bot_left[] =
{ "25 25 3 1", " 	c None", ".	c #EEEEEE", "+	c #000000",
		"                         ",
		"                         ",
		"            +++          ",
		"          ++...++        ",
		"        ++.......++      ",
		"       +...........+     ",
		"      +.....+++.....+    ",
		"      +...++   ++...+    ",
		"     +...+       +...+   ",
		"     +...+       +...+   ",
		"   +++..+++       +...+  ",
		"  +........+      +...+  ",
		"   +......+       +...+  ",
		"    +....+   +   +...+   ",
		"     +..+   +.+  +...+   ",
		"      ++   +..+++...+    ",
		"      ++  +...+.....+    ",
		"     +..++.........+     ",
		"     +..++.......++      ",
		"      ++  +...+++        ",
		"           +..+          ",
		"            +.+          ",
		"             +           ",
		"                         ",
		"                         "};

static char * rotate_bot_right[] =
{ "25 25 3 1", " 	c None", ".	c #EEEEEE", "+	c #000000",
		"                         ",
		"                         ",
		"          +++            ",
		"        ++...++          ",
		"      ++.......++        ",
		"     +...........+       ",
		"    +.....+++.....+      ",
		"    +...++   ++...+      ",
		"   +...+       +...+     ",
		"   +...+       +...+     ",
		"  +...+       +++..+++   ",
		"  +...+      +........+  ",
		"  +...+       +......+   ",
		"   +...+   +   +....+    ",
		"   +...+  +.+   +..+     ",
		"    +...+++..+   ++      ",
		"    +.....+...+  ++      ",
		"     +.........++..+     ",
		"      ++.......++..+     ",
		"        +++...+  ++      ",
		"          +..+           ",
		"          +.+            ",
		"           +             ",
		"                         ",
		"                         "};


static char * cursor_arrow_xpm[] =
{ "25 25 3 1", " 	c None", ".	c #EEEEEE", "+	c #000000",
		"                         ", "                         ",
		"       +                 ", "       ++                ",
		"       +.+               ", "       +..+              ",
		"       +...+             ", "       +....+            ",
		"       +.....+           ", "       +......+          ",
		"       +.......+         ", "       +........+        ",
		"       +.........+       ", "       +......+++++      ",
		"       +...+..+          ", "       +..++..+          ",
		"       +.+  +..+         ", "       ++   +..+         ",
		"       +     +..+        ", "             +..+        ",
		"              +..+       ", "              +..+       ",
		"               ++        ", "                         ",
		"                         " };

static char * cursor_openhand_xpm[] =
{ "16 16 3 1", " 	g None", ".	g #000000", "+	g #EEEEEE", "       ..       ",
		"   .. .++...    ", "  .++..++.++.   ", "  .++..++.++. . ",
		"   .++.++.++..+.", "   .++.++.++.++.", " .. .+++++++.++.",
		".++..++++++++++.", ".+++.+++++++++. ", " .++++++++++++. ",
		"  .+++++++++++. ", "  .++++++++++.  ", "   .+++++++++.  ",
		"    .+++++++.   ", "     .++++++.   ", "                " };

static char * cursor_closedhand_xpm[] =
{ "16 16 3 1", " 	g None", ".	g #000000", "+	g #EEEEEE", "                ",
		"                ", "                ", "    .. .. ..    ",
		"   .++.++.++..  ", "   .++++++++.+. ", "    .+++++++++. ",
		"   ..+++++++++. ", "  .+++++++++++. ", "  .++++++++++.  ",
		"   .+++++++++.  ", "    .+++++++.   ", "     .++++++.   ",
		"     .++++++.   ", "                ", "                " };

static char * cursor_sizef_xpm[] =
{ "25 25 3 1", " 	c None", ".	c #EEEEEE", "+	c #000000",
		"                         ", "                         ",
		"                         ", "                         ",
		"    +++++++++            ", "    +.......+            ",
		"    +......+             ", "    +.....+              ",
		"    +.....+              ", "    +......+             ",
		"    +..++...+            ", "    +.+  +...+           ",
		"    ++    +...+    ++    ", "           +...+  +.+    ",
		"            +...++..+    ", "             +......+    ",
		"              +.....+    ", "              +.....+    ",
		"             +......+    ", "            +.......+    ",
		"            +++++++++    ", "                         ",
		"                         ", "                         ",
		"                         " };

static char * cursor_sizeb_xpm[] =
{ "25 25 3 1", " 	c None", ".	c #EEEEEE", "+	c #000000",
		"                         ", "                         ",
		"                         ", "                         ",
		"            +++++++++    ", "            +.......+    ",
		"             +......+    ", "              +.....+    ",
		"              +.....+    ", "             +......+    ",
		"            +...++..+    ", "           +...+  +.+    ",
		"    ++    +...+    ++    ", "    +.+  +...+           ",
		"    +..++...+            ", "    +......+             ",
		"    +.....+              ", "    +.....+              ",
		"    +......+             ", "    +.......+            ",
		"    +++++++++            ", "                         ",
		"                         ", "                         ",
		"                         " };

static char * cursor_question_xpm[] =
{ "25 25 3 1", " 	c None", ".	c #EEEEEE", "+	c #000000",
		"+                        ", "++          .......      ",
		"+.+        .+++++++.     ", "+..+      .++....+++.    ",
		"+...+    .+++.  .+++.    ", "+....+   .+++.  .+++.    ",
		"+.....+  .+++.  .+++.    ", "+......+ .+++.  .+++.    ",
		"+.......+ ...  .+++.     ", "+........+    .+++.      ",
		"+.....+++++  .+++.       ", "+..+..+      .+++.       ",
		"+.+ +..+     .+++.       ", "++  +..+     .+++.       ",
		"+    +..+    .....       ", "     +..+    .+++.       ",
		"      +..+   .+++.       ", "      +..+   .....       ",
		"       ++                ", "                         ",
		"                         ", "                         ",
		"                         ", "                         ",
		"                         " };

static char * cursor_sizeall_xpm[] =
{ "25 25 3 1", " 	c None", ".	c #EEEEEE", "+	c #000000",
		"                         ", "           ++            ",
		"          +..+           ", "         +....+          ",
		"        +......+         ", "       +........+        ",
		"        +++..+++         ", "     +    +..+    +      ",
		"    +.+   +..+   +.+     ", "   +..+   +..+   +..+    ",
		"  +...+++++..+++++...+   ", " +....................+  ",
		" +....................+  ", "  +...+++++..+++++...+   ",
		"   +..+   +..+   +..+    ", "    +.+   +..+   +.+     ",
		"     +    +..+    +      ", "        +++..+++         ",
		"       +........+        ", "        +......+         ",
		"         +....+          ", "          +..+           ",
		"           ++            ", "                         ",
		"                         " };

static char * cursor_hand_xpm[] =
{ "25 25 3 1", " 	c None", ".	c #EEEEEE", "+	c #000000",
		"         ..              ", "        .++.             ",
		"        +..+             ", "        +..+             ",
		"        +..+             ", "        +..+             ",
		"        +..+             ", "        +..+++           ",
		"        +..+..+++        ", "        +..+..+..++      ",
		"     ++ +..+..+..+.+     ", "    +..++..+..+..+.+     ",
		"    +...+..........+     ", "     +.............+     ",
		"      +............+     ", "      +............+     ",
		"       +..........+      ", "       +..........+      ",
		"        +........+       ", "        +........+       ",
		"        ++++++++++       ", "        ++++++++++       ",
		"        ++++++++++       ", "                         ",
		"                         " };

void ViewRenderWidget::setMouseCursor(mouseCursor c)
{
	// create QCursors for each pixmap
	static QCursor arrowCursor = QCursor(QPixmap(cursor_arrow_xpm), 8, 3);
	static QCursor handOpenCursor = QCursor(QPixmap(cursor_openhand_xpm), 8, 8);
	static QCursor handCloseCursor = QCursor(QPixmap(cursor_closedhand_xpm), 8, 8);
	static QCursor scaleBCursor = QCursor(QPixmap(cursor_sizeb_xpm), 12, 12);
	static QCursor scaleFCursor = QCursor(QPixmap(cursor_sizef_xpm), 12, 12);
	static QCursor rotTopRightCursor = QCursor(QPixmap(rotate_top_right), 12, 12);
	static QCursor rotTopLeftCursor = QCursor(QPixmap(rotate_top_left), 12, 12);
	static QCursor rotBottomRightCursor = QCursor(QPixmap(rotate_bot_right), 12, 12);
	static QCursor rotBottomLeftCursor = QCursor(QPixmap(rotate_bot_left), 12, 12);
	static QCursor questionCursor = QCursor(QPixmap(cursor_question_xpm), 1, 1);
	static QCursor sizeallCursor = QCursor(QPixmap(cursor_sizeall_xpm), 12, 12);
	static QCursor handIndexCursor = QCursor(QPixmap(cursor_hand_xpm), 10, 1);

	switch (c)
	{
	case MOUSE_HAND_OPEN:
		setCursor(handOpenCursor);
		break;
	case MOUSE_HAND_CLOSED:
		setCursor(handCloseCursor);
		break;
	case MOUSE_SCALE_B:
		setCursor(scaleBCursor);
		break;
	case MOUSE_SCALE_F:
		setCursor(scaleFCursor);
		break;
	case MOUSE_ROT_BOTTOM_LEFT:
		setCursor(rotBottomLeftCursor);
		break;
	case MOUSE_ROT_BOTTOM_RIGHT:
		setCursor(rotBottomRightCursor);
		break;
	case MOUSE_ROT_TOP_LEFT:
		setCursor(rotTopLeftCursor);
		break;
	case MOUSE_ROT_TOP_RIGHT:
		setCursor(rotTopRightCursor);
		break;
	case MOUSE_QUESTION:
		setCursor(questionCursor);
		break;
	case MOUSE_SIZEALL:
		setCursor(sizeallCursor);
		break;
	case MOUSE_HAND_INDEX:
		setCursor(handIndexCursor);
		break;
	default:
	case MOUSE_ARROW:
		setCursor(arrowCursor);
	}
}

