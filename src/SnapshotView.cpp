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
        }
        // cannot be visible without a valid view
        else
            _visible = false;
    }

    // no action by default
    setAction(View::NONE);
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
    // discard if not visible
    if (!_visible)
        return;

    // opengl tools
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

    // setup drawing for background
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    gluOrtho2D(-1, 1, -1, 1);

    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();

    // 0) background fading
    glColor4ub(COLOR_FADING, 100 - (int) (_factor * 90.0));
    glCallList(ViewRenderWidget::snapshot);

    // 1) draw line

    // setup drawing for source
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glMultMatrixd(projection);

    glMatrixMode(GL_MODELVIEW);
    glMultMatrixd(modelview);

    glPushMatrix();
    glScaled(ABS(_begin), ABS(_y), 1.0);
    glCallList(ViewRenderWidget::snapshot + 1);
    glPopMatrix();

    // 2) draw cursor (render preview with loopback)

    // set mode for source
    ViewRenderWidget::setSourceDrawingMode(true);
    ViewRenderWidget::resetShaderAttributes();

    // bind the source textures
    _renderSource->bind();

    // draw the source cursor at the position for given factor
    double ax = _begin + _factor * (_end - _begin) ;
    _renderSource->setAlphaCoordinates(ax, _y);
    glTranslated(_renderSource->getAlphaX(), _renderSource->getAlphaY(), 0.0);

    renderingAspectRatio = _renderSource->getAspectRatio();
    if ( ABS(renderingAspectRatio) > 1.0)
        glScaled(DEFAULT_ICON_SIZE * SOURCE_UNIT, DEFAULT_ICON_SIZE * SOURCE_UNIT / renderingAspectRatio,  1.0);
    else
        glScaled(DEFAULT_ICON_SIZE * SOURCE_UNIT * renderingAspectRatio, DEFAULT_ICON_SIZE * SOURCE_UNIT,  1.0);

    // draw flat version of the source
    ViewRenderWidget::program->setUniformValue( _baseAlpha, 1.f);
    _renderSource->draw();

    // draw border
    ViewRenderWidget::program->setUniformValue(_baseColor, QColor(COLOR_FRAME));
    if ( currentAction == View::NONE)
        glCallList(ViewRenderWidget::border_thin_shadow);
    else
        glCallList(ViewRenderWidget::border_large_shadow);

    // unset mode for source
    ViewRenderWidget::setSourceDrawingMode(false);

    // done drawing
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();

    glMatrixMode(GL_MODELVIEW);
    glPopMatrix();

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
        _view->applyTargetSnapshot(_factor, _snapshots);
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
    _view->applyTargetSnapshot(_factor, _snapshots);

    return true;
}

bool SnapshotView::keyPressEvent ( QKeyEvent * event ){

    double factor = 0.01;
    // ALTERNATE ACTION
    if ( QApplication::keyboardModifiers() & Qt::AltModifier )
        factor *= 10.0;

    switch (event->key()) {
    case Qt::Key_Down:
    case Qt::Key_Left:
        _factor = qBound(0.0, _factor - factor, 1.0);
        break;
    case Qt::Key_Up:
    case Qt::Key_Right:
        _factor = qBound(0.0, _factor + factor, 1.0);
        break;
    case Qt::Key_Escape:
        _visible = false;
    default:
        return false;
    }

    _view->applyTargetSnapshot(_factor, _snapshots);

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
        glScaled(DEFAULT_ICON_SIZE * SOURCE_UNIT, DEFAULT_ICON_SIZE * SOURCE_UNIT / renderingAspectRatio,  1.0);
    else
        glScaled(DEFAULT_ICON_SIZE * SOURCE_UNIT * renderingAspectRatio, DEFAULT_ICON_SIZE * SOURCE_UNIT,  1.0);

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


void SnapshotView::setAction(ActionType a){

    View::setAction(a);

    switch(a) {
    case View::OVER:
        RenderingManager::getRenderingWidget()->setMouseCursor(ViewRenderWidget::MOUSE_HAND_OPEN);
        break;
    case View::TOOL:
        RenderingManager::getRenderingWidget()->setMouseCursor(ViewRenderWidget::MOUSE_HAND_CLOSED);
        break;
    default:
        RenderingManager::getRenderingWidget()->setMouseCursor(ViewRenderWidget::MOUSE_ARROW);
        break;
    }
}


void SnapshotView::setTargetSnapshot(QString id)
{
    // reset targets
    _snapshots.clear();

    // read destination
    QMap<Source *, QVector<double> > destinations = SnapshotManager::getInstance()->getSnapshot(id);

    // create snapshot coordinate target list
    QMapIterator<Source *, QVector<double> > it(destinations);
    while (it.hasNext()) {
        it.next();

        // read snapshot values
        QVector<double> dest = it.value();
        QVector < QPair<double,double> >  snap;

        // alpha
        snap << qMakePair( dest[0], dest[0] - it.key()->getAlphaX() );
        snap << qMakePair( dest[1], dest[1] - it.key()->getAlphaY() );

        // geometry
        snap << qMakePair( dest[2], dest[2] - it.key()->getX() );
        snap << qMakePair( dest[3], dest[3] - it.key()->getY() );
        snap << qMakePair( dest[4], dest[4] - it.key()->getScaleX() );
        snap << qMakePair( dest[5], dest[5] - it.key()->getScaleY() );
        // special case for angles ; make sure we turn left or right to minimize angle
        double da = dest[6] - it.key()->getRotationAngle();
        if ( da > 180.0 )
            da = -(360.0 - da);
        snap << qMakePair( dest[6], da );

        // texture coordinates
        QRectF tc =  it.key()->getTextureCoordinates();
        snap << qMakePair( dest[7], dest[7] - tc.x() );
        snap << qMakePair( dest[8], dest[8] - tc.y() );
        snap << qMakePair( dest[9], dest[9] - tc.width() );
        snap << qMakePair( dest[10], dest[10] - tc.height() );

        // layer
        snap << qMakePair( dest[11], dest[11] - it.key()->getDepth() );

        _snapshots[it.key()] = snap;
    }

}

