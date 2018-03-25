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
#include "WorkspaceManager.h"
#include "Tag.h"

#define TOPMARGIN 20

CatalogView::CatalogView() : View(), _visible(true), _alpha(1.0), _catalogfbo(0)
{
    _size[SMALL] = 80.0;
    _iconSize[SMALL] = 60.0;
    _spacing[SMALL] = 9.0;

    _size[MEDIUM] = 100.0;
    _iconSize[MEDIUM] = 80.0;
    _spacing[MEDIUM] = 10.0;

    _size[LARGE] = 128.0;
    _iconSize[LARGE] = 102.0;
    _spacing[LARGE] = 14.0;

    _currentSize = MEDIUM;
    title = "Catalog";

}

CatalogView::~CatalogView() {

    clear();
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

    // TODO : better texture atlas to avoid this limitation
    // Check limits of the openGL texture
    GLint maxtexturewidth = glMaximumTextureWidth();

    if (!_catalogfbo)
        _catalogfbo = new QGLFramebufferObject(maxtexturewidth, CATALOG_TEXTURE_HEIGHT);
    Q_CHECK_PTR(_catalogfbo);

    if (_catalogfbo->bind()) {

        // clear to transparent
        glClearColor( 0.0, 0.0, 0.0, 0.0);
        glClear(GL_COLOR_BUFFER_BIT);

        _catalogfbo->release();
    }
    else
        qFatal( "%s", qPrintable( QObject::tr("OpenGL Frame Buffer Objects is not accessible  (CatalogView FBO %1 x %2 bind failed).").arg(maxtexturewidth).arg(CATALOG_TEXTURE_HEIGHT)));

    // clear the list of catalog sources
    qDeleteAll(_icons);
    _icons.clear();

}

void CatalogView::drawSource(Source *s)
{
    // This method is called by the rendering manager
    if (_catalogfbo->bind()) {

        // 1 attribute a locations for rendering the source in the FBO
        // NB : icons are rendered as squares of the size of FBO height
        QRect coords(0, 0, _catalogfbo->height(), _catalogfbo->height());
        if (!_icons.empty())
            coords.moveLeft( (_icons.top())->fbocoordinates.right() + 1); // add a margin to avoid dripping

        // 2 render the source into the section of FBO texture attibuted
        glViewport(coords.left(), coords.top(), coords.width(), coords.height());
        glMatrixMode(GL_PROJECTION);
        glLoadIdentity();
        gluOrtho2D(-1.0, 1.0, -1.0, 1.0);
        glMatrixMode(GL_MODELVIEW);
        glLoadIdentity();

        // non-transparency blending
        static int _baseAlpha = ViewRenderWidget::program->uniformLocation("baseAlpha");
        ViewRenderWidget::program->setUniformValue( _baseAlpha, 1.f);

        // standard transparency blending
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glBlendEquation(GL_FUNC_ADD);

        // draw source in FBO
        // texture coordinate to default
        ViewRenderWidget::texc[0] = ViewRenderWidget::texc[6] = 0.0;
        ViewRenderWidget::texc[1] = ViewRenderWidget::texc[3] = 0.0;
        ViewRenderWidget::texc[2] = ViewRenderWidget::texc[4] = 1.0;
        ViewRenderWidget::texc[5] = ViewRenderWidget::texc[7] = 1.0;
        // draw vertex array
        glCallList(ViewRenderWidget::vertex_array_coords);
        glDrawArrays(GL_QUADS, 0, 4);

        // restore source alpha
        ViewRenderWidget::program->setUniformValue( _baseAlpha, (GLfloat) s->getAlpha());

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
        int borderleft = 2 + (int) (_size[_currentSize] - _iconSize[_currentSize]) / 2.0;
        double ar = item->source->getAspectRatio();
        if ( ar < 1 ) {
            int w = (int) _iconSize[_currentSize] * item->source->getAspectRatio();
            item->coordinates.setLeft( borderleft + (_iconSize[_currentSize] - w) / 2 );
            item->coordinates.setTop( previous.bottom() + _spacing[_currentSize]);
            item->coordinates.setWidth( w );
            item->coordinates.setHeight( (int) _iconSize[_currentSize]  );
        }
        else {
            item->coordinates.setLeft( borderleft );
            item->coordinates.setTop( previous.bottom() + _spacing[_currentSize]);
            item->coordinates.setWidth( (int) _iconSize[_currentSize] );
            item->coordinates.setHeight( (int) _iconSize[_currentSize] / item->source->getAspectRatio() );
        }

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

    // select source icons texture
    glColor4f(1.0, 1.0, 1.0, _alpha);
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, _catalogfbo->texture());

    // draw image of source icon
    foreach ( Icon *item, _icons) {
        // source texture
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

        // Tag color
        QColor c = Tag::get(item->source)->getColor();
        glColor4ub(c.red(), c.green(), c.blue(), 255 * _alpha);

        //  border
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
            glLineStipple(1, 0x7777);
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

    glEnable(GL_TEXTURE_2D);

    // icons for workspace (active and innactive)
    static GLuint texid[WORKSPACE_MAX * 2] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
    if (texid[0] == 0) {
        // generate the textures with optimal performance ;
        glGenTextures(WORKSPACE_MAX * 2, texid);
        for (int i = 0; i < WORKSPACE_MAX; ++i) {
            glBindTexture(GL_TEXTURE_2D, texid[i]);
            QImage p(WorkspaceManager::getPixmap(i+1, true).toImage());
            gluBuild2DMipmaps(GL_TEXTURE_2D, GL_COMPRESSED_RGBA, p.width(), p. height(), GL_RGBA, GL_UNSIGNED_BYTE, p.bits());
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        }
        for (int i = WORKSPACE_MAX; i < WORKSPACE_MAX * 2; ++i) {
            glBindTexture(GL_TEXTURE_2D, texid[i]);
            QImage p(WorkspaceManager::getPixmap(i-WORKSPACE_MAX+1, false).toImage());
            gluBuild2DMipmaps(GL_TEXTURE_2D, GL_COMPRESSED_RGBA, p.width(), p. height(), GL_RGBA, GL_UNSIGNED_BYTE, p.bits());
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        }
    }

    // draw icon of source workspace
    int p =  _size[_currentSize] / 5;
    foreach ( Icon *item, _icons) {
        QRect coordinates( item->coordinates.topLeft() - QPoint(p/3, p/3), QSize(p,p));
        int i = item->source->getWorkspace();
        i += i == WorkspaceManager::getInstance()->current() ? 0 : WORKSPACE_MAX;
        glBindTexture(GL_TEXTURE_2D, texid[i]);
        glColor4f(0.8, 0.8, 0.8, _alpha );
        glBegin(GL_QUADS);
            glTexCoord2f( 0.f, 1.f);
            glVertex2i( coordinates.left(), coordinates.top()); // Top Left
            glTexCoord2f( 1.f, 1.f);
            glVertex2i( coordinates.right(), coordinates.top()); // Top Right
            glTexCoord2f( 1.f, 0.f);
            glVertex2i( coordinates.right(), coordinates.bottom()); // Bottom Right
            glTexCoord2f( 0.f, 0.f);
            glVertex2i( coordinates.left(), coordinates.bottom()); // Bottom Left
        glEnd();
    }

    // not drawing sources anymore (no zoom)
    glLoadIdentity();
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
    glLineWidth(1.0);
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
            else {
                RenderingManager::getInstance()->setCurrentSource( clicked->getId() );

                if (isUserInput(event, INPUT_CONTEXT_MENU)) {
                    RenderingManager::getRenderingWidget()->showContextMenu(
                                ViewRenderWidget::CONTEXT_MENU_SOURCE, event->pos());
                }
            }
        }
        else if (isUserInput(event, INPUT_CONTEXT_MENU)) {
            RenderingManager::getRenderingWidget()->showContextMenu(
                        ViewRenderWidget::CONTEXT_MENU_CATALOG, event->pos());
        }
        setAction(View::OVER);
        return true;
    }
    setAction(View::NONE);
    return false;
}


bool CatalogView::mouseMoveEvent(QMouseEvent *event)
{
    bool isin = isInside(event->pos());

    if (isin != !isTransparent())
        setTransparent(!isin);

    if (isin) {

        // default cursor
        ViewRenderWidget::mouseCursor cursor = ViewRenderWidget::MOUSE_ARROW;
        // if mouse is over a source icon
        if ( getSourcesAtCoordinates(event->pos().x(), event->pos().y(), true)) {

            // if the mouse is over a source in the current workspace
            Source *s = *clickedSources.begin();
            if ( WorkspaceManager::getInstance()->current() == s->getWorkspace() )
                // set cursor to indicate
                cursor = ViewRenderWidget::MOUSE_HAND_INDEX;
        }
        // change cursor
        RenderingManager::getRenderingWidget()->setMouseCursor(cursor);

        // apply scroll when on borders
        if ( (_widgetArea.height() - event->pos().y()) > viewport[3] - _spacing[_currentSize] ) {
            pany =  maxpany;
        }
        else if ( (_widgetArea.height() - event->pos().y()) <  _spacing[_currentSize] ) {
            pany =  0.0;
        }
        else
            pany = zoom;

    }

    // indicate if event was used
    return isin && currentAction == View::NONE;
}


void CatalogView::setTransparent(bool on)
{
    _alpha = on ? 0.5 : 1.0;
    RenderingManager::getRenderingWidget()->setFaded(!on);
}

bool CatalogView::mouseDoubleClickEvent ( QMouseEvent * event )
{
    setAction(View::NONE);
    return isInside(event->pos());
}

bool CatalogView::mouseReleaseEvent ( QMouseEvent * event )
{
    setAction(View::NONE);
    return false;
}

bool CatalogView::wheelEvent ( QWheelEvent * event )
{
    if (isInside(event->pos())) {

        // scroll
        pany = qBound( 0.0, pany + double(event->delta()) / 2.0, maxpany);

        // reset cursor
        RenderingManager::getRenderingWidget()->setMouseCursor(ViewRenderWidget::MOUSE_ARROW);

        // accept event
        return true;
    }

    return false;
}




