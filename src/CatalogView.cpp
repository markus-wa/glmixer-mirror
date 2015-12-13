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

#include "defines.h"
#include "CatalogView.h"
#include "ViewRenderWidget.h"
#include "RenderingManager.h"
#include "SelectionManager.h"
#include "OutputRenderWindow.h"
#include "Tag.h"

#define TOPMARGIN 20

CatalogView::CatalogView() : View(), _visible(true), _alpha(1.0), _catalogfbo(0)
{
    _size[SMALL] = 80.0;
    _iconSize[SMALL] = 60.0;
    _spacing[SMALL] = 10.0;

    _size[MEDIUM] = 100.0;
    _iconSize[MEDIUM] = 80.0;
    _spacing[MEDIUM] = 14.0;

    _size[LARGE] = 130.0;
    _iconSize[LARGE] = 100.0;
    _spacing[LARGE] = 18.0;

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
    viewport[1] = 0;
	viewport[2] = _size[_currentSize];
    viewport[3] = h -TOPMARGIN;

    _widgetArea.setWidth(w);
    _widgetArea.setHeight(h);

}


void CatalogView::setSize(catalogSize s){

	_currentSize = s;
	resize(RenderingManager::getRenderingWidget()->width(), RenderingManager::getRenderingWidget()->height());

}


void CatalogView::setModelview()
{
//    glTranslatef(0, getPanningY(), 0.0);

}

void CatalogView::setVisible(bool on){

    _visible = on;
    resize(RenderingManager::getRenderingWidget()->width(), RenderingManager::getRenderingWidget()->height());
}

void CatalogView::clear() {

    // Check limits of the openGL texture
    GLint maxtexturewidth = TEXTURE_REQUIRED_MAXIMUM;

    if (glewIsSupported("GL_ARB_internalformat_query2"))
        glGetInternalformativ(GL_TEXTURE_2D, GL_RGBA8, GL_MAX_WIDTH, 1, &maxtexturewidth);
    else
        glGetIntegerv(GL_MAX_TEXTURE_SIZE, &maxtexturewidth);

    maxtexturewidth = qMin(maxtexturewidth, GL_MAX_FRAMEBUFFER_WIDTH);

    if (!_catalogfbo)
        _catalogfbo = new QGLFramebufferObject(maxtexturewidth, CATALOG_TEXTURE_HEIGHT);
    Q_CHECK_PTR(_catalogfbo);

    if (_catalogfbo->bind()) {

        glViewport(0, 0, _catalogfbo->width(), _catalogfbo->height());
        // clear to transparent
        glClearColor( 0.0, 0.0, 0.0, 1.0);
        glClear(GL_COLOR_BUFFER_BIT);

        _catalogfbo->release();
    }
    else
        qFatal( "%s", qPrintable( QObject::tr("OpenGL Frame Buffer Objects is not accessible  (cannot initialize the catalog View buffer %1x%2).").arg(maxtexturewidth).arg(CATALOG_TEXTURE_HEIGHT)));

    // clear the list of catalog sources
    while (!_icons.isEmpty()) {
        delete _icons.pop();
    }

}

void CatalogView::drawSource(Source *s)
{
    // This method is called by the rendering manager
    if (_catalogfbo->bind()) {

        // 1 attribute a locations for rendering the source in the FBO
        // NB : icons are rendered as squares of the size of FBO height
        QRect coords(0, 0, _catalogfbo->height(), _catalogfbo->height());
        if (!_icons.empty())
            coords.moveLeft( (_icons.top())->fbocoordinates.right() );

        // 2 render the source into the section of FBO texture attibuted
        glViewport(coords.left(), coords.top(), coords.width(), coords.height());
        glMatrixMode(GL_PROJECTION);
        glLoadIdentity();
        gluOrtho2D(-1.0, 1.0, -1.0, 1.0);
        glMatrixMode(GL_MODELVIEW);
        glLoadIdentity();

        // non-transparency blending
        glBlendFunc(GL_ONE, GL_ONE);
        glBlendEquation(GL_FUNC_ADD);

        // draw source in FBO
        s->draw();

        // done drawing
        _catalogfbo->release();

        // 3 create icon information
        Icon *i = new Icon;
        i->source = s;
        // coordinates of pixels where the source is rendered in FBO
        i->fbocoordinates = coords;
        // normalized texture coordinates
        i->texturecoordinates = QRectF( coords.left() / (qreal) _catalogfbo->width(), 0.0,
                                        coords.width() / (qreal) _catalogfbo->width(), 1.0);
        // stack information about icon
        _icons.push(i);

    }
    else
        qFatal( "%s", qPrintable( QObject::tr("OpenGL Frame Buffer Objects is not accessible."
            "(The program cannot render in the catalog view.)")));

}


void CatalogView::reorganize() {

    QRect previous(0,0,0,0);
    _allSourcesArea = QRect(0, 0, 0, 0);

    // compute position in global surface of the source icons
    foreach ( Icon *item, _icons) {

        // stack icons vertically
        item->coordinates.setLeft( (int) (_size[_currentSize] - _iconSize[_currentSize]) / 2.0  );
        item->coordinates.setTop( previous.bottom() + _spacing[_currentSize]);
        item->coordinates.setWidth( (int) _iconSize[_currentSize] );
        item->coordinates.setHeight( (int) _iconSize[_currentSize] / item->source->getAspectRatio() );

        // remember last for loop
        previous = item->coordinates;

        // rect united : get bbox of all items to define limits of scrolling
        _allSourcesArea = _allSourcesArea.united(previous);

    }

    // add borders to area
    _allSourcesArea.adjust(0, 0, 0, (int) (_spacing[_currentSize]) * 2 );

    // adjust maximum vertical scrolling
    int diff = _allSourcesArea.height() - (_widgetArea.height() - TOPMARGIN );
    maxpany = qBound(0.0, (double) diff, (double)_allSourcesArea.height());
    pany = qBound( 0.0, pany, maxpany);
}

void CatalogView::paint() {

    if (!_catalogfbo)
        return;

	// Paint the catalog view is only drawing the off-screen rendered catalog texture.
	// This texture is filled during rendering manager fbo update with the clear and drawSource methods.
	//
	// This paint is called by ViewRenderWidget

	glPushAttrib(GL_COLOR_BUFFER_BIT  | GL_VIEWPORT_BIT);

    // draw only in the area of the screen covered by the catalog
    viewport[3] =  qBound( (int) (_spacing[_currentSize]), _allSourcesArea.height(), _widgetArea.height() - TOPMARGIN);
    glViewport(viewport[0],viewport[1], viewport[2], viewport[3]);

	// use a standard ortho projection to paint the quad with catalog
	glMatrixMode(GL_PROJECTION);
	glPushMatrix();
    glLoadIdentity();
    gluOrtho2D(0, viewport[2], 0, viewport[3]);

	glMatrixMode(GL_MODELVIEW);
	glPushMatrix();
    glLoadIdentity();

    // standard transparency
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glBlendEquation(GL_FUNC_ADD);

    // background
    glColor4f(0.4, 0.4, 0.4, _alpha - 0.2);
    glBegin(GL_QUADS);
        glVertex2i( _size[_currentSize] + 1, 0);
        glVertex2i( 1, 0);
        glVertex2i( 1, viewport[3] );
        glVertex2i( _size[_currentSize] + 1,  viewport[3] );
    glEnd();

    // scroll
    zoom += (pany - zoom) * 0.1;
    glTranslated(0.0, -zoom, 0.0);

    // draw source icons
    glColor4f(1.0, 1.0, 1.0, _alpha);
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, _catalogfbo->texture());

    // draw image of source icon
    foreach ( Icon *item, _icons) {
        glBegin(GL_QUADS);
            glTexCoord2f( item->texturecoordinates.left(), item->texturecoordinates.top());
            glVertex2i( item->coordinates.left(), item->coordinates.top()); // Top Left
            glTexCoord2f( item->texturecoordinates.right(), item->texturecoordinates.top());
            glVertex2i( item->coordinates.right(), item->coordinates.top()); // Top Right
            glTexCoord2f( item->texturecoordinates.right(), item->texturecoordinates.bottom());
            glVertex2i( item->coordinates.right(), item->coordinates.bottom()); // Bottom Right
            glTexCoord2f( item->texturecoordinates.left(), item->texturecoordinates.bottom());
            glVertex2i( item->coordinates.left(), item->coordinates.bottom()); // Bottom Left
        glEnd();
    }

    glDisable(GL_TEXTURE_2D);

    // draw borders of source icon
    foreach ( Icon *item, _icons) {

        // normal border
        if (item->source->isModifiable()) {
            // Tag color
            glColor4ub(item->source->getTag()->getColor().red(), item->source->getTag()->getColor().green(), item->source->getTag()->getColor().blue(), 255 * _alpha);
        } else {
            glColor4ub(COLOR_SOURCE_STATIC, 255 * _alpha);
        }

        if (RenderingManager::getInstance()->isCurrentSource(item->source))
            glLineWidth(3.0);
        else
            glLineWidth(1.0);

        glBegin(GL_LINE_LOOP);
            glVertex2i( item->coordinates.left(), item->coordinates.top()); // Top Left
            glVertex2i( item->coordinates.right(), item->coordinates.top()); // Top Right
            glVertex2i( item->coordinates.right(), item->coordinates.bottom()); // Bottom Right
            glVertex2i( item->coordinates.left(), item->coordinates.bottom()); // Bottom Left
        glEnd();

        // selection border
        if ( SelectionManager::getInstance()->isInSelection(item->source) ) {
            glColor4ub(COLOR_SELECTION, 255 * _alpha);
            glLineStipple(1, 0x9999);
            glLineWidth(2.0);
            glEnable(GL_LINE_STIPPLE);
            glBegin(GL_LINE_LOOP);
                glVertex2i( item->coordinates.left() - 3, item->coordinates.top() -3); // Top Left
                glVertex2i( item->coordinates.right() + 3, item->coordinates.top() -3); // Top Right
                glVertex2i( item->coordinates.right() + 3, item->coordinates.bottom() +3); // Bottom Right
                glVertex2i( item->coordinates.left() - 3, item->coordinates.bottom() +3); // Bottom Left
            glEnd();
            glDisable(GL_LINE_STIPPLE);
        }
    }

    // not drawing sources anymore (no zoom)
    glLoadIdentity();
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, ViewRenderWidget::mask_textures[8]);

    // draw overlay gradient to black (top and bottom) to indicate overflow
    if (zoom > 0.1) {
        glColor4f(0.8, 0.8, 0.8, _alpha * qBound(0.0, zoom , 1.0) );
        glBegin(GL_QUADS);
            glTexCoord2f( 0.f, 1.f);
            glVertex2i( _size[_currentSize] + 1, 0);
            glTexCoord2f( 1.f, 1.f);
            glVertex2i( 1, 0);
            glTexCoord2f( 1.f, 0.f);
            glVertex2i( 1, 2 * _spacing[_currentSize]);
            glTexCoord2f( 0.f, 0.f);
            glVertex2i( _size[_currentSize] + 1, 2 * _spacing[_currentSize]);
        glEnd();
    }
    if ( maxpany - zoom > 0.1)
    {
        glColor4f(0.8, 0.8, 0.8, _alpha * qBound(0.0, maxpany -zoom , 1.0) );
        glBegin(GL_QUADS);
            glTexCoord2f( 0.f, 1.f);
            glVertex2i( _size[_currentSize] + 1, viewport[3]);
            glTexCoord2f( 1.f, 1.f);
            glVertex2i( 1, viewport[3]);
            glTexCoord2f( 1.f, 0.f);
            glVertex2i( 1, viewport[3] - (2 * _spacing[_currentSize]));
            glTexCoord2f( 0.f, 0.f);
            glVertex2i( _size[_currentSize] + 1, viewport[3] - (2 * _spacing[_currentSize]));
        glEnd();
    }

    glDisable(GL_TEXTURE_2D);

    // draw frame
    glColor4f(0.8, 0.8, 0.8, 1.0);
    glBegin(GL_LINES);
        glVertex2i( 1, 1);
        glVertex2i( 1, viewport[3] - 1);
        glVertex2i( 1, viewport[3] - 1);
        glVertex2i( _size[_currentSize],  viewport[3] - 1);
    glEnd();

	glMatrixMode(GL_PROJECTION);
	glPopMatrix();
	glMatrixMode(GL_MODELVIEW);
	glPopMatrix();

	glPopAttrib();

}



bool CatalogView::isInside(const QPoint &pos){

    if (_visible && pos.x() > viewport[0] && pos.y() > (_widgetArea.height()-viewport[3]) )
		return true;

	return false;
}


bool CatalogView::getSourcesAtCoordinates(int mouseX, int mouseY, bool clic) {

    bool foundsource = false;
    clickedSources.clear();

    QPoint pos(mouseX - viewport[0], (_widgetArea.height() - mouseY) - viewport[1] + (int)pany );

    foreach ( Icon *item, _icons) {

        if (item->coordinates.contains(pos)) {
            foundsource = true;
            if (clic)
                clickedSources.insert( item->source );
            break;
        }
    }

    return foundsource;
}

bool CatalogView::mousePressEvent(QMouseEvent *event)
{
    if ( isInside(event->pos()) ) {

        if (isUserInput(event, INPUT_CONTEXT_MENU)) {
            RenderingManager::getRenderingWidget()->showContextMenu(ViewRenderWidget::CONTEXT_MENU_CATALOG, event->pos());
            return true;
        }

        if (getSourcesAtCoordinates(event->pos().x(), event->pos().y(), true)) {

            Source *clicked = *clickedSources.begin();

            // SELECT MODE
            if ( isUserInput(event, INPUT_SELECT) ) {
                // add / remove the clicked source from the selection
                if ( SelectionManager::getInstance()->isInSelection(clicked) )
                    SelectionManager::getInstance()->deselect(clicked);
                else
                    SelectionManager::getInstance()->select(clicked);
            }
            else
                RenderingManager::getInstance()->setCurrentSource( clicked->getId() );

            return true;
        }
    }
	return false;
}


bool CatalogView::mouseMoveEvent(QMouseEvent *event)
{
	bool isin = isInside(event->pos());

	if (isin != !isTransparent())
		setTransparent(!isin);

    if (isin) {

        if ( (_widgetArea.height() - event->pos().y()) > viewport[3] - _spacing[_currentSize] ) {
            pany =  maxpany;
            return true;
        }
        else if ( (_widgetArea.height() - event->pos().y()) <  _spacing[_currentSize] ) {
            pany =  0.0;
            return true;
        }
        else
            pany = zoom;


        if ( getSourcesAtCoordinates(event->pos().x(), event->pos().y()) )
            RenderingManager::getRenderingWidget()->setMouseCursor(ViewRenderWidget::MOUSE_HAND_INDEX);
        else
            RenderingManager::getRenderingWidget()->setMouseCursor(ViewRenderWidget::MOUSE_ARROW);

    }

	return isin;
}


void CatalogView::setTransparent(bool on)
{
	_alpha = on ? 0.5 : 1.0;
	RenderingManager::getRenderingWidget()->setFaded(!on);
}

bool CatalogView::mouseReleaseEvent ( QMouseEvent * event )
{
//	// the clic (when mouse press was down) was on a source ?
//	if (sourceClicked) {

//		// detect select mode
//		if ( isUserInput(cause, INPUT_SELECT) )
//			SelectionManager::getInstance()->select(sourceClicked);
//		else if ( isUserInput(cause, INPUT_TOOL) || isUserInput(cause, INPUT_TOOL_INDIVIDUAL) ||  isUserInput(cause, INPUT_ZOOM) ) {
//			// make this source the current
//			RenderingManager::getInstance()->setCurrentSource(sourceClicked->getId());
//			// zoom current
//			if ( isUserInput(cause, INPUT_ZOOM) )
//				RenderingManager::getRenderingWidget()->zoomCurrentSource();
//		}

//		sourceClicked = 0;
//		return true;
//	}

	return false;
}


bool CatalogView::wheelEvent ( QWheelEvent * event )
{
    if (isInside(event->pos())) {

        pany = qBound( 0.0, pany + double(event->delta()) / 2.0, maxpany);
        return true;
	}

	return false;
}




