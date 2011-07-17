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
#include "AxisCursor.h"
#include "LineCursor.h"
#include "FuzzyCursor.h"


GLuint ViewRenderWidget::border_thin_shadow = 0,
		ViewRenderWidget::border_large_shadow = 0;
GLuint ViewRenderWidget::border_thin = 0, ViewRenderWidget::border_large = 0;
GLuint ViewRenderWidget::border_scale = 0;
GLuint ViewRenderWidget::quad_texured = 0, ViewRenderWidget::quad_window[] = {0, 0};
GLuint ViewRenderWidget::frame_selection = 0, ViewRenderWidget::frame_screen = 0, ViewRenderWidget::frame_screen_thin = 0;
GLuint ViewRenderWidget::circle_mixing = 0, ViewRenderWidget::layerbg = 0;
GLuint ViewRenderWidget::mask_textures[] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
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


GLfloat ViewRenderWidget::coords[12] = { -1.f, 1.f, 0.f,  1.f, 1.f, 0.f,  1.f, -1.f, 0.f,  -1.f, -1.f, 0.f };
GLfloat ViewRenderWidget::texc[8] = {0.f, 0.f,  1.f, 0.f,  1.f, 1.f,  0.f, 1.f};
GLfloat ViewRenderWidget::maskc[8] = {0.f, 0.f,  1.f, 0.f,  1.f, 1.f,  0.f, 1.f};
QGLShaderProgram *ViewRenderWidget::program = 0;
bool ViewRenderWidget::disableFiltering = false;

ViewRenderWidget::ViewRenderWidget() :
	glRenderWidget(), faded(false), messageLabel(0), fpsLabel(0), viewMenu(0), catalogMenu(0), sourceMenu(0), showFps_(0)
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
	_springCursor = new SpringCursor;
	Q_CHECK_PTR(_springCursor);
	_delayCursor = new DelayCursor;
	Q_CHECK_PTR(_delayCursor);
	_axisCursor = new AxisCursor;
	Q_CHECK_PTR(_axisCursor);
	_lineCursor = new LineCursor;
	Q_CHECK_PTR(_lineCursor);
	_fuzzyCursor = new FuzzyCursor;
	Q_CHECK_PTR(_fuzzyCursor);
	// sets the current cursor
	_currentCursor = 0;
	cursorEnabled = false;

	// opengl HID display
	connect(&messageTimer, SIGNAL(timeout()), SLOT(hideMessage()));
	messageTimer.setSingleShot(true);
	fpsTime_.start();
	fpsCounter_ = 0;
	f_p_s_ = 1000.0 / updatePeriod();

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
	if (_springCursor)
		delete _springCursor;
	if (_delayCursor)
		delete _delayCursor;
	if (_axisCursor)
		delete _axisCursor;
	if (_lineCursor)
		delete _lineCursor;
}


void ViewRenderWidget::initializeGL()
{
	glRenderWidget::initializeGL();
	setBackgroundColor(QColor(COLOR_BGROUND));

	// Create display lists
	quad_texured = buildTexturedQuadList();
	border_thin_shadow = buildLineList();
	border_large_shadow = border_thin_shadow + 1;
	frame_selection = buildSelectList();
	circle_mixing = buildCircleList();
	layerbg = buildLayerbgList();
	quad_window[0] = buildWindowList(0, 0, 0);
	quad_window[1] = buildWindowList(255, 255, 255);
	frame_screen = buildFrameList();
	frame_screen_thin = frame_screen + 1;
	border_thin = buildBordersList();
	border_large = border_thin + 1;
	border_scale = border_thin + 2;
	fading = buildFadingList();

	// Create mask textures
	if (!mask_textures[0])
	{
		glActiveTexture(GL_TEXTURE1);
		glEnable(GL_TEXTURE_2D);
		const char * const black_xpm[] = { "2 2 1 1", ". c #000000", "..", ".."};
		mask_textures[Source::NO_MASK] = bindTexture(QPixmap(black_xpm), GL_TEXTURE_2D);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
		mask_textures[Source::ANTIALIASING_MASK] = bindTexture(QPixmap(QString::fromUtf8(
				":/glmixer/textures/mask_antialiasing.png")), GL_TEXTURE_2D);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
		mask_textures[Source::ROUNDCORNER_MASK] = bindTexture(QPixmap(QString::fromUtf8(
				":/glmixer/textures/mask_roundcorner.png")), GL_TEXTURE_2D);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
		mask_textures[Source::CIRCLE_MASK] = bindTexture(QPixmap(QString::fromUtf8(
				":/glmixer/textures/mask_circle.png")), GL_TEXTURE_2D);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
		mask_textures[Source::GRADIENT_CIRCLE_MASK] = bindTexture(QPixmap(QString::fromUtf8(
				":/glmixer/textures/mask_linear_circle.png")), GL_TEXTURE_2D);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
		mask_textures[Source::GRADIENT_SQUARE_MASK] = bindTexture(QPixmap(QString::fromUtf8(
				":/glmixer/textures/mask_linear_square.png")), GL_TEXTURE_2D);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
		mask_textures[Source::GRADIENT_LEFT_MASK] = bindTexture(QPixmap(QString::fromUtf8(
				":/glmixer/textures/mask_linear_left.png")), GL_TEXTURE_2D);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
		mask_textures[Source::GRADIENT_RIGHT_MASK] = bindTexture(QPixmap(QString::fromUtf8(
				":/glmixer/textures/mask_linear_right.png")), GL_TEXTURE_2D);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
		mask_textures[Source::GRADIENT_TOP_MASK] = bindTexture(QPixmap(QString::fromUtf8(
				":/glmixer/textures/mask_linear_bottom.png")), GL_TEXTURE_2D);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
		mask_textures[Source::GRADIENT_BOTTOM_MASK] = bindTexture(QPixmap(QString::fromUtf8(
				":/glmixer/textures/mask_linear_top.png")), GL_TEXTURE_2D);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
		mask_textures[Source::GRADIENT_HORIZONTAL_MASK] = bindTexture(QPixmap(QString::fromUtf8(
				":/glmixer/textures/mask_linear_horizontal.png")), GL_TEXTURE_2D);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
		mask_textures[Source::GRADIENT_VERTICAL_MASK] = bindTexture(QPixmap(QString::fromUtf8(
				":/glmixer/textures/mask_linear_vertical.png")), GL_TEXTURE_2D);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);

		glDisable(GL_TEXTURE_2D);
		glActiveTexture(GL_TEXTURE0);
	}

	// store render View matrices
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	gluOrtho2D(-SOURCE_UNIT * OutputRenderWindow::getInstance()->getAspectRatio(), SOURCE_UNIT * OutputRenderWindow::getInstance()->getAspectRatio(), -SOURCE_UNIT, SOURCE_UNIT);
	glGetDoublev(GL_PROJECTION_MATRIX, _renderView->projection);
	glGetDoublev(GL_PROJECTION_MATRIX, _catalogView->projection);

	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	glGetDoublev(GL_MODELVIEW_MATRIX, _renderView->modelview);
	glGetDoublev(GL_MODELVIEW_MATRIX, _catalogView->modelview);

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

	_currentView->setAction(View::NONE);

	// update view to match with the changes in modelview and projection matrices (e.g. resized widget)
	makeCurrent();
	refresh();
}

void ViewRenderWidget::removeFromSelections(Source *s)
{
	View::deselect(s);
	_mixingView->removeFromGroup(s);
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

void ViewRenderWidget::showContextMenu(ViewContextMenu m, const QPoint &pos)
{
	switch (m) {
	case CONTEXT_MENU_VIEW:
		if (viewMenu)
			viewMenu->exec(mapToGlobal(pos));
		break;
	case CONTEXT_MENU_SOURCE:
		if (sourceMenu)
			sourceMenu->exec(mapToGlobal(pos));
		break;
	case CONTEXT_MENU_CATALOG:
		if (catalogMenu)
			catalogMenu->exec(mapToGlobal(pos));
		break;
	case CONTEXT_MENU_DROP:
		{
			QMenu menu(this);
			QAction *newAct = new QAction(tr("Cancel"), this);
			menu.addAction(newAct);
			connect(newAct, SIGNAL(triggered()), RenderingManager::getInstance(), SLOT(clearBasket()));
			menu.exec(mapToGlobal(pos));
		}
		break;
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


void ViewRenderWidget::setCursorEnabled(bool on) {

	if (_currentCursor == 0)
		cursorEnabled = false;
	else
		cursorEnabled = on;
}

void ViewRenderWidget::setCursorMode(cursorMode m){

	switch(m) {
	case ViewRenderWidget::CURSOR_DELAY:
		_currentCursor = _delayCursor;
		break;
	case ViewRenderWidget::CURSOR_SPRING:
		_currentCursor = _springCursor;
		break;
	case ViewRenderWidget::CURSOR_AXIS:
		_currentCursor = _axisCursor;
		break;
	case ViewRenderWidget::CURSOR_LINE:
		_currentCursor = _lineCursor;
		break;
		break;
	case ViewRenderWidget::CURSOR_FUZZY:
		_currentCursor = _fuzzyCursor;
		break;
	default:
	case ViewRenderWidget::CURSOR_NORMAL:
		_currentCursor = 0;
	}

	cursorEnabled = false;
}

ViewRenderWidget::cursorMode ViewRenderWidget::getCursorMode(){

	if (_currentCursor == _springCursor)
		return ViewRenderWidget::CURSOR_SPRING;
	if (_currentCursor == _delayCursor)
		return ViewRenderWidget::CURSOR_DELAY;
	if (_currentCursor == _axisCursor)
		return ViewRenderWidget::CURSOR_AXIS;
	if (_currentCursor == _lineCursor)
		return ViewRenderWidget::CURSOR_LINE;
	if (_currentCursor == _fuzzyCursor)
		return ViewRenderWidget::CURSOR_FUZZY;

	return ViewRenderWidget::CURSOR_NORMAL;
}

Cursor *ViewRenderWidget::getCursor(cursorMode m)
{
	switch(m) {
		case ViewRenderWidget::CURSOR_DELAY:
			return (Cursor*)_delayCursor;
		case ViewRenderWidget::CURSOR_SPRING:
			return (Cursor*)_springCursor;
		case ViewRenderWidget::CURSOR_AXIS:
			return (Cursor*)_axisCursor;
		case ViewRenderWidget::CURSOR_LINE:
			return (Cursor*)_lineCursor;
		case ViewRenderWidget::CURSOR_FUZZY:
			return (Cursor*)_fuzzyCursor;
			break;
		default:
		case ViewRenderWidget::CURSOR_NORMAL:
			return 0;
	}

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

	//
	// 3. draw a semi-transparent overlay if view should be faded out
	//
	//
	if (faded) {
		glCallList(ViewRenderWidget::fading);
		setMouseCursor(MOUSE_ARROW);
	}
	// if not faded, means the area is active
	else
	{
	//
	// 2. the shadow of the cursor
	//
		if (cursorEnabled && _currentCursor->apply(f_p_s_) ) {

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
	}
	//
	// 4. The extra information
	//
	// Catalog
	if (_catalogView->visible())
		_catalogView->paint();

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
	if (showFps_ || ( f_p_s_ < 800.0 / (float)updatePeriod() && f_p_s_ > 0) )
		displayFramerate();

	// Pause logo
	if (RenderingManager::getInstance()->isPaused()){
		glMatrixMode(GL_PROJECTION);
		glPushMatrix();
		glLoadIdentity();
		gluOrtho2D(0.0, width(), 0.0, height());

		glMatrixMode(GL_MODELVIEW);
		glPushMatrix();
		glLoadIdentity();

		qglColor(Qt::lightGray);
		glRecti(15, height() - 5, 25, height() - 30);
		glRecti(30, height() - 5, 40, height() - 30);

		glMatrixMode(GL_PROJECTION);
		glPopMatrix();
		glMatrixMode(GL_MODELVIEW);
		glPopMatrix();
	}
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
	glRecti(width() - 61, height() - 1, width() - 9, height() - 11);
	qglColor(f_p_s_ > 800.f / (float)updatePeriod() ? Qt::darkGreen : (f_p_s_ > 500.f / (float)updatePeriod() ? Qt::yellow : Qt::red));
	// Draw a filled rectangle of lengh proportionnal to % of target fps
	glRecti(width() - 60, height() - 2, width() - 60 + (int)( 0.05 * f_p_s_  * (float) updatePeriod()), height() - 10);

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

	// ask the catalog view if it wants this mouse press event and then
	// inform the view of the mouse press event
	if (!_catalogView->mousePressEvent(event) )
	{

		if (_currentView->mousePressEvent(event))
			// enable cursor on a clic
			setCursorEnabled(true);
		else
		// if there is something to drop, inform the rendering manager that it can drop the source at the clic coordinates
		if (RenderingManager::getInstance()->getSourceBasketTop() && event->buttons() & Qt::LeftButton)
		{
			double x = 0.0, y = 0.0;
			_currentView->coordinatesFromMouse(event->x(), event->y(), &x, &y);

			// depending on the view, ask the rendering manager to drop the source with the user parameters
			if (_currentView == _mixingView)
				emit sourceMixingDrop(x, y);
			else if (_currentView == _geometryView)
				emit sourceGeometryDrop(x, y);
			else if (_currentView == _layersView)
				emit sourceLayerDrop(x);
		}
		else
			// the mouse press was not treated ; forward it
			QWidget::mousePressEvent(event);
	}

	if (cursorEnabled)
		_currentCursor->update(event);
}

void ViewRenderWidget::mouseMoveEvent(QMouseEvent *event)
{
	makeCurrent();

	// ask the catalog view if it wants this mouse move event
	if (_catalogView->mouseMoveEvent(event))
		return;

	if (cursorEnabled && _currentCursor->isActive())
		_currentCursor->update(event);
	else if (_currentView->mouseMoveEvent(event)){
		// the view 'mouseMoveEvent' returns true ; there was something changed!
		if (_currentView == _mixingView)
			emit sourceMixingModified();
		else if (_currentView == _geometryView)
			emit sourceGeometryModified();
		else if (_currentView == _layersView)
			emit sourceLayerModified();
	}

}

void ViewRenderWidget::mouseReleaseEvent(QMouseEvent * event)
{
	makeCurrent();

	if (cursorEnabled) {
		_currentCursor->update(event);
		// disable cursor
		setCursorEnabled(false);
	}

	// ask the catalog view if it wants this mouse release event
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

	if (cursorEnabled && _currentCursor->wheelEvent(event))
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


void ViewRenderWidget::keyReleaseEvent(QKeyEvent * event)
{
	makeCurrent();

	if (!_currentView->keyReleaseEvent(event))
		QWidget::keyReleaseEvent(event);
	else
	{   // the view 'keyReleaseEvent' returns true ; there was something changed!
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
			if (QApplication::keyboardModifiers() & Qt::ShiftModifier)
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

	// cancel current action
	_currentView->setAction(View::NONE);
	// set the catalog  off
	_catalogView->setTransparent(true);

	QWidget::leaveEvent(event);
}

void ViewRenderWidget::enterEvent ( QEvent * event ){

	// just to be 100% sure no action is current
	_currentView->setAction(View::NONE);
	setMouseCursor(ViewRenderWidget::MOUSE_ARROW);

	QWidget::enterEvent(event);
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

void ViewRenderWidget::zoomCurrentSource()
{
	makeCurrent();
	_currentView->zoomBestFit(true);

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

	QDomElement mix = _mixingView->getConfiguration(doc);
	mix.setAttribute("name", "Mixing");
	config.appendChild(mix);

	QDomElement geom = _geometryView->getConfiguration(doc);
	geom.setAttribute("name", "Geometry");
	config.appendChild(geom);

	QDomElement depth = _layersView->getConfiguration(doc);
	depth.setAttribute("name", "Depth");
	config.appendChild(depth);

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
        // apply configuration node
		if (child.attribute("name") == "Mixing")
			_mixingView->setConfiguration(child);
		else if (child.attribute("name") == "Geometry")
			_geometryView->setConfiguration(child);
		else if (child.attribute("name") == "Depth")
			_layersView->setConfiguration(child);

		child = child.nextSiblingElement();
	}
	// NB: the catalog is restored in GLMixer::openSessionFile because GLMixer has access to the actions
}

void ViewRenderWidget::setFilteringEnabled(bool on)
{
	makeCurrent();

	// if the GLSL program was already created, delete it
	if( program ) {
		// except if the filtering is already at the same configuration
		if ( disableFiltering == !on )
			return;
		program->release();
		delete program;
	}
	// apply flag
	disableFiltering = !on;

	// instanciate the GLSL program
	program = new QGLShaderProgram(this);

	QString fshfile;
	if (disableFiltering)
		fshfile = ":/glmixer/shaders/imageProcessing_fragment_simplified.glsl";
	else
		fshfile = ":/glmixer/shaders/imageProcessing_fragment.glsl";

	if (!program->addShaderFromSourceFile(QGLShader::Fragment, fshfile))
		qFatal( "%s", qPrintable( tr("OpenGL GLSL error in fragment shader; \n\n%1").arg(program->log()) ) );
	else if (program->log().contains("warning"))
		qCritical() << fshfile << tr(":OpenGL GLSL warning in fragment shader;%1").arg(program->log());

	if (!program->addShaderFromSourceFile(QGLShader::Vertex, ":/glmixer/shaders/imageProcessing_vertex.glsl"))
		qFatal( "%s", qPrintable( tr("OpenGL GLSL error in vertex shader; \n\n%1").arg(program->log()) ) );
	else if (program->log().contains("warning"))
		qCritical() << "imageProcessing_vertex.glsl" << tr(":OpenGL GLSL warning in vertex shader;%1").arg(program->log());

	if (!program->link())
		qFatal( "%s", qPrintable( tr("OpenGL GLSL linking error; \n\n%1").arg(program->log()) ) );

	if (!program->bind())
		qFatal( "%s", qPrintable( tr("OpenGL GLSL binding error; \n\n%1").arg(program->log()) ) );

	// set the pointer to the array for the texture attributes
	program->enableAttributeArray("texCoord");
	program->setAttributeArray ("texCoord", texc, 2, 0);
	program->enableAttributeArray("maskCoord");
	program->setAttributeArray ("maskCoord", maskc, 2, 0);

	// set the default values for the uniform variables
	program->setUniformValue("sourceTexture", 0);
	program->setUniformValue("maskTexture", 1);
	program->setUniformValue("utilityTexture", 2);
	program->setUniformValue("sourceDrawing", false);
	program->setUniformValue("gamma", 1.f);
	program->setUniformValue("levels", 0.f, 1.f, 0.f, 1.f); // gamma levels : minInput, maxInput, minOutput, maxOutput:

	if (!disableFiltering) {
		program->setUniformValue("step", 1.f / 640.f, 1.f / 480.f);
		program->setUniformValue("contrast", 1.f);
		program->setUniformValue("saturation", 1.f);
		program->setUniformValue("brightness", 0.f);
		program->setUniformValue("hueshift", 0.f);
		program->setUniformValue("chromakey", 0.0, 0.0, 0.0 );
		program->setUniformValue("chromadelta", 0.1f);
		program->setUniformValue("threshold", 0.0f);
		program->setUniformValue("nbColors", (GLint) -1);
		program->setUniformValue("invertMode", (GLint) 0);
		program->setUniformValue("filter", (GLint) 0);
	}

	program->release();

	// create and enable the vertex array for drawing a QUAD
	glEnableClientState(GL_VERTEX_ARRAY);
	glVertexPointer(3, GL_FLOAT, 0, coords);

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
		glColor4ub(COLOR_SELECTION, 255);

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
		glVertex2f(-1.0f, -1.0f); // Bottom Left
		glTexCoord2f(1.f, 1.f);
		glVertex2f(1.0f, -1.0f); // Bottom Right
		glTexCoord2f(1.f, 0.f);
		glVertex2f(1.0f, 1.0f); // Top Right
		glTexCoord2f(0.f, 0.f);
		glVertex2f(-1.0f, 1.0f); // Top Left

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

	GLuint base = glGenLists(4);
	glListBase(base);

	// default thin border
	glNewList(base, GL_COMPILE);

		glBindTexture(GL_TEXTURE_2D, texid); // 2d texture (x and y size)

		glPushMatrix();
		glScalef(1.23, 1.23, 1.0);
		glColor4ub(0, 0, 0, 0);
		glDrawArrays(GL_QUADS, 0, 4);
		glPopMatrix();

		glLineWidth(1.0);
		glColor4ub(COLOR_SOURCE, 180);
		glBegin(GL_LINE_LOOP); // begin drawing a square
		glVertex2f(-1.05f, -1.05f); // Bottom Left
		glVertex2f(1.05f, -1.05f); // Bottom Right
		glVertex2f(1.05f, 1.05f); // Top Right
		glVertex2f(-1.05f, 1.05f); // Top Left
		glEnd();

	glEndList();

	// over
	glNewList(base + 1, GL_COMPILE);

		glBindTexture(GL_TEXTURE_2D, texid2); // 2d texture (x and y size)

		glPushMatrix();
		glScalef(1.23, 1.23, 1.0);
		glColor4ub(0, 0, 0, 0);
		glDrawArrays(GL_QUADS, 0, 4);
		glPopMatrix();

		glLineWidth(3.0);
		glColor4ub(COLOR_SOURCE, 180);
		glBegin(GL_LINE_LOOP); // begin drawing a square
		glVertex2f(-1.05f, -1.05f); // Bottom Left
		glVertex2f(1.05f, -1.05f); // Bottom Right
		glVertex2f(1.05f, 1.05f); // Top Right
		glVertex2f(-1.05f, 1.05f); // Top Left
		glEnd();

	glEndList();

	// default thin border STATIC
	glNewList(base + 2, GL_COMPILE);

		glBindTexture(GL_TEXTURE_2D, texid); // 2d texture (x and y size)

		glPushMatrix();
		glScalef(1.23, 1.23, 1.0);
		glColor4ub(0, 0, 0, 0);
		glDrawArrays(GL_QUADS, 0, 4);
		glPopMatrix();

		glLineWidth(1.0);
		glColor4ub(COLOR_SOURCE_STATIC, 180);
		glBegin(GL_LINE_LOOP); // begin drawing a square
		glVertex2f(-1.05f, -1.05f); // Bottom Left
		glVertex2f(1.05f, -1.05f); // Bottom Right
		glVertex2f(1.05f, 1.05f); // Top Right
		glVertex2f(-1.05f, 1.05f); // Top Left
		glEnd();

		glPointSize(3.0);
		glColor4ub(COLOR_SOURCE_STATIC, 180);
		glBegin(GL_POINTS); // draw nails
		glVertex2f(-0.9f, -0.9f); // Bottom Left
		glVertex2f(0.9f, -0.9f); // Bottom Right
		glVertex2f(0.9f, 0.9f); // Top Right
		glVertex2f(-0.9f, 0.9f); // Top Left
		glEnd();

	glEndList();

	// over STATIC
	glNewList(base + 3, GL_COMPILE);

		glBindTexture(GL_TEXTURE_2D, texid2); // 2d texture (x and y size)

		glPushMatrix();
		glScalef(1.23, 1.23, 1.0);
		glColor4ub(0, 0, 0, 0);
		glDrawArrays(GL_QUADS, 0, 4);
		glPopMatrix();

		glLineWidth(3.0);
		glColor4ub(COLOR_SOURCE_STATIC, 180);
		glBegin(GL_LINE_LOOP); // begin drawing a square
		glVertex2f(-1.05f, -1.05f); // Bottom Left
		glVertex2f(1.05f, -1.05f); // Bottom Right
		glVertex2f(1.05f, 1.05f); // Top Right
		glVertex2f(-1.05f, 1.05f); // Top Left
		glEnd();

		glPointSize(3.0);
		glColor4ub(COLOR_SOURCE_STATIC, 180);
		glBegin(GL_POINTS); // draw nails
		glVertex2f(-0.9f, -0.9f); // Bottom Left
		glVertex2f(0.9f, -0.9f); // Bottom Right
		glVertex2f(0.9f, 0.9f); // Top Right
		glVertex2f(-0.9f, 0.9f); // Top Left
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
		glColor4ub(255, 255, 255, 255);
		gluQuadricTexture(quadObj, GL_TRUE);
		gluDisk(quadObj, 0.0, CIRCLE_SIZE * SOURCE_UNIT, 50, 3);
		glDisable(GL_TEXTURE_2D);

		// blended antialiasing
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		glBlendEquation(GL_FUNC_ADD);
		glColor4ub(COLOR_DRAWINGS, 250);
		glLineWidth(5.0);

		glBegin(GL_LINE_LOOP);
		for (float i = 0; i < 2.0 * M_PI; i += 0.07)
			glVertex2f(CIRCLE_SIZE * SOURCE_UNIT * cos(i), CIRCLE_SIZE * SOURCE_UNIT * sin(i));
		glEnd();

		//limbo
		glColor4ub(COLOR_LIMBO, 180);
		gluDisk(quadObj, CIRCLE_SIZE * SOURCE_UNIT * 2.5, CIRCLE_SIZE * SOURCE_UNIT * 10.0, 50, 3);

		glPopMatrix();

	glEndList();

	return id;
}

GLuint ViewRenderWidget::buildLayerbgList()
{
	GLuint id = glGenLists(1);

	glNewList(id, GL_COMPILE);

		glColor4ub(COLOR_DRAWINGS, 255);
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
				glVertex2f(-1.f, -1.f); // Bottom Left
				glTexCoord2f(1.0f, 1.0f);
				glVertex2f(1.f, -1.f); // Bottom Right
				glTexCoord2f(1.0f, 0.0f);
				glVertex2f(1.f, 1.f); // Top Right
				glTexCoord2f(0.0f, 0.0f);
				glVertex2f(-1.f, 1.f); // Top Left
			glEnd();
		glPopMatrix();

		glDisable(GL_TEXTURE_2D);
		glColor4ub(r, g, b, 255);
		glBegin(GL_QUADS); // begin drawing a square
			// Front Face
			glNormal3f(0.0f, 0.0f, 1.0f); // front face points out of the screen on z.
			glVertex2f(-1.0f * SOURCE_UNIT, -1.0f * SOURCE_UNIT); // Bottom Left
			glVertex2f(1.0f * SOURCE_UNIT, -1.0f * SOURCE_UNIT); // Bottom Right
			glVertex2f(1.0f * SOURCE_UNIT, 1.0f * SOURCE_UNIT); // Top Right
			glVertex2f(-1.0f * SOURCE_UNIT, 1.0f * SOURCE_UNIT); // Top Left
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
	glColor4ub(COLOR_FRAME, 250);

	glBegin(GL_LINE_LOOP); // begin drawing the frame (with marks on axis)
		glVertex2f(-1.01f * SOURCE_UNIT, -1.01f * SOURCE_UNIT); // Bottom Left
		glVertex2f(0.0f, -1.01f * SOURCE_UNIT);
		glVertex2f(0.0f, -1.07f * SOURCE_UNIT);
		glVertex2f(0.0f, -1.01f * SOURCE_UNIT);
		glVertex2f(1.01f * SOURCE_UNIT, -1.01f * SOURCE_UNIT); // Bottom Right
		glVertex2f(1.01f * SOURCE_UNIT, 0.0f);
		glVertex2f(1.05f * SOURCE_UNIT, 0.0f);
		glVertex2f(1.01f * SOURCE_UNIT, 0.0f);
		glVertex2f(1.01f * SOURCE_UNIT, 1.01f * SOURCE_UNIT); // Top Right
		glVertex2f(0.0f, 1.01f * SOURCE_UNIT);
		glVertex2f(0.0f, 1.07f * SOURCE_UNIT);
		glVertex2f(0.0f, 1.01f * SOURCE_UNIT);
		glVertex2f(-1.01f * SOURCE_UNIT, 1.01f * SOURCE_UNIT); // Top Left
		glVertex2f(-1.01f * SOURCE_UNIT, 0.0f);
		glVertex2f(-1.05f * SOURCE_UNIT, 0.0f);
		glVertex2f(-1.01f * SOURCE_UNIT, 0.0f);
	glEnd();

	glEndList();


	// thin
	glNewList(base + 1, GL_COMPILE);

	// blended antialiasing
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glBlendEquation(GL_FUNC_ADD);

	glLineWidth(1.0);
	glColor4ub(COLOR_FRAME, 250);

	glBegin(GL_LINE_LOOP); // begin drawing the frame (with marks on axis)
		glVertex2f(-1.01f * SOURCE_UNIT, -1.01f * SOURCE_UNIT); // Bottom Left
		glVertex2f(0.0f, -1.01f * SOURCE_UNIT);
		glVertex2f(0.0f, -1.05f * SOURCE_UNIT);
		glVertex2f(0.0f, -1.01f * SOURCE_UNIT);
		glVertex2f(1.01f * SOURCE_UNIT, -1.01f * SOURCE_UNIT); // Bottom Right
		glVertex2f(1.01f * SOURCE_UNIT, 0.0f);
		glVertex2f(1.05f * SOURCE_UNIT, 0.0f);
		glVertex2f(1.01f * SOURCE_UNIT, 0.0f);
		glVertex2f(1.01f * SOURCE_UNIT, 1.01f * SOURCE_UNIT); // Top Right
		glVertex2f(0.0f, 1.01f * SOURCE_UNIT);
		glVertex2f(0.0f, 1.05f * SOURCE_UNIT);
		glVertex2f(0.0f, 1.01f * SOURCE_UNIT);
		glVertex2f(-1.01f * SOURCE_UNIT, 1.01f * SOURCE_UNIT); // Top Left
		glVertex2f(-1.01f * SOURCE_UNIT, 0.0f);
		glVertex2f(-1.05f * SOURCE_UNIT, 0.0f);
		glVertex2f(-1.01f * SOURCE_UNIT, 0.0f);
	glEnd();

	glEndList();
	return base;
}

/**
 * Build 3 display lists for the line borders of sources and returns the base id
 **/
GLuint ViewRenderWidget::buildBordersList()
{
	GLuint base = glGenLists(12);
	glListBase(base);

	// default thin border
	glNewList(base, GL_COMPILE);
	glLineWidth(1.0);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glBlendEquation(GL_FUNC_ADD);
	glBegin(GL_LINE_LOOP); // begin drawing a square
	glVertex2f(-1.0f, -1.0f); // Bottom Left
	glVertex2f(1.0f, -1.0f); // Bottom Right
	glVertex2f(1.0f, 1.0f); // Top Right
	glVertex2f(-1.0f, 1.0f); // Top Left
	glEnd();
	glEndList();

	// selected large border (no action)
	glNewList(base + 1, GL_COMPILE);
	glLineWidth(3.0);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glBlendEquation(GL_FUNC_ADD);
	glBegin(GL_LINE_LOOP); // begin drawing a square
	glVertex2f(-1.0f, -1.0f); // Bottom Left
	glVertex2f(1.0f, -1.0f); // Bottom Right
	glVertex2f(1.0f, 1.0f); // Top Right
	glVertex2f(-1.0f, 1.0f); // Top Left
	glEnd();
	glEndList();

	// selected for TOOL
	glNewList(base + 2, GL_COMPILE);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glBlendEquation(GL_FUNC_ADD);
	glLineWidth(3.0);
	glBegin(GL_LINE_LOOP); // begin drawing a square
	glVertex2f(-1.0f, -1.0f); // Bottom Left
	glVertex2f(1.0f, -1.0f); // Bottom Right
	glVertex2f(1.0f, 1.0f); // Top Right
	glVertex2f(-1.0f, 1.0f); // Top Left
	glEnd();
	glLineWidth(1.0);
	glBegin(GL_LINES); // begin drawing handles
	// Bottom Left
	glVertex2f(-BORDER_SIZE, -1.0f);
	glVertex2f(-BORDER_SIZE, -BORDER_SIZE);
	glVertex2f(-BORDER_SIZE, -BORDER_SIZE);
	glVertex2f(-1.0f, -BORDER_SIZE);
	// Bottom Right
	glVertex2f(1.0f, -BORDER_SIZE);
	glVertex2f(BORDER_SIZE, -BORDER_SIZE);
	glVertex2f(BORDER_SIZE, -BORDER_SIZE);
	glVertex2f(BORDER_SIZE, -1.0f);
	// Top Right
	glVertex2f(BORDER_SIZE, 1.0f);
	glVertex2f(BORDER_SIZE, BORDER_SIZE);
	glVertex2f(BORDER_SIZE, BORDER_SIZE);
	glVertex2f(1.0f, BORDER_SIZE);
	// Top Left
	glVertex2f(-BORDER_SIZE, 1.0f);
	glVertex2f(-BORDER_SIZE, BORDER_SIZE);
	glVertex2f(-BORDER_SIZE, BORDER_SIZE);
	glVertex2f(-1.0f, BORDER_SIZE);
	glEnd();
	glEndList();

	// Normal source color
	glNewList(base + 3, GL_COMPILE);
	glColor4ub(COLOR_SOURCE, 180);
	glCallList(base);
	glEndList();

	glNewList(base + 4, GL_COMPILE);
	glColor4ub(COLOR_SOURCE, 200);
	glCallList(base+1);
	glEndList();

	glNewList(base + 5, GL_COMPILE);
	glColor4ub(COLOR_SOURCE, 220);
	glCallList(base+2);
	glEndList();


	// Static source color
	glNewList(base + 6, GL_COMPILE);
	glColor4ub(COLOR_SOURCE_STATIC, 180);
	glCallList(base);
	glPointSize(3.0);
	glColor4ub(COLOR_SOURCE_STATIC, 180);
	glBegin(GL_POINTS); // draw nails
	glVertex2f(-0.9f, -0.9f); // Bottom Left
	glVertex2f(0.9f, -0.9f); // Bottom Right
	glVertex2f(0.9f, 0.9f); // Top Right
	glVertex2f(-0.9f, 0.9f); // Top Left
	glEnd();
	glEndList();

	glNewList(base + 7, GL_COMPILE);
	glColor4ub(COLOR_SOURCE_STATIC, 200);
	glCallList(base+1);
	glPointSize(3.0);
	glColor4ub(COLOR_SOURCE_STATIC, 180);
	glBegin(GL_POINTS); // draw nails
	glVertex2f(-0.9f, -0.9f); // Bottom Left
	glVertex2f(0.9f, -0.9f); // Bottom Right
	glVertex2f(0.9f, 0.9f); // Top Right
	glVertex2f(-0.9f, 0.9f); // Top Left
	glEnd();
	glEndList();

	glNewList(base + 8, GL_COMPILE);
	glColor4ub(COLOR_SOURCE_STATIC, 220);
	glCallList(base+1);
	glPointSize(3.0);
	glColor4ub(COLOR_SOURCE_STATIC, 180);
	glBegin(GL_POINTS); // draw nails
	glVertex2f(-0.9f, -0.9f); // Bottom Left
	glVertex2f(0.9f, -0.9f); // Bottom Right
	glVertex2f(0.9f, 0.9f); // Top Right
	glVertex2f(-0.9f, 0.9f); // Top Left
	glEnd();
	glEndList();


	// Selection source color
	glNewList(base + 9, GL_COMPILE);
	glColor4ub(COLOR_SELECTION , 180);
	glLineStipple(1, 0x9999);
	glEnable(GL_LINE_STIPPLE);
	glScalef(1.01, 1.01, 1.01);
	glCallList(base);
	glDisable(GL_LINE_STIPPLE);
	glEndList();

	glNewList(base + 10, GL_COMPILE);
	glColor4ub(COLOR_SELECTION, 200);
	glLineStipple(1, 0x9999);
	glEnable(GL_LINE_STIPPLE);
	glScalef(1.01, 1.01, 1.01);
	glCallList(base+1);
	glDisable(GL_LINE_STIPPLE);
	glEndList();

	glNewList(base + 11, GL_COMPILE);
	glColor4ub(COLOR_SELECTION, 220);
	glLineStipple(1, 0x9999);
	glEnable(GL_LINE_STIPPLE);
	glScalef(1.01, 1.01, 1.01);
	glCallList(base+2);
	glDisable(GL_LINE_STIPPLE);
	glPointSize(10);
	glColor4ub(COLOR_SELECTION, 200);
	glBegin(GL_POINTS);
	glVertex2f(0,0);
	glEnd();
	glEndList();

	return base + 3;
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
	glColor4ub(COLOR_FADING, 128);
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
