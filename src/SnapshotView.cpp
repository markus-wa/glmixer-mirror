/*
 * SnapshotView.cpp
 *
 *  Created on: Feb 3, 2018
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
 *   Copyright 2009, 2018 Bruno Herbelin
 *
 */

#include "SnapshotView.h"

#include "SnapshotManager.h"
#include "RenderingManager.h"
#include "RenderingSource.h"
#include "ViewRenderWidget.h"
#include "OutputRenderWindow.h"

SnapshotView::SnapshotView(): View(), _visible(false), _view(0), _factor(0.0), _renderSource(0)
{
    currentAction = View::NONE;
    zoom = 0.1;
    title = " Snapshot view";


    _begin = -8.0 * SOURCE_UNIT;
    _end = 8.0 * SOURCE_UNIT;
    _y = -8.0 * SOURCE_UNIT;
}

SnapshotView::~SnapshotView() {

    clear();
}


void SnapshotView::setVisible(bool on, View *activeview){

    // do not update status if no change requested
    if ( _visible == on )
        return;

    // change status : reset
    _view = 0;
    _factor = 0.0;
    _visible = on;

    // new status is ON
    if (_visible) {
        resize(RenderingManager::getRenderingWidget()->width(), RenderingManager::getRenderingWidget()->height());

        // update view
        if (activeview) {
            // set view to manipulate
            _view = activeview;
            // back to no action
            setAction(View::NONE);
        }
        // cannot be visible without a valid view
        else
            _visible = false;
    }
}

void SnapshotView::resize(int w, int h)
{
    View::resize(w, h);
    glViewport(0, 0, viewport[2], viewport[3]);

    // Setup specific projection and view for this window
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();

    if (w > h)
         glOrtho(-SOURCE_UNIT* (double) viewport[2] / (double) viewport[3], SOURCE_UNIT*(double) viewport[2] / (double) viewport[3], -SOURCE_UNIT, SOURCE_UNIT, -MAX_DEPTH_LAYER, 1.0);
     else
         glOrtho(-SOURCE_UNIT, SOURCE_UNIT, -SOURCE_UNIT*(double) viewport[3] / (double) viewport[2], SOURCE_UNIT*(double) viewport[3] / (double) viewport[2], -MAX_DEPTH_LAYER, 1.0);

    glGetDoublev(GL_PROJECTION_MATRIX, projection);

    glMatrixMode(GL_MODELVIEW);
    setModelview();
}

void SnapshotView::setModelview()
{
    View::setModelview();
    glScaled(zoom, zoom, zoom);
    glGetDoublev(GL_MODELVIEW_MATRIX, modelview);
}


void SnapshotView::paint()
{
    static double renderingAspectRatio = 1.0;
    static int _baseAlpha = ViewRenderWidget::program->uniformLocation("baseAlpha");
    static int _baseColor = ViewRenderWidget::program->uniformLocation("baseColor");

    if (!_renderSource) {
        // create render source on first occurence
        try {
            // create a source appropriate
            _renderSource = new RenderingSource(false, 0.0);
            _renderSource->setAlphaCoordinates(_begin, _y);

        } catch (AllocationException &e){
            qWarning() << "Cannot create source; " << e.message();
            // return an invalid pointer
            _renderSource = 0;
            return;
        }
    }

    // discard if not visible
    if (!_visible)
        return;

    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();
    glScaled(zoom, zoom, zoom);

    // 0) background fading
    glColor4ub(COLOR_FADING, 200 - (int) (_factor * 50.0));
    glCallList(ViewRenderWidget::fading);

    // 1) draw line
    glLineWidth(15.0);
    glColor4ub(250, 250, 250, 20);
    glBegin(GL_LINES);
    glVertex3d(_begin, _y, 0.0);
    glVertex3d(_end, _y, 0.0);
    glEnd();
    glLineWidth(2.0);
    glColor4ub(250, 250, 250, 230);
    glBegin(GL_LINES);
    glVertex3d(_begin, _y, 0.0);
    glVertex3d(_end, _y, 0.0);
    glEnd();
    glPointSize(13);
    glBegin(GL_POINTS);
    glVertex3d(_begin, _y, 0.0);
    glVertex3d(_end, _y, 0.0);
    glEnd();

    // set mode for source
    ViewRenderWidget::setSourceDrawingMode(true);

    // 3) draw destination icon

    // 4) draw cursor

    // bind the source textures
    _renderSource->bind();
    _renderSource->setShaderAttributes();

    // draw the source cursor at the position for given factor
    double ax = _begin + _factor * (_end - _begin) ;
    _renderSource->setAlphaCoordinates(ax, _y);
    glTranslated(_renderSource->getAlphaX(), _renderSource->getAlphaY(), 0.0);

    renderingAspectRatio = _renderSource->getAspectRatio();
    if ( ABS(renderingAspectRatio) > 1.0)
        glScaled(ViewRenderWidget::iconSize * SOURCE_UNIT, ViewRenderWidget::iconSize * SOURCE_UNIT / renderingAspectRatio,  1.0);
    else
        glScaled(ViewRenderWidget::iconSize * SOURCE_UNIT * renderingAspectRatio, ViewRenderWidget::iconSize * SOURCE_UNIT,  1.0);

    // draw flat version of the source
    ViewRenderWidget::program->setUniformValue( _baseAlpha, 1.f);
    _renderSource->draw();

    // draw border
    ViewRenderWidget::program->setUniformValue(_baseColor, QColor(COLOR_FRAME));
    if ( currentAction == View::NONE)
        glCallList(ViewRenderWidget::border_thin_shadow);
    else
        glCallList(ViewRenderWidget::border_large_shadow);

    // done geometry
    glPopMatrix();

    // unset mode for source
    ViewRenderWidget::setSourceDrawingMode(false);
}

bool SnapshotView::mousePressEvent(QMouseEvent *event)
{
    if (!_visible || !event)
        return false;

    if ( getSourcesAtCoordinates(event->x(), viewport[3] - event->y()) ) {

        if ( isUserInput(event, View::INPUT_TOOL) )
            // ready for grabbing the current source
            setAction(View::TOOL);
    }
    else {
        // clic in background to escape view
        _visible = false;
    }

    return true;
}

bool SnapshotView::mouseMoveEvent(QMouseEvent *event)
{
    if (!_visible || !event)
        return false;

    if ( currentAction == View::TOOL ) {
        // grab source under cursor
        grabSource(_renderSource, event->x(), viewport[3] - event->y());
        // apply factor change
        _view->applyTargetSnapshot(_factor);
    }
    // else Show mouse over cursor only if no user input
    else if ( isUserInput(event, View::INPUT_NONE ) )
    {
        if ( getSourcesAtCoordinates(event->x(), viewport[3] - event->y(), false) )
            setAction(View::OVER);
        else
            setAction(View::NONE);
    }

    return true;
}

void SnapshotView::grabSource(Source *s, int x, int y) {

    if (!s) return;

    double ax, ay, az; // after  movement
    gluUnProject((double) x, (double) y, 0.0,
            modelview, projection, viewport, &ax, &ay, &az);

    _factor = qBound(0.0, ( ax - _begin) / ( _end - _begin ), 1.0);
}


bool SnapshotView::mouseReleaseEvent ( QMouseEvent * event )
{
    // make sure none action
    setAction(View::NONE);

    if (!_visible || !event)
        return false;

    if ( getSourcesAtCoordinates(event->x(), viewport[3] - event->y(), false) )
        setAction(View::OVER);

    return true;
}

bool SnapshotView::mouseDoubleClickEvent ( QMouseEvent * event )
{
    if (!_visible || !event)
        return false;

    // clic in background to escape view
    _visible = false;

    return true;
}

bool SnapshotView::wheelEvent ( QWheelEvent * event )
{
    if (!_visible || !event)
        return false;

    // increment factor
    _factor = qBound(0.0, _factor + (double) event->delta() * 0.0005, 1.0);
    _view->applyTargetSnapshot(_factor);

    return true;
}

bool SnapshotView::getSourcesAtCoordinates(int mouseX, int mouseY, bool clic) {

    if (!_renderSource)
        return false;

    // prepare variables
    GLuint selectBuf[SELECTBUFSIZE] = { 0 };
    GLint hits = 0;

    // init picking
    glSelectBuffer(SELECTBUFSIZE, selectBuf);
    (void) glRenderMode(GL_SELECT);

    // picking in name 0, labels set later
    glInitNames();
    glPushName(0);

    // use the projection as it is, but remember it.
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    // setup the projection for picking
    glLoadIdentity();
    gluPickMatrix((double) mouseX, (double) mouseY, 1.0, 1.0, viewport);
    glMultMatrixd(projection);

    // rendering for select mode
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();
    glMultMatrixd(modelview);

    glTranslated(_renderSource->getAlphaX(), _renderSource->getAlphaY(), 0.0);
    double renderingAspectRatio = _renderSource->getAspectRatio();
    if ( ABS(renderingAspectRatio) > 1.0)
        glScaled(ViewRenderWidget::iconSize * SOURCE_UNIT, ViewRenderWidget::iconSize * SOURCE_UNIT / renderingAspectRatio,  1.0);
    else
        glScaled(ViewRenderWidget::iconSize * SOURCE_UNIT * renderingAspectRatio, ViewRenderWidget::iconSize * SOURCE_UNIT,  1.0);

    _renderSource->draw(GL_SELECT);
    glPopMatrix();

    // compute picking . return to rendering mode
    hits = glRenderMode(GL_RENDER);

    // set the matrices back
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);


    return (hits != 0);

}
