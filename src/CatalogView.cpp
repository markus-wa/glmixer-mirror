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

CatalogView::CatalogView() : View(), _visible(true), _height(0), h_unit(1.0), v_unit(1.0), _alpha(1.0) {

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

	if ( w > 0 && h > 0) {
		viewport[2] = w;
		viewport[3] = h;

	}

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

	glClearColor( 1.0, 1.0, 1.0, 0.0);
	glClear(GL_COLOR_BUFFER_BIT);

	// TODO : make a displaylist

//	glTranslatef( -SOURCE_UNIT + float(_size) * h_unit, 0.0, 0.0 );
//	glScalef( float(_size) / float(viewport[2]), 1.0, 1.0 );
//    glCallList(ViewRenderWidget::catalogbg);


	glColor4f(0.6, 0.6, 0.6, 0.6);
    glDisable(GL_TEXTURE_2D);

    float bl_x = -SOURCE_UNIT + 0.3 * _size[_currentSize] * h_unit;
    float bl_y = -SOURCE_UNIT + ( 2.0 * SOURCE_UNIT - _height) - 0.5;
    float tr_x = -SOURCE_UNIT + _size[_currentSize] * h_unit;
    float tr_y = SOURCE_UNIT;
    glRectf( bl_x, bl_y, tr_x, tr_y);

	glColor4f(0.8, 0.8, 0.8, 0.9);
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

void CatalogView::drawSource(Source *s, int index){

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
		double sheight_pixels = ( s->isActive() ? _largeIconSize[_currentSize] : _iconSize[_currentSize])	 / s->getAspectRatio() * v_unit;

		// increment y height by the height of this source + margin
		_height += 2.0 * sheight_pixels + 0.1 * _size[_currentSize] * v_unit;

		// place the icon at center of width, and vertically spaced
		glTranslatef( -SOURCE_UNIT + _size[_currentSize] * h_unit * 0.5, SOURCE_UNIT - _height + sheight_pixels, 0.0);
		if (s->isActive())
			glTranslatef( (_iconSize[_currentSize] -_largeIconSize[_currentSize]) * h_unit , 0.0, 0.0);
		glScalef( swidth_pixels, -sheight_pixels, 1.f);

	    glDisable(GL_BLEND);
		// draw source texture (without shading)
		s->draw(false);
	    glEnable(GL_BLEND);

	    // draw source border
		glScalef( 1.05, 1.05, 1.0);
		if (s->isActive())
			glCallList(ViewRenderWidget::border_large);
		else
			glCallList(ViewRenderWidget::border_thin);

		// TODO: store in map the coordinates of source per index
	}
}


void CatalogView::paint() {

	// Paint the catalog view is only drawing the off-screen rendered catalog texture.
	// This texture is filled during rendering manager fbo update with the clear and drawSource methods.
	//
	// This paint is called by ViewRenderWidget with a viewport covering the full window
	// and a projection matrix set according to the current view.

	glPushAttrib(GL_COLOR_BUFFER_BIT  | GL_VIEWPORT_BIT);

	glViewport(viewport[0],viewport[1],_size[_currentSize], RenderingManager::getInstance()->getFrameBufferHeight());

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
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST );
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST );
	glBindTexture(GL_TEXTURE_2D, RenderingManager::getInstance()->getCatalogTexture());

    glBegin(GL_QUADS); // begin drawing a square

    // Front Face (note that the texture's corners have to match the quad's corners)
    glNormal3f(0.0f, 0.0f, 1.0f); // front face points out of the screen on z.

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
//	if (_visible && event->x() > viewport[0] && (viewport[3] - event->y()) < (int)(_height / v_unit) + 10 ) {
	if ( isInside(event->pos()) ) {

		// TODO: left clic = select source

		// TODO: right clic = context menu [ change size (tiny, normal, big), show top, show botton ]

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
//	if (_visible) {
//		if ( event->x() > viewport[0] && (viewport[3] - event->y()) < (int)(_height / v_unit) + 10 ) {
//			_alpha = 1.0;
//			return true;
//		} else
//	}

	if (isInside(event->pos())) {
		_alpha = 1.0;
		return true;
	}

	_alpha = 0.6;
	return false;
}


bool CatalogView::mouseReleaseEvent ( QMouseEvent * event )
{
	if (isInside(event->pos()) ) {
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




