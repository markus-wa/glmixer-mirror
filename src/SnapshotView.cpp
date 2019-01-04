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
#include "CaptureSource.h"
#include "ViewRenderWidget.h"
#include "OutputRenderWindow.h"
#include "UndoManager.h"

SnapshotView::SnapshotView(): View(), _active(false), _interpolate(true), _view(0), _factor(0.0), _renderSource(0), _departureSource(0), _destinationSource(0)
{
    // init view
    currentAction = View::NONE;
    zoom = 0.1;
    title = " Snapshot view";

    // init cursor
    _begin = -8.0 * SOURCE_UNIT;
    _end = 8.0 * SOURCE_UNIT;
    _y = -8.0 * SOURCE_UNIT;

    // disable animation
    _animationTimer.invalidate();
    _animationSpeed = DEFAULT_ANIMATION_SPEED;
}

SnapshotView::~SnapshotView() {

    clear();
}


void SnapshotView::deactivate()
{
    // do not update status if no change requested
    if ( _active == false )
        return;

    _destination = QImage();

    // disable animation
    _animationTimer.invalidate();

    // change status : reset
    _active = false;
}

bool SnapshotView::activate(View *activeview, QString id)
{
    bool ret = false;

    // do not update status if no change requested
    if ( _active == true )
        return ret;

    // change status : reset
    _view = 0;
    _factor = 0.0;
    _active = true;
    _interpolate = true;
    _destinationId = QString();

    // store status
    SnapshotManager::getInstance()->storeTemporarySnapshotDescription();

    // test if everything is fine
    if (setTargetSnapshot(id) && activeview && activeview->usableTargetSnapshot(_snapshots)) {

#ifdef GLM_UNDO
    // inform undo manager
    UndoManager::getInstance()->store();
#endif

        // activate config
        _destinationId = id;
        // set view to manipulate
        _view = activeview;
        // update icon for starting point
        _departure = RenderingManager::getInstance()->captureFrameBuffer();
        // convert image to ICON_SIZE
        _departure = SnapshotManager::generateSnapshotIcon(_departure);
        // update source if already created
        if (_departureSource)
            _departureSource->setImage(_departure);

        // success
        ret = true;
    }
    // cannot be visible without a valid view
    else
        _active = false;

    // no action by default
    setAction(View::NONE);
    _animationTimer.invalidate();

    return ret;
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


void drawSource(Source *s, double scale, GLenum mode = GL_RENDER)
{
    glTranslated(s->getAlphaX(), s->getAlphaY(), 0.0);

    double renderingAspectRatio = s->getAspectRatio();
    if ( ABS(renderingAspectRatio) < 1.0)
        glScaled( scale * DEFAULT_ICON_SIZE * SOURCE_UNIT, scale * DEFAULT_ICON_SIZE * SOURCE_UNIT / renderingAspectRatio,  1.0);
    else
        glScaled(scale * DEFAULT_ICON_SIZE * SOURCE_UNIT * renderingAspectRatio, scale * DEFAULT_ICON_SIZE * SOURCE_UNIT,  1.0);

    // draw flat version of the source
    s->draw(mode);
}

void SnapshotView::paint()
{
    // discard if not visible
    if (!_active)
        return;

    // create render source on first occurence
    if (!_renderSource) {
        try {
            // create a source appropriate
            _renderSource = new RenderingSource(false, 0.0);
            _renderSource->setAlphaCoordinates(_begin, _y);
        } catch (AllocationException &e){
            qWarning() << "Cannot create snapshot slider icon; " << e.message();
            // return an invalid pointer
            _renderSource = 0;
            return;
        }
    }
    // create destination source on first occurence
    if (!_destinationSource && !_destination.isNull() ) {
        try {
            // create the texture for this source
            GLuint textureIndex;
            glGenTextures(1, &textureIndex);
            // create a source appropriate
            _destinationSource = new CaptureSource(_destination, textureIndex, 0.0);
            _destinationSource->setAlphaCoordinates(_end, _y);
        } catch (AllocationException &e){
            qWarning() << "Cannot create snapshot destination icon; " << e.message();
            // return an invalid pointer
            _destinationSource = 0;
            return;
        }
    }
    // create departure source on first occurence
    if (!_departureSource && !_departure.isNull()) {
        try {
            // create the texture for this source
            GLuint textureIndex;
            glGenTextures(1, &textureIndex);
            // create a source appropriate
            _departureSource = new CaptureSource(_departure, textureIndex, 0.0);
            _departureSource->setAlphaCoordinates(_begin, _y);
        } catch (AllocationException &e){
            qWarning() << "Cannot create snapshot departure icon; " << e.message();
            // return an invalid pointer
            _departureSource = 0;
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


    // setup drawing for source
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glMultMatrixd(projection);

    glMatrixMode(GL_MODELVIEW);
    glMultMatrixd(modelview);

    // 1) draw line

    if (_interpolate){

        glPushMatrix();
        // line between circles
        glScaled(ABS(_begin) - 1.95 * ICON_BORDER_SCALE * SOURCE_UNIT, ABS(_y), 1.0);
        glCallList(ViewRenderWidget::snapshot + 1);
        // marks
        glPointSize(4);
        glBegin(GL_POINTS);
        for (float i = -1.0; i < 1.0; i += 200.0 * ABS(_animationSpeed) )
            glVertex2d( i, -1.0);
        glEnd();
        glPointSize(7);
        glBegin(GL_POINTS);
        for (float i = -1.0; i < 1.0; i += 2000.0 * ABS(_animationSpeed) )
            glVertex2d( i, -1.0);
        glEnd();

        glPopMatrix();
    }

    // 2) draw tools and cursor

    // set mode for source
    ViewRenderWidget::setSourceDrawingMode(true);
    ViewRenderWidget::resetShaderAttributes();
    // draw flat version of sources
    ViewRenderWidget::program->setUniformValue(ViewRenderWidget::_baseAlpha, 1.f);

    //
    //  DEPARTURE ICON
    //
    // bind the source destination
    _departureSource->update();
    _departureSource->bind();
    // draw the source destination
    glPushMatrix();

    ViewRenderWidget::setBaseColor(QColor(Qt::white), 1.0);
    drawSource(_departureSource, ICON_BORDER_SCALE);
    // draw circle border
    ViewRenderWidget::setBaseColor(QColor(COLOR_DRAWINGS));
    if ( *clickedSources.begin() == _departureSource)
        glCallList(ViewRenderWidget::snapshot + 3);
    else
        glCallList(ViewRenderWidget::snapshot + 2);
    glPopMatrix();

    //
    //  DESTINATION ICON
    //
    // bind the source destination
    _destinationSource->update();
    _destinationSource->bind();
    // draw the source destination
    glPushMatrix();
    ViewRenderWidget::setBaseColor(QColor(Qt::white));
    drawSource(_destinationSource, ICON_BORDER_SCALE);
    // draw circle border
    ViewRenderWidget::setBaseColor(QColor(COLOR_DRAWINGS));
    if ( *clickedSources.begin() == _destinationSource)
        glCallList(ViewRenderWidget::snapshot + 3);
    else
        glCallList(ViewRenderWidget::snapshot + 2);
    glPopMatrix();

    //
    //  CURSOR ICON (render preview with loopback)
    //
    // bind the source slider (no need to update)
    _renderSource->bind();
    // draw the source slider at the position for given factor
    _renderSource->setAlphaCoordinates(_begin + _factor * (_end - _begin), _y);
    ViewRenderWidget::setBaseColor(QColor(Qt::white));
    drawSource(_renderSource, ICON_CURSOR_SCALE);
    // draw border
    ViewRenderWidget::setBaseColor(QColor(COLOR_FRAME));
    if ( currentAction == View::OVER || currentAction == View::GRAB)
        glCallList(ViewRenderWidget::border_large_shadow);
    else
        glCallList(ViewRenderWidget::border_thin_shadow);

    // unset mode for source
    ViewRenderWidget::setSourceDrawingMode(false);

    // done drawing
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();

    glMatrixMode(GL_MODELVIEW);
    glPopMatrix();

    // 3) animation
    if (_animationTimer.isValid()) {
        // increment
        _factor += (double) _animationTimer.restart() * _animationSpeed;
        // end animation
        if ( _factor > 1.0 || _factor < 0.0)
            _animationTimer.invalidate();
        // animate factor
        _factor = qBound(0.0, _factor, 1.0);
        _view->applyTargetSnapshot(_factor, _snapshots);
    }

}

void SnapshotView::activateTarget(bool positive)
{
    // in interpolation mode, start the animation
    if (_interpolate) {
        if (_animationTimer.isValid()) {
            // stop animation
            setAction(View::NONE);
            _animationTimer.invalidate();
        }
        else {
            // trigger animation
            setAction(View::TOOL);
            // start animation negative direction
            _animationSpeed = positive ? ABS(_animationSpeed) : -ABS(_animationSpeed);
            _animationTimer.start();
        }
    }
    // no interpolation: restore the entire snapshot
    else {
        if (positive) {
            SnapshotManager::getInstance()->restoreSnapshot(_destinationId);
            _factor = 1.0;
        } else {
            SnapshotManager::getInstance()->restoreSnapshot();
            _factor = 0.0;
        }
    }
}

bool SnapshotView::mouseDoubleClickEvent ( QMouseEvent * event )
{
    if (!_active || !event)
        return false;

    if ( getSourcesAtCoordinates(event->x(), viewport[3] - event->y()) ) {

        Source *clicsource =  *clickedSources.begin();

        // clic on the destination
        if (clicsource == _destinationSource) {
            // no interpolation: restore the entire snapshot
            SnapshotManager::getInstance()->restoreSnapshot(_destinationId);
            _factor = 1.0;
        }
        // clic on the departure
        else if (clicsource == _departureSource){
            SnapshotManager::getInstance()->restoreSnapshot();
            _factor = 0.0;
        }

        return true;
    }

    return false;
}

bool SnapshotView::mousePressEvent(QMouseEvent *event)
{
    if (!_active || !event)
        return false;

    if ( getSourcesAtCoordinates(event->x(), viewport[3] - event->y()) ) {

        // get the top most clicked source
        // (always one as getSourcesAtCoordinates returned true)
        Source *clicsource =  *clickedSources.begin();

        // tool action on a source
        if ( isUserInput(event, View::INPUT_TOOL) ) {
            // clic on the slider
            if (clicsource == _renderSource && _interpolate) {
                // interrupt animation
                if (_animationTimer.isValid())
                    _animationTimer.invalidate();
                // ready for grabbing the slider source
                setAction(View::GRAB);
            }
            // clic on the destination
            else if (clicsource == _destinationSource)
                // toggle on/off the animation, in positive direction
                activateTarget(true);
            // clic on the departure
            else if (clicsource == _departureSource)
                // toggle on/off the animation, in negative direction
                activateTarget(false);
        }
        // do not react to other mouse action on a source
    }
    // clic in background
    else {
        // interrupt animation
        if (_animationTimer.isValid())
            _animationTimer.invalidate();
        // or escape view
        else
            _active = false;
    }

    return true;
}

bool SnapshotView::mouseMoveEvent(QMouseEvent *event)
{
    if (!_active || !event)
        return false;

    if ( currentAction == View::GRAB ) {
        // grab source under cursor
        grabSource(_renderSource, event->x(), viewport[3] - event->y());
        // apply factor change
        _view->applyTargetSnapshot(_factor, _snapshots);
    }
    // else Show mouse over cursor only if no user input
    else if ( isUserInput(event, View::INPUT_NONE ) )
    {
        // set mouse cursor
        setCursorAction(event->pos());
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


void SnapshotView::setCursorAction(QPoint mousepos)
{
    if ( getSourcesAtCoordinates(mousepos.x(), viewport[3] - mousepos.y()) ) {
        // get the top most source
        // (always one as getSourcesAtCoordinates returned true)
        Source *oversource =  *clickedSources.begin();

        // on the slider
        if (oversource == _renderSource && _interpolate)
            setAction(View::OVER);
        // on the destination
        else if (oversource == _destinationSource || oversource == _departureSource)
            setAction(View::TOOL);
    }
    else
        setAction(View::NONE);
}

bool SnapshotView::mouseReleaseEvent ( QMouseEvent * event )
{
    if (!_active || !event)
        return false;

    // set mouse cursor
    setCursorAction(event->pos());

    return true;
}

bool SnapshotView::wheelEvent ( QWheelEvent * event )
{
    if (!_active || !event)
        return false;

    // set action from mouse cursor
    setCursorAction(event->pos());


    // Tool action : control the speed of animation by scroll wheel
    if (currentAction == View::TOOL) {

        double dir = SIGN(_animationSpeed);
        double speed = ABS(_animationSpeed) + SIGN(event->delta()) * 0.0002;
        _interpolate = speed < MAX_ANIMATION_SPEED;
        _animationSpeed = dir * qBound(MIN_ANIMATION_SPEED, speed, MAX_ANIMATION_SPEED);

    }
    // background : control directly factor with scroll wheel
    else if (_interpolate) {
        // increment factor
        _factor = qBound(0.0, _factor + (double) event->delta() * 0.0005, 1.0);
        _view->applyTargetSnapshot(_factor, _snapshots);
    }


    return true;
}

bool SnapshotView::keyPressEvent ( QKeyEvent * event )
{
    if (!_active || !event)
        return false;

    if (_interpolate) {

        double delta = 100.0 * ABS(_animationSpeed);
        // ALTERNATIVE ACTION
        if ( QApplication::keyboardModifiers() & Qt::AltModifier )
            delta *= 10.0;

        switch (event->key()) {
        case Qt::Key_Down:
        case Qt::Key_Left:
            _factor = qBound(0.0, _factor - delta, 1.0);
            break;
        case Qt::Key_Up:
        case Qt::Key_Right:
            _factor = qBound(0.0, _factor + delta, 1.0);
            break;
        default:
            return false;
        }

        _view->applyTargetSnapshot(_factor, _snapshots);
    }
    else {
        switch (event->key()) {
        case Qt::Key_Down:
        case Qt::Key_Left:
            SnapshotManager::getInstance()->restoreSnapshot();
            _factor = 0.0;
            break;
        case Qt::Key_Up:
        case Qt::Key_Right:
            SnapshotManager::getInstance()->restoreSnapshot(_destinationId);
            _factor = 1.0;
            break;
        default:
            return false;
        }
    }

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

    // Simulate rendering

    // destination source
    if (_destinationSource) {
        glPushMatrix();
        drawSource(_destinationSource, ICON_BORDER_SCALE, GL_SELECT);
        glPopMatrix();
    }
    // departure source
    if (_departureSource) {
        glPushMatrix();
        drawSource(_departureSource, ICON_BORDER_SCALE, GL_SELECT);
        glPopMatrix();
    }

    // slider source
    drawSource(_renderSource, ICON_CURSOR_SCALE, GL_SELECT);

    glPopMatrix();

    // compute picking . return to rendering mode
    hits = glRenderMode(GL_RENDER);

    // set the matrices back
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);

    if (clic) {
        clickedSources.clear();
        while (hits != 0) {
            int id = selectBuf[ (hits-1) * 4 + 3];
            if ( id == _renderSource->getId() )
                clickedSources.insert( _renderSource );
            else if ( id == _destinationSource->getId() )
                clickedSources.insert( _destinationSource );
            else if ( id == _departureSource->getId() )
                clickedSources.insert( _departureSource );
            hits--;
        }

        return sourceClicked();
    }
    else
        return (hits != 0);
}


void SnapshotView::setAction(ActionType a){

    View::setAction(a);

    switch(a) {
    case View::OVER:
        RenderingManager::getRenderingWidget()->setMouseCursor(ViewRenderWidget::MOUSE_HAND_OPEN);
//        RenderingManager::getRenderingWidget()->showMessage("Grab to interpolate.", 1000);
        break;
    case View::GRAB:
        RenderingManager::getRenderingWidget()->setMouseCursor(ViewRenderWidget::MOUSE_HAND_CLOSED);
        break;
    case View::TOOL:
        RenderingManager::getRenderingWidget()->setMouseCursor(ViewRenderWidget::MOUSE_HAND_INDEX);
//        RenderingManager::getRenderingWidget()->showMessage("Clic to animate, scroll to control speed.", 1000);
        break;
    default:
        RenderingManager::getRenderingWidget()->setMouseCursor(ViewRenderWidget::MOUSE_ARROW);
    }
}


bool SnapshotView::setTargetSnapshot(QString id)
{
    // get the target image
    QImage image = SnapshotManager::getInstance()->getSnapshotImage(id);

    // null image means there is not such snapshot: abort
    if (image.isNull())
        return false;

    // remember dest image
    _destination = image;
    if (_destinationSource)
        _destinationSource->setImage(_destination);

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
        double da = fmod( 360.0 + dest[6] - it.key()->getRotationAngle(), 360.0);
        if ( ABS(da) > 180.0 )
            da = -(360.0 - ABS(da));
        snap << qMakePair( dest[6], da );

        // texture coordinates
        QRectF tc =  it.key()->getTextureCoordinates();
        snap << qMakePair( dest[7], dest[7] - tc.x() );
        snap << qMakePair( dest[8], dest[8] - tc.y() );
        snap << qMakePair( dest[9], dest[9] - tc.width() );
        snap << qMakePair( dest[10], dest[10] - tc.height() );

        // layers
        snap << qMakePair( dest[11], dest[11] - it.key()->getDepth() );

        _snapshots[it.key()] = snap;
    }

    return true;
}
