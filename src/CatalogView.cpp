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
 *   Copyright 2009, 2012 Bruno Herbelin
 *
 */

#include "common.h"
#include "CatalogView.h"
#include "ViewRenderWidget.h"
#include "RenderingManager.h"
#include "SelectionManager.h"
#include "OutputRenderWindow.h"
#include "Tag.h"

#define CATALOG_TEXTURE_WIDTH 10000
#define CATALOG_TEXTURE_HEIGHT 100
#define BORDER 20

CatalogView::CatalogView() : View(), _visible(true), _width(0),_height(0), h_unit(1.0), v_unit(1.0), _alpha(1.0),
                            first_index(0), last_index(0), sourceClicked(0), cause(0), _catalogfbo(0), _scroll(0)
{
	_size[SMALL] = 49.0;
    _iconSize[SMALL] = 38.0;
	_largeIconSize[SMALL] = 21.0;

	_size[MEDIUM] = 101.0;
    _iconSize[MEDIUM] = 76.0;
	_largeIconSize[MEDIUM] = 42.0;

	_size[LARGE] = 151.0;
    _iconSize[LARGE] = 108.0;
	_largeIconSize[LARGE] = 62.0;

	_currentSize = MEDIUM;
    title = "Catalog";

}

CatalogView::~CatalogView() {

    if (_catalogfbo)
        delete _catalogfbo;
}

void CatalogView::resize(int w, int h) {

	// compute viewport considering width
	// TODO : switch depending on side (top, bottom, left, right..)
	viewport[0] = w - _size[_currentSize];
    viewport[1] = BORDER;
	viewport[2] = _size[_currentSize];
    viewport[3] = h -BORDER*2;

//    _surface = QRect(0, 0, _size[_currentSize], 0);
//	h_unit = 2.0 * SOURCE_UNIT * OutputRenderWindow::getInstance()->getAspectRatio() / double(RenderingManager::getInstance()->getFrameBufferWidth());
//	v_unit = 2.0 * SOURCE_UNIT / double(viewport[3]);
}

void CatalogView::setSize(catalogSize s){

    _scroll = 0;
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

    if (!_catalogfbo)
        _catalogfbo = new QGLFramebufferObject(CATALOG_TEXTURE_WIDTH, CATALOG_TEXTURE_HEIGHT);

    if (_catalogfbo->bind()) {

        glViewport(0, 0, _catalogfbo->width(), _catalogfbo->height());
        // clear to transparent
        glClearColor( 0.0, 0.0, 0.0, 1.0);
        glClear(GL_COLOR_BUFFER_BIT);

        _catalogfbo->release();
    }
    else
        qFatal( "%s", qPrintable( QObject::tr("OpenGL Frame Buffer Objects is not accessible."
            "\n\nThe program cannot operate properly anymore.")));

    // clear the list of catalog sources
    while (!_icons.isEmpty()) {
        delete _icons.pop();
    }

}

void CatalogView::drawSource(const Source *s)
{
    // This method is called by the rendering manager
    if (_catalogfbo->bind()) {

        Icon *i = new Icon;
        i->source = s;

        // 1st attribute a locations for rendering the source in the FBO
        i->texturecoords = QRect(0, 0, _catalogfbo->height(), _catalogfbo->height());
        if (!_icons.empty())
            i->texturecoords.moveLeft( (_icons.top())->texturecoords.right() );

        // 2nd add the information of coordinates of pixels where the source is rendered in FBO
        _icons.push(i);

        // 3rd render the source into the section of FBO texture attibuted
        glViewport(i->texturecoords.left(), i->texturecoords.top(), i->texturecoords.width(), i->texturecoords.height());
        glMatrixMode(GL_PROJECTION);
        glLoadIdentity();
        gluOrtho2D(-1.0, 1.0, -1.0, 1.0);
        glMatrixMode(GL_MODELVIEW);
        glLoadIdentity();

        // standard blending
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glBlendEquation(GL_FUNC_ADD);

        // draw source
        s->draw();

        _catalogfbo->release();
    }
    else
        qFatal( "%s", qPrintable( QObject::tr("OpenGL Frame Buffer Objects is not accessible."
            "\n\nThe program cannot operate properly anymore.")));

//	static double swidth_pixels = 0.0, sheight_pixels = 0.0, height = 0.0;
//	// Drawing a source is rendering a quad with the source texture in the catalog bar.
//	// This method is called by the rendering manager with a viewport covering the fbo
//	// and a projection matrix set to
//	// gluOrtho2D(-SOURCE_UNIT * OutputRenderWindow::getInstance()->getAspectRatio(), SOURCE_UNIT * OutputRenderWindow::getInstance()->getAspectRatio(), -SOURCE_UNIT, SOURCE_UNIT);
//	// Modelview is Identity

//	// reset height to 0 at first icon
//	if (index == 0)
//		_height = 0.0;

//	if ( s ) {

//		bool iscurrent = RenderingManager::getInstance()->isCurrentSource(s);

//		// target 60 pixels wide icons (height depending on aspect ratio)
//		// each source is a quad [-1 +1]
//		swidth_pixels = ( iscurrent ? _largeIconSize[_currentSize] : _iconSize[_currentSize]) * h_unit;
//		sheight_pixels = ( iscurrent ? _largeIconSize[_currentSize] : _iconSize[_currentSize]) / s->getAspectRatio() * v_unit;

//		// increment y height by the height of this source + margin
//		height = _height + 2.0 * sheight_pixels + 0.1 * _size[_currentSize] * v_unit;

//        // if getting out of available drawing area, skip this source and draw arrow instead
//        if (height > 2.0 * SOURCE_UNIT) {
//            if (_currentSize == LARGE)
//                setSize(MEDIUM);
//            else if (_currentSize == MEDIUM)
//                setSize(SMALL);
//            else {
//                glColor4ub(COLOR_DRAWINGS, 200);
//                glTranslatef( -_width + _size[_currentSize] * h_unit * 0.5, SOURCE_UNIT - height + _iconSize[_currentSize] * v_unit, 0.0);
//                glLineWidth(2);
//                glBegin(GL_LINE_LOOP); // draw a triangle
//                    glVertex2f(0.0, 0.50);
//                    glVertex2f(0.50, 1.0);
//                    glVertex2f(-0.50, 1.0);
//                glEnd();
//            }
//            return;
//        }

//		_height = height;

//		// place the icon at center of width, and vertically spaced
//		glTranslatef( -_width + _size[_currentSize] * h_unit * 0.5, SOURCE_UNIT - _height + sheight_pixels, 0.0);
//		if (iscurrent)
//			glTranslatef( (_iconSize[_currentSize] -_largeIconSize[_currentSize]) * h_unit , 0.0, 0.0);

//        // scale to match aspect ratio
//        glScalef( swidth_pixels, -sheight_pixels, 1.f);

//		// draw source texture (without shading)
//        glDisable(GL_BLEND);
//        glBindTexture(GL_TEXTURE_2D, s->getTextureIndex());
//        glColor4f(0.0, 0.0, 0.0, 1.0);
//        glDrawArrays(GL_QUADS, 0, 4);

//        // draw source borders (disabled texture required)
//        glBindTexture(GL_TEXTURE_2D, 0);

//        // Tag color
//        glColor4ub(s->getTag()->getColor().red(), s->getTag()->getColor().green(), s->getTag()->getColor().blue(), 200);

//		if (iscurrent)
//			glCallList(ViewRenderWidget::border_large + (s->isModifiable() ? 0 : 3));
//		else
//			glCallList(ViewRenderWidget::border_thin + (s->isModifiable() ? 0 : 3));

//		if ( SelectionManager::getInstance()->isInSelection(s) )
//			glCallList(ViewRenderWidget::frame_selection);

//        // restore state
//        glEnable(GL_BLEND);

////        // draw icon : NOT CONCLUSIVE :(
////        glTranslatef( -swidth_pixels, -sheight_pixels, 0.0);
////        glScalef(_iconSize[_currentSize] * h_unit * 0.5, _iconSize[_currentSize] * h_unit * 0.5, 1.0);
////        glBindTexture(GL_TEXTURE_2D, texid);
////        glColor4f(0.0, 0.0, 0.0, 0.0);
////        glDrawArrays(GL_QUADS, 0, 4);

//		// was it clicked ?
//		if ( cause ) {
//			// get coordinates of clic in object space
//			GLdouble x, y, z;
//			gluUnProject((double)cause->x(), (double)(RenderingManager::getRenderingWidget()->height() - cause->y()), 1.0, modelview, projection, viewport, &x, &y, &z);

//			if ( ABS( y + SOURCE_UNIT - _height + sheight_pixels) < sheight_pixels )
//				sourceClicked = s;
//		}

//	}

}


void CatalogView::reorganize() {

    QRect previous(0,0,0,0);
    _surface = QRect(0, 0, 0, 0);

    // compute position in global surface of the source icons
    foreach ( Icon *item, _icons) {

        // stack icons vertically
        item->coordinates.setLeft( (_size[_currentSize] - _iconSize[_currentSize]) / 2.0  );
        item->coordinates.setTop( previous.bottom() + 10);
        item->coordinates.setWidth( (int) _iconSize[_currentSize] );
        item->coordinates.setHeight( (int) _iconSize[_currentSize] / item->source->getAspectRatio() );

        // remember last for loop
        previous = item->coordinates;

        // rect united : get bbox of all items to define limits of scrolling
        _surface = _surface.united(item->coordinates);

        // normalize texture coordinates
        item->texturecoords = QRectF(item->texturecoords.left() / (qreal) _catalogfbo->width(),
                                     item->texturecoords.top() / (qreal) _catalogfbo->height(),
                                     item->texturecoords.width() / (qreal) _catalogfbo->width(),
                                     item->texturecoords.height() / (qreal) _catalogfbo->height());

    }

}

void CatalogView::paint() {

	// Paint the catalog view is only drawing the off-screen rendered catalog texture.
	// This texture is filled during rendering manager fbo update with the clear and drawSource methods.
	//
	// This paint is called by ViewRenderWidget

	glPushAttrib(GL_COLOR_BUFFER_BIT  | GL_VIEWPORT_BIT);

    // draw only in the area of the screen covered by the catalog
    int H =  qBound( BORDER * 2, _surface.height(), viewport[3]);
    glViewport(viewport[0],viewport[1], viewport[2], H);

	// use a standard ortho projection to paint the quad with catalog
	glMatrixMode(GL_PROJECTION);
	glPushMatrix();
    glLoadIdentity();
    gluOrtho2D(0, viewport[2], 0, H);

	glMatrixMode(GL_MODELVIEW);
	glPushMatrix();
    glLoadIdentity();

    // standard transparency
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glBlendEquation(GL_FUNC_ADD);

    // background
    glColor4f(0.4, 0.4, 0.4, _alpha - 0.2);
    glBegin(GL_QUADS);
        glVertex2i( _size[_currentSize] + 1, 1);
        glVertex2i( 1, 1);
        glVertex2i( 1, H - 1);
        glVertex2i( _size[_currentSize] + 1,  H - 1);
    glEnd();


    // scroll
    glTranslated(0, _scroll, 0);

    // draw source icons
    glColor4f(1.0, 1.0, 1.0, _alpha);
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, _catalogfbo->texture());

    while (!_icons.isEmpty()) {
        Icon *item = _icons.pop();

        glBegin(GL_QUADS);
            glTexCoord2f( item->texturecoords.left(), item->texturecoords.top());
            glVertex2i( item->coordinates.left(), item->coordinates.top()); // Top Left
            glTexCoord2f( item->texturecoords.right(), item->texturecoords.top());
            glVertex2i( item->coordinates.right(), item->coordinates.top()); // Top Right
            glTexCoord2f( item->texturecoords.right(), item->texturecoords.bottom());
            glVertex2i( item->coordinates.right(), item->coordinates.bottom()); // Bottom Right
            glTexCoord2f( item->texturecoords.left(), item->texturecoords.bottom());
            glVertex2i( item->coordinates.left(), item->coordinates.bottom()); // Bottom Left
        glEnd();

        delete item;
    }

    // frame
    glDisable(GL_TEXTURE_2D);
    glLoadIdentity();
    glColor4f(0.8, 0.8, 0.8, 1.0);
    glBegin(GL_LINE_LOOP);
        glVertex2i( _size[_currentSize] + 1, 1);
        glVertex2i( 1, 1);
        glVertex2i( 1, H - 1);
        glVertex2i( _size[_currentSize] + 1,  H - 1);
    glEnd();


	glMatrixMode(GL_PROJECTION);
	glPopMatrix();
	glMatrixMode(GL_MODELVIEW);
	glPopMatrix();

	glPopAttrib();
}



bool CatalogView::isInside(const QPoint &pos){

    if (_visible && pos.x() > viewport[0] )
//	if (_visible && pos.x() > viewport[0] && (RenderingManager::getRenderingWidget()->height() - pos.y()) < (int)(_height / v_unit) )
		return true;

	return false;
}

bool CatalogView::mousePressEvent(QMouseEvent *event)
{
	if (cause)
		delete cause;
	cause = 0;
	sourceClicked = 0;

	if ( isInside(event->pos()) ) {

		if ( isUserInput(event, INPUT_CONTEXT_MENU) )
			RenderingManager::getRenderingWidget()->showContextMenu(ViewRenderWidget::CONTEXT_MENU_CATALOG, event->pos());
		else
			cause = new QMouseEvent(event->type(), event->pos(), event->button(), event->buttons(), event->modifiers());

		return true;
	}
	return false;
}


bool CatalogView::mouseMoveEvent(QMouseEvent *event)
{
	bool isin = isInside(event->pos());

	if (isin != !isTransparent())
		setTransparent(!isin);

	return isin;
}


void CatalogView::setTransparent(bool on)
{
	_alpha = on ? 0.5 : 1.0;
	RenderingManager::getRenderingWidget()->setFaded(!on);
}

bool CatalogView::mouseReleaseEvent ( QMouseEvent * event )
{
	// the clic (when mouse press was down) was on a source ?
	if (sourceClicked) {

		// detect select mode
		if ( isUserInput(cause, INPUT_SELECT) )
			SelectionManager::getInstance()->select(sourceClicked);
		else if ( isUserInput(cause, INPUT_TOOL) || isUserInput(cause, INPUT_TOOL_INDIVIDUAL) ||  isUserInput(cause, INPUT_ZOOM) ) {
			// make this source the current
			RenderingManager::getInstance()->setCurrentSource(sourceClicked->getId());
			// zoom current
			if ( isUserInput(cause, INPUT_ZOOM) )
				RenderingManager::getRenderingWidget()->zoomCurrentSource();
		}

		sourceClicked = 0;
		return true;
	}

	return false;
}


bool CatalogView::wheelEvent ( QWheelEvent * event )
{
	if (isInside(event->pos())) {
		// TODO implement scrolling in the list

        _scroll = qBound( viewport[3] - _surface.height() - BORDER , _scroll - event->delta(), 0);
//        _scroll = qBound( 0, _scroll + event->delta(), _surface.height() - viewport[3]);

        return true;
	}

	return false;
}




