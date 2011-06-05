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
 *   Copyright 2009, 2010 Bruno Herbelin
 *
 */

#include "GeometryView.h"

#include "RenderingManager.h"
#include "ViewRenderWidget.h"
#include "OutputRenderWindow.h"
#include <algorithm>

#define MINZOOM 0.1
#define MAXZOOM 3.0
#define DEFAULTZOOM 0.5

GeometryView::GeometryView() : View(), quadrant(0), currentTool(SCALE), currentSource(0)
{
	zoom = DEFAULTZOOM;
	minzoom = MINZOOM;
	maxzoom = MAXZOOM;
	maxpanx = SOURCE_UNIT*MAXZOOM*2.0;
	maxpany = SOURCE_UNIT*MAXZOOM*2.0;
	currentAction = View::NONE;

	borderType = ViewRenderWidget::border_large;

    icon.load(QString::fromUtf8(":/glmixer/icons/manipulation.png"));
    title = " Geometry view";

}


void GeometryView::setModelview()
{
	View::setModelview();
    glScalef(zoom, zoom, zoom);
    glTranslatef(getPanningX(), getPanningY(), 0.0);
    glGetDoublev(GL_MODELVIEW_MATRIX, modelview);
}

void GeometryView::paint()
{
	static bool first = true;

    // first the background (as the rendering black clear color) with shadow
	glPushMatrix();
    glScalef( OutputRenderWindow::getInstance()->getAspectRatio(), 1.0, 1.0);
    glCallList(ViewRenderWidget::quad_window[RenderingManager::getInstance()->clearToWhite()?1:0]);
    glPopMatrix();

    // we use the shader to render sources
    if (ViewRenderWidget::program->bind()) {
		first = true;
		// The icons of the sources (reversed depth order)
		for(SourceSet::iterator  its = RenderingManager::getInstance()->getBegin(); its != RenderingManager::getInstance()->getEnd(); its++) {

			if ((*its)->isStandby())
				continue;

			//
			// 1. Render it into current view
			//
			ViewRenderWidget::setSourceDrawingMode(true);

			// place and scale
			glPushMatrix();
			glTranslated((*its)->getX(), (*its)->getY(), (*its)->getDepth());
			glRotated((*its)->getRotationAngle(), 0.0, 0.0, 1.0);
			glScaled((*its)->getScaleX(), (*its)->getScaleY(), 1.f);

			// Blending Function For mixing like in the rendering window
			(*its)->beginEffectsSection();
			// bind the source texture and update its content
			(*its)->update();
			// test for culling
			(*its)->testGeometryCulling();
			// Draw it !
			(*its)->blend();
			(*its)->draw();

			//
			// 2. Render it into FBO
			//
			RenderingManager::getInstance()->renderToFrameBuffer(*its, first);
			first = false;

			glPopMatrix();
		}

		// draw borders on top
		ViewRenderWidget::setSourceDrawingMode(false);

		for(SourceSet::iterator  its = RenderingManager::getInstance()->getBegin(); its != RenderingManager::getInstance()->getEnd(); its++) {
			if ((*its)->isStandby())
				continue;

			//
			// 3. draw border and handles if active
			//
			// place and scale
			glPushMatrix();
			glTranslated((*its)->getX(), (*its)->getY(), (*its)->getDepth());
			glRotated((*its)->getRotationAngle(), 0.0, 0.0, 1.0);
			glScaled((*its)->getScaleX(), (*its)->getScaleY(), 1.f);

			if (RenderingManager::getInstance()->isCurrentSource(its))
				glCallList(borderType + ((*its)->isModifiable() ? 0 : 3));
			else
				glCallList(ViewRenderWidget::border_thin + ((*its)->isModifiable() ? 0 : 3));

			glPopMatrix();

		}
		ViewRenderWidget::program->release();
    }
	glActiveTexture(GL_TEXTURE0);

	// if no source was rendered, clear anyway
	RenderingManager::getInstance()->renderToFrameBuffer(0, first, true);

	// post render draw (loop back and recorder)
	RenderingManager::getInstance()->postRenderToFrameBuffer();

    // Then the selection outlines
	if ( View::hasSelection() ) {
		// Draw center point
		glPushMatrix();
		glTranslated(View::selectionSource()->getX(), View::selectionSource()->getY(), 0);
		glPointSize(10);
		glColor4ub(COLOR_SELECTION, 200);
		glBegin(GL_POINTS);
		glVertex2f(0,0);
		glEnd();

		// Draw selection source
		glRotated(View::selectionSource()->getRotationAngle(), 0.0, 0.0, 1.0);
		glScaled(View::selectionSource()->getScaleX(), View::selectionSource()->getScaleY(), 1.f);
		if ( currentSource == View::selectionSource() )
			glCallList(borderType + 6);
		else
			glCallList(ViewRenderWidget::border_thin + 6);

		glPopMatrix();
	}

    // last the frame thing
	glPushMatrix();
    glScalef( OutputRenderWindow::getInstance()->getAspectRatio(), 1.0, 1.0);
    glCallList(ViewRenderWidget::frame_screen);
    glPopMatrix();

    // the source dropping icon
    Source *s = RenderingManager::getInstance()->getSourceBasketTop();
    if ( s ){
    	double ax, ay, az; // mouse cursor in rendering coordinates:
		gluUnProject( GLdouble (lastClicPos.x()), GLdouble (viewport[3] - lastClicPos.y()), 1.0,
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
		glScalef( s->getScaleX(), s->getScaleY(), 1.f);
		glCallList(ViewRenderWidget::border_large);
		glPopMatrix();
    }

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
}


bool GeometryView::mousePressEvent(QMouseEvent *event)
{
	if (!event)
		return false;

	lastClicPos = event->pos();

	// MIDDLE BUTTON ; panning cursor
	if ((event->button() == Qt::MidButton) || ( (event->button() == Qt::LeftButton) && (event->modifiers() & Qt::MetaModifier) ) ) {
		setAction(View::PANNING);
		return false;
	}

	// DRoP MODE ; explicitly do nothing
	if ( RenderingManager::getInstance()->getSourceBasketTop() ) {
		setAction(View::DROP);
		// don't interpret other mouse events in drop mode
		return false;
	}

	// if at least one source was clicked
	if ( getSourcesAtCoordinates(event->x(), viewport[3] - event->y(), QApplication::keyboardModifiers () & Qt::ShiftModifier) )
	{
		// get the top most clicked source
		Source * s =  *clickedSources.begin();
		if (!s)
			return false;

		// if there was no current source
		// OR
		// if the currently active source is NOT in the set of clicked sources,
		// then : just take the top most source clicked as current
		if ( currentSource == 0 || clickedSources.count(currentSource) == 0 )
			setCurrentSource(s);

		if ( event->button() == Qt::LeftButton ) {
			// SELECT add/remove new current from selection
			if ( currentAction == View::SELECT ) {
				View::select(currentSource);
				setCurrentSource(View::selectionSource());
			}
			else {
				quadrant = getSourceQuadrant(currentSource, event->x(), viewport[3] - event->y());
				// now manipulate the current one ; the action depends on the quadrant clicked (4 corners).
				if(quadrant == 0 || currentTool == MOVE)
					setAction(View::GRAB);
				else
					setAction(View::TOOL);
			}
		}

		// we clicked a source
		return true;
	}

	// set current to none
	RenderingManager::getInstance()->unsetCurrentSource();
	currentSource = 0;

	// back to no action
	if ( currentAction == View::SELECT )
		View::clearSelection();
	else
		setAction(View::NONE);

	return false;
}

bool GeometryView::mouseMoveEvent(QMouseEvent *event)
{
	if (!event)
		return false;

    int dx = event->x() - lastClicPos.x();
    int dy = lastClicPos.y() - event->y();
    lastClicPos = event->pos();

	// DROP MODE ; show a question mark cursor and avoid other actions
	if ( RenderingManager::getInstance()->getSourceBasketTop() ) {
		setAction(View::DROP);
		// don't interpret mouse events in drop mode
		return false;
	}

	// PANNING of the background
	if ( currentAction == View::PANNING ) {
		// SHIFT ?
		if ( QApplication::keyboardModifiers() & Qt::ShiftModifier ) {
			// special move ; move the sources in the opposite
			for(SourceSet::iterator  its = RenderingManager::getInstance()->getBegin(); its != RenderingManager::getInstance()->getEnd(); its++) {
				grabSource( *its, event->x(), viewport[3] - event->y(), -dx, -dy);
			}
		}
		panningBy(event->x(), viewport[3] - event->y(), dx, dy);
		return false;
	}

	// SELECT MODE : no motion
	// TODO : draw a rectangle to select multiple sources
	if ( currentAction == View::SELECT )
		return false;

	if ( currentSource && (currentAction == View::GRAB || currentAction == View::TOOL) ) {

		if (currentAction == View::TOOL) {
			if (currentTool == GeometryView::MOVE)
				grabSources(currentSource, event->x(), viewport[3] - event->y(), dx, dy);
			else if (currentTool == GeometryView::SCALE)
				scaleSources(currentSource, event->x(), viewport[3] - event->y(), dx, dy);
			else if (currentTool == GeometryView::CROP)
				cropSource(currentSource, event->x(), viewport[3] - event->y(), dx, dy);
			else if (currentTool == GeometryView::ROTATE) {
				rotateSource(currentSource, event->x(), viewport[3] - event->y(), dx, dy);
				setTool(currentTool);
			}
		} else
			grabSources(currentSource, event->x(), viewport[3] - event->y(), dx, dy);

		return true;
	}

	// mouse over (no buttons)
	if ( getSourcesAtCoordinates(event->x(), viewport[3] - event->y()) )
	{
		// if there was no current source
		// OR
		// if the currently active source is NOT in the set of sources under the cursor,
		// THEN
		// set quadrant to 0 (grab)
		// ELSE
		// use the current source for quadrant computation
		if ( currentSource == 0 || clickedSources.count( currentSource ) == 0 )
			quadrant = 0;
		else
			quadrant = getSourceQuadrant(currentSource, event->x(), viewport[3] - event->y());

		if(quadrant == 0 || currentTool == MOVE)
			borderType = ViewRenderWidget::border_large;
		else
			borderType = ViewRenderWidget::border_scale;

		setAction(View::OVER);
	}
	else
		setAction(View::NONE);

	return false;
}

bool GeometryView::mouseReleaseEvent ( QMouseEvent * event )
{
	if (!event)
		return false;

	if ( RenderingManager::getInstance()->getSourceBasketTop() )
		RenderingManager::getRenderingWidget()->setMouseCursor(ViewRenderWidget::MOUSE_QUESTION);
	else if (currentAction == View::PANNING )
		setAction(previousAction);
	else if (currentAction != View::SELECT ){
		if ( getSourcesAtCoordinates(event->x(), viewport[3] - event->y()) )
			setAction(View::OVER);
		else
			setAction(View::NONE);
	}

    // enforces minimal size ; check that the rescaling did not go bellow the limits and fix it
	if ( RenderingManager::getInstance()->notAtEnd( RenderingManager::getInstance()->getCurrentSource()) )
		(*RenderingManager::getInstance()->getCurrentSource())->clampScale();

	return true;
}

bool GeometryView::wheelEvent ( QWheelEvent * event ){


	float previous = zoom;
	if (QApplication::keyboardModifiers () == Qt::MetaModifier)
		setZoom (zoom + ((float) event->delta() * zoom * minzoom) / (30.0 * maxzoom) );
	else
		setZoom (zoom + ((float) event->delta() * zoom * minzoom) / (120.0 * maxzoom) );

	if (currentAction == View::GRAB || currentAction == View::TOOL ){
		deltazoom = 1.0 - (zoom / previous);
		// if there is a current source
		if ( currentSource ) {
			// manipulate the current source according to the ongoing action
			if (currentTool == MOVE || currentAction == View::GRAB)
				grabSources(currentSource, event->x(), viewport[3] - event->y(), 0, 0);
			else if (currentTool == SCALE)
				scaleSources(currentSource, event->x(), viewport[3] - event->y(), 0, 0);
			else if (currentTool == ROTATE)
				rotateSource(currentSource, event->x(), viewport[3] - event->y(), 0, 0);

		}
		// reset deltazoom
		deltazoom = 0;
	} else
		// do not show action indication (as it is likely to become invalid with view change)
		borderType = ViewRenderWidget::border_large;

	return true;
}


bool GeometryView::mouseDoubleClickEvent ( QMouseEvent * event )
{
	if (!event)
		return false;

	if (currentAction == View::DROP)
		return false;

	//	 left double click = select next source
	if ( event->buttons() & Qt::LeftButton ) {
		// get the top most clicked source
		if ( getSourcesAtCoordinates(event->x(), viewport[3] - event->y()) ) {

	    	// get the top most clicked source
	    	SourceSet::iterator clicked = clickedSources.begin();

			// if there is no source selected, select the top most
			if ( currentSource == 0 ) {
				setCurrentSource(*clicked);
				// Meta + double click = zoom best fit on current source
				if (event->modifiers () & Qt::MetaModifier)
					zoomBestFit(true);
			} // else, try to take another one bellow it
			else {
				// Meta + double click = zoom best fit on current source
				if (event->modifiers () & Qt::MetaModifier)
					zoomBestFit(true);
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
			}
		}
		// default action ; zoom best fit on whole screen
		else
			zoomBestFit(false);
	}

	return false;
}


bool GeometryView::keyPressEvent ( QKeyEvent * event ){

	// enter SELECT mode on press of control key
	if (event->modifiers() & Qt::ControlModifier){
		setAction(View::SELECT);
		return false;
	}

	SourceSet::iterator cs = RenderingManager::getInstance()->getCurrentSource();

	if (cs != RenderingManager::getInstance()->getEnd()) {
		int dx =0, dy = 0, factor = 1;
		if (event->modifiers() & Qt::MetaModifier)
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
		grabSource(*cs, 0, 0, dx, dy);

		return true;
	}

	return false;
}

bool GeometryView::keyReleaseEvent(QKeyEvent * event) {

	// leave SELECT mode
	if ( currentAction == View::SELECT )
		setAction(previousAction);

	return false;
}

void GeometryView::setTool(toolType t)
{
	currentTool = t;

	if (quadrant == 0)
		t = MOVE;

	switch (t) {
	case SCALE:
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
		}

		break;
	case CROP:
		if ( quadrant % 2 )
			RenderingManager::getRenderingWidget()->setMouseCursor(ViewRenderWidget::MOUSE_SCALE_F);
		else
			RenderingManager::getRenderingWidget()->setMouseCursor(ViewRenderWidget::MOUSE_SCALE_B);
		break;
	default:
	case MOVE:
		if (currentAction == View::TOOL  || currentAction == View::GRAB)
			RenderingManager::getRenderingWidget()->setMouseCursor(ViewRenderWidget::MOUSE_HAND_CLOSED);
		else
			RenderingManager::getRenderingWidget()->setMouseCursor(ViewRenderWidget::MOUSE_HAND_OPEN);
	}
}

void GeometryView::setAction(actionType a){

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
	}
}


void GeometryView::zoomReset() {
	setZoom(DEFAULTZOOM);
	setPanningX(0);
	setPanningY(0);
}

void GeometryView::zoomBestFit( bool onlyClickedSource ) {

	// nothing to do if there is no source
	if (RenderingManager::getInstance()->getBegin() == RenderingManager::getInstance()->getEnd()){
		zoomReset();
		return;
	}
	// 0. consider either the clicked source, either the full list
    SourceSet::iterator beginning, end;
    if (onlyClickedSource ) {

    	if (currentSource == View::selectionSource()) {
    		beginning = View::selectionBegin();
    		end = View::selectionEnd();
    	}
    	else if ( RenderingManager::getInstance()->getCurrentSource() != RenderingManager::getInstance()->getEnd() ){
    		beginning = end = RenderingManager::getInstance()->getCurrentSource();
    		end++;
    	}
    	else
    		return;

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
    	// get coordinates
		x_min = MINI (x_min, (*its)->getX() - (*its)->getScaleX());
		x_max = MAXI (x_max, (*its)->getX() + (*its)->getScaleX());
		y_min = MINI (y_min, (*its)->getY() - (*its)->getScaleY());
		y_max = MAXI (y_max, (*its)->getY() + (*its)->getScaleY());
	}

	// 2. Apply the panning to the new center
	setPanningX	( -( x_min + ABS(x_max - x_min)/ 2.0 ) );
	setPanningY	( -( y_min + ABS(y_max - y_min)/ 2.0 )  );

	// 3. get the extend of the area covered in the viewport (the matrices have been updated just above)
    double LLcorner[3];
    double URcorner[3];
    gluUnProject(viewport[0], viewport[1], 1, modelview, projection, viewport, LLcorner, LLcorner+1, LLcorner+2);
    gluUnProject(viewport[2], viewport[3], 1, modelview, projection, viewport, URcorner, URcorner+1, URcorner+2);

	// 4. compute zoom factor to fit to the boundaries
    // initial value = a margin scale of 5%
    double scalex = 0.95 * ABS(URcorner[0]-LLcorner[0]) / ABS(x_max-x_min);
    double scaley = 0.95 * ABS(URcorner[1]-LLcorner[1]) / ABS(y_max-y_min);
    // depending on the axis having the largest extend
    // apply the scaling
	setZoom( zoom * (scalex < scaley ? scalex : scaley  ));

}


bool GeometryView::getSourcesAtCoordinates(int mouseX, int mouseY, bool excludeSelection) {

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

	if ( !excludeSelection && View::hasSelection() ) {
		glPushMatrix();
        // place and scale
        glTranslated(View::selectionSource()->getX(), View::selectionSource()->getY(), 40);
        glRotated(View::selectionSource()->getRotationAngle(), 0.0, 0.0, 1.0);
        glScaled(View::selectionSource()->getScaleX(), View::selectionSource()->getScaleY(), 1.f);
        View::selectionSource()->draw(false, GL_SELECT);
        glPopMatrix();
	}

	for(SourceSet::iterator  its = RenderingManager::getInstance()->getBegin(); its != RenderingManager::getInstance()->getEnd(); its++) {
		if ((*its)->isStandby())
			continue;
		glPushMatrix();
        // place and scale
        glTranslated((*its)->getX(), (*its)->getY(), (*its)->getDepth());
        glRotated((*its)->getRotationAngle(), 0.0, 0.0, 1.0);
        glScaled((*its)->getScaleX(), (*its)->getScaleY(), 1.f);
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


	clickedSources.clear();
	while (hits != 0) {
		GLuint id = selectBuf[ (hits-1) * 4 + 3];
		if ( id == View::selectionSource()->getId() )
			clickedSources.insert(View::selectionSource());
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

    gluUnProject((GLdouble) (x - dx), (GLdouble) (y - dy),
            0.0, modelview, projection, viewport, &bx, &by, &bz);
    gluUnProject((GLdouble) x, (GLdouble) y, 0.0,
            modelview, projection, viewport, &ax, &ay, &az);

    // apply panning
    setPanningX(getPanningX() + ax - bx);
    setPanningY(getPanningY() + ay - by);
}


/**
 * Grabbing the source
 *
 * move by (dx dy)
 **/
void GeometryView::grabSource(Source *s, int x, int y, int dx, int dy) {

	if (!s) return;

	double dum;
    double bx, by; // before movement
    double ax, ay; // after  movement

    gluUnProject((GLdouble) (x - dx), (GLdouble) (y - dy),
            0.0, modelview, projection, viewport, &bx, &by, &dum);
    gluUnProject((GLdouble) x, (GLdouble) y, 0.0,
            modelview, projection, viewport, &ax, &ay, &dum);

	// take into account movement of the cursor due to zoom with scroll wheel
    ax += (ax + getPanningX()) * deltazoom;
    ay += (ay + getPanningY()) * deltazoom;

    ax = s->getX() + (ax - bx);
    ay = s->getY() + (ay - by);

    // move source
    s->moveTo(ax, ay);

}

void GeometryView::grabSources(Source *s, int x, int y, int dx, int dy) {

	if (!s) return;

	// move the source individually
	grabSource(s, x, y, dx, dy);

	// if the source is the selection, move the selection too
	if ( s == View::selectionSource() ) {
		for(SourceList::iterator  its = View::selectionBegin(); its != View::selectionEnd(); its++)
			grabSource( *its, x, y, dx, dy);
	}
	// otherwise, update selection source if we move a source of the selection
	else if (View::isInSelection(s))
		View::updateSelectionSource();

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
	gluUnProject((GLdouble) (X - dx), (GLdouble) (Y - dy),
			1.0, modelview, projection, viewport, &bx, &by, &dum);
	gluUnProject((GLdouble) X, (GLdouble) Y, 1.0,
			modelview, projection, viewport, &ax, &ay, &dum);

	// take into account movement of the cursor due to zoom with scroll wheel
	ax += (ax + getPanningX()) * deltazoom;
	ay += (ay + getPanningY()) * deltazoom;

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
	sy = option ? sx : (ay + h) / (by + h);  // proportional scaling if option is ON
	ay = h * (sy - 1.0);

    // reverse rotation to apply translation shift in the world reference
    cosa = cos(s->getRotationAngle() / 180.0 * M_PI);
    sina = sin(s->getRotationAngle() / 180.0 * M_PI);

	dum = ax * cosa - ay * sina;
	ay = ay  * cosa + ax * sina;
	ax = dum;

    s->scaleBy(sx, sy);
    s->moveTo(x + ax, y + ay);
}

void GeometryView::scaleSources(Source *s, int x, int y, int dx, int dy, bool option) {

	if (!s) return;

	// move the source individually
	scaleSource(s, x, y, dx, dy, quadrant, option);

	// if the source is the View::selection, move the selection
	if ( s == View::selectionSource() ) {

//		double sx, sy;

		for(SourceList::iterator  its = View::selectionBegin(); its != View::selectionEnd(); its++) {



			scaleSource( *its, x, y, dx, dy, getSourceQuadrant(*its, x, y));
		}
	}
	// otherwise, update selection source if we move a source of the selection
	else if (View::isInSelection(s))
		View::updateSelectionSource();

}

// in case i want to implement it : center scaling

//double x = (*currentSource)->getX();
//double y = (*currentSource)->getY();
//double cosa = cos(-(*currentSource)->getRotationAngle() / 180.0 * M_PI);
//double sina = sin(-(*currentSource)->getRotationAngle() / 180.0 * M_PI);
//
//// convert to vectors ( source center -> clic position)
//ax -= x; ay -= y;
//bx -= x; by -= y;
//
//// rotate to compute scaling into the source orientation
//dum = ax * cosa - ay * sina;
//ay = ay  * cosa + ax * sina;
//ax = dum;
//
//dum = bx * cosa - by * sina;
//by = by  * cosa + bx * sina;
//bx = dum;
//
//// Scaling
//(*currentSource)->scaleBy(ax/bx, ay/by);

/**
 * Rotation of the source
 *
 * Like for scaling, this implementation ensures that a point clicked on the source is "grabbed" by the cursor
 * and remains attached to the mouse.
 * This is not garanteed anymore when the 'option' flag is on because then it preserves the
 * scale of the source.
 *
 **/
void GeometryView::rotateSource(Source *s, int X, int Y, int dx, int dy, bool option) {

	double dum;
    double bx, by; // before movement
    double ax, ay; // after  movement

    gluUnProject((GLdouble) (X - dx), (GLdouble) (Y - dy),
            0.0, modelview, projection, viewport, &bx, &by, &dum);
    gluUnProject((GLdouble) X, (GLdouble) Y, 0.0,
            modelview, projection, viewport, &ax, &ay, &dum);

	// take into account movement of the cursor due to zoom with scroll wheel
	ax += (ax + getPanningX()) * deltazoom;
	ay += (ay + getPanningY()) * deltazoom;

    // convert to vectors ( source center -> clic position)
    double x = s->getX();
    double y = s->getY();
	ax -= x; ay -= y;
	bx -= x; by -= y;

	// scale (center scaling) if option is OFF
	if (!option) {
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
	dum += s->getRotationAngle() + 360.0;
	// modulo 360
	dum -= (double)( (int) dum / 360 ) * 360.0;

	s->setRotationAngle( ABS(dum) );

}


void GeometryView::rotateSources(Source *s, int x, int y, int dx, int dy, bool option) {

	if (!s) return;

	// move the source individually
	rotateSource(s, x, y, dx, dy, option);

	// if the source is the View::selection, move the selection
	if ( s == View::selectionSource() ) {
		for(SourceList::iterator  its = View::selectionBegin(); its != View::selectionEnd(); its++) {
			rotateSource( *its, x, y, dx, dy, option);
		}
	}
	// otherwise, update selection source if we move a source of the selection
	else if (View::isInSelection(s))
		View::updateSelectionSource();

}

// QT implementation
//    // center of rotation
//    QPointF center((*currentSource)->getX(), (*currentSource)->getY());
//    // what's the angle between lines drawn before and after movement?
//    QLineF linea(center, QPointF(ax, ay));
//    QLineF lineb(center, QPointF(bx, by));
//    double angle = linea.angle(lineb);
//    // correction of rotation direction
//    if ( (linea.dx() > 0 && linea.dy() < lineb.dy()) || (linea.dx() < 0 && linea.dy() > lineb.dy()) ||
//    		(linea.dy() > 0 && linea.dx() > lineb.dx()) || (linea.dy() < 0 && linea.dx() < lineb.dx()) )
//        angle = 360.0 - angle;
//
//    // incremental relative rotation
//    angle += (*currentSource)->getRotationAngle();
//    // modulo 360
//    angle -= (double)( (int) angle / 360 ) * 360.0;
//	(*currentSource)->setRotationAngle(angle);


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

	double dum;
    double bx, by; // before movement
    double ax, ay; // after  movement

    // get clic coordinates in Geometry view coordinate system
	gluUnProject((GLdouble) (X - dx), (GLdouble) (Y - dy),
			1.0, modelview, projection, viewport, &bx, &by, &dum);
	gluUnProject((GLdouble) X, (GLdouble) Y, 1.0,
			modelview, projection, viewport, &ax, &ay, &dum);

	// take into account movement of the cursor due to zoom with scroll wheel
	ax += (ax + getPanningX()) * deltazoom;
	ay += (ay + getPanningY()) * deltazoom;

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

	if ( quadrant > 2 )					  // BOTTOM
		h = -h;
	sy = option ? sx : (ay + h) / (by + h);  // proportional scaling if option is ON

	// translate
	double tx = w * (sx - 1.0);
	double ty = h * (sy - 1.0);

	// Crop
	QRectF tex = s->getTextureCoordinates();
	// compute texture coordinate of point clicked before movement
	double bs = bx / (2.0 * s->getScaleX());
	double bt = by / (2.0 * s->getScaleY());
	// compute texture coordinate of point clicked after movement, considering the movement changes
	double as =  ( ax + tx ) / (2.0 * s->getScaleX() * sx);
	double at =  ( ay + ty ) / (2.0 * s->getScaleY() * sy);
	// depending on the quadrant, it is not the same corner of texture coordinates to change
	if ( quadrant == 1 ) {
		tex.setLeft( tex.left() + tex.width() * (as - bs) );
		tex.setTop( tex.top() + tex.height() * (bt - at) );
	} else if ( quadrant == 2 ) {
		tex.setRight( tex.right() + tex.width() * (as - bs) );
		tex.setTop( tex.top() + tex.height() * (bt - at) );
	} else if ( quadrant == 3 ) {
		tex.setRight( tex.right() + tex.width() * (as - bs) );
		tex.setBottom( tex.bottom() + tex.height() * (bt - at) );
	} else {
		tex.setLeft( tex.left() + tex.width() * (as - bs) );
		tex.setBottom( tex.bottom() + tex.height() * (bt - at) );
	}
	s->setTextureCoordinates(tex);

    // reverse rotation to apply translation shift in the world reference
    cosa = cos(s->getRotationAngle() / 180.0 * M_PI);
    sina = sin(s->getRotationAngle() / 180.0 * M_PI);

	dum = tx * cosa - ty * sina;
	ty = ty  * cosa + tx * sina;
	tx = dum;

    s->scaleBy(sx, sy);
    s->moveTo(x + tx, y + ty);

}


/**
 *
 **/
char GeometryView::getSourceQuadrant(Source *s, int X, int Y) {
    //      ax
    //      ^
    //  ----|----
    //  | 1 | 2 |
    //  ----+---- > ay
    //  | 4 | 3 |
    //  ---------
    char quadrant = 0;
    double ax, ay, az;

    gluUnProject((GLdouble) X, (GLdouble) Y, 0.0,
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
//    if ( ABS(x) > ABS(w)  || ABS(y) > ABS(h)) {
//    	return 0;
//    }

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

	if (s && s != View::selectionSource())
		RenderingManager::getInstance()->setCurrentSource( s->getId() );
	else
		// set current to none (end of list)
		RenderingManager::getInstance()->unsetCurrentSource();

}

//char GeometryView::getBoundingRectangleQuadrant(int X, int Y){
//
//    char quadrant = 0;
//    double ax, ay, az;
//
//    gluUnProject((GLdouble) X, (GLdouble) Y, 0.0,
//            modelview, projection, viewport, &ax, &ay, &az);
//
//    // exclude mouse cursors out of the area
//    if ( ax < selectionRectangleOut[0] || ay < selectionRectangleOut[1] || ax > selectionRectangleOut[2] || ay > selectionRectangleOut[3])
//    	return -1;
//
//    // which quadrant ?
//    if ( ax > selectionRectangleOut[0] && ax < selectionRectangleIn[0] && ay > selectionRectangleOut[1] && ay < selectionRectangleIn[1] )
//    	quadrant = 4;
//    else  if ( ax > selectionRectangleOut[0] && ax < selectionRectangleIn[0] && ay < selectionRectangleOut[3] && ay > selectionRectangleIn[3] )
//    	quadrant = 1;
//    else  if ( ax < selectionRectangleOut[2] && ax > selectionRectangleIn[2] && ay > selectionRectangleOut[1] && ay < selectionRectangleIn[1] )
//    	quadrant = 3;
//    else  if ( ax < selectionRectangleOut[2] && ax > selectionRectangleIn[2] && ay < selectionRectangleOut[3] && ay > selectionRectangleIn[3] )
//    	quadrant = 2;
//
//    return quadrant;
//}

//
//void GeometryView::updateSelectionSource(SourceList l){
//
//	// prepare vars
//	GLdouble point[2], center[2], furthest[2][2];
//	GLdouble cosa, sina, d;
//	center[0] = 0;
//	center[1] = 0;
//	d = 0;
//	// compute center of all sources in the list
//	for(SourceList::iterator  its = l.begin(); its != l.end(); its++) {
//		// center
//		center[0] += (*its)->getX();
//		center[1] += (*its)->getY();
//	}
//
//	center[0] /= l.size();
//	center[1] /= l.size();
//
////	SourceList::iterator furthest;
//	for(SourceList::iterator its = l.begin(); its != l.end(); its++) {
//		cosa = cos((*its)->getRotationAngle() / 180.0 * M_PI);
//		sina = sin((*its)->getRotationAngle() / 180.0 * M_PI);
//		for (GLdouble i = -1.0; i < 2.0; i += 2.0)
//			for (GLdouble j = -1.0; j < 2.0; j += 2.0) {
//				// corner with apply rotation
//				point[0] = i * (*its)->getScaleX() * cosa - j * (*its)->getScaleY() * sina + (*its)->getX();
//				point[1] = j * (*its)->getScaleY() * cosa + i * (*its)->getScaleX() * sina + (*its)->getY();
//
//				GLdouble dist = (point[0]-center[0]) * (point[0]-center[0]) + (point[1]-center[1]) * (point[1]-center[1]);
//				if (dist > d) {
//					furthest[1][0] = furthest[0][0];
//					furthest[1][1] = furthest[0][1];
//					furthest[0][0] = point[0];
//					furthest[0][1] = point[1];
//					d = dist;
//				}
//			}
//	}
//
//	View::selectionSource()->setScaleX( ABS(furthest[0][0] - center[0]));
//	View::selectionSource()->setScaleY( ABS(furthest[0][1] - center[1]));
//
//	View::selectionSource()->setX( center[0] );
//	View::selectionSource()->setY( center[1] );
//
//}
//
//void GeometryView::setBoundingRectangle(SourceList l){
//
//	// init bbox to max size
//	selectionRectangleOut[0] = maxpanx;
//	selectionRectangleOut[1] = maxpany;
//	selectionRectangleOut[2] = -maxpanx;
//	selectionRectangleOut[3] = -maxpany;
//	// prepare vars
//	GLdouble point[2];
//	GLdouble cosa, sina;
//	// compute bounding box of all sources in the list
//	for(SourceList::iterator  its = l.begin(); its != l.end(); its++) {
//		cosa = cos((*its)->getRotationAngle() / 180.0 * M_PI);
//		sina = sin((*its)->getRotationAngle() / 180.0 * M_PI);
//		for (GLdouble i = -1.0; i < 2.0; i += 2.0)
//			for (GLdouble j = -1.0; j < 2.0; j += 2.0) {
//				// corner with apply rotation
//				point[0] = i * (*its)->getScaleX() * cosa - j * (*its)->getScaleY() * sina + (*its)->getX();
//				point[1] = j * (*its)->getScaleY() * cosa + i * (*its)->getScaleX() * sina + (*its)->getY();
//				// keep max and min
//				selectionRectangleOut[0] = qMin(point[0], selectionRectangleOut[0]);
//				selectionRectangleOut[1] = qMin(point[1], selectionRectangleOut[1]);
//				selectionRectangleOut[2] = qMax(point[0], selectionRectangleOut[2]);
//				selectionRectangleOut[3] = qMax(point[1], selectionRectangleOut[3]);
//			}
//	}
//	selectionRectangleOut[0] -= zoom;
//	selectionRectangleOut[1] -= zoom;
//	selectionRectangleOut[2] += zoom;
//	selectionRectangleOut[3] += zoom;
//
//	selectionRectangleIn[0] = selectionRectangleOut[0] + (1.0 - BORDER_SIZE ) * 0.5  * (selectionRectangleOut[2] - selectionRectangleOut[0]);
//	selectionRectangleIn[1] = selectionRectangleOut[1] + (1.0 - BORDER_SIZE ) * 0.5  * (selectionRectangleOut[3] - selectionRectangleOut[1]);
//	selectionRectangleIn[2] = selectionRectangleOut[2] - (1.0 - BORDER_SIZE ) * 0.5  * (selectionRectangleOut[2] - selectionRectangleOut[0]);
//	selectionRectangleIn[3] = selectionRectangleOut[3] - (1.0 - BORDER_SIZE ) * 0.5  * (selectionRectangleOut[3] - selectionRectangleOut[1]);
//
//}

