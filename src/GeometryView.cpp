/*
 * GeometryView.cpp
 *
 *  Created on: Jan 31, 2010
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

#include "GeometryView.h"

#include "Tag.h"
#include "RenderingManager.h"
#include "SelectionManager.h"
#include "ViewRenderWidget.h"
#include "OutputRenderWindow.h"
#include "WorkspaceManager.h"
#ifdef GLM_SNAPSHOT
#include "SnapshotManager.h"
#endif

#include <algorithm>

#define MINZOOM 0.1
#define MAXZOOM 3.0
#define DEFAULTZOOM 0.5
#define DEFAULT_PANNING 0.0, 0.0


bool GeometrySelectionArea::contains(SourceSet::iterator s)
{
    SourceList singletonSource;
    singletonSource.insert(*s);
    return area.contains( GeometryView::getBoundingBox(singletonSource) );
}


GeometryView::GeometryView() : View(View::GEOMETRY), quadrant(0), currentSource(0), borderType(0)
{
    zoom = DEFAULTZOOM;
    minzoom = MINZOOM;
    maxzoom = MAXZOOM;
    maxpanx = SOURCE_UNIT*MAXZOOM*2.0;
    maxpany = SOURCE_UNIT*MAXZOOM*2.0;
    currentAction = View::NONE;
    _modeMoveFrame = false;

    icon.load(QString::fromUtf8(":/glmixer/icons/manipulation.png"));
    title = " Geometry";

    _snap = false;
    _snap_distance = 0.0; // DISABLED PROTOTYPE 0.5;
}


void GeometryView::setModelview()
{
    View::setModelview();
    glScaled(zoom, zoom, zoom);
    glTranslatef(getPanningX(), getPanningY(), 0.0);
    glGetDoublev(GL_MODELVIEW_MATRIX, modelview);
}

void GeometryView::paint()
{
    // first the background (as the rendering black clear color) with shadow
    glPushMatrix();
    glScaled( OutputRenderWindow::getInstance()->getAspectRatio(), 1.0, 1.0);
    glCallList(ViewRenderWidget::quad_window[RenderingManager::getInstance()->clearToWhite()?1:0]);
    glPopMatrix();


    // pre render draw (clear and prepare)
    RenderingManager::getInstance()->preRenderToFrameBuffer();

    // set mode for source
    ViewRenderWidget::setSourceDrawingMode(true);

    // loop over the sources (reversed depth order)
    for(SourceSet::iterator  its = RenderingManager::getInstance()->getBegin(); its != RenderingManager::getInstance()->getEnd(); its++) {

        // prevent obvious problem
        Source *s = *its;
        if (!s)
            continue;

        // do not display (and remove from selection) sources in stanby
        if (s->isStandby()) {
            SelectionManager::getInstance()->deselect(s);
            continue;
        }

        //
        // 0. prepare texture
        //

        // bind the source textures
        s->bind();
        s->setShaderAttributes();

        //
        // 1. Draw it into FBO
        //
        RenderingManager::getInstance()->sourceRenderToFrameBuffer(s);

        //
        // 2. Draw it into current view
        //

        // place and scale
        glPushMatrix();
        glTranslated(s->getX(), s->getY(), s->getDepth());
        glRotated(s->getRotationAngle(), 0.0, 0.0, 1.0);
        glScaled(s->getScaleX(), s->getScaleY(), 1.f);

        // Blending Function For mixing like in the rendering window
        s->blend();

        // test workspace
        if ( !WorkspaceManager::getInstance()->isInCurrent(s) ) {
            // Draw source in canvas if not exclusive display
            if ( !WorkspaceManager::getInstance()->isExclusiveDisplay() ) {
                // draw shadow version of the source
                ViewRenderWidget::program->setUniformValue(ViewRenderWidget::_baseAlpha, (GLfloat) s->getAlpha() * WORKSPACE_MAX_ALPHA);
                s->draw();
            }
        }
        else
        // Draw source in canvas
            s->draw();

        // done geometry
        glPopMatrix();

    }

    // Re-Draw frame buffer in the render window
    // With correct rendering on top of the different workspaces
    ViewRenderWidget::resetShaderAttributes(); // switch to drawing mode
    // in exclusive workspace or if rendering is paused, still show the outcome, but faded
    if ( WorkspaceManager::getInstance()->isExclusiveDisplay() || RenderingManager::getInstance()->isPaused())
        ViewRenderWidget::program->setUniformValue(ViewRenderWidget::_baseAlpha, WORKSPACE_MAX_ALPHA);
    // draw
    glPushMatrix();
    glScaled( OutputRenderWindow::getInstance()->getAspectRatio()* SOURCE_UNIT, 1.0* SOURCE_UNIT, 1.0);
    glBindTexture(GL_TEXTURE_2D, RenderingManager::getInstance()->getFrameBufferTexture());
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glCallList(ViewRenderWidget::vertex_array_coords);
    glDrawArrays(GL_QUADS, 0, 4);
    glPopMatrix();

    // unset mode for source
    ViewRenderWidget::setSourceDrawingMode(false);

    // post render draw (loop back and recorder)
    RenderingManager::getInstance()->postRenderToFrameBuffer();

    //
    //  Draw borders
    //
    for(SourceSet::iterator  its = RenderingManager::getInstance()->getBegin(); its != RenderingManager::getInstance()->getEnd(); its++) {

        Source *s = *its;
        if (!s || s->isStandby())
            continue;

        //
        // 3. draw border and handles if active
        //
        GLuint border_workspace = 0;
        int alpha = 200;
        QColor c = Tag::get(s)->getColor();
        if ( !WorkspaceManager::getInstance()->isInCurrent(s) ) {

            if ( WorkspaceManager::getInstance()->isExclusiveDisplay() )
                continue;

            border_workspace = 3;
            alpha = (float) alpha * WORKSPACE_MAX_ALPHA;
            c = c.darker(WORKSPACE_COLOR_SHIFT);
        }

        // place and scale
        glPushMatrix();
        glTranslated(s->getX(), s->getY(), s->getDepth());
        glRotated(s->getRotationAngle(), 0.0, 0.0, 1.0);
        glScaled(s->getScaleX(), s->getScaleY(), 1.0);

        // Tag color
        glColor4ub(c.red(), c.green(), c.blue(), alpha);

        if (RenderingManager::getInstance()->isCurrentSource(s)) {
            // Border according to type and to status of source !!! GL ERROR invalid value
            int b = borderType + border_workspace;
            glCallList(b);
            // Draw extra overlay information depending on tool
            if (currentAction == View::TOOL ) {
                // show that the source has a fixed aspect ratio
                if (s->isFixedAspectRatio() || currentTool == GeometryView::ROTATE ){
                    glCallList(ViewRenderWidget::border_tooloverlay + 1);
                }
                // show the rotation center when ROTATE
                if (currentTool == GeometryView::ROTATE) {
                    glScaled(1.0 / s->getScaleX(), 1.0 / s->getScaleY(), 1.0);
                    glCallList(ViewRenderWidget::border_tooloverlay);
                }
                else if (currentTool == GeometryView::CROP) {
                    glScaled( 1.0 + 0.07 * ( SOURCE_UNIT / s->getScaleX() ),  1.0 + 0.07 * ( SOURCE_UNIT / s->getScaleY() ), 1.0);
                    glCallList(ViewRenderWidget::border_tooloverlay + 2);
                }
            }
            else if ( currentAction == View::GRAB  && currentTool == GeometryView::CROP) {
                glScaled( 1.0 + 0.07 * ( SOURCE_UNIT / s->getScaleX() ),  1.0 + 0.07 * ( SOURCE_UNIT / s->getScaleY() ), 1.0);
                glCallList(ViewRenderWidget::border_tooloverlay + 2);
            }

        } else
            glCallList(ViewRenderWidget::border_thin + border_workspace);

        glPopMatrix();

    }

    // Then the selection outlines
    if ( SelectionManager::getInstance()->hasSelection() ) {
        // Draw center point
        glPushMatrix();
        glTranslated(SelectionManager::getInstance()->selectionSource()->getX(), SelectionManager::getInstance()->selectionSource()->getY(), 0);
        // Draw selection source
        glRotated(SelectionManager::getInstance()->selectionSource()->getRotationAngle(), 0.0, 0.0, 1.0);
        glScaled(SelectionManager::getInstance()->selectionSource()->getScaleX(), SelectionManager::getInstance()->selectionSource()->getScaleY(), 1.f);
        if ( currentSource == SelectionManager::getInstance()->selectionSource() ) {
            glCallList(borderType + 6);
            // Draw extra overlay information depending on tool
            if (currentAction == View::TOOL ) {
                // show that the source has a fixed aspect ratio
                glCallList(ViewRenderWidget::border_tooloverlay + 1);
                // show the rotation center when ROTATE
                if (currentTool == GeometryView::ROTATE) {
                    glScaled(1.0 / SelectionManager::getInstance()->selectionSource()->getScaleX(), 1.0 / SelectionManager::getInstance()->selectionSource()->getScaleY(), 1.0);
                    glCallList(ViewRenderWidget::border_tooloverlay);
                } else if (currentTool == GeometryView::CROP) {
                    glScaled( 1.0 + 0.07 * ( SOURCE_UNIT / SelectionManager::getInstance()->selectionSource()->getScaleX() ),  1.0 + 0.07 * ( SOURCE_UNIT / SelectionManager::getInstance()->selectionSource()->getScaleY() ), 1.0);
                    glCallList(ViewRenderWidget::border_tooloverlay + 2);
                }
            }
        } else
            glCallList(ViewRenderWidget::border_thin + 6);

        glPopMatrix();
    }

    // last the frame thing
    glPushMatrix();
    glScaled( OutputRenderWindow::getInstance()->getAspectRatio(), 1.0, 1.0);
    glCallList(ViewRenderWidget::frame_screen + (_modeMoveFrame ? 1 : 2) );
    glPopMatrix();

    // the source dropping icon
    Source *s = RenderingManager::getInstance()->getSourceBasketTop();
    if ( s ){
        glColor4ub(COLOR_SOURCE, 180);
        double ax, ay, az; // mouse cursor in rendering coordinates:
        gluUnProject( double (mousePos.x()), double (viewport[3] - mousePos.y()), 1.0,
                modelview, projection, viewport, &ax, &ay, &az);
        glPushMatrix();
        glTranslated( ax, ay, az);
            glPushMatrix();
            glTranslated( s->getScaleX() - 1.0, -s->getScaleY() + 1.0, 0.0);
            for (int i = 1; i < RenderingManager::getInstance()->getSourceBasketSize(); ++i ) {
                glTranslated(  2.1, 0.0, 0.0);
                glCallList(ViewRenderWidget::border_thin);
            }
            glPopMatrix();
        glScaled( s->getScaleX(), s->getScaleY(), 1.f);
        glCallList(ViewRenderWidget::border_large);
        glPopMatrix();
    }

    // The rectangle for selection
    _selectionArea.draw();
}


void GeometryView::resize(int w, int h)
{
    View::resize(w, h);
    glViewport(0, 0, viewport[2], viewport[3]);

    // Setup specific projection and view for this window
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();

    if (w > h)
         glOrtho(-SOURCE_UNIT* (double) viewport[2] / (double) viewport[3], SOURCE_UNIT*(double) viewport[2] / (double) viewport[3], -SOURCE_UNIT, SOURCE_UNIT, -MAX_DEPTH_LAYER*MAXZOOM, 1.0);
     else
         glOrtho(-SOURCE_UNIT, SOURCE_UNIT, -SOURCE_UNIT*(double) viewport[3] / (double) viewport[2], SOURCE_UNIT*(double) viewport[3] / (double) viewport[2], -MAX_DEPTH_LAYER*MAXZOOM, 1.0);

    glGetDoublev(GL_PROJECTION_MATRIX, projection);

    // compute largest geometry area for Geometry View (minimum zoom and max panning to both sides)
    double dum;
    double maxmodelview[16];
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();
    glScaled(minzoom, minzoom, minzoom);
    glTranslated(maxpanx, maxpany, 0.0);
    glGetDoublev(GL_MODELVIEW_MATRIX, maxmodelview);
    gluUnProject(0.0, 0.0, 0.0, maxmodelview, projection, viewport, _geometryArea, _geometryArea+1, &dum);
    glTranslated(-2.0*maxpanx, -2.0*maxpany, 0.0);
    glGetDoublev(GL_MODELVIEW_MATRIX, maxmodelview);
    gluUnProject((double)viewport[2], (double)viewport[3], 0.0, maxmodelview, projection, viewport, _geometryArea+2, _geometryArea+3, &dum);
    glPopMatrix();

    // init border type
    borderType = ViewRenderWidget::border_large;
}


bool GeometryView::mousePressEvent(QMouseEvent *event)
{
    if (!event)
        return false;

    lastClicPos = mousePos = event->pos();

    // remember coordinates of clic
    double cursorx = 0.0, cursory = 0.0, dumm = 0.0;
    gluUnProject((double) event->x(), (double) viewport[3] - event->y(), 0.0, modelview, projection, viewport, &cursorx, &cursory, &dumm);
    _selectionArea.markStart(QPointF(cursorx,cursory));

    //  panning
    if (  isUserInput(event, INPUT_NAVIGATE) ||  isUserInput(event, INPUT_DRAG) || _modeMoveFrame) {
        // priority to panning of the view (even in drop mode)
        setAction(View::PANNING);
        return false;
    }

    // DRoP MODE ; explicitly do nothing
    if ( RenderingManager::getInstance()->getSourceBasketTop() ) {
        setAction(View::DROP);
        if (isUserInput(event, INPUT_CONTEXT_MENU))
            RenderingManager::getRenderingWidget()->showContextMenu(ViewRenderWidget::CONTEXT_MENU_DROP, event->pos());
        // don't interpret other mouse events in drop mode
        return false;
    }

    // if at least one source was clicked
    if ( getSourcesAtCoordinates(event->x(), viewport[3] - event->y()) )	{

        // get the top most clicked source (always one as getSourcesAtCoordinates returned true)
        Source *clicked =  *clickedSources.begin();
        // get current source
        Source *cs = getCurrentSource();

        // selection source was clicked ? (part of clickedsource)
        if ( clickedSources.erase(SelectionManager::getInstance()->selectionSource()) > 0) {
            // the selection source was part of the clicked source (now removed)

            // tool use requested:
            // operate on selection
            if (isUserInput(event, INPUT_TOOL))
                clicked = SelectionManager::getInstance()->selectionSource();
            // individual tool or other actions requested:
            // operate on underlying source, if any
            else if (!clickedSources.empty())
                clicked =  *clickedSources.begin();
            // no underlying source, invalid action
            else
                clicked = NULL;
        }
        // current source was clicked ? (part of clickedsource)
        else if ( cs && clickedSources.erase(cs) > 0) {
            // keep it selected
            clicked = cs;
        }

        // set the current active source (can now be selection source or none)
        setCurrentSource(clicked);

        // get actual current source
        cs = getCurrentSource();

        // operate on current source, if available
        if (cs) {

            // context menu
            if ( isUserInput(event, View::INPUT_CONTEXT_MENU) )
                RenderingManager::getRenderingWidget()->showContextMenu(ViewRenderWidget::CONTEXT_MENU_SOURCE, event->pos());
            // zoom
            else if ( isUserInput(event, View::INPUT_ZOOM) )
                zoomBestFit(true);
            // toggle selection of the source
            else if ( isUserInput(event, View::INPUT_SELECT) ) {
                SelectionManager::getInstance()->select(cs);
                setCurrentSource(SelectionManager::getInstance()->selectionSource());
            }
            // other cases : TOOL
            else {

                // disable selection if a source out of the selection is clicked
                if (cs != SelectionManager::getInstance()->selectionSource() &&  !SelectionManager::getInstance()->isInSelection(cs))
                    SelectionManager::getInstance()->clearSelection();

                // manipulate the source
                quadrant = getSourceQuadrant(cs, event->x(), viewport[3] - event->y());
                // the action depends on the quadrant clicked (4 corners).
                if (quadrant == 0 || currentTool == MOVE) {
                    setAction(View::GRAB);
                    borderType = ViewRenderWidget::border_large;
                } else {
                    setAction(View::TOOL);
                    borderType = ViewRenderWidget::border_scale;
                }
            }
        }

        // current source changed in some way
        return true;
    }
    // else = click in background

    // context menu on the background
    if ( isUserInput(event, INPUT_CONTEXT_MENU) ) {
        RenderingManager::getRenderingWidget()->showContextMenu(ViewRenderWidget::CONTEXT_MENU_VIEW, event->pos());
    }
    // zoom button in the background : zoom best fit
    else if ( isUserInput(event, INPUT_ZOOM) ) {
        zoomBestFit(false);
    }
    // reset selection and action
    else {
        // unset current source
        setCurrentSource(NULL);
        // back to no action
        setAction(View::NONE);
    }


    return false;
}

bool GeometryView::mouseMoveEvent(QMouseEvent *event)
{
    if (!event)
        return false;

    int dx = event->x() - mousePos.x();
    int dy = mousePos.y() - event->y();
    mousePos = event->pos();

    // DROP MODE ; show a question mark cursor and avoid other actions
    if ( RenderingManager::getInstance()->getSourceBasketTop() ) {
        setAction(View::DROP);
        // don't interpret mouse events in drop mode
        return false;
    }

    // PANNING of the background
    if ( currentAction == View::PANNING ) {
        // panning background
        panningBy(mousePos.x(), viewport[3] - mousePos.y(), dx, dy);
        // DRAG ?
        if ( isUserInput(event, INPUT_DRAG)  || ( isUserInput(event, INPUT_TOOL) && _modeMoveFrame ) ) {
            // special move ; move the sources in the opposite
            for(SourceSet::iterator  its = RenderingManager::getInstance()->getBegin();
                its != RenderingManager::getInstance()->getEnd(); its++) {
                grabSource( *its, mousePos.x(), viewport[3] - mousePos.y(), -dx, -dy);
            }
            if ( SelectionManager::getInstance()->hasSelection() )
                grabSource( SelectionManager::getInstance()->selectionSource(), mousePos.x(), viewport[3] - mousePos.y(), -dx, -dy);
            // return true as we may have moved the current source
            return true;
        }
        // return false as nothing changed
        return false;
    }

    // get current source
    Source *cs = getCurrentSource();

    if ( cs && (currentAction == View::GRAB || currentAction == View::TOOL)) {

        if (currentAction == View::TOOL) {
            if (currentTool == View::MOVE)
                grabSources(cs, mousePos.x(), viewport[3] - mousePos.y(), dx, dy);
            else if (currentTool == View::SCALE)
                scaleSources(cs, mousePos.x(), viewport[3] - mousePos.y(), dx, dy);
            else if (currentTool == View::CROP)
                cropSources(cs, mousePos.x(), viewport[3] - mousePos.y(), dx, dy);
            else if (currentTool == View::ROTATE) {
                rotateSources(cs, mousePos.x(), viewport[3] - mousePos.y(), dx, dy);
                setTool(currentTool);
            }
        }
        else {
            if (currentTool == View::CROP)
                cropSources(cs, mousePos.x(), viewport[3] - mousePos.y(), dx, dy, true);
            else
                grabSources(cs, mousePos.x(), viewport[3] - mousePos.y(), dx, dy);
        }
        // the current source has been modified
        return true;
    }

    // other cause for action without a current source ; selection area
    if ( !currentSource  && isUserInput(event, INPUT_TOOL) ) {

        // get coordinate of cursor
        double cursorx = 0.0, cursory = 0.0, dumm = 0.0;
        gluUnProject((double) mousePos.x(), (double) viewport[3] - mousePos.y(), 0.0, modelview, projection, viewport, &cursorx, &cursory, &dumm);

        // enable drawing of selection area
        _selectionArea.setEnabled(true);

        // set coordinate of end of rectangle selection
        _selectionArea.markEnd(QPointF(cursorx, cursory));

        // consider only large enough surface of selection
        if (_selectionArea.surface() * zoom * zoom > 0.1 )
        {
            // loop over every sources to check if it is in the rectangle area
            SourceList rectSources;
            for(SourceSet::iterator  its = RenderingManager::getInstance()->getBegin(); its != RenderingManager::getInstance()->getEnd(); its++) {
                if ((*its)->isStandby())
                    continue;
                if (_selectionArea.contains( its ) )
                    rectSources.insert(*its);
            }

            if ( currentAction == View::SELECT )
                // extend selection
                SelectionManager::getInstance()->select(rectSources);
            else  // new selection
                SelectionManager::getInstance()->setSelection(rectSources);

        }

        return false;
    }

    // mouse over only if no user action (not selection)
    if ( isUserInput(event, INPUT_NONE) ) {

        // Mouse over BORDER OF RENDER AREA
        // are we over the border of the frame ?
        _modeMoveFrame = hasObjectAtCoordinates(mousePos.x(), viewport[3] - mousePos.y(), ViewRenderWidget::frame_screen, 5.0);

        // by default, reset quadrant
        quadrant = 0;
        // mouse over which sources ? fill in clickedSources list
        if ( !_modeMoveFrame && getSourcesAtCoordinates(mousePos.x(), viewport[3] - mousePos.y()) )
        {
            // if there is a current source
            // AND
            // if the currently active source is in the set of sources under the cursor,
            // THEN
            // use the current source for quadrant computation
            if ( cs && clickedSources.count( cs ) > 0 )
                quadrant = getSourceQuadrant(cs, mousePos.x(), viewport[3] - mousePos.y());

            // deal with quadrant appropriately
            if (quadrant == 0 || currentTool == MOVE)
                borderType = ViewRenderWidget::border_large;
            else
                borderType = ViewRenderWidget::border_scale;

            // show we are over a source
            setAction(View::OVER);
        }
        else
            // show we are not over a source
            setAction(View::NONE);

    }

    return false;
}

bool GeometryView::mouseReleaseEvent ( QMouseEvent * event )
{
    if (!event)
        return false;

    Source *cs = getCurrentSource();

    // enforces minimal size ;
    // check that the tool action did not go beyond the limits and fix it
    if ( cs && currentAction == View::TOOL) {

        if ( cs == SelectionManager::getInstance()->selectionSource() ) {

            for(SourceList::iterator  its = SelectionManager::getInstance()->selectionBegin(); its != SelectionManager::getInstance()->selectionEnd(); its++) {

                (*its)->clampScale();

            }
            SelectionManager::getInstance()->updateSelectionSource();
        }
        else
            cs->clampScale( currentTool == GeometryView::CROP );

    }

    if ( RenderingManager::getInstance()->getSourceBasketTop() )
        setAction(DROP);
    else if (currentAction == View::PANNING )
        setAction(previousAction);
    else if (cs && currentAction != View::SELECT )
        setAction(View::OVER);

    // end of selection area
    if (_selectionArea.isEnabled()) {
        _selectionArea.setEnabled(false);
        setCurrentSource(SelectionManager::getInstance()->selectionSource());
    }

    // reset list of clicked sources
    clickedSources.clear();

    _snap = false;

    return true;
}

bool GeometryView::wheelEvent ( QWheelEvent * event ){

    bool ret = true;
    // remember position of cursor before zoom
    double bx, by, z;
    gluUnProject((double) event->x(), (double) (viewport[3] - event->y()), 0.0,
            modelview, projection, viewport, &bx, &by, &z);

    setZoom (zoom + ((double) event->delta() * zoom * minzoom) / (View::zoomSpeed() * maxzoom) );

    double ax, ay;
    gluUnProject((double) event->x(), (double) (viewport[3] - event->y()), 0.0,
            modelview, projection, viewport, &ax, &ay, &z);

    if (View::zoomCentered())
        // Center view on cursor when zooming ( panning = panning + ( mouse position after zoom - position before zoom ) )
        // BUT with a non linear correction factor when approaching to MINZOOM (close to 0) which allows
        // to re-center the view on the center when zooming out maximally
        setPanning(( getPanningX() + ax - bx) * 1.0 / ( 1.0 + exp(13.0 - 65.0 * zoom) ), ( getPanningY() + ay - by) * 1.0 / ( 1.0 + exp(13.0 - 65.0 * zoom) ) );

    // keep sources under the cursor if simultaneous grab & zoom
    if (currentAction == View::GRAB || currentAction == View::TOOL ){

        // where is the mouse cursor now (after zoom and panning)?
        gluUnProject((double) event->x(), (double) (viewport[3] - event->y()), 0.0, modelview, projection, viewport, &ax, &ay, &z);
        // this means we have a delta of mouse position
        deltax = ax - bx;
        deltay = ay - by;

        // simulate a movement of the mouse
        QMouseEvent *e = new QMouseEvent(QEvent::MouseMove, mousePos, Qt::NoButton, qtMouseButtons(INPUT_TOOL), qtMouseModifiers(INPUT_TOOL));
        ret = mouseMoveEvent(e);
        delete e;

        // reset delta
        deltax = 0;
        deltay = 0;

    } else
        // do not show action indication (as it is likely to become invalid with view change)
        borderType = ViewRenderWidget::border_large;

    return ret;
}


bool GeometryView::mouseDoubleClickEvent ( QMouseEvent * event )
{
    if (!event)
        return false;

    if (currentAction == View::DROP)
        return false;

    // for double tool clic
    if ( isUserInput(event, INPUT_TOOL) || isUserInput(event, INPUT_TOOL_INDIVIDUAL) ) {
        // get the top most clicked source
        if ( getSourcesAtCoordinates(event->x(), viewport[3] - event->y()) ) {

            // get the top most clicked source
            reverseSourceSet::iterator clicked = clickedSources.begin();

            // if there is no source selected, select the top most
            if ( currentSource == 0 )
                setCurrentSource(*clicked);
            // else, try to take another one bellow it
            else {
                // find where the current source is in the clickedSources
                clicked = clickedSources.find(currentSource) ;
                // decrement the clicked iterator forward in the clickedSources (and jump back to end when at beginning)
                if ( clicked == clickedSources.begin() )
                    clicked = clickedSources.end();
                clicked--;

                // set this newly clicked source as the current one
                setCurrentSource(*clicked);
                // update quadrant to match newly current source
                quadrant = getSourceQuadrant(*clicked, event->x(), viewport[3] - event->y());
            }
            return true;
        }
    }  // zoom
    else if ( isUserInput(event, View::INPUT_ZOOM) ) {
        zoomReset();
        return true;
    }

    return false;
}


bool GeometryView::keyPressEvent ( QKeyEvent * event ){

    // keyboard move source
    Source * cs = getCurrentSource();
    if (cs) {
        int dx =0, dy = 0, factor = 1;
        // ALTERNATE ACTION
        if ( QApplication::keyboardModifiers() & Qt::AltModifier )
            factor *= 10;
        switch (event->key()) {
            case Qt::Key_Left:
                dx = -factor;
                break;
            case Qt::Key_Right:
                dx = factor;
                break;
            case Qt::Key_Down:
                dy = -factor;
                break;
            case Qt::Key_Up:
                dy = factor;
                break;
            default:
                return false;
        }
        grabSources(cs, 0, 0, dx, dy);

        // TODO

        return true;
    }

    return false;
}


void GeometryView::setTool(toolType t)
{
    currentTool = t;

    if (quadrant == 0)
        t = MOVE;

    switch (t) {
    case SCALE:
    case CROP:
        if ( quadrant % 2 )
            RenderingManager::getRenderingWidget()->setMouseCursor(ViewRenderWidget::MOUSE_SCALE_F);
        else
            RenderingManager::getRenderingWidget()->setMouseCursor(ViewRenderWidget::MOUSE_SCALE_B);
        break;
    case ROTATE:
        switch (quadrant) {
        case 1:
            RenderingManager::getRenderingWidget()->setMouseCursor(ViewRenderWidget::MOUSE_ROT_TOP_LEFT);
            break;
        case 2:
            RenderingManager::getRenderingWidget()->setMouseCursor(ViewRenderWidget::MOUSE_ROT_TOP_RIGHT);
            break;
        case 3:
            RenderingManager::getRenderingWidget()->setMouseCursor(ViewRenderWidget::MOUSE_ROT_BOTTOM_RIGHT);
            break;
        default:
        case 4:
            RenderingManager::getRenderingWidget()->setMouseCursor(ViewRenderWidget::MOUSE_ROT_BOTTOM_LEFT);
            break;
        }
        break;
    case MOVE:
        if (currentAction == View::TOOL  || currentAction == View::GRAB)
            RenderingManager::getRenderingWidget()->setMouseCursor(ViewRenderWidget::MOUSE_HAND_CLOSED);
        else
            RenderingManager::getRenderingWidget()->setMouseCursor(ViewRenderWidget::MOUSE_HAND_OPEN);
        break;
    }
}

void GeometryView::setAction(ActionType a){

    View::setAction(a);

    switch(a) {
    case View::GRAB:
        RenderingManager::getRenderingWidget()->setMouseCursor(ViewRenderWidget::MOUSE_HAND_CLOSED);
        break;
    case View::OVER:
    case View::TOOL:
        setTool(currentTool);
        break;
    case View::SELECT:
        RenderingManager::getRenderingWidget()->setMouseCursor(ViewRenderWidget::MOUSE_HAND_INDEX);
        break;
    case View::PANNING:
        RenderingManager::getRenderingWidget()->setMouseCursor(ViewRenderWidget::MOUSE_SIZEALL);
        break;
    case View::DROP:
        RenderingManager::getRenderingWidget()->setMouseCursor(ViewRenderWidget::MOUSE_QUESTION);
        break;
    default:
        RenderingManager::getRenderingWidget()->setMouseCursor(ViewRenderWidget::MOUSE_ARROW);
        break;
    }
}


void GeometryView::zoomReset() {
    setZoom(DEFAULTZOOM);
    setPanning(DEFAULT_PANNING);
}

void GeometryView::zoomBestFit( bool onlyClickedSource ) {

    // nothing to do if there is no source
    if (RenderingManager::getInstance()->empty()){
        zoomReset();
        return;
    }

    // 0. consider either the clicked source, either the full list
    SourceList l;
    if ( onlyClickedSource ) {

        if (currentSource == SelectionManager::getInstance()->selectionSource())
            // list the selection
            l = SelectionManager::getInstance()->copySelection();
        else if ( RenderingManager::getInstance()->getCurrentSource() != RenderingManager::getInstance()->getEnd() )
            // add only the current source in the list
            l.insert(*RenderingManager::getInstance()->getCurrentSource());
        else
            return;

    } else
        // make a list of all the sources
        std::copy( RenderingManager::getInstance()->getBegin(), RenderingManager::getInstance()->getEnd(), std::inserter( l, l.end() ) );

    // 1. compute bounding box of every sources to consider
    QRectF bbox = getBoundingBox(l);

    // 2. Apply the panning to the new center
    setPanning( -bbox.center().x(),  -bbox.center().y()  );

    // 3. get the extend of the area covered in the viewport (the matrices have been updated just above)
    double LLcorner[3];
    double URcorner[3];
    gluUnProject(viewport[0], viewport[1], 1, modelview, projection, viewport, LLcorner, LLcorner+1, LLcorner+2);
    gluUnProject(viewport[2], viewport[3], 1, modelview, projection, viewport, URcorner, URcorner+1, URcorner+2);

    // 4. compute zoom factor to fit to the boundaries
    // initial value = a margin scale of 5%
    double scalex = 0.98 * ABS(URcorner[0]-LLcorner[0]) / ABS(bbox.width());
    double scaley = 0.98 * ABS(URcorner[1]-LLcorner[1]) / ABS(bbox.height());
    // depending on the axis having the largest extend
    // apply the scaling
    setZoom( zoom * (scalex < scaley ? scalex : scaley  ));

}


bool GeometryView::hasObjectAtCoordinates(int mouseX, int mouseY, int objectdisplaylist, double tolerance)
{
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
    gluPickMatrix((double) mouseX, (double) mouseY, tolerance, tolerance, viewport);
    glMultMatrixd(projection);

    // rendering for select mode
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();

    glScaled( OutputRenderWindow::getInstance()->getAspectRatio(), 1.0, 1.0);
    glCallList(objectdisplaylist);

    // compute picking . return to rendering mode
    hits = glRenderMode(GL_RENDER);

    // set the matrices back
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
    glPopMatrix();

    return hits > 0;
}


bool GeometryView::getSourcesAtCoordinates(int mouseX, int mouseY) {

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

    if ( SelectionManager::getInstance()->hasSelection() ) {
        glPushMatrix();
        // place and scale
        glTranslated(SelectionManager::getInstance()->selectionSource()->getX(), SelectionManager::getInstance()->selectionSource()->getY(), 40);
        glRotated(SelectionManager::getInstance()->selectionSource()->getRotationAngle(), 0.0, 0.0, 1.0);
        glScaled(SelectionManager::getInstance()->selectionSource()->getScaleX(), SelectionManager::getInstance()->selectionSource()->getScaleY(), 1.f);
        SelectionManager::getInstance()->selectionSource()->draw(GL_SELECT);
        glPopMatrix();
    }

    for(SourceSet::iterator  its = RenderingManager::getInstance()->getBegin(); its != RenderingManager::getInstance()->getEnd(); its++) {
        if ((*its)->isStandby() || !WorkspaceManager::getInstance()->isInCurrent(*its) )
            continue;
        glPushMatrix();
        // place and scale
        glTranslated((*its)->getX(), (*its)->getY(), (*its)->getDepth());
        glRotated((*its)->getRotationAngle(), 0.0, 0.0, 1.0);
        glScaled((*its)->getScaleX(), (*its)->getScaleY(), 1.f);
        (*its)->draw(GL_SELECT);
        glPopMatrix();
    }

    // compute picking . return to rendering mode
    hits = glRenderMode(GL_RENDER);

//    qDebug ("%d hits @ (%d,%d) vp (%d, %d, %d, %d)", hits, mouseX, mouseY, viewport[0], viewport[1], viewport[2], viewport[3]);

    // set the matrices back
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);


    clickedSources.clear();
    while (hits != 0) {
        GLuint id = selectBuf[ (hits-1) * 4 + 3];
        if ( id == SelectionManager::getInstance()->selectionSource()->getId() )
            clickedSources.insert(SelectionManager::getInstance()->selectionSource());
        else
            clickedSources.insert( *(RenderingManager::getInstance()->getById(id)) );
        hits--;
    }

    return sourceClicked();

}


/**
 *
 **/
void GeometryView::panningBy(int x, int y, int dx, int dy) {

    double bx, by, bz; // before movement
    double ax, ay, az; // after  movement

    gluUnProject((double) (x - dx), (double) (y - dy),
            0.0, modelview, projection, viewport, &bx, &by, &bz);
    gluUnProject((double) x, (double) y, 0.0,
            modelview, projection, viewport, &ax, &ay, &az);

    // apply panning
    setPanning(getPanningX() + ax - bx, getPanningY() + ay - by);
}


/**
 * Grabbing the source
 *
 * move by (dx dy)
 **/
void GeometryView::grabSource(Source *s, int x, int y, int dx, int dy, bool snap) {

    if (!s) return;

    // source position
    double sx = s->getX();
    double sy = s->getY();

    double dum;
    double bx, by; // before movement
    double ax, ay; // after  movement

    gluUnProject((double) (x- dx), (double) (y - dy),
            0.0, modelview, projection, viewport, &bx, &by, &dum);
    gluUnProject((double) x, (double) y, 0.0,
            modelview, projection, viewport, &ax, &ay, &dum);

    // take into account movement of the cursor due to zoom with scroll wheel
    ax += deltax;
    ay += deltay;
    // delta after minus before
    ax -= bx;
    ay -= by;

    // by default, target position is source destination
    double tx =  sx + ax;
    double ty =  sy + ay;

    if (snap) {

        // grab position
        static double gx = 0.0, gy = 0.0;

        // detect beginning of snap
        if (!_snap) {
            // initial pos of the grab movement
            gx = sx;
            gy = sy;
            // done initializing snap ; will be resetted on Mouse Release
            _snap = true;
        }

        // update grab destination with delta
        gx += ax;
        gy += ay;

        // by default, target position is grabbing destination
        tx = gx;
        ty = gy;

        // if snap if possible
        if ( _snap_distance > 0.0 ) {

            // build boxes to be snapped
            QRectF sbox = GeometryView::getBoundingBox(s); // bounding box of source
            QRectF ref = QRectF(-SOURCE_UNIT*OutputRenderWindow::getInstance()->getAspectRatio(),-SOURCE_UNIT, 2.0*SOURCE_UNIT*OutputRenderWindow::getInstance()->getAspectRatio(), 2.0*SOURCE_UNIT); // reference to the geometry frame

            //
            //  HORIZONTAL X AXIS
            //
            // distance to left border
            double d1 = ref.left() - sbox.left();
            // distance to right border
            double d2 = ref.right() - sbox.right();
            // distance to the non-snapped grabbing destination
            double dg = ABS(tx - sx);

            // condition for snap : close to a border, AND not too far from grab destination
            if ( MINI( ABS(d1), ABS(d2)) < _snap_distance && dg < _snap_distance) {
                // decide for the closer
                tx = sx + ( ABS(d1) < ABS(d2) ? d1 : d2 );
            }

            //
            //  VERTICAL Y AXIS
            //
            // distance to top border
            d1 = ref.top() - sbox.top();
            // distance to bottom border
            d2 = ref.bottom() - sbox.bottom();
            // distance to the non-snapped grabbing destination
            dg = ABS(ty - sy);

            // condition for snap : close to a border, AND not too far from grab destination
            if ( MINI( ABS(d1) , ABS(d2)) < _snap_distance && dg < _snap_distance) {
                // decide for the closer
                ty = sy + ( ABS(d1) < ABS(d2) ? d1 : d2 );
            }

        }
    }

    // move source
    s->setPosition( qBound(_geometryArea[0], tx, _geometryArea[2]), qBound(_geometryArea[1], ty, _geometryArea[3]) );
}

void GeometryView::grabSources(Source *s, int x, int y, int dx, int dy) {

    if (!s) return;

    // remember position of source
    double sx = s->getX();
    double sy = s->getY();

    // move the source individually, with snap
    grabSource(s, x, y, dx, dy, true);

    // if the source is the selection, move the selection too
    if ( s == SelectionManager::getInstance()->selectionSource() ) {

        // compute displacement of selection
        sx -= s->getX();
        sy -= s->getY();

        for(SourceList::iterator  its = SelectionManager::getInstance()->selectionBegin(); its != SelectionManager::getInstance()->selectionEnd(); its++){
            // apply displacement to all selected sources
            (*its)->setPosition( (*its)->getX() - sx, (*its)->getY() - sy );
        }
    }
    // otherwise, update selection source if we move a source of the selection
    else if (SelectionManager::getInstance()->isInSelection(s))
        SelectionManager::getInstance()->updateSelectionSource();

}

/**
 * Scaling the source
 *
 * it looks easy, BUT :
 * - i didn't want a scaling from the center, but a scaling which grabs to the opposite corner (which should remain in place)
 * - with rotation, its a bit tricky to adjust the scaling factor
 *
 * This implementation ensures that a point clicked on the source is "grabbed" by the cursor
 * and remains attached to the mouse.
 * This is not garanteed anymore when the 'option' flag is on because then it preserves the
 * aspect ration of the source.
 *
 **/
void GeometryView::scaleSource(Source *s, int X, int Y, int dx, int dy, char quadrant, bool option) {

    if (!s) return;

    double dum;
    double bx, by; // before movement
    double ax, ay; // after  movement

    // get clic coordinates in Geometry view coordinate system
    gluUnProject((double) (X - dx), (double) (Y - dy),
            1.0, modelview, projection, viewport, &bx, &by, &dum);
    gluUnProject((double) X, (double) Y, 1.0,
            modelview, projection, viewport, &ax, &ay, &dum);

    // take into account movement of the cursor due to zoom with scroll wheel
    ax += deltax;
    ay += deltay;

    double w = s->getScaleX();
    double x = s->getX();
    double h = s->getScaleY();
    double y = s->getY();
    double cosa = cos(-s->getRotationAngle() / 180.0 * M_PI);
    double sina = sin(-s->getRotationAngle() / 180.0 * M_PI);

    // convert to vectors ( source center -> clic position)
    ax -= x; ay -= y;
    bx -= x; by -= y;

    // rotate to compute scaling into the source orientation
    dum = ax * cosa - ay * sina;
    ay = ay  * cosa + ax * sina;
    ax = dum;

    dum = bx * cosa - by * sina;
    by = by  * cosa + bx * sina;
    bx = dum;

    // Scaling, according to the quadrant in which we clicked
    double sx = 1.0, sy = 1.0;

    if ( quadrant == 1 || quadrant == 4)   // LEFT
        w = -w;
    sx = (ax + w) / (bx + w);
    ax = w * (sx - 1.0);

    if ( quadrant > 2 )					  // BOTTOM
        h = -h;
    sy = (option || s->isFixedAspectRatio()) ? sx : (ay + h) / (by + h);  // proportional scaling if option is ON
    ay = h * (sy - 1.0);

    // reverse rotation to apply translation shift in the world reference
    cosa = cos(s->getRotationAngle() / 180.0 * M_PI);
    sina = sin(s->getRotationAngle() / 180.0 * M_PI);

    dum = ax * cosa - ay * sina;
    ay = ay  * cosa + ax * sina;
    ax = dum;

    s->scaleBy(sx, sy);
    s->setPosition( qBound(_geometryArea[0], x + ax, _geometryArea[2]), qBound(_geometryArea[1], y + ay, _geometryArea[3]) );
}

void GeometryView::scaleSources(Source *s, int x, int y, int dx, int dy, bool option) {

    if (!s) return;

    // keep the position and scale before modifying the source
    double sxratio = 1.0/ s->getScaleX();
    double syratio = 1.0/ s->getScaleY();
    double sx = s->getX();
    double sy = s->getY();

    // move the source individually (proportional scale if is selection source)
    scaleSource(s, x, y, dx, dy, quadrant, s == SelectionManager::getInstance()->selectionSource());

    // if the source is the View::selection, move the selection
    if ( s == SelectionManager::getInstance()->selectionSource() ) {
        // ratio of scaling now / before
        sxratio *= s->getScaleX();
        syratio *= s->getScaleY();
        // compute displacement of selection
        sx -= s->getX();
        sy -= s->getY();
        // apply scaling to all sources
        for(SourceList::iterator  its = SelectionManager::getInstance()->selectionBegin(); its != SelectionManager::getInstance()->selectionEnd(); its++) {
            (*its)->setPosition( s->getX() + ((*its)->getX() - sx) * sxratio,
                                 s->getY() + ((*its)->getY() - sy) * syratio);
            (*its)->scaleBy(sxratio , syratio);
        }
    }
    // otherwise, update selection source if we move a source of the selection
    else if (SelectionManager::getInstance()->isInSelection(s))
        SelectionManager::getInstance()->updateSelectionSource();

}


/**
 * Rotation of the source
 *
 * Like for scaling, this implementation ensures that a point clicked on the source is "grabbed" by the cursor
 * and remains attached to the mouse.
 * This is not garanteed anymore when the 'option' flag is on because then it preserves the
 * scale of the source.
 *
 **/
void GeometryView::rotateSource(Source *s, int X, int Y, int dx, int dy, bool noscale) {

    if (!s) return;

    double dum;
    double bx, by; // before movement
    double ax, ay; // after  movement

    gluUnProject((double) (X - dx), (double) (Y - dy),
            0.0, modelview, projection, viewport, &bx, &by, &dum);
    gluUnProject((double) X, (double) Y, 0.0,
            modelview, projection, viewport, &ax, &ay, &dum);

    // take into account movement of the cursor due to zoom with scroll wheel
    ax += deltax;
    ay += deltay;

    // convert to vectors ( source center -> clic position)
    double x = s->getX();
    double y = s->getY();
    ax -= x; ay -= y;
    bx -= x; by -= y;

    // scale (center scaling) if option is OFF
    if (!noscale) {
        // compute scaling according to distances change
        dum = sqrt(ax * ax + ay * ay) / sqrt(bx * bx + by * by);
        // Scaling
        s->scaleBy(dum, dum);
    }

    // compute angle between before and after
    ax = atan(ax/ay) * 180.0 / M_PI;
    bx = atan(bx/by) * 180.0 / M_PI;
    // special case of opposing angles around 180
    dum = (bx * ax) > 0 ? bx - ax : SIGN(ax) * (bx + ax);

    // incremental relative rotation
   // s->setRotationAngle( s->getRotationAngle() + dum );

    static double a = 0.0;
    // detect beginning of snap
    if (!_snap) {
        // initial pos of the grab movement
        a = s->getRotationAngle();
        // done initializing snap ; will be resetted on Mouse Release
        _snap = true;
    }

    a += dum;

    double npi = qRound( a / 90.0) ;
//    fprintf(stderr, "angle %f %f \n", a, qAbs( qAbs(a / 90.0) - qAbs(npi) ) );

    s->setRotationAngle( qAbs( qAbs(a / 90.0) - qAbs(npi) ) < 0.02 ? npi * 90 : a );

}


void GeometryView::rotateSources(Source *s, int x, int y, int dx, int dy, bool noscale) {

    if (!s) return;

    // keep the position and scale before modifying the source
    double sxratio = 1.0/ s->getScaleX();
    double syratio = 1.0/ s->getScaleY();
    double angle = s->getRotationAngle();

    // rotate the source individually
    rotateSource(s, x, y, dx, dy, noscale);

    // if the source is the View::selection, move the selection
    if ( s == SelectionManager::getInstance()->selectionSource() ) {

        // ratio of scaling now / before
        sxratio *= s->getScaleX();
        syratio *= s->getScaleY();
        angle -= s->getRotationAngle();
        angle -= (double)( (int) angle / 360 ) * 360.0;

        for(SourceList::iterator  its = SelectionManager::getInstance()->selectionBegin(); its != SelectionManager::getInstance()->selectionEnd(); its++) {

            (*its)->scaleBy(sxratio , syratio);
            (*its)->setRotationAngle( (*its)->getRotationAngle() - angle);

            double cosa = cos(-angle / 180.0 * M_PI);
            double sina = sin(-angle / 180.0 * M_PI);
            double dx = (*its)->getX() - s->getX();
            double dy = (*its)->getY() - s->getY();

            double dum = dx * cosa - dy * sina;
            dy =  dy * cosa + dx * sina ;
            dx = dum;

            (*its)->setPosition( s->getX() + dx * sxratio, s->getY() + dy * syratio );

        }
    }
    // otherwise, update selection source if we move a source of the selection
    else if (SelectionManager::getInstance()->isInSelection(s))
        SelectionManager::getInstance()->updateSelectionSource();

}


/**
 * Crop the source
 *
 * it looks easy, BUT :
 * - I wanted a crop similar to a scaling, with the source geometry changing (scaling)  ; the texture have to be adapted to the
 *  new scale and position on the fly...
 *
 * This implementation performs a scaling of the 'source geometry' (the border) while keeps the texels in place.
 * Cropping OUTSIDE the geometry repeats the texture.
 *
 *
 **/
void GeometryView::cropSource(Source *s, int X, int Y, int dx, int dy, bool option) {

    if (!s) return;

    double dum;
    double bx, by; // before movement
    double ax, ay; // after  movement

    // get clic coordinates in Geometry view coordinate system
    gluUnProject((double) (X - dx), (double) (Y - dy),
            1.0, modelview, projection, viewport, &bx, &by, &dum);
    gluUnProject((double) X, (double) Y, 1.0,
            modelview, projection, viewport, &ax, &ay, &dum);

    // take into account movement of the cursor due to zoom with scroll wheel
    ax += deltax;
    ay += deltay;

    double w = s->getScaleX();
    double x = s->getX();
    double h = s->getScaleY();
    double y = s->getY();
    double cosa = cos(-s->getRotationAngle() / 180.0 * M_PI);
    double sina = sin(-s->getRotationAngle() / 180.0 * M_PI);
    QRectF tex = s->getTextureCoordinates();

    // convert to vectors ( source center -> clic position)
    ax -= x; ay -= y;
    bx -= x; by -= y;

    // rotate to compute scaling into the source orientation
    dum = ax * cosa - ay * sina;
    ay = ay  * cosa + ax * sina;
    ax = dum;

    dum = bx * cosa - by * sina;
    by = by  * cosa + bx * sina;
    bx = dum;

    // option mode is used for only shifting texture coordinates
    if (option) {

        // shift texture coordinates
        double ds = - tex.width() * ( ax  - bx ) / (2.0 * s->getScaleX());
        double dt = - tex.height() * ( by  - ay ) / (2.0 * s->getScaleY());
        tex.adjust(ds, dt, ds, dt);

    }
    // default mode crops and rescales
    else {

        // Scaling, according to the quadrant in which we clicked
        double sx = 1.0, sy = 1.0;

        if ( quadrant == 1 || quadrant == 4)   // LEFT
            w = -w;
        if ( quadrant > 2 )					  // BOTTOM
            h = -h;

        // compute x scaling factor
        sx = (ax + w) / (bx + w);
        // proportional scaling if option is ON or source has fixed aspect ratio,
        // compute y scaling factor otherwise
        if (s->isFixedAspectRatio()) {
            sy = sx;
            ay = sy * (by + h) - h;
        } else
            sy = (ay + h) / (by + h);

        // translate
        double tx = w * (sx - 1.0);
        double ty = h * (sy - 1.0);

        // Crop texture coordinates
        double ds = tex.width() * ( ax + tx - bx * sx) / (2.0 * s->getScaleX());
        double dt = tex.height() * ( by * sy - ay - ty) / (2.0 * s->getScaleY());

        // depending on the quadrant,
        // it is not the same corner of texture coordinates to change
        if ( quadrant == 1 ) {
            tex.setLeft( tex.left() +  ds );
            tex.setTop( tex.top() +  dt );
        } else if ( quadrant == 2 ) {
            tex.setRight( tex.right() +  ds );
            tex.setTop( tex.top() +  dt );
        } else if ( quadrant == 3 ) {
            tex.setRight( tex.right() + ds );
            tex.setBottom( tex.bottom() + dt );
        } else {
            tex.setLeft( tex.left() + ds );
            tex.setBottom( tex.bottom() + dt );
        }

        // reverse rotation to apply translation shift in the world reference
        cosa = cos(s->getRotationAngle() / 180.0 * M_PI);
        sina = sin(s->getRotationAngle() / 180.0 * M_PI);

        dum = tx * cosa - ty * sina;
        ty = ty  * cosa + tx * sina;
        tx = dum;

        // Apply changes in scale and translation
        s->scaleBy(sx, sy);
        s->setPosition(x + tx, y + ty);

    }

    // Apply the change of texture coordinates
    s->setTextureCoordinates(tex);

}


void GeometryView::cropSources(Source *s, int x, int y, int dx, int dy, bool option) {

    if (!s) return;

    // crop the source individually
    cropSource(s, x, y, dx, dy, option);

    // if the source is the View::selection, move the selection
    if ( s == SelectionManager::getInstance()->selectionSource() ) {
        for(SourceList::iterator  its = SelectionManager::getInstance()->selectionBegin(); its != SelectionManager::getInstance()->selectionEnd(); its++) {

            // TODO implement cropping of selection

        }
//        SelectionManager::getInstance()->updateSelectionSource();
    }
    // otherwise, update selection source if we move a source of the selection
    else if (SelectionManager::getInstance()->isInSelection(s))
        SelectionManager::getInstance()->updateSelectionSource();

}

/**
 *
 **/
char GeometryView::getSourceQuadrant(Source *s, int X, int Y) {

    if (!s) return 0;

    //      ax
    //      ^
    //  ----|----
    //  | 1 | 2 |
    //  ----+---- > ay
    //  | 4 | 3 |
    //  ---------
    char quadrant = 0;
    double ax, ay, az;

    gluUnProject((double) X, (double) Y, 0.0,
            modelview, projection, viewport, &ax, &ay, &az);

    // vector (source center -> cursor position)
    ax -= s->getX();
    ay -= s->getY();
    // quadrant is relative to source orientation
    double cosa = cos(-s->getRotationAngle() / 180.0 * M_PI);
    double sina = sin(-s->getRotationAngle() / 180.0 * M_PI);
    double x = ax * cosa - ay * sina;
    double y = ay * cosa + ax * sina;

    double w = s->getScaleX();
    double h = s->getScaleY();

    // exclude mouse cursors out of the area
    if ( ABS(x) > ABS(w)  || ABS(y) > ABS(h)) {
        return 0;
    }

    // compute the quadrant code : this is tricky as scales can be negative !
    if (( x > BORDER_SIZE * ABS(w) ) && ( y < -BORDER_SIZE * ABS(h) ) ) // RIGHT BOTTOM
        quadrant = h > 0 ? (w > 0 ? (3) : (4)) : (w > 0 ? (2) : (1));
    else if  (( x > BORDER_SIZE * ABS(w)) && ( y > BORDER_SIZE * ABS(h) ) ) // RIGHT TOP
        quadrant = h > 0 ? (w > 0 ? (2) : (1)) : (w > 0 ? (3) : (4));
    else if  (( x < -BORDER_SIZE * ABS(w)) && ( y < -BORDER_SIZE * ABS(h) ) ) // LEFT BOTTOM
        quadrant = h > 0 ? (w > 0 ? (4) : (3)) : (w > 0 ? (1) : (2));
    else if  (( x < -BORDER_SIZE * ABS(w)) && ( y > BORDER_SIZE * ABS(h) ) ) // LEFT TOP
        quadrant = h > 0 ? (w > 0 ? (1) : (2)) : (w > 0 ? (4) : (3));

    return quadrant;
}



void GeometryView::setCurrentSource(Source *s)
{
    currentSource = s;

    // is the current set to a valid pointer ?
    if (currentSource && currentSource != SelectionManager::getInstance()->selectionSource())
        // set the current source of the rendering manager too
        RenderingManager::getInstance()->setCurrentSource( s->getId() );
    else
        // set current of rendering manager to none (end of list)
        RenderingManager::getInstance()->unsetCurrentSource();

}


Source *GeometryView::getCurrentSource()
{
    // first is the current source of the rendering manager valid ?
    if (RenderingManager::getInstance()->isValid(RenderingManager::getInstance()->getCurrentSource()))
        // if yes, set it as current (it may has been selected in another view)
        currentSource = *RenderingManager::getInstance()->getCurrentSource();

    // if current source is a valid source
    if (currentSource) {
        return currentSource;
    }

    return 0;
}

QRectF GeometryView::getBoundingBox(const Source *s, bool invert_y)
{
    double cosa, sina, bbox[2][2];
    double point[2];

    // init bbox to max size
    // bottom left (initialized to extreme top right)
    bbox[0][0] = SOURCE_UNIT * MAXZOOM*2.0;
    bbox[0][1] = SOURCE_UNIT * MAXZOOM*2.0;
    // top right (initialized to extreme bottom left)
    bbox[1][0] = -SOURCE_UNIT * MAXZOOM*2.0;
    bbox[1][1] = -SOURCE_UNIT * MAXZOOM*2.0;

    // compute Axis aligned bounding box
    cosa = cos(s->getRotationAngle() / 180.0 * M_PI);
    sina = sin(s->getRotationAngle() / 180.0 * M_PI);
    for (double i = -1.0; i < 2.0; i += 2.0)
        for (double j = -1.0; j < 2.0; j += 2.0) {
            // corner with apply rotation
            point[0] = i * s->getScaleX() * cosa - j * s->getScaleY() * sina + s->getX();
            point[1] = j * s->getScaleY() * cosa + i * s->getScaleX() * sina + s->getY();
            // keep max and min
            bbox[0][0] = qMin( point[0], bbox[0][0]);
            bbox[0][1] = qMin( point[1], bbox[0][1]);
            bbox[1][0] = qMax( point[0], bbox[1][0]);
            bbox[1][1] = qMax( point[1], bbox[1][1]);
        }

//	qDebug("%s  w= %.2f   bbw = %.2f", qPrintable(s->getName()), s->getScaleX()*2.0, QRectF(QPointF(bbox[0][0], -bbox[1][1]), QPointF(bbox[1][0], -bbox[0][1])).normalized().width());

    if (invert_y)
        // return top-left ; bottom-right (Qt compatible orientation)
        return QRectF(QPointF(bbox[0][0], -bbox[1][1]), QPointF(bbox[1][0], -bbox[0][1])).normalized();
    else
        // return bottom-left ; top-right (OpenGL compatible orientation)
        return QRectF(QPointF(bbox[0][0], bbox[0][1]), QPointF(bbox[1][0], bbox[1][1]));
}

QRectF GeometryView::getBoundingBox(const SourceList &l, bool invert_y)
{
    double cosa, sina, bbox[2][2];
    double point[2];

    // init bbox to max size
    bbox[0][0] = SOURCE_UNIT * MAXZOOM*2.0;
    bbox[0][1] = SOURCE_UNIT * MAXZOOM*2.0;
    bbox[1][0] = -SOURCE_UNIT * MAXZOOM*2.0;
    bbox[1][1] = -SOURCE_UNIT * MAXZOOM*2.0;

    // compute Axis aligned bounding box of all sources in the list
    for(SourceList::const_iterator  its = l.begin(); its != l.end(); its++) {
        cosa = cos((*its)->getRotationAngle() / 180.0 * M_PI);
        sina = sin((*its)->getRotationAngle() / 180.0 * M_PI);
        for (double i = -1.0; i < 2.0; i += 2.0)
            for (double j = -1.0; j < 2.0; j += 2.0) {
                // corner with apply rotation
                point[0] = i * (*its)->getScaleX() * cosa - j * (*its)->getScaleY() * sina + (*its)->getX();
                point[1] = j * (*its)->getScaleY() * cosa + i * (*its)->getScaleX() * sina + (*its)->getY();
                // keep max and min
                bbox[0][0] = qMin( point[0], bbox[0][0]);
                bbox[0][1] = qMin( point[1], bbox[0][1]);
                bbox[1][0] = qMax( point[0], bbox[1][0]);
                bbox[1][1] = qMax( point[1], bbox[1][1]);
            }
    }

    if (invert_y)
        // return top-left ; bottom-right (Qt compatible orientation)
        return QRectF(QPointF(bbox[0][0], -bbox[1][1]), QPointF(bbox[1][0], -bbox[0][1])).normalized();
    else
        // return bottom-left ; top-right (OpenGL compatible orientation)
        return QRectF(QPointF(bbox[0][0], bbox[0][1]), QPointF(bbox[1][0], bbox[1][1]));
}

void alignSource(Source *s, QRectF box, View::Axis a, View::RelativePoint p)
{
    QPointF delta;
    QRectF sbox = GeometryView::getBoundingBox(s,true);

    switch (p) {
    case View::ALIGN_BOTTOM_LEFT:
        delta = box.bottomLeft() - sbox.bottomLeft();
        break;
    case View::ALIGN_EQUAL_GAPS:
    case View::ALIGN_TOP_RIGHT:
        delta = box.topRight() - sbox.topRight();
        break;
    // View::CENTER (or View::GAPS)
    default:
        delta = box.center() - sbox.center();
        break;
    }

    // if the source is the selection, move the selection
    if ( s == SelectionManager::getInstance()->selectionSource() ) {

        for(SourceList::iterator  its = SelectionManager::getInstance()->selectionBegin(); its != SelectionManager::getInstance()->selectionEnd(); its++){
            // move horizontally or vertically
            if (a==View::AXIS_HORIZONTAL)
                (*its)->setX( (*its)->getX() + delta.x() );
            else // View::VERTICAL (inverted y)
                (*its)->setY( (*its)->getY() - delta.y() );
        }

    }
    // default case : move the source
    else {
        // move horizontally or vertically
        if (a==View::AXIS_HORIZONTAL)
            s->setX( s->getX() + delta.x() );
        else // View::VERTICAL (inverted y)
            s->setY( s->getY() - delta.y() );
    }

}

void GeometryView::alignSelection(View::Axis a, View::RelativePoint p, View::Reference r)
{
    // move all the sources relative to each others
    if (r == View::REFERENCE_SOURCES) {
        // reference is the bounding box
        QRectF ref = GeometryView::getBoundingBox(SelectionManager::getInstance()->copySelection(), true);
        // loop over sources of the selection
        for(SourceList::iterator  its = SelectionManager::getInstance()->selectionBegin(); its != SelectionManager::getInstance()->selectionEnd(); its++){
            // perform the computations
            alignSource(*its, ref, a, p);
        }
    }
    // move the selection relative to the frame (View::REFERENCE_FRAME)
    else {
        // reference is the frame
        QRectF ref = QRectF(-SOURCE_UNIT*OutputRenderWindow::getInstance()->getAspectRatio(),-SOURCE_UNIT, 2.0*SOURCE_UNIT*OutputRenderWindow::getInstance()->getAspectRatio(), 2.0*SOURCE_UNIT);
        // perform the computations
        alignSource(SelectionManager::getInstance()->selectionSource(), ref, a, p);
    }
}


void resizeSource(Source *s, QRectF ref, View::Axis a)
{
    // if the source is the selection, resize the selection to match the reference (frame)
    if ( s == SelectionManager::getInstance()->selectionSource() ) {

        // get bounding box of the selection (to be scaled to match size of the ref)
        QRectF selectionbox = GeometryView::getBoundingBox(SelectionManager::getInstance()->copySelection());

        // compute rescaling and displacement of every source in the selection
        for(SourceList::iterator  its = SelectionManager::getInstance()->selectionBegin(); its != SelectionManager::getInstance()->selectionEnd(); its++){

            double scale = 1.0;
            // remember aspect ration and sign of the original source
            double ar = ABS( (*its)->getScaleX() / (*its)->getScaleY());
            double sign_x = SIGN( (*its)->getScaleX() );
            double sign_y = SIGN( (*its)->getScaleY() );

            // resize the diagonal (pythagore) proportionnally
            double K = 4.0 * (*its)->getScaleX() * (*its)->getScaleX() +  4.0 * (*its)->getScaleY()  *  (*its)->getScaleY();

            // Compute scaling to target the scaling of the WIDTH
            if (a == View::AXIS_HORIZONTAL) {
                scale = ref.width() / selectionbox.width();
                K *= scale * scale;
                // appy aspect ratio of original source
                K /= 1.0 + 1.0 / ( ar * ar );
                K = sqrt(K) / 2.0;
                (*its)->setScaleX( sign_x * K );
                // keep aspect ratio
                (*its)->setScaleY( sign_y * K / ar);
                // solves the rescaling of the bounding box to find the new position
                QRectF sbox = GeometryView::getBoundingBox(*its);
                (*its)->setPosition( selectionbox.center().x() + scale * ( sbox.center().x() - selectionbox.center().x()),
                                    selectionbox.center().y() + scale * ( sbox.center().y() - selectionbox.center().y()) );
            }
            // Compute scaling to target the scaling of the HEIGHT
            else if (a == View::AXIS_VERTICAL) {
                scale = ref.height() / selectionbox.height();
                K *= scale * scale;
                // appy aspect ratio of original source
                K /= 1.0 + ar * ar ;
                K = sqrt(K) / 2.0;
                (*its)->setScaleY( sign_y * K );
                // keep aspect ratio
                (*its)->setScaleX( sign_x * K * ar);
                // solves the rescaling of the bounding box to find the new position
                QRectF sbox = GeometryView::getBoundingBox(*its);
                (*its)->setPosition( selectionbox.center().x() + scale * ( sbox.center().x() - selectionbox.center().x()),
                                    selectionbox.center().y() + scale * ( sbox.center().y() - selectionbox.center().y()) );
            }
            // Rescale all source to the aspect ratio of the FRAME (View::AXIS_BOTH)
            else {

                scale = (*its)->getScaleX();
                (*its)->setScaleY( sign_y * qAbs(scale) * ref.height() / ref.width() );
            }

        }

    }
    // default case : resize the source to match the size of the ref
    else {
        double scale = 1.0;

        // remember aspect ration of the original source
        double ar = ABS( s->getScaleX() /  s->getScaleY() );
        double sign_x = SIGN( s->getScaleX() );
        double sign_y = SIGN( s->getScaleY() );

        // resize the diagonal (pythagore) proportionnally
        double K = 4.0 * s->getScaleX() * s->getScaleX() +  4.0 * s->getScaleY()  *  s->getScaleY();

        // Compute scaling to target the scaling of the WIDTH
        if (a == View::AXIS_HORIZONTAL) {
            scale = ref.width() / GeometryView::getBoundingBox(s).width();
            K *= scale * scale;
            // appy aspect ratio of original source
            K /= 1.0 + 1.0 / ( ar * ar );
            K = sqrt(K) / 2.0;
            s->setScaleX( sign_x * K );
            // keep aspect ratio
            s->setScaleY( sign_y * K / ar);
        }
        // Compute scaling to target the scaling of the HEIGHT
        else if (a == View::AXIS_VERTICAL) {
            scale = ref.height() / GeometryView::getBoundingBox(s).height();
            K *= scale * scale;
            // appy aspect ratio of original source
            K /= 1.0 + ar * ar ;
            K = sqrt(K) / 2.0;
            s->setScaleY( sign_y * K );
            // keep aspect ratio
            s->setScaleX( sign_x * K * ar);
        }
        // Rescale all source to their original aspect ratio (View::AXIS_BOTH)
        else {
            scale = s->getScaleX();
            s->setScaleY( sign_y * qAbs(scale) / s->getAspectRatio() );
        }

    }
}


void rotate90Source(Source *s, QRectF box, View::Axis a)
{
    // if the source is the selection, rotate the selection in reference to the frame
    if ( s == SelectionManager::getInstance()->selectionSource() ) {
        // this means reset the rotation of all sources selected
        for(SourceList::iterator  its = SelectionManager::getInstance()->selectionBegin(); its != SelectionManager::getInstance()->selectionEnd(); its++){
            // reset angle
            (*its)->setRotationAngle( 0.0 );
        }
    }
    // default case : rotate the sources in the selection
    else {
        // rotate
        if (a==View::AXIS_HORIZONTAL) {
            // rotate CLOCKWISE
            s->setRotationAngle( s->getRotationAngle() - 90.0);
            // rotate 90 is inverting x and y
            s->setPosition( s->getY() - box.center().y() + box.center().x(), - s->getX() + box.center().x() + box.center().y() );
        } else {
            // rotate COUNTER CLOCKWISE
            s->setRotationAngle( s->getRotationAngle() + 90.0);
            // rotate 90 is inverting x and y
            s->setPosition( - s->getY() + box.center().y() + box.center().x(), s->getX() - box.center().x() + box.center().y() );

        }
    }
}

void flipSource(Source *s, QRectF box, View::Axis a)
{
    // if the source is the selection, flip the selection in reference to the frame
    if ( s == SelectionManager::getInstance()->selectionSource() ) {
        // this means reset the flipping of all sources selected
        for(SourceList::iterator  its = SelectionManager::getInstance()->selectionBegin(); its != SelectionManager::getInstance()->selectionEnd(); its++){
            // reset sign of scales
            (*its)->setScaleX( qAbs( (*its)->getScaleX()) );
            (*its)->setScaleY( qAbs( (*its)->getScaleY()) );
        }
    }
    // default case : flip the sources in the selection
    else {
        // flip
        if (a==View::AXIS_HORIZONTAL) {
            s->setRotationAngle(-s->getRotationAngle());
            s->setX( box.center().x() + ( box.center().x() - s->getX()) );
            s->setScaleX( -s->getScaleX() );
        } else { // View::VERTICAL
            s->setRotationAngle(-s->getRotationAngle());
            s->setY( box.center().y() + ( box.center().y() - s->getY()));
            s->setScaleY( -s->getScaleY() );
        }

    }
}

void rescaleSource(Source *s, QRectF ref, View::Axis a)
{
    // set aspect ratio of all sources selected
    for(SourceList::iterator  its = SelectionManager::getInstance()->selectionBegin(); its != SelectionManager::getInstance()->selectionEnd(); its++){

        double sign_x = SIGN( (*its)->getScaleX() );
        double sign_y = SIGN( (*its)->getScaleY() );

        // set aspect ratio to match rendering resolution
        if (a == View::AXIS_BOTH) {
            double U = MINI( ref.width(), ref.height() );
            (*its)->setScaleX( sign_x * SOURCE_UNIT * (double) (*its)->getFrameWidth() / U );
            (*its)->setScaleY( sign_y * SOURCE_UNIT * (double) (*its)->getFrameHeight() / U );
        }
        // set widh and height so that aspect ratio is 1.0
        else {
            double U = MAXI( ABS((*its)->getScaleX()), ABS((*its)->getScaleY()) );
            (*its)->setScaleX( sign_x * U );
            (*its)->setScaleY( sign_y * U );
        }
    }

}

void GeometryView::transformSelection(View::Transformation t, View::Axis a, View::Reference r)
{
    // transform all the sources in reference to the selection
    if (r == View::REFERENCE_SOURCES) {

        // reference is the selection boundingbox
        QRectF ref = GeometryView::getBoundingBox(SelectionManager::getInstance()->selectionSource());

        // different logic for scaling:
        if (t == View::TRANSFORM_SCALE ) {
            // reference is the larger dimension of the sources in the selection, not the selection box itself
            ref.setSize(QSizeF(0.0, 0.0));
            for(SourceList::iterator  its = SelectionManager::getInstance()->selectionBegin(); its != SelectionManager::getInstance()->selectionEnd(); its++){
                QRectF sbox = GeometryView::getBoundingBox(*its);
                ref.setSize( QSizeF( qMax( ref.width(), sbox.width()), qMax(ref.height(), sbox.height()) ));
            }
        }

        // perform the computations
        for(SourceList::iterator  its = SelectionManager::getInstance()->selectionBegin(); its != SelectionManager::getInstance()->selectionEnd(); its++){
            // perform the computations
            if (t == View::TRANSFORM_ROTATE )
                // rotate by 90 the source into the selection ref
                rotate90Source(*its, ref, a);
            else if (t == View::TRANSFORM_FLIP )
                // flip the source into the selection ref
                flipSource(*its, ref, a);
            else if (t == View::TRANSFORM_SCALE )
                // scale the source into the selection ref
                resizeSource(*its, ref, a);
        }
    }
    // transform the selection in reference to the frame (View::REFERENCE_FRAME)
    else if (r == View::REFERENCE_FRAME) {
        // reference is the frame
        QRectF ref = QRectF(-SOURCE_UNIT*OutputRenderWindow::getInstance()->getAspectRatio(),-SOURCE_UNIT, 2.0*SOURCE_UNIT*OutputRenderWindow::getInstance()->getAspectRatio(), 2.0*SOURCE_UNIT);
        // perform the computations
        if (t == View::TRANSFORM_SCALE )
            resizeSource(SelectionManager::getInstance()->selectionSource(), ref, a);
        else if (t == View::TRANSFORM_FLIP )
            // reset the flipping of the sources
            flipSource(SelectionManager::getInstance()->selectionSource(), ref, a);
        else if (t == View::TRANSFORM_ROTATE )
            // reset the rotation of the sources
            rotate90Source(SelectionManager::getInstance()->selectionSource(), ref, a);
    }
    // transform the selection in reference to the resolution of the rendering (View::REFERENCE_PIXEL)
    else {
        // reference is the pixel resolution
        QRectF ref = QRectF(0.0, 0.0,
                            (double) RenderingManager::getInstance()->getFrameBufferWidth(),
                            (double) RenderingManager::getInstance()->getFrameBufferHeight());

        if (t == View::TRANSFORM_SCALE )
            rescaleSource(SelectionManager::getInstance()->selectionSource(), ref, a);

    }

}

void GeometryView::distributeSelection(View::Axis a, View::RelativePoint p){

    // get selection and discard useless operation
    SourceList selection = SelectionManager::getInstance()->copySelection();
    if (selection.size() < 2)
        return;

    QSizeF total(0.0, 0.0);
    // sort the sources according to the requested relative point
    // and keep a pair ( pointer to source, size to consider )
    QMap< int, QPair<Source*, QSizeF> > sortedlist;
    // do this for horizontal borders
    if (a==View::AXIS_HORIZONTAL) {
        for(SourceList::iterator i = SelectionManager::getInstance()->selectionBegin(); i != SelectionManager::getInstance()->selectionEnd(); i++){
            QRectF sbox = getBoundingBox(*i, true);
            switch (p) {
            case View::ALIGN_BOTTOM_LEFT:
                // sort the list of sources by increasing x of left border (left to right)
                sortedlist[int(sbox.left()*1000)] = qMakePair(*i, sbox.size());
                break;
            case View::ALIGN_TOP_RIGHT:
                // sort the list of sources by decreasing x of right border (right to left)
                sortedlist[int(-sbox.right()*1000)] = qMakePair(*i, sbox.size());
                break;
            // View::CENTER (or View::GAPS)
            default:
                // sort the list of sources by increasing x of center (left to right)
                sortedlist[int(sbox.center().x()*1000)] = qMakePair(*i, sbox.size() / 2.0);
                break;
            }
            total += sbox.size();
        }
    }
    // do this for the vertical borders
    else {
        // sort the list of sources by  y (inverted)
        for(SourceList::iterator i = SelectionManager::getInstance()->selectionBegin(); i != SelectionManager::getInstance()->selectionEnd(); i++){
            QRectF sbox = getBoundingBox(*i, true);
            switch (p) {
            case View::ALIGN_BOTTOM_LEFT:
                sortedlist[int(-sbox.bottom()*1000)] = qMakePair(*i, sbox.size());
                break;
            case View::ALIGN_TOP_RIGHT:
                sortedlist[int(sbox.top()*1000)] = qMakePair(*i, sbox.size());
                break;
            // View::CENTER (or View::GAPS)
            default:
                sortedlist[int(-sbox.center().y()*1000)] = qMakePair(*i, sbox.size() / 2.0);
                break;
            }
            total += sbox.size();
        }
    }

    // compute bbox of the two sources bounding the selection
    // (avoid to take intermediate sources which may enlarge the bbox without being a border)
    selection.clear();
    selection.insert(sortedlist[sortedlist.keys().first()].first);
    selection.insert(sortedlist[sortedlist.keys().last()].first);
    QRectF targetbox = getBoundingBox(selection, true);

    // compute the step of translation
    QSizeF translation;
    if (p == View::ALIGN_CENTER) {
        // special case for centered
        QSizeF s = sortedlist[sortedlist.keys().first()].second;
        translation = (targetbox.size() - s - sortedlist[sortedlist.keys().last()].second ) / double(sortedlist.count()-1);
        targetbox = QRectF(targetbox.bottomLeft() + QPointF(s.width(), -s.height()), QSizeF(0.0,0.0));

    } else if (p == View::ALIGN_EQUAL_GAPS) {
        // special case for GAPS
        translation = (targetbox.size() - total) / double(sortedlist.count()-1);
        QSizeF s = sortedlist[sortedlist.keys().first()].second * 2.0;
        targetbox = QRectF(targetbox.bottomLeft() + QPointF(s.width(), -s.height()), QSizeF(0.0,0.0));
    }
    else
        translation = (targetbox.size() - sortedlist[sortedlist.keys().last()].second ) / double(sortedlist.count()-1);

    // loop over source list, except bottom-left & top-right most
    sortedlist.remove( sortedlist.keys().first() );
    sortedlist.remove( sortedlist.keys().last() );
    QMapIterator< int, QPair<Source*, QSizeF> > its(sortedlist);
    while (its.hasNext()) {
        its.next();

        // Translate bbox to the next position to be aligned with
        switch (p) {
        case View::ALIGN_TOP_RIGHT:
            targetbox.translate(-translation.width(), translation.height());
            break;
        case View::ALIGN_EQUAL_GAPS:
            targetbox.translate(its.value().second.width()*2.0, -its.value().second.height()*2.0);
        case View::ALIGN_CENTER:
        case View::ALIGN_BOTTOM_LEFT:
            targetbox.translate(translation.width(), -translation.height());
            break;
        default:
            break;
        }

        // align source to the border of the target box
        alignSource(its.value().first, targetbox, a, p);
    }
}

#ifdef GLM_SNAPSHOT

void GeometryView::applyTargetSnapshot(double percent, QMap<Source *, QVector< QPair<double,double> > > config)
{
    // linear interpolation to dest by percent of delta
    double a = 1.0 - qBound(0.0, percent, 1.0);
    a = a < EPSILON ? 0.0 : a;

    // loop over all source alpha coordinates
    QMapIterator<Source *, QVector< QPair<double,double> > > it(config);
    while (it.hasNext()) {
        it.next();
        // if in exclusive workspace mode, do not apply changes to sources in other workspaces
        if ( WorkspaceManager::getInstance()->isExclusiveDisplay() ) {
            if ( !WorkspaceManager::getInstance()->isInCurrent(it.key()) )
                continue;
        }
        // interpolate change for this source
        double x = it.value()[2].first - a * it.value()[2].second;
        double y = it.value()[3].first - a * it.value()[3].second;
        double sx = it.value()[4].first - a * it.value()[4].second;
        double sy = it.value()[5].first - a * it.value()[5].second;
        double an = it.value()[6].first - a * it.value()[6].second;
        it.key()->_setGeometry(x, y, sx, sy, 0.0, 0.0, an);

        QRectF tc;
        tc.setX( it.value()[7].first - a * it.value()[7].second );
        tc.setY( it.value()[8].first - a * it.value()[8].second );
        tc.setWidth( it.value()[9].first - a * it.value()[9].second );
        tc.setHeight( it.value()[10].first - a * it.value()[10].second );
        it.key()->_setTextureCoordinates(tc);

    }
    // selection might have changed
    SelectionManager::getInstance()->updateSelectionSource();

}


bool GeometryView::usableTargetSnapshot(QMap<Source *, QVector< QPair<double,double> > > config)
{
    QMapIterator<Source *,  QVector< QPair<double,double> > > it(config);
    while (it.hasNext()) {
        it.next();
        // ignore sources in standby
        if ( it.key()->isStandby())
            continue;
        // if in exclusive workspace mode, do not apply changes to sources in other workspaces
        if ( WorkspaceManager::getInstance()->isExclusiveDisplay() ) {
            if ( !WorkspaceManager::getInstance()->isInCurrent(it.key()) )
                continue;
        }
        // return true whenever a source can be modified
        if ( qAbs(it.value()[2].second) > EPSILON ||
             qAbs(it.value()[3].second) > EPSILON ||
             qAbs(it.value()[4].second) > EPSILON ||
             qAbs(it.value()[5].second) > EPSILON ||
             qAbs(it.value()[6].second) > EPSILON ||
             qAbs(it.value()[7].second) > EPSILON ||
             qAbs(it.value()[8].second) > EPSILON ||
             qAbs(it.value()[9].second) > EPSILON ||
             qAbs(it.value()[10].second) > EPSILON )
            return true;
    }
    return false;
}


#endif
