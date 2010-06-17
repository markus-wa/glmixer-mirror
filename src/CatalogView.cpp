/*
 * selectionView.cpp
 *
 *  Created on: Mar 7, 2010
 *      Author: bh
 */

#include "common.h"
#include "CatalogView.h"
#include "ViewRenderWidget.h"
#include "RenderingManager.h"

CatalogView::CatalogView() : View(), _visible(true), _height(0), h_unit(1.0), v_unit(1.0), _alpha(1.0),
							first_index(0), last_index(0)
{
	_size[SMALL] = 60.0;
	_iconSize[SMALL] = 23.0;
	_largeIconSize[SMALL] = 26.0;

	_size[MEDIUM] = 100.0;
	_iconSize[MEDIUM] = 38.0;
	_largeIconSize[MEDIUM] = 42.0;

	_size[LARGE] = 150.0;
	_iconSize[LARGE] = 58.0;
	_largeIconSize[LARGE] = 70.0;

	_currentSize = MEDIUM;
}

CatalogView::~CatalogView() {

}

void CatalogView::resize(int w, int h) {

	View::resize(w, h);
	// TODO : switch depending on side (top, bottom, left, right..)

	// compute viewport considering width
	viewport[0] = viewport[2] - _size[_currentSize];
	viewport[1] = 0;

	h_unit = 2.0 * SOURCE_UNIT / double(RenderingManager::getInstance()->getFrameBufferWidth());
	v_unit = 2.0 * SOURCE_UNIT / double(RenderingManager::getInstance()->getFrameBufferHeight());
}

void CatalogView::setSize(catalogSize s){

	_currentSize = s;
	resize();
}


void CatalogView::setModelview()
{
    glTranslatef(0, getPanningY(), 0.0);

}

void CatalogView::setVisible(bool on){

	_visible = on;
	resize();
}

void CatalogView::clear() {

	// Clearing the catalog is actually drawing the decoration of the catalog bar
	// This method is called by the rendering manager with a viewport covering the fbo
	// and a projection matrix set to gluOrtho2D(-SOURCE_UNIT, SOURCE_UNIT, -SOURCE_UNIT, SOURCE_UNIT);

	glPushAttrib(GL_COLOR_BUFFER_BIT);

	// clear to transparent
	glClearColor( 1.0, 1.0, 1.0, 0.0);
	glClear(GL_COLOR_BUFFER_BIT);

	// draw the catalog gb and its frame
	glColor4f(0.6, 0.6, 0.6, 0.6);
    glDisable(GL_TEXTURE_2D);

    float bl_x = -SOURCE_UNIT + 0.3 * _size[_currentSize] * h_unit;
    float bl_y = -SOURCE_UNIT + ( 2.0 * SOURCE_UNIT - _height) - 0.5;
    float tr_x = -SOURCE_UNIT + _size[_currentSize] * h_unit;
    float tr_y = SOURCE_UNIT;
    glRectf( bl_x, bl_y, tr_x, tr_y);

	glColor4f(0.8, 0.8, 0.8, 1.0);
	glLineWidth(2);
    glBegin(GL_LINE_LOOP); // begin drawing a square

		glVertex2f(bl_x, bl_y); // Bottom Left
		glVertex2f(tr_x, bl_y); // Bottom Right
		glVertex2f(tr_x, tr_y); // Top Right
		glVertex2f(bl_x, tr_y); // Top Right

    glEnd();

    glEnable(GL_TEXTURE_2D);

	glPopAttrib();
}

void CatalogView::drawSource(Source *s, int index)
{
	// Drawing a source is rendering a quad with the source texture in the catalog bar.
	// This method is called by the rendering manager with a viewport covering the fbo
	// and a projection matrix set to gluOrtho2D(-SOURCE_UNIT, SOURCE_UNIT, -SOURCE_UNIT, SOURCE_UNIT);

	// reset height to 0 at first icon
	if (index == 0)
		_height = 0.0;

	if (s) {
		// target 60 pixels wide icons (height depending on aspect ratio)
		// each source is a quad [-1 +1]
		double swidth_pixels = ( s->isActive() ? _largeIconSize[_currentSize] : _iconSize[_currentSize]) * h_unit;
		double sheight_pixels = ( s->isActive() ? _largeIconSize[_currentSize] : _iconSize[_currentSize]) / s->getAspectRatio() * v_unit;

		// increment y height by the height of this source + margin
		double height = _height + 2.0 * sheight_pixels + 0.1 * _size[_currentSize] * v_unit;

		// if getting out of available drawing area, skip this source and draw arrow instead
		if (height > 2.0 * SOURCE_UNIT) {
			glColor4f(0.8, 0.8, 0.8, 0.6);
			glTranslatef( -SOURCE_UNIT + _size[_currentSize] * h_unit * 0.5, SOURCE_UNIT - height + _iconSize[_currentSize] * v_unit, 0.0);
		    glDisable(GL_TEXTURE_2D);
		    glLineWidth(2);
		    glBegin(GL_LINE_LOOP); // begin drawing a square
				glVertex2f(0.0, 0.50); //
				glVertex2f(0.50, 1.0); //
				glVertex2f(-0.50, 1.0); //
		    glEnd();
		    glEnable(GL_TEXTURE_2D);

			return;
		}

		_height = height;

		// place the icon at center of width, and vertically spaced
		glTranslatef( -SOURCE_UNIT + _size[_currentSize] * h_unit * 0.5, SOURCE_UNIT - _height + sheight_pixels, 0.0);
		if (s->isActive())
			glTranslatef( (_iconSize[_currentSize] -_largeIconSize[_currentSize]) * h_unit , 0.0, 0.0);
		glScalef( swidth_pixels, -sheight_pixels, 1.f);

	    glDisable(GL_BLEND);
		// draw source texture (without shading)
		s->draw(false);
	    glEnable(GL_BLEND);

		// was it clicked ?
		if ( ABS(_clicX) > 0.1 || ABS(_clicY) > 0.1 ) {
			//if ( )

			qDebug("clic at %f %f", _clicX, _clicY);

			double px = -SOURCE_UNIT + _size[_currentSize] * h_unit * 0.5;
			double py = SOURCE_UNIT - _height + sheight_pixels;

//
			qDebug("s    at %f %f", px, py);
			qDebug("        %f %f", swidth_pixels, sheight_pixels);

			if ( ABS( _clicX - px) <  swidth_pixels && ABS( _clicY - py) < sheight_pixels ) {

				RenderingManager::getInstance()->setCurrentSource(s->getId());
				// done with click
				_clicX = _clicY = 0.0;
			}
		}

	    // draw source border
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
	// This paint is called by ViewRenderWidget with a viewport covering the full window
	// and a projection matrix set according to the current view.

	glPushAttrib(GL_COLOR_BUFFER_BIT  | GL_VIEWPORT_BIT);

	// draw only in the area of the screen covered by the catalog
	glViewport(viewport[0],viewport[1],_size[_currentSize], RenderingManager::getInstance()->getFrameBufferHeight());

	// use a standard ortho projection to paint the quad with catalog
	// (use SOURCE_UNIT width to match drawing scale in the fbo texture, but not necessary)
	glMatrixMode(GL_PROJECTION);
	glPushMatrix();
	glLoadIdentity();
	gluOrtho2D(-SOURCE_UNIT, SOURCE_UNIT,  SOURCE_UNIT, -SOURCE_UNIT);

	glMatrixMode(GL_MODELVIEW);
	glPushMatrix();
	glLoadIdentity();

	glColor4f(1.0, 1.0, 1.0, _alpha);

    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glBlendEquation(GL_FUNC_ADD);

	// draw the texture rendered with fbo during rendering
	glBindTexture(GL_TEXTURE_2D, RenderingManager::getInstance()->getCatalogTexture());

    glBegin(GL_QUADS); // begin drawing a square

    glNormal3f(0.0f, 0.0f, 1.0f); // front face points out of the screen on z.

    // specific texture coordinates to take only the section corresponding to the catalog
    glTexCoord2d(0.0, 1.0);
    glVertex2f(-SOURCE_UNIT, SOURCE_UNIT); // Bottom Left
    glTexCoord2d( _size[_currentSize] / double(RenderingManager::getInstance()->getFrameBufferWidth()), 1.0);
    glVertex2f( SOURCE_UNIT, SOURCE_UNIT); // Bottom Right
    glTexCoord2d( _size[_currentSize] / double(RenderingManager::getInstance()->getFrameBufferWidth()), 0.0);
    glVertex2f( SOURCE_UNIT, -SOURCE_UNIT); // Top Right
    glTexCoord2d(0.0, 0.0);
    glVertex2f(-SOURCE_UNIT, -SOURCE_UNIT); // Top Left

    glEnd();

	glMatrixMode(GL_PROJECTION);
	glPopMatrix();
	glMatrixMode(GL_MODELVIEW);
	glPopMatrix();

	glPopAttrib();
}



bool CatalogView::isInside(const QPoint &pos){

	if (_visible && pos.x() > viewport[0] && (viewport[3] - pos.y()) < (int)(_height / v_unit) + 10 )
		return true;

	return false;
}

bool CatalogView::mousePressEvent(QMouseEvent *event)
{
	if ( isInside(event->pos()) ) {
		// get coordinates of clic in object space
		GLdouble z;
		gluUnProject((double)event->x(), (double)event->y(), 1.0, modelview, projection, viewport, &_clicX, &_clicY, &z);

		return true;
	}

	return false;
}

bool CatalogView::mouseDoubleClickEvent ( QMouseEvent * event )
{
	if (isInside(event->pos()) ) {
		return true;
	}

	return false;
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
		return true;
	}

	return false;
}




