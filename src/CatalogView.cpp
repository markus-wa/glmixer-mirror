/*
 * selectionView.cpp
 *
 *  Created on: Mar 7, 2010
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

#include "common.h"
#include "CatalogView.h"
#include "ViewRenderWidget.h"
#include "RenderingManager.h"
#include "OutputRenderWindow.h"

CatalogView::CatalogView() : View(), _visible(true), _height(0), h_unit(1.0), v_unit(1.0), _alpha(1.0),
							first_index(0), last_index(0), _clicX(0.0), _clicY(0.0)
{
	_size[SMALL] = 61.0;
	_iconSize[SMALL] = 23.0;
	_largeIconSize[SMALL] = 26.0;

	_size[MEDIUM] = 101.0;
	_iconSize[MEDIUM] = 38.0;
	_largeIconSize[MEDIUM] = 42.0;

	_size[LARGE] = 151.0;
	_iconSize[LARGE] = 54.0;
	_largeIconSize[LARGE] = 62.0;

	_currentSize = MEDIUM;
    title = "Catalog";
}

CatalogView::~CatalogView() {

}

void CatalogView::resize(int w, int h) {

	// compute viewport considering width
	// TODO : switch depending on side (top, bottom, left, right..)
	viewport[0] = w - _size[_currentSize];
	viewport[2] = _size[_currentSize];

	h_unit = 2.0 * SOURCE_UNIT * OutputRenderWindow::getInstance()->getAspectRatio() / double(RenderingManager::getInstance()->getFrameBufferWidth());
	v_unit = 2.0 * SOURCE_UNIT / double(viewport[3]);
}

void CatalogView::setSize(catalogSize s){

	_currentSize = s;
	resize(RenderingManager::getRenderingWidget()->width(), RenderingManager::getRenderingWidget()->height());
}


void CatalogView::setModelview()
{
    glTranslatef(0, getPanningY(), 0.0);

}

void CatalogView::setVisible(bool on){

	_visible = on;
	resize(RenderingManager::getRenderingWidget()->width(), RenderingManager::getRenderingWidget()->height());
}

void CatalogView::clear() {

	// Clearing the catalog is actually drawing the decoration of the catalog bar
	// This method is called by the rendering manager with a viewport covering the fbo
	// and a projection matrix set to
	// gluOrtho2D(-SOURCE_UNIT * OutputRenderWindow::getInstance()->getAspectRatio(), SOURCE_UNIT * OutputRenderWindow::getInstance()->getAspectRatio(), -SOURCE_UNIT, SOURCE_UNIT);
	// Modelview is Identity

	// clear to transparent
	glClearColor( 0.0, 0.0, 0.0, 0.0);
	glClear(GL_COLOR_BUFFER_BIT);

	_width = SOURCE_UNIT * OutputRenderWindow::getInstance()->getAspectRatio();

	// draw the catalog background and its frame
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glBlendEquation(GL_FUNC_ADD);
	glColor4f(0.5, 0.5, 0.5, 0.2);
    float bl_x = -_width + 0.3 * _size[_currentSize] * h_unit;
    float bl_y = -SOURCE_UNIT + ( 2.0 * SOURCE_UNIT - _height) - 0.5;
    float tr_x = -_width + _size[_currentSize] * h_unit;
    float tr_y = SOURCE_UNIT;
    glRectf( bl_x, bl_y, tr_x, tr_y);

	glColor4f(0.8, 0.8, 0.8, 0.9);
	glLineWidth(3);
    glBegin(GL_LINES); // drawing borders
		glVertex2f(tr_x, bl_y); // Bottom Right
		glVertex2f(bl_x, bl_y); // Bottom Left
		glVertex2f(bl_x, bl_y); // Bottom Left
		glVertex2f(bl_x, tr_y); // Top Left
    glEnd();

}

void CatalogView::drawSource(Source *s, int index)
{
	// Drawing a source is rendering a quad with the source texture in the catalog bar.
	// This method is called by the rendering manager with a viewport covering the fbo
	// and a projection matrix set to
	// gluOrtho2D(-SOURCE_UNIT * OutputRenderWindow::getInstance()->getAspectRatio(), SOURCE_UNIT * OutputRenderWindow::getInstance()->getAspectRatio(), -SOURCE_UNIT, SOURCE_UNIT);
	// Modelview is Identity

	// reset height to 0 at first icon
	if (index == 0)
		_height = 0.0;

	if ( s ) {

		// target 60 pixels wide icons (height depending on aspect ratio)
		// each source is a quad [-1 +1]
		double swidth_pixels = ( s->isActive() ? _largeIconSize[_currentSize] : _iconSize[_currentSize]) * h_unit;
		double sheight_pixels = ( s->isActive() ? _largeIconSize[_currentSize] : _iconSize[_currentSize]) / s->getAspectRatio() * v_unit;

		// increment y height by the height of this source + margin
		double height = _height + 2.0 * sheight_pixels + 0.1 * _size[_currentSize] * v_unit;

		// if getting out of available drawing area, skip this source and draw arrow instead
		if (height > 2.0 * SOURCE_UNIT) {
			glColor4f(0.8, 0.8, 0.8, 0.6);
			glTranslatef( -_width + _size[_currentSize] * h_unit * 0.5, SOURCE_UNIT - height + _iconSize[_currentSize] * v_unit, 0.0);
		    glLineWidth(2);
		    glBegin(GL_LINE_LOOP); // draw a triangle
				glVertex2f(0.0, 0.50);
				glVertex2f(0.50, 1.0);
				glVertex2f(-0.50, 1.0);
		    glEnd();

			return;
		}

		_height = height;

		// place the icon at center of width, and vertically spaced
		glTranslatef( -_width + _size[_currentSize] * h_unit * 0.5, SOURCE_UNIT - _height + sheight_pixels, 0.0);
		if (s->isActive())
			glTranslatef( (_iconSize[_currentSize] -_largeIconSize[_currentSize]) * h_unit , 0.0, 0.0);
		glScalef( swidth_pixels, -sheight_pixels, 1.f);


		// draw source texture (without shading)
		glBindTexture(GL_TEXTURE_2D, s->getTextureIndex() );

	    glDisable(GL_BLEND);
		glColor4f(0.0, 0.0, 0.0, 1.0);
		glDrawArrays(GL_QUADS, 0, 4);
	    glEnable(GL_BLEND);

		// was it clicked ?
		if ( ABS(_clicX) > 0.1 || ABS(_clicY) > 0.1 ) {
			if ( ABS( _clicY + SOURCE_UNIT - _height + sheight_pixels) < sheight_pixels ) {
				RenderingManager::getInstance()->setCurrentSource(s->getId());
				// done with click
				_clicX = _clicY = 0.0;
			}
		}

	    // draw source border
		ViewRenderWidget::setSourceDrawingMode(false);
		glScalef( 1.05, 1.05, 1.0);
		if (s->isActive())
			glCallList(ViewRenderWidget::border_large);
		else
			glCallList(ViewRenderWidget::border_thin);


	}

}


void CatalogView::paint() {

	// Paint the catalog view is only drawing the off-screen rendered catalog texture.
	// This texture is filled during rendering manager fbo update with the clear and drawSource methods.
	//
	// This paint is called by ViewRenderWidget

	glPushAttrib(GL_COLOR_BUFFER_BIT  | GL_VIEWPORT_BIT);

	// draw only in the area of the screen covered by the catalog
	glViewport(viewport[0],viewport[1], viewport[2], viewport[3]);

	// use a standard ortho projection to paint the quad with catalog
	glMatrixMode(GL_PROJECTION);
	glPushMatrix();
	glLoadIdentity();
	gluOrtho2D(0, viewport[2], 0, viewport[3]);

	glMatrixMode(GL_MODELVIEW);
	glPushMatrix();
	glLoadIdentity();

	glColor4f(1.0, 1.0, 1.0, _alpha);

    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glBlendEquation(GL_FUNC_ADD);

	// draw the texture rendered with fbo during rendering
	glActiveTexture(GL_TEXTURE0);
	glEnable(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, RenderingManager::getInstance()->getCatalogTexture());

    glBegin(GL_QUADS); // begin drawing a square
		// specific texture coordinates to take only the section corresponding to the catalog
		glTexCoord2d(0.0, 1.0);
		glVertex2i(0, 0); // Bottom Left
		glTexCoord2d( viewport[2] / double(RenderingManager::getInstance()->getFrameBufferWidth()), 1.0);
		glVertex2i( viewport[2], 0); // Bottom Right
		glTexCoord2d( viewport[2] / double(RenderingManager::getInstance()->getFrameBufferWidth()), 0.0);
		glVertex2i( viewport[2], viewport[3]); // Top Right
		glTexCoord2d(0.0, 0.0);
		glVertex2i(0, viewport[3]); // Top Left
    glEnd();

    glDisable(GL_TEXTURE_2D);

	glMatrixMode(GL_PROJECTION);
	glPopMatrix();
	glMatrixMode(GL_MODELVIEW);
	glPopMatrix();

	glPopAttrib();
}



bool CatalogView::isInside(const QPoint &pos){

//	if (_visible && pos.x() > viewport[0] )
	if (_visible && pos.x() > viewport[0] && (RenderingManager::getRenderingWidget()->height() - pos.y()) < (int)(_height / v_unit) )
		return true;

	return false;
}

bool CatalogView::mousePressEvent(QMouseEvent *event)
{
	if ( isInside(event->pos()) ) {
		// get coordinates of clic in object space
		GLdouble z;
		gluUnProject((double)event->x(), (double)(RenderingManager::getRenderingWidget()->height() - event->y()), 1.0, modelview, projection, viewport, &_clicX, &_clicY, &z);
		return true;
	}

	return false;
}

bool CatalogView::mouseDoubleClickEvent ( QMouseEvent * event )
{
	return isInside(event->pos());
}


bool CatalogView::mouseMoveEvent(QMouseEvent *event)
{
	if (isInside(event->pos())) {
		setTransparent(false);
		return true;
	}

	setTransparent(true);
	return false;
}


bool CatalogView::mouseReleaseEvent ( QMouseEvent * event )
{
	if (isInside(event->pos()) ) {
		_clicX = _clicY = 0.0;
		return true;
	}

	return false;
}


bool CatalogView::wheelEvent ( QWheelEvent * event )
{
	if (isInside(event->pos())) {

		// TODO implement scrolling in the list
		return true;
	}

	return false;
}



