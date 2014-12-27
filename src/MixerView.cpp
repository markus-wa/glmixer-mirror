/*
 * MixerView.cpp
 *
 *  Created on: Nov 9, 2009
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

#include "MixerView.h"

#include "common.h"
#include "RenderingManager.h"
#include "SelectionManager.h"
#include "ViewRenderWidget.h"
#include "CatalogView.h"

#define MINZOOM 0.04
#define MAXZOOM 1.0
#define DEFAULTZOOM 0.075
#define DEFAULT_PANNING 0.f, 0.f


bool MixerSelectionArea::contains(SourceSet::iterator s)
{
    return area.contains(QPointF((*s)->getAlphaX(),(*s)->getAlphaY()));
}

MixerView::MixerView() : View()
{
    currentAction = View::NONE;
    zoom = DEFAULTZOOM;
    minzoom = MINZOOM;
    maxzoom = MAXZOOM;
    maxpanx = 2.0*SOURCE_UNIT*MAXZOOM*CIRCLE_SIZE;
    maxpany = 2.0*SOURCE_UNIT*MAXZOOM*CIRCLE_SIZE;
    limboSize = DEFAULT_LIMBO_SIZE;
    _modeScaleLimbo = false;
    _modeMoveCircle = false;

    icon.load(QString::fromUtf8(":/glmixer/icons/mixer.png"));
    title = " Mixing view";
}

void MixerView::setModelview()
{
    View::setModelview();
    glScaled(zoom, zoom, zoom);
    glTranslated(getPanningX(), getPanningY(), 0.0);
    glGetDoublev(GL_MODELVIEW_MATRIX, modelview);
}

void MixerView::paint()
{
    static double renderingAspectRatio = 1.0;
    static bool first = true;
    static GLdouble ax, ay;

    // First the background stuff

    glCallList(ViewRenderWidget::circle_mixing + 2);


    glCallList(ViewRenderWidget::circle_mixing);
    if (_modeMoveCircle)
        glCallList(ViewRenderWidget::circle_mixing + 1);

    glPushMatrix();
    glScaled( limboSize,  limboSize, 1.0);
    glCallList(ViewRenderWidget::circle_limbo);
    if (_modeScaleLimbo)
        glCallList(ViewRenderWidget::circle_limbo + 1);
    glPopMatrix();


    // and the selection connection lines
    glEnable(GL_LINE_STIPPLE);
    glLineStipple(1, 0x00FC);
    glLineWidth(2.0);
    glColor4ub(COLOR_SELECTION, 255);
    glBegin(GL_LINES);
    for(SourceList::iterator  its1 = SelectionManager::getInstance()->selectionBegin(); its1 != SelectionManager::getInstance()->selectionEnd(); its1++) {
        for(SourceList::iterator  its2 = its1; its2 != SelectionManager::getInstance()->selectionEnd(); its2++) {
            glVertex3d((*its1)->getAlphaX(), (*its1)->getAlphaY(), 0.0);
            glVertex3d((*its2)->getAlphaX(), (*its2)->getAlphaY(), 0.0);
        }
    }
    glEnd();
    glDisable(GL_LINE_STIPPLE);


    // Second the icons of the sources (reversed depth order)
    // render in the depth order
    if (ViewRenderWidget::program->bind()) {
        first = true;
        // The icons of the sources (reversed depth order)
        for(SourceSet::iterator  its = RenderingManager::getInstance()->getBegin(); its != RenderingManager::getInstance()->getEnd(); its++) {

            //
            // 1. Render it into current view
            //
            glPushMatrix();
            ax = (*its)->getAlphaX();
            ay = (*its)->getAlphaY();
            glTranslated(ax, ay, (*its)->getDepth());
            renderingAspectRatio = ABS( (*its)->getScaleX() / (*its)->getScaleY() );
            if ( ABS(renderingAspectRatio) > 1.0)
                glScaled(SOURCE_UNIT, SOURCE_UNIT / renderingAspectRatio,  1.0);
            else
                glScaled(SOURCE_UNIT * renderingAspectRatio, SOURCE_UNIT,  1.0);

            // test if the source is passed the standby line
            (*its)->setStandby( CIRCLE_SQUARE_DIST(ax, ay) > (limboSize * limboSize) );

            // setup multi-texturing and effect drawing mode for active sources
            ViewRenderWidget::setSourceDrawingMode(!(*its)->isStandby());

            // Blending Function For mixing like in the rendering window
            (*its)->beginEffectsSection();
            // bind the source texture
            (*its)->bind();
            // standard transparency blending
            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
            glBlendEquation(GL_FUNC_ADD);
            // draw surface
            (*its)->draw();

            if (!(*its)->isStandby())
            {
                // draw stippled version of the source on top
                glEnable(GL_POLYGON_STIPPLE);
                glPolygonStipple(ViewRenderWidget::stippling + ViewRenderWidget::stipplingMode * 128);
                (*its)->draw(false);
                glDisable(GL_POLYGON_STIPPLE);

                //
                // 2. Render it into FBO
                //
                RenderingManager::getInstance()->renderToFrameBuffer(*its, first);
                first = false;

            }
            //
            // 3. draw border and handles if active
            //
            // disable multi-texturing and effect drawing mode
            ViewRenderWidget::setSourceDrawingMode(false);

            if (RenderingManager::getInstance()->isCurrentSource(its))
                glCallList(ViewRenderWidget::border_large_shadow + ((*its)->isModifiable() ? 0 :2) );
            else
                glCallList(ViewRenderWidget::border_thin_shadow + ((*its)->isModifiable() ? 0 :2) );

            glPopMatrix();
        }

        ViewRenderWidget::program->release();
    }

    // if no source was rendered, clear anyway
    RenderingManager::getInstance()->renderToFrameBuffer(0, first, true);

    // post render draw (loop back and recorder)
    RenderingManager::getInstance()->postRenderToFrameBuffer();

    glActiveTexture(GL_TEXTURE0);

    // Then the selection outlines
    for(SourceList::iterator  its = SelectionManager::getInstance()->selectionBegin(); its != SelectionManager::getInstance()->selectionEnd(); its++) {
        glPushMatrix();
        glTranslated((*its)->getAlphaX(), (*its)->getAlphaY(), (*its)->getDepth());
        renderingAspectRatio = (*its)->getScaleX() / (*its)->getScaleY();
        if ( ABS(renderingAspectRatio) > 1.0)
            glScaled(SOURCE_UNIT , SOURCE_UNIT / renderingAspectRatio,  1.0);
        else
            glScaled(SOURCE_UNIT * renderingAspectRatio, SOURCE_UNIT,  1.0);
        glCallList(ViewRenderWidget::frame_selection);
        glPopMatrix();

    }

    // and the groups connection lines
    glEnable(GL_LINE_STIPPLE);
    glLineStipple(1, 0xFC00);
    glLineWidth(2.0);
    glPointSize(15);
    for(SourceListArray::iterator itss = groupSources.begin(); itss != groupSources.end(); itss++) {
        for(SourceListArray::iterator itss = groupSources.begin(); itss != groupSources.end(); itss++) {
           // use color of the group
           glColor4f(groupColor[itss].redF(), groupColor[itss].greenF(),groupColor[itss].blueF(), 0.8);
           for(SourceList::iterator  its1 = (*itss).begin(); its1 != (*itss).end(); its1++) {
               // Connect to every source
               glBegin(GL_LINES);
               for(SourceList::iterator  its2 = its1; its2 != (*itss).end(); its2++) {
                   glVertex3d((*its1)->getAlphaX(), (*its1)->getAlphaY(), 0.0);
                   glVertex3d((*its2)->getAlphaX(), (*its2)->getAlphaY(), 0.0);
               }
               glEnd();
               // dot to identifiy source in the group
               glBegin(GL_POINTS);
               glVertex3d((*its1)->getAlphaX(), (*its1)->getAlphaY(), 0.0);
               glEnd();
           }
        }

    }
    glDisable(GL_LINE_STIPPLE);

    // the source dropping icon
    Source *s = RenderingManager::getInstance()->getSourceBasketTop();
    if ( s ){
        double ax, ay, az; // mouse cursor in rendering coordinates:
        gluUnProject(GLdouble (lastClicPos.x()), GLdouble (viewport[3] - lastClicPos.y()), 1.0,
                modelview, projection, viewport, &ax, &ay, &az);
        glPushMatrix();
        glTranslated( ax, ay, az);
        glPushMatrix();
        if ( ABS(s->getAspectRatio()) > 1.0)
            glTranslated(SOURCE_UNIT + 1.0, -SOURCE_UNIT / s->getAspectRatio() + 1.0,  0.0);
        else
            glTranslated(SOURCE_UNIT * s->getAspectRatio() + 1.0, -SOURCE_UNIT + 1.0,  0.0);
        for (int i = 1; i < RenderingManager::getInstance()->getSourceBasketSize(); ++i ) {
            glTranslated(2.1, 0.0, 0.0);
            glCallList(ViewRenderWidget::border_thin);
        }
        glPopMatrix();
        renderingAspectRatio = s->getScaleX() / s->getScaleY();
        if ( ABS(renderingAspectRatio) > 1.0)
            glScaled(SOURCE_UNIT , SOURCE_UNIT / renderingAspectRatio,  1.0);
        else
            glScaled(SOURCE_UNIT * renderingAspectRatio, SOURCE_UNIT,  1.0);

        glCallList(ViewRenderWidget::border_large_shadow);
        glPopMatrix();
    }

    // The rectangle for selection
    _selectionArea.draw();

}



void MixerView::clear()
{
    View::clear();

    groupSources.clear();
    groupColor.clear();
    limboSize = DEFAULT_LIMBO_SIZE;

}


void MixerView::resize(int w, int h)
{
    View::resize(w, h);
    glViewport(0, 0, viewport[2], viewport[3]);

    // Setup specific projection and view for this window
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();

    if (viewport[2] > viewport[3])
         glOrtho(-SOURCE_UNIT* (double) viewport[2] / (double) viewport[3], SOURCE_UNIT*(double) viewport[2] / (double) viewport[3], -SOURCE_UNIT, SOURCE_UNIT, -MAX_DEPTH_LAYER, 10.0);
     else
         glOrtho(-SOURCE_UNIT, SOURCE_UNIT, -SOURCE_UNIT*(double) viewport[3] / (double) viewport[2], SOURCE_UNIT*(double) viewport[3] / (double) viewport[2], -MAX_DEPTH_LAYER, 10.0);

    glGetDoublev(GL_PROJECTION_MATRIX, projection);


    // compute largest mixing area for Mixing View (minimum zoom and max panning to both sides)
    double dum;
    GLdouble maxmodelview[16];
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();
    glScaled(minzoom, minzoom, minzoom);
    glTranslated(maxpanx, maxpany, 0.0);
    glGetDoublev(GL_MODELVIEW_MATRIX, maxmodelview);
    gluUnProject(0.0, 0.0, 0.0, maxmodelview, projection, viewport, _mixingArea, _mixingArea+1, &dum);
    glTranslated(-2.0*maxpanx, -2.0*maxpany, 0.0);
    glGetDoublev(GL_MODELVIEW_MATRIX, maxmodelview);
    gluUnProject((double)viewport[2], (double)viewport[3], 0.0, maxmodelview, projection, viewport, _mixingArea+2, _mixingArea+3, &dum);
    glPopMatrix();

}



void MixerView::setAction(ActionType a){

    if (a == currentAction)
        return;

    View::setAction(a);

    switch(a) {
    case View::OVER:
        RenderingManager::getRenderingWidget()->setMouseCursor(ViewRenderWidget::MOUSE_HAND_OPEN);
        break;
    case View::GRAB:
        RenderingManager::getRenderingWidget()->setMouseCursor(ViewRenderWidget::MOUSE_HAND_CLOSED);
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

bool MixerView::mousePressEvent(QMouseEvent *event)
{
    if (!event)
        return false;

    lastClicPos = event->pos();

    //  panning
    if (  isUserInput(event, View::INPUT_NAVIGATE) ||  isUserInput(event, View::INPUT_DRAG) || _modeMoveCircle ) {
        // priority to panning of the view (even in drop mode)
        setAction(View::PANNING);
        return false;
    }

    // DRoP MODE ; explicitly do nothing
    if ( RenderingManager::getInstance()->getSourceBasketTop() ) {
        setAction(View::DROP);
        if (isUserInput(event, View::INPUT_CONTEXT_MENU))
            RenderingManager::getRenderingWidget()->showContextMenu(ViewRenderWidget::CONTEXT_MENU_DROP, event->pos());
        // don't interpret other mouse events in drop mode
        return false;
    }

    // OTHER USER INPUT ; initiate action
    if ( getSourcesAtCoordinates(event->x(), viewport[3] - event->y()) ) { // if at least one source icon was clicked

        // get the top most clicked source (always one as getSourcesAtCoordinates returned true)
        Source *clicked =  *clickedSources.begin();

        // SELECT MODE : add/remove from selection
        if ( isUserInput(event, View::INPUT_SELECT) ) {

                if ( SelectionManager::getInstance()->isInSelection(clicked) )
                    SelectionManager::getInstance()->deselect(clicked);
                else
                    SelectionManager::getInstance()->select(clicked);
        }
        // not in (INPUT_SELECT) action mode,
        else {
            // then set the current active source
            RenderingManager::getInstance()->setCurrentSource( clicked->getId() );

            // context menu
            if ( isUserInput(event, View::INPUT_CONTEXT_MENU) )
                RenderingManager::getRenderingWidget()->showContextMenu(ViewRenderWidget::CONTEXT_MENU_SOURCE, event->pos());
            // zoom
            else if ( isUserInput(event, View::INPUT_ZOOM) )
                zoomBestFit(true);
            // other cases
            else {

                if (isUserInput(event, View::INPUT_TOOL_INDIVIDUAL))
                    // in individual selection mode, the selection is cleared
                    SelectionManager::getInstance()->clearSelection();
                else {
                    // test if source is in a group
                    SourceListArray::iterator itss = findGroup(clicked);
                    // if the source is not in the selection
                    if ( !SelectionManager::getInstance()->isInSelection(clicked) ) {
                        // NOT in a selection but in a group : select only the group
                        if (  itss != groupSources.end() ) {
                            SelectionManager::getInstance()->clearSelection();
                            SelectionManager::getInstance()->select(*itss);
                        }
                        // NOT in a selection and NOT in a group (single source clicked)
                        else
                            SelectionManager::getInstance()->clearSelection();
                    }
                }
                // tool use
                if ( isUserInput(event, View::INPUT_TOOL) || isUserInput(event, View::INPUT_TOOL_INDIVIDUAL) ) {
                    // ready for grabbing the current source
                    if ( clicked->isModifiable() )
                        setAction(View::GRAB);
                }
            }
        }
        // current source changed in some way
        return true;
    }
    // else
    // click in background

    // remember coordinates of clic
    GLdouble cursorx = 0.0, cursory = 0.0, dumm = 0.0;
    gluUnProject((GLdouble) event->x(), (GLdouble) viewport[3] - event->y(), 0.0, modelview, projection, viewport, &cursorx, &cursory, &dumm);
    _selectionArea.markStart(QPointF(cursorx,cursory));

    // context menu on the background
    if ( isUserInput(event, View::INPUT_CONTEXT_MENU) ) {
        RenderingManager::getRenderingWidget()->showContextMenu(ViewRenderWidget::CONTEXT_MENU_VIEW, event->pos());
        return false;
    }
    // zoom button in the background : zoom best fit
    else if ( isUserInput(event, View::INPUT_ZOOM) ) {
        zoomBestFit(false);
        return false;
    }
    // selection mode, clic background is ineffective
    else if ( isUserInput(event, View::INPUT_SELECT) )
        return false;

    // set current to none (end of list)
    RenderingManager::getInstance()->unsetCurrentSource();
    // back to no action
    setAction(View::NONE);

    return false;
}

bool MixerView::mouseDoubleClickEvent ( QMouseEvent * event )
{
    if (!event)
        return false;

    if (currentAction == View::DROP)
        return false;

    // for double tool clic
    if ( isUserInput(event, View::INPUT_TOOL) /*|| isUserInput(event, View::INPUT_TOOL_INDIVIDUAL)*/  ) {

        // left double click on a source : change the group / selection
        if ( getSourcesAtCoordinates(event->x(), viewport[3] - event->y()) ) {

            // get the top most clicked source
            Source *clicked = *clickedSources.begin();

            SourceListArray::iterator itss = findGroup(clicked);
            // if double clic on a group which is not part of a selection ; convert group into selection
            if ( itss != groupSources.end() && (*itss).size() >= SelectionManager::getInstance()->copySelection().size()  ) {
                SelectionManager::getInstance()->setSelection(*itss);
                // erase group and its color
                groupColor.remove(itss);
                groupSources.erase(itss);
            }
            // if double clic on a selection ; convert selection into a new group
            else {
                SourceList selection = SelectionManager::getInstance()->copySelection();
                // convert every groups into selection
                for(SourceList::iterator  its = SelectionManager::getInstance()->selectionBegin(); its != SelectionManager::getInstance()->selectionEnd(); its++) {
                    SourceListArray::iterator itss = findGroup(*its);
                    if (itss != groupSources.end()) {
                        SourceList result;
                        std::set_union(selection.begin(), selection.end(), (*itss).begin(), (*itss).end(), std::inserter(result, result.begin()) );
                        // set new selection
                        selection = SourceList(result);
                        // remove old group
                        removeFromGroup(*its);
                    }
                }
                // if the selection is big enough to form a group
                if ( selection.size()>1 ) {
                    // create a new group from the selection
                    groupSources.push_front(selection);
                    groupColor[groupSources.begin()] = QColor::fromHsv ( rand()%180 + 179, 200, 255);
                }
            }
            return true;
        }
    }
    // zoom
    else if ( isUserInput(event, View::INPUT_ZOOM) ) {
        zoomReset();
        return true;
    }

    return false;
}


bool MixerView::mouseMoveEvent(QMouseEvent *event)
{
    if (!event)
        return false;

    int dx = event->x() - lastClicPos.x();
    int dy = lastClicPos.y() - event->y();
    lastClicPos = event->pos();

    // DROP MODE : avoid other actions
    if ( RenderingManager::getInstance()->getSourceBasketTop() ) {
        setAction(View::DROP);
        // don't interpret mouse events in drop mode
        return false;
    }

    // PANNING ; move the background
    if ( currentAction == View::PANNING ) {
        // panning background
        panningBy(event->x(), viewport[3] - event->y(), dx, dy);
        // SHIFT ?
        if ( isUserInput(event, View::INPUT_DRAG) || ( isUserInput(event, View::INPUT_NAVIGATE) && _modeMoveCircle ) ) {
            // special move ; move the sources in the opposite
            for(SourceSet::iterator its = RenderingManager::getInstance()->getBegin(); its != RenderingManager::getInstance()->getEnd(); its++)
                grabSource( *its, event->x(), viewport[3] - event->y(), -dx, -dy);
            // return true as we may have moved the current source
            return true;
        }
        // return false as nothing changed
        return false;
    }


    if ( isUserInput(event, View::INPUT_TOOL) || isUserInput(event, View::INPUT_TOOL_INDIVIDUAL) ||  isUserInput(event, View::INPUT_SELECT) )
    {
        // get the top most clicked source, if there is one
        Source *clicked = 0;
        if (sourceClicked())
            clicked = *clickedSources.begin();
        else
            clicked = 0;

        // No source clicked but mouse button down
        if ( !clicked ) {

            // get coordinate of cursor
            GLdouble cursorx = 0.0, cursory = 0.0, dumm = 0.0;
            gluUnProject((GLdouble) event->x(), (GLdouble) viewport[3] - event->y(), 0.0, modelview, projection, viewport, &cursorx, &cursory, &dumm);

            // Are we scaling the limbo area ?
            if ( _modeScaleLimbo ) {
                GLdouble sqr_limboSize = CIRCLE_SQUARE_DIST(cursorx, cursory);
                setLimboSize( sqrt(sqr_limboSize) );
            }
            // no, then we are SELECTING AREA
            else {
                // enable drawing of selection area
                _selectionArea.setEnabled(true);

                // set coordinate of end of rectangle selection
                _selectionArea.markEnd(QPointF(cursorx, cursory));

                // loop over every sources to check if it is in the rectangle area
                SourceList rectSources;
                for(SourceSet::iterator  its = RenderingManager::getInstance()->getBegin(); its != RenderingManager::getInstance()->getEnd(); its++)
                    if (_selectionArea.contains(its) )
                        rectSources.insert(*its);

                if ( isUserInput(event, View::INPUT_SELECT) )
                    // extend selection
                    SelectionManager::getInstance()->select(rectSources);
                else  // new selection
                    SelectionManager::getInstance()->setSelection(rectSources);
            }
        }
        // clicked source not null and grab action
        else if (currentAction == View::GRAB )
            grabSources(clicked, event->x(), viewport[3] - event->y(), dx, dy);

        // return true if we modified (grabbed) the source
        return (bool) clicked;
    }

    // Show mouse over cursor only if no user input
    if ( isUserInput(event, View::INPUT_NONE ) )
    {
        //  change action cursor if over a source
        if ( getSourcesAtCoordinates(event->x(), viewport[3] - event->y(), false) ) {
            setAction(View::OVER);
            _modeScaleLimbo = _modeMoveCircle = false;
        } else {
            setAction(View::NONE);
            // on the border of the limbo area ?
            _modeScaleLimbo = hasObjectAtCoordinates(event->x(), viewport[3] - event->y(), ViewRenderWidget::circle_limbo + 1, limboSize, 4.0);

            // on the border of the cirle area ?
            _modeMoveCircle = hasObjectAtCoordinates(event->x(), viewport[3] - event->y(), ViewRenderWidget::circle_mixing + 1, 1.0, 4.0);
        }
    }

    return false;
}

bool MixerView::mouseReleaseEvent ( QMouseEvent * event )
{
    if ( RenderingManager::getInstance()->getSourceBasketTop() )
        RenderingManager::getRenderingWidget()->setMouseCursor(ViewRenderWidget::MOUSE_QUESTION);
    else if (currentAction == View::GRAB  || currentAction == View::DROP)
        setAction(OVER);
    else if (currentAction == View::PANNING)
        setAction(previousAction);
    else if ( currentAction == View::SELECT && !sourceClicked() && !_selectionArea.isEnabled())
        SelectionManager::getInstance()->clearSelection();

    // end of selection area in any case
    _selectionArea.setEnabled(false);

    return true;
}

bool MixerView::wheelEvent ( QWheelEvent * event )
{
    bool ret = true;
    // remember position of cursor before zoom
    double bx, by, z;
    gluUnProject((GLdouble) event->x(), (GLdouble) (viewport[3] - event->y()), 0.0,
            modelview, projection, viewport, &bx, &by, &z);

    // apply zoom
    setZoom (zoom + ((double) event->delta() * zoom * minzoom) / (View::zoomSpeed() * maxzoom) );

    // compute position of cursor after zoom
    double ax, ay;
    gluUnProject((GLdouble) event->x(), (GLdouble) (viewport[3] - event->y()), 0.0,
            modelview, projection, viewport, &ax, &ay, &z);

    if (View::zoomCentered()) {
        // Center view on cursor when zooming ( panning = panning + ( cursor position after zoom - position before zoom ) )
        // BUT with a non linear correction factor when approaching to MINZOOM (close to 0) which allows
        // to re-center the view on the center when zooming out maximally
        double expfactor = 1.0 / ( 1.0 + exp(7.0 - 100.0 * zoom) );
        setPanning((getPanningX() + ax - bx) * expfactor, (getPanningY() + ay - by) * expfactor );
    }

    // in case we were already performing an action
    if ( currentAction == View::GRAB || _modeScaleLimbo || _modeMoveCircle || _selectionArea.isEnabled() ){

        // where is the mouse cursor now (after zoom and panning)?
        gluUnProject((GLdouble) event->x(), (GLdouble) (viewport[3] - event->y()), 0.0,
            modelview, projection, viewport, &ax, &ay, &z);
        // this means we have a delta of mouse position
        deltax = ax - bx;
        deltay = ay - by;

        // simulate a movement of the mouse
        QMouseEvent *e = new QMouseEvent(QEvent::MouseMove, event->pos(), Qt::NoButton, qtMouseButtons(INPUT_TOOL), qtMouseModifiers(INPUT_TOOL));
        ret = mouseMoveEvent(e);
        delete e;

        // reset delta
        deltax = 0;
        deltay = 0;
    }

    return ret;
}

void MixerView::zoomReset()
{
    setZoom(DEFAULTZOOM);
    setPanning(DEFAULT_PANNING);
}

void MixerView::zoomBestFit( bool onlyClickedSource )
{
    // nothing to do if there is no source
    if (RenderingManager::getInstance()->empty()){
        zoomReset();
        return;
    }

    // 0. consider either the list of clicked sources, either the full list
    SourceSet::iterator beginning, end;
    if (onlyClickedSource && RenderingManager::getInstance()->getCurrentSource() != RenderingManager::getInstance()->getEnd()) {
        beginning = end = RenderingManager::getInstance()->getCurrentSource();
        end++;
    } else {
        beginning = RenderingManager::getInstance()->getBegin();
        end = RenderingManager::getInstance()->getEnd();
    }

    // 1. compute bounding box of every sources to consider
    double x_min = 10000, x_max = -10000, y_min = 10000, y_max = -10000;
    for(SourceSet::iterator  its = beginning; its != end; its++) {
        // ignore standby sources
        if ((*its)->isStandby())
            continue;
        // get alpha coordinates
        x_min = MINI (x_min, (*its)->getAlphaX() - SOURCE_UNIT * (*its)->getAspectRatio());
        x_max = MAXI (x_max, (*its)->getAlphaX() + SOURCE_UNIT * (*its)->getAspectRatio());
        y_min = MINI (y_min, (*its)->getAlphaY() - SOURCE_UNIT );
        y_max = MAXI (y_max, (*its)->getAlphaY() + SOURCE_UNIT );
    }

    // 2. Apply the panning to the new center
    setPanning( -( x_min + ABS(x_max - x_min)/ 2.0 ) ,  -( y_min + ABS(y_max - y_min)/ 2.0 )  );

    // 3. get the extend of the area covered in the viewport (the matrices have been updated just above)
    double LLcorner[3];
    double URcorner[3];
    gluUnProject(viewport[0], viewport[1], 0, modelview, projection, viewport, LLcorner, LLcorner+1, LLcorner+2);
    gluUnProject(viewport[2], viewport[3], 0, modelview, projection, viewport, URcorner, URcorner+1, URcorner+2);

    // 4. compute zoom factor to fit to the boundaries
    // initial value = a margin scale of 5%
    double scale = 0.98;
    double scale1 = ABS(URcorner[0]-LLcorner[0]) / ABS(x_max-x_min);
    double scale2 = ABS(URcorner[1]-LLcorner[1]) / ABS(y_max-y_min);
    // depending on the axis having the largest extend
    if ( scale1 < scale2 )
        scale *= scale1;
    else
        scale *= scale2;
    // apply the scaling
    setZoom( zoom * scale );
}


bool MixerView::keyPressEvent ( QKeyEvent * event ){

    // detect select mode
    if ( !(QApplication::keyboardModifiers() ^ View::qtMouseModifiers(INPUT_SELECT)) ){
        setAction(View::SELECT);
        return true;
    }

    // else move a source
    SourceSet::iterator its = RenderingManager::getInstance()->getCurrentSource();
    if (its != RenderingManager::getInstance()->getEnd()) {
        int dx =0, dy = 0, factor = 1;
// TODO : find a way to configure modifier or forget about the special zoom
//	    if (event->modifiers() & Qt::ShiftModifier)
//	    	factor *= 10;
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
        grabSources(*its, 0, 0, dx, dy);
        return true;
    }

    return false;
}

bool MixerView::keyReleaseEvent(QKeyEvent * event) {

    // default to no action
    setAction(View::NONE);

    if ( currentAction == View::SELECT && !(QApplication::keyboardModifiers() & View::qtMouseModifiers(INPUT_SELECT)) )
        setAction(previousAction);

    return false;
}



SourceListArray::iterator  MixerView::findGroup(Source *s){

    SourceListArray::iterator itss = groupSources.begin();
    for(; itss != groupSources.end(); itss++) {
        if ( (*itss).count(s) > 0 )
            break;
    }
    return ( itss );
}

bool MixerView::isInAGroup(Source *s){

    return ( findGroup(s) != groupSources.end() );
}

void MixerView::removeFromGroup(Source *s)
{

    // find the group containing the source to delete
    SourceListArray::iterator itss = findGroup(s);

    // if there is a group containing the source to delete
    if(itss != groupSources.end()){
        // remove the source from this group
        (*itss).erase(s);

        // if the group is now a singleton, delete it
        if( (*itss).size() < 2 ){
            groupSources.erase(itss);
        }
    }

}


bool MixerView::hasObjectAtCoordinates(int mouseX, int mouseY, int objectdisplaylist, GLdouble scale, GLdouble tolerance)
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
    gluPickMatrix((GLdouble) mouseX, (GLdouble) mouseY, tolerance, tolerance, viewport);
    glMultMatrixd(projection);

    // rendering for select mode
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();

    glScaled(scale, scale, 1.0);
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

bool MixerView::getSourcesAtCoordinates(int mouseX, int mouseY, bool clic) {

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
    gluPickMatrix((GLdouble) mouseX, (GLdouble) mouseY, 1.0, 1.0, viewport);
    glMultMatrixd(projection);

    // rendering for select mode
    glMatrixMode(GL_MODELVIEW);

    for(SourceSet::iterator  its = RenderingManager::getInstance()->getBegin(); its != RenderingManager::getInstance()->getEnd(); its++) {
        glPushMatrix();
        glTranslated( (*its)->getAlphaX(), (*its)->getAlphaY(), (*its)->getDepth());
        double renderingAspectRatio = (*its)->getScaleX() / (*its)->getScaleY();
        if ( ABS(renderingAspectRatio) > 1.0)
            glScaled(SOURCE_UNIT , SOURCE_UNIT / renderingAspectRatio,  1.0);
        else
            glScaled(SOURCE_UNIT * renderingAspectRatio, SOURCE_UNIT,  1.0);

        (*its)->draw(false, GL_SELECT);
        glPopMatrix();
    }

    // compute picking . return to rendering mode
    hits = glRenderMode(GL_RENDER);

//    qDebug ("%d hits @ (%d,%d) vp (%d, %d, %d, %d)", hits, mouseX, mouseY, viewport[0], viewport[1], viewport[2], viewport[3]);

    // set the matrices back
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);

    if (clic) {
        clickedSources.clear();
        while (hits != 0) {
            clickedSources.insert( *(RenderingManager::getInstance()->getById (selectBuf[ (hits-1) * 4 + 3])) );
            hits--;
        }

        return sourceClicked();
    } else
        return (hits != 0 && (*(RenderingManager::getInstance()->getById (selectBuf[ (hits-1) * 4 + 3])))->isModifiable() );

}

/**
 *
 **/
void MixerView::grabSources(Source *s, int x, int y, int dx, int dy) {

    if (!s) return;

    // if the source is in the selection, move the selection
    if ( SelectionManager::getInstance()->isInSelection(s) ){
        for(SourceList::iterator  its = SelectionManager::getInstance()->selectionBegin(); its != SelectionManager::getInstance()->selectionEnd(); its++) {
            grabSource( *its, x, y, dx, dy);
        }
    }
    // nothing special, move the source individually
    else
        grabSource(s, x, y, dx, dy);

}

/**
 *
 **/
void MixerView::grabSource(Source *s, int x, int y, int dx, int dy) {

    if (!s || !s->isModifiable()) return;

    double bx, by, bz; // before movement
    double ax, ay, az; // after  movement

    gluUnProject((GLdouble) (x - dx), (GLdouble) (y - dy),
            0.0, modelview, projection, viewport, &bx, &by, &bz);
    gluUnProject((GLdouble) x, (GLdouble) y, 0.0,
            modelview, projection, viewport, &ax, &ay, &az);

    double ix = s->getAlphaX() + ax - bx + deltax;
    double iy = s->getAlphaY() + ay - by + deltay;

    // move icon
    s->setAlphaCoordinates( qBound(_mixingArea[0], ix, _mixingArea[2]), qBound(_mixingArea[1], iy, _mixingArea[3]) );
}


/**
 *
 **/
void MixerView::panningBy(int x, int y, int dx, int dy) {

    double bx, by, bz; // before movement
    double ax, ay, az; // after  movement

    gluUnProject((GLdouble) (x - dx), (GLdouble) (y - dy),
            0.0, modelview, projection, viewport, &bx, &by, &bz);
    gluUnProject((GLdouble) x, (GLdouble) y, 0.0,
            modelview, projection, viewport, &ax, &ay, &az);

    // apply panning
    setPanning(getPanningX() + ax - bx, getPanningY() + ay - by);
}


QDomElement MixerView::getConfiguration(QDomDocument &doc){

    QDomElement mixviewelem = View::getConfiguration(doc);

    // Mixer View limbo size parameter
    QDomElement l = doc.createElement("Limbo");
    l.setAttribute("value", QString::number(getLimboSize(),'f',4) );
    mixviewelem.appendChild(l);

    // Mixer View groups
    QDomElement groups = doc.createElement("Groups");
    for(SourceListArray::iterator itss = groupSources.begin(); itss != groupSources.end(); itss++) {
        // if this group has more than 1 element
        if (itss->size() > 1) {
            // create a group dom element, with color
            QDomElement group = doc.createElement("Group");
            group.setAttribute("R", groupColor[itss].red());
            group.setAttribute("G", groupColor[itss].green());
            group.setAttribute("B", groupColor[itss].blue());
            // fill in the group with the list of sources.
            for(SourceList::iterator  its = (*itss).begin(); its != (*itss).end(); its++) {
                QDomElement s = doc.createElement("Source");
                QDomText sname = doc.createTextNode((*its)->getName());
                s.appendChild(sname);
                group.appendChild(s);
            }
            groups.appendChild(group);
        }
    }
    mixviewelem.appendChild(groups);

    return mixviewelem;
}


void MixerView::setConfiguration(QDomElement xmlconfig){

    // apply generic View config
    View::setConfiguration(xmlconfig);

    // Mixer View limbo size parameter
    setLimboSize(xmlconfig.firstChildElement("Limbo").attribute("value", "2.5").toFloat());

    QDomElement groups = xmlconfig.firstChildElement("Groups");
    // if there is a list of groups
    if (!groups.isNull()){
        QDomElement group = groups.firstChildElement("Group");
        // if there is a group in the list
        while (!group.isNull()) {
            SourceList newgroup;
            // if this group has more than 1 element (singleton group would be a bug)
            if (group.childNodes().count() > 1) {
                QDomElement sourceelem = group.firstChildElement("Source");
                // Add every source which name is in the list
                while (!sourceelem.isNull()) {
                    SourceSet::iterator s = RenderingManager::getInstance()->getByName(sourceelem.text());
                    if (RenderingManager::getInstance()->isValid(s))
                        newgroup.insert( *s );
                    sourceelem = sourceelem.nextSiblingElement();
                }

                groupSources.push_front(newgroup);
                groupColor[groupSources.begin()] = QColor( group.attribute("R").toInt(),group.attribute("G").toInt(), group.attribute("B").toInt() );
            }
            group = group.nextSiblingElement();
        }
    }

}

void MixerView::setLimboSize(GLdouble s) {
    limboSize = CLAMP(s, MIN_LIMBO_SIZE, MAX_LIMBO_SIZE);
    modified = true;
}

double MixerView::getLimboSize() {
    return ( limboSize );
}


QRectF MixerView::getBoundingBox(const SourceList &l)
{
    double bbox[2][2];

    // init bbox to max size
    bbox[0][0] = 2.0*SOURCE_UNIT*MAXZOOM*CIRCLE_SIZE;
    bbox[0][1] = 2.0*SOURCE_UNIT*MAXZOOM*CIRCLE_SIZE;
    bbox[1][0] = -2.0*SOURCE_UNIT*MAXZOOM*CIRCLE_SIZE;
    bbox[1][1] = -2.0*SOURCE_UNIT*MAXZOOM*CIRCLE_SIZE;
    // compute Axis aligned bounding box of all sources in the list
    for(SourceList::const_iterator  its = l.begin(); its != l.end(); its++) {
        bbox[0][0] = qMin( (*its)->getAlphaX(), bbox[0][0]);
        bbox[0][1] = qMin( (*its)->getAlphaY(), bbox[0][1]);
        bbox[1][0] = qMax( (*its)->getAlphaX(), bbox[1][0]);
        bbox[1][1] = qMax( (*its)->getAlphaY(), bbox[1][1]);

//        double renderingAspectRatio = ABS( (*its)->getScaleX() / (*its)->getScaleY());
//        if ( ABS(renderingAspectRatio) > 1.0) {

//            bbox[0][0] = qMin( (*its)->getAlphaX() - SOURCE_UNIT, bbox[0][0]);
//            bbox[0][1] = qMin( (*its)->getAlphaY() - SOURCE_UNIT / renderingAspectRatio, bbox[0][1]);
//            bbox[1][0] = qMax( (*its)->getAlphaX() + SOURCE_UNIT, bbox[1][0]);
//            bbox[1][1] = qMax( (*its)->getAlphaY() + SOURCE_UNIT / renderingAspectRatio, bbox[1][1]);

//        } else {
//            // keep max and min
//            bbox[0][0] = qMin( (*its)->getAlphaX() - SOURCE_UNIT * renderingAspectRatio, bbox[0][0]);
//            bbox[0][1] = qMin( (*its)->getAlphaY() - SOURCE_UNIT, bbox[0][1]);
//            bbox[1][0] = qMax( (*its)->getAlphaX() + SOURCE_UNIT * renderingAspectRatio, bbox[1][0]);
//            bbox[1][1] = qMax( (*its)->getAlphaY() + SOURCE_UNIT, bbox[1][1]);

//        }
    }
    // return bottom-left ; top-right
    return QRectF(QPointF(bbox[0][0], bbox[0][1]), QPointF(bbox[1][0], bbox[1][1]));
}

void MixerView::alignSelection(View::Axis a, View::RelativePoint p, View::Reference r)
{
    QRectF selectionBox = MixerView::getBoundingBox(SelectionManager::getInstance()->copySelection());
    if (r == View::REFERENCE_SOURCES) {
        // perform the computations
        for(SourceList::iterator  its = SelectionManager::getInstance()->selectionBegin(); its != SelectionManager::getInstance()->selectionEnd(); its++){

            QPointF point = QPointF((*its)->getAlphaX(), (*its)->getAlphaY());

            // CENTERED
            if (p==View::ALIGN_CENTER) {
                if (a==View::AXIS_HORIZONTAL)
                    point.setX( selectionBox.center().x());
                else
                    point.setY( selectionBox.center().y());
            }
            // View::ALIGN_BOTTOM_LEFT (inverted y)
            else if (p==View::ALIGN_BOTTOM_LEFT) {
                if (a==View::AXIS_HORIZONTAL)
                    point.setX( selectionBox.topLeft().x());
                else
                    point.setY( selectionBox.topLeft().y());
            }
            // View::ALIGN_TOP_RIGHT (inverted y)
            else if (p==View::ALIGN_TOP_RIGHT) {
                if (a==View::AXIS_HORIZONTAL)
                    point.setX( selectionBox.bottomRight().x());
                else
                    point.setY( selectionBox.bottomRight().y());
            }
            // move icon
            (*its)->setAlphaCoordinates( point.x() , point.y() );
        }
    }
    // REFERENCE_FRAME
    else {
        QRectF circleBox = QRectF(-CIRCLE_SIZE * SOURCE_UNIT,-CIRCLE_SIZE * SOURCE_UNIT,2.0*CIRCLE_SIZE * SOURCE_UNIT, 2.0*CIRCLE_SIZE * SOURCE_UNIT);

        QPointF delta;
        // CENTERED
        if (p==View::ALIGN_CENTER)
            delta = circleBox.center() -selectionBox.center();
        // View::ALIGN_BOTTOM_LEFT (inverted y)
        else if (p==View::ALIGN_BOTTOM_LEFT)
            delta = circleBox.topLeft() - selectionBox.topLeft();
        // View::ALIGN_TOP_RIGHT (inverted y)
        else if (p==View::ALIGN_TOP_RIGHT)
            delta = circleBox.bottomRight() - selectionBox.bottomRight();

        // perform the computations
        for(SourceList::iterator  its = SelectionManager::getInstance()->selectionBegin(); its != SelectionManager::getInstance()->selectionEnd(); its++){

            if (a==View::AXIS_HORIZONTAL)
                (*its)->setAlphaCoordinates( (*its)->getAlphaX() + delta.x(), (*its)->getAlphaY() );
            else
                (*its)->setAlphaCoordinates( (*its)->getAlphaX(), (*its)->getAlphaY() + delta.y() );
        }

    }
}

void MixerView::distributeSelection(View::Axis a, View::RelativePoint p)
{
    // get selection and discard useless operation
    SourceList selection = SelectionManager::getInstance()->copySelection();
    if (selection.size() < 2)
        return;

    QMap< int, QPair<Source*, QPointF> > sortedlist;
    // do this for horizontal alignment
    if (a==View::AXIS_HORIZONTAL) {
        for(SourceList::iterator i = SelectionManager::getInstance()->selectionBegin(); i != SelectionManager::getInstance()->selectionEnd(); i++){
            QPointF point = QPointF((*i)->getAlphaX(), (*i)->getAlphaY());
            sortedlist[int(point.x()*1000)] = qMakePair(*i, point);
        }
    }
    // do this for the vertical alignment
    else {
        // sort the list of sources by  y (inverted)
        for(SourceList::iterator i = SelectionManager::getInstance()->selectionBegin(); i != SelectionManager::getInstance()->selectionEnd(); i++){
            QPointF point = QPointF((*i)->getAlphaX(), (*i)->getAlphaY());
            sortedlist[int(point.y()*1000)] = qMakePair(*i, point);
        }
    }

    // compute the step of translation
    QSizeF s = MixerView::getBoundingBox(selection).size() / double(sortedlist.count()-1);
    QPointF translation(s.width(),s.height());
    QPointF position = sortedlist[sortedlist.keys().first()].second;

    // loop over source list, except bottom-left & top-right most
    sortedlist.remove( sortedlist.keys().first() );
    sortedlist.remove( sortedlist.keys().last() );
    QMapIterator< int, QPair<Source*, QPointF> > its(sortedlist);
    while (its.hasNext()) {
        its.next();
        position += translation;

        QPointF point =  its.value().second;

        if (a==View::AXIS_HORIZONTAL)
            point.setX(position.x());
        else // View::VERTICAL (inverted y)
            point.setY(position.y());

        // move icon
        its.value().first->setAlphaCoordinates( point.x() , point.y()  );
    }

}

