/*
 * LayersView.cpp
 *
 *  Created on: Feb 26, 2010
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

#include "LayersView.h"

#include "common.h"
#include "RenderingManager.h"
#include "ViewRenderWidget.h"
#include "OutputRenderWindow.h"

#define MINZOOM 5.0
#define DEFAULTZOOM 7.0
#define MAXDISPLACEMENT 1.6
#define MIN_LOOKAT 3.0
#define MAX_LOOKAT 9.0
#define DEFAULT_LOOKAT 4.0
#define DEFAULT_PANNING -2.f, 0.f, 0.f

LayersView::LayersView(): lookatdistance(DEFAULT_LOOKAT), forwardDisplacement(0) {

	zoom = DEFAULTZOOM;
	minzoom = MINZOOM;
	maxzoom = MAX_DEPTH_LAYER;
	maxpanx = lookatdistance;
	maxpany = lookatdistance;
	maxpanz = lookatdistance;
	zoomReset();
	currentAction = View::NONE;

	icon.load(QString::fromUtf8(":/glmixer/icons/depth.png"));
    title = " Layers view";
}


void LayersView::setModelview()
{
	View::setModelview();
    glTranslatef(getPanningX(), getPanningY(), getPanningZ());
    gluLookAt(lookatdistance, lookatdistance, lookatdistance + zoom, 0.0, 0.0, zoom, 0.0, 1.0, 0.0);
    glGetDoublev(GL_MODELVIEW_MATRIX, modelview);
}



void LayersView::paint()
{
	static bool first = true;

    // First the background stuff
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glBlendEquation(GL_FUNC_ADD);
    glCallList(ViewRenderWidget::layerbg);


	glPushMatrix();

	double renderingAspectRatio = OutputRenderWindow::getInstance()->getAspectRatio();
	if ( renderingAspectRatio < 1.0)
		glScaled(1.0 / SOURCE_UNIT , 1.0 / (renderingAspectRatio * SOURCE_UNIT),  1.0 / SOURCE_UNIT);
	else
		glScaled(renderingAspectRatio /  SOURCE_UNIT, 1.0 / SOURCE_UNIT,  1.0 / SOURCE_UNIT);
	glCallList(ViewRenderWidget::quad_window[RenderingManager::getInstance()->clearToWhite()?1:0]);
    glCallList(ViewRenderWidget::frame_screen_thin);
	glPopMatrix();

    // Second the icons of the sources (reversed depth order)
    // render in the depth order
    if (ViewRenderWidget::program->bind()) {

		first = true;
		for(SourceSet::iterator  its = RenderingManager::getInstance()->getBegin(); its != RenderingManager::getInstance()->getEnd(); its++) {

			if ((*its)->isStandby())
				continue;
			//
			// 1. Render it into current view
			//
			glPushMatrix();

			// if the source is active or part of the selection which is active
			if ( forwardSources.count(*its) > 0 ) {
				// animated displacement
				if (forwardDisplacement < MAXDISPLACEMENT)
					forwardDisplacement += ( MAXDISPLACEMENT + 0.1 - forwardDisplacement) * 10.f / RenderingManager::getRenderingWidget()->getFramerate();
				glTranslatef( forwardDisplacement, 0.0, 0.0);
			}
			else {
				// draw stippled version of the source on top
				glEnable(GL_POLYGON_STIPPLE);
				glPolygonStipple(ViewRenderWidget::stippling + ViewRenderWidget::stipplingMode * 128);
			}

			glTranslatef( 0.0, 0.0,  1.0 + (*its)->getDepth());
	        glScalef((*its)->getAspectRatio(), 1.0, 1.0);

			// standard transparency blending
			glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
			glBlendEquation(GL_FUNC_ADD);

			ViewRenderWidget::setSourceDrawingMode(false);
			// draw border (larger if active)
			if (RenderingManager::getInstance()->isCurrentSource(its))
				glCallList(ViewRenderWidget::border_large_shadow + ((*its)->isModifiable() ? 0 :2));
			else
				glCallList(ViewRenderWidget::border_thin_shadow + ((*its)->isModifiable() ? 0 :2));

			// draw border for selection
			if (View::isInSelection(*its))
				glCallList(ViewRenderWidget::frame_selection);

			ViewRenderWidget::setSourceDrawingMode(true);

			// Blending Function for mixing like in the rendering window
			(*its)->beginEffectsSection();
			// bind the source texture and update its content
			(*its)->update();

			// draw surface
			(*its)->blend();
			(*its)->draw();

			// draw stippled version of the source on top
//			glEnable(GL_POLYGON_STIPPLE);
//			glPolygonStipple(ViewRenderWidget::stippling + ViewRenderWidget::stipplingMode * 128);
			(*its)->draw(false);
			glDisable(GL_POLYGON_STIPPLE);

			glPopMatrix();

			//
			// 2. Render it into FBO
			//
			RenderingManager::getInstance()->renderToFrameBuffer(*its, first);
			first = false;

		}
		ViewRenderWidget::program->release();
    }

    // restore state
	glActiveTexture(GL_TEXTURE0);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glBlendEquation(GL_FUNC_ADD);

	// if no source was rendered, clear anyway
	RenderingManager::getInstance()->renderToFrameBuffer(0, first, true);

	// post render draw (loop back and recorder)
	RenderingManager::getInstance()->postRenderToFrameBuffer();

    // the source dropping icon
    Source *s = RenderingManager::getInstance()->getSourceBasketTop();
    if ( s ){
    	double depth = 0.0, dumm = 0.0;
    	unProjectDepth(lastClicPos.x(), lastClicPos.y(), 0.0, 0.0, &depth, &dumm);

		glPushMatrix();
		forwardDisplacement = MAXDISPLACEMENT;
		glTranslated( forwardDisplacement, 0.0, 1.0 + depth);
			glPushMatrix();
			glTranslated( s->getAspectRatio(), -0.9, 0.0);
	        glScalef(0.1, 0.1, 1.0);
			for (int i = 1; i < RenderingManager::getInstance()->getSourceBasketSize(); ++i ) {
				glTranslated( 2.1, 0.0, 0.0);
				glCallList(ViewRenderWidget::border_thin);
			}
			glPopMatrix();
        glScalef(s->getAspectRatio(), 1.0, 1.0);
		glCallList(ViewRenderWidget::border_thin);
		glPopMatrix();
    }


}

void LayersView::resize(int w, int h)
{
	View::resize(w, h);
	glViewport(0, 0, viewport[2], viewport[3]);

    // Setup specific projection and view for this window
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(50.0f, (float)  viewport[2] / (float)  viewport[3], 0.1f, lookatdistance * 10.0f);

	glGetDoublev(GL_PROJECTION_MATRIX, projection);

}


void LayersView::setAction(ActionType a){

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
	}
}

void LayersView::bringForward(Source *s)
{
	//reset forward if the source is not already in
	if (forwardSources.count(s) == 0)
		forwardDisplacement = 0;

	// if the source is part of a selection, set the whole selection to be forward
	if (!individual && (View::isInSelection(s) && View::isInSelection(*RenderingManager::getInstance()->getCurrentSource())) )
		forwardSources = View::copySelection();
	else {
		// else only this source is forward
		forwardSources = SourceList();
		forwardSources.insert(s);
	}
}

bool LayersView::mousePressEvent(QMouseEvent *event)
{
	lastClicPos = event->pos();

	// MIDDLE BUTTON ; panning cursor
	if ( isUserInput(event, INPUT_NAVIGATE) ||  isUserInput(event, INPUT_DRAG) ) {
		setAction(View::PANNING);
		return false;
	}

	// DRoP MODE ; explicitly do nothing
	if ( RenderingManager::getInstance()->getSourceBasketTop() ) {
		setAction(View::DROP);
		// don't interpret other mouse events in drop mode
		if (isUserInput(event, INPUT_CONTEXT_MENU))
			RenderingManager::getRenderingWidget()->showContextMenu(ViewRenderWidget::CONTEXT_MENU_DROP, event->pos());
		return false;
	}

	// if at least one source icon was clicked
	if (getSourcesAtCoordinates(event->x(), viewport[3] - event->y()) ) {

    	// get the top most clicked source
    	Source *clicked = *clickedSources.begin();
    	if (!clicked)
    		return false;

		// CTRL clic = add/remove from selection
		if ( isUserInput(event, INPUT_SELECT) )
			View::select(clicked);
		// else not SELECTION ; normal action
		else {
			// not in selection (SELECT) action mode, then set the current active source
			RenderingManager::getInstance()->setCurrentSource( clicked->getId() );
			// tool
			individual = isUserInput(event, INPUT_TOOL_INDIVIDUAL);
			if ( isUserInput(event, INPUT_TOOL) || individual ) {
				// ready for grabbing the current source
				if ( clicked->isModifiable() ){
					// put this source in the forward list (single source if SHIFT)
					bringForward(clicked);
					// ready for grabbing the current source
					setAction(View::GRAB);
				}
			}
			// context menu
			else if ( isUserInput(event, INPUT_CONTEXT_MENU) )
				RenderingManager::getRenderingWidget()->showContextMenu(ViewRenderWidget::CONTEXT_MENU_SOURCE, event->pos());
			// zoom
			else if ( isUserInput(event, INPUT_ZOOM) )
				zoomBestFit(true);

		}

		return true;
    }

	// clicked in the background

	// set current source to none (end of list)
	RenderingManager::getInstance()->unsetCurrentSource();
	// clear the list of sources forward
	forwardSources.clear();

	// back to no action
	if ( currentAction == View::SELECT )
		View::clearSelection();
	else
		setAction(View::NONE);

	// context menu
	if ( isUserInput(event, INPUT_CONTEXT_MENU) )
		RenderingManager::getRenderingWidget()->showContextMenu(ViewRenderWidget::CONTEXT_MENU_VIEW, event->pos());
	// zoom
	else if ( isUserInput(event, INPUT_ZOOM) )
		zoomBestFit(false);

	return false;
}

bool LayersView::mouseMoveEvent(QMouseEvent *event)
{
    int dx = event->x() - lastClicPos.x();
    int dy = lastClicPos.y() - event->y();
    lastClicPos = event->pos();

    // DROP MODE : avoid other actions
	if ( RenderingManager::getInstance()->getSourceBasketTop() ) {
		setAction(View::DROP);
		// don't interpret mouse events in drop mode
		return false;
	}

	//  PANNING ; move the background
	if ( currentAction == View::PANNING ) {
		// move the view
		panningBy(event->x(), event->y(), dx, dy);
		return false;
	}

	// SELECT MODE : no motion
	// TODO : draw a rectangle to select multiple sources
	if ( currentAction == View::SELECT )
		return false;

	// LEFT BUTTON : grab
	if ( isUserInput(event, INPUT_TOOL) || isUserInput(event, INPUT_TOOL_INDIVIDUAL) ) {

		// keep the iterator of the current source under the shoulder ; it will be used
		SourceSet::iterator cs = RenderingManager::getInstance()->getCurrentSource();

		// if there is a current source, grab it (with other forward sources)
		if ( currentAction == View::GRAB && RenderingManager::getInstance()->isValid(cs))
			grabSources( *cs, event->x(), event->y(), dx, dy);

		return true;
	}

	// mouse over (no buttons)
	// Show mouse over cursor only if no user input
	if ( isUserInput(event, INPUT_NONE)) {
		//  change action cursor if over a source
		if ( getSourcesAtCoordinates(event->x(), viewport[3] - event->y(), false) )
			setAction(View::OVER);
		else
			setAction(View::NONE);
	}

	return false;
}

bool LayersView::mouseReleaseEvent ( QMouseEvent * event ){

	// restore action mode
	if ( RenderingManager::getInstance()->getSourceBasketTop() )
		RenderingManager::getRenderingWidget()->setMouseCursor(ViewRenderWidget::MOUSE_QUESTION);
	else if (currentAction == View::GRAB || currentAction == View::DROP)
		setAction(View::OVER);
	else if (currentAction == View::PANNING )
		setAction(previousAction);

	return true;
}


bool LayersView::wheelEvent ( QWheelEvent * event ){

	bool ret = false;
    lastClicPos = event->pos();

	float previous = zoom;
	setZoom (zoom + ((float) event->delta() * zoom * minzoom) / (-2.0 * View::zoomSpeed() * maxzoom) );

	if (currentAction == View::GRAB) {
		deltax = zoom - previous;

		// simulate a movement of the mouse
		QMouseEvent *e = new QMouseEvent(QEvent::MouseMove, event->pos(), Qt::NoButton, qtMouseButtons(INPUT_TOOL), qtMouseModifiers(INPUT_TOOL));
		ret = mouseMoveEvent(e);
		delete e;

		// reset deltazoom
		deltax = 0;
	}

	return ret;
}

void LayersView::zoomReset() {

	lookatdistance = DEFAULT_LOOKAT;
	setZoom(DEFAULTZOOM);
	setPanning(DEFAULT_PANNING);

}

void LayersView::zoomBestFit( bool onlyClickedSource ) {

	// nothing to do if there is no source
	if (RenderingManager::getInstance()->empty() ){
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

	// Compute bounding depths of every sources
    double z_min = 10000, z_max = -10000;
	for(SourceSet::iterator  its = beginning; its != end; its++) {

		if ((*its)->isStandby())
			continue;

		z_min = MINI (z_min, (*its)->getDepth());
		z_max = MAXI (z_max, (*its)->getDepth());
	}

	setZoom	( z_max + 1.0);

	// TODO : LayersView::zoomBestFit() also adjust panning
}


bool LayersView::keyPressEvent ( QKeyEvent * event ){

	// detect select mode
	if ( !(QApplication::keyboardModifiers() ^ View::qtMouseModifiers(INPUT_SELECT)) ){
		setAction(View::SELECT);
		return true;
	}

	SourceSet::iterator currentSource = RenderingManager::getInstance()->getCurrentSource();
	if (currentSource != RenderingManager::getInstance()->getEnd()) {
		double dz = 0.0, newdepth = 0.0;

		switch (event->key()) {
			case Qt::Key_Down:
			case Qt::Key_Left:
				dz = 1.0;
				break;
			case Qt::Key_Up:
			case Qt::Key_Right:
				dz = -1.0;
				break;
			default:
				return false;
		}

		// move all if not in individual mode
		if (!individual) {
			// move all the source placed forward
			for(SourceList::iterator its = forwardSources.begin(); its != forwardSources.end(); its++) {
				if ( (*its)->getId() == (*currentSource)->getId())
					continue;
				newdepth =  (*its)->getDepth() + dz;
				RenderingManager::getInstance()->changeDepth(RenderingManager::getInstance()->getById((*its)->getId()), newdepth > 0 ? newdepth : 0.0);
			}
		}

		// change depth of current source
		newdepth =  (*currentSource)->getDepth() + dz;
		currentSource = RenderingManager::getInstance()->changeDepth(currentSource, newdepth > 0 ? newdepth : 0.0);
		// we need to set current again
		RenderingManager::getInstance()->setCurrentSource(currentSource);

		return true;
	}

	return false;
}

bool LayersView::keyReleaseEvent(QKeyEvent * event) {

	if ( currentAction == View::SELECT && !(QApplication::keyboardModifiers() & View::qtMouseModifiers(INPUT_SELECT)) )
		setAction(previousAction);

	return false;
}

bool LayersView::getSourcesAtCoordinates(int mouseX, int mouseY, bool clic) {

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

		if ((*its)->isStandby())
			continue;

		glPushMatrix();
        // place and scale
		if ( forwardSources.count(*its) > 0 )
	        glTranslatef(forwardDisplacement, 0.0,  1.0 +(*its)->getDepth());
		else
			glTranslatef(0.0, 0.0,  1.0 +(*its)->getDepth());
        glScalef((*its)->getAspectRatio(), 1.0, 1.0);

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


void LayersView::coordinatesFromMouse(int mouseX, int mouseY, double *X, double *Y){

	unProjectDepth(mouseX, mouseY, 0.0, 0.0, X, Y);

}

void LayersView::unProjectDepth(int x, int y, int dx, int dy, double *depth, double *depthBeforeDelta){

	// Y correction between Qt and OpenGL coordinates
	y = viewport[3] - y;

	// in a perspective, we need to know the pseudo depth of the object of interest in order
	// to use gluUnproject ; this is obtained by a quick pseudo rendering in FEEDBACK mode

    // feedback rendering to determine a depth
    GLfloat feedbuffer[4];
    glFeedbackBuffer(4, GL_3D, feedbuffer);
    (void) glRenderMode(GL_FEEDBACK);

    // Fake rendering of point (0,0,0)
    glBegin(GL_POINTS);
    glVertex3f(0, 0, zoom);
    glEnd();

    // dummy vars
	double bx, by, ax, ay;

    // we can make the un-projection if we got the 4 values we need :
    if (glRenderMode(GL_RENDER) == 4) {
		gluUnProject((GLdouble) (x - dx), (GLdouble) (y - dy),
				feedbuffer[3], modelview, projection, viewport, &bx, &by, depthBeforeDelta);
		gluUnProject((GLdouble) x, (GLdouble) y, feedbuffer[3],
				modelview, projection, viewport, &ax, &ay, depth);

    }
    // otherwise compute with a depth of 1.0 (this not correct but should never happen)
    else {
		gluUnProject((GLdouble) (x - dx), (GLdouble) (y - dy),
				1.0, modelview, projection, viewport, &bx, &by, depthBeforeDelta);
		gluUnProject((GLdouble) x, (GLdouble) y, 1.0,
				modelview, projection, viewport, &ax, &ay, depth);
    }

}

/**
 *
 **/
void LayersView::grabSource(Source *s, int x, int y, int dx, int dy, bool setcurrent) {

	if (!s || !s->isModifiable()) return;

	double bz = 0.0; // depth before delta movement
	double az = 0.0; // depth at current x and y

	unProjectDepth(x, y, dx, dy, &az, &bz);

    // (az-bz) is the depth change caused by the mouse mouvement
    // deltazoom is the depth change due to zooming in/out while grabbing
    double newdepth = s->getDepth() +  az - bz  +  deltax;
    SourceSet::iterator currentSource = RenderingManager::getInstance()->getById(s->getId());
	currentSource = RenderingManager::getInstance()->changeDepth(currentSource, newdepth > 0 ? newdepth : 0.0);

	// if we need to set current again
	if (setcurrent)
		RenderingManager::getInstance()->setCurrentSource(currentSource);

}


void LayersView::grabSources(Source *s, int x, int y, int dx, int dy)
{
	// move all the source placed forward
	for(SourceList::iterator its = forwardSources.begin(); its != forwardSources.end(); its++) {
		grabSource( *its, x, y, dx, dy, (*its)->getId() == s->getId());
		s = *RenderingManager::getInstance()->getCurrentSource();
	}
}

/**
 *
 **/
void LayersView::panningBy(int x, int y, int dx, int dy) {

	// Y correction between Qt and OpenGL coordinates
	y = viewport[3] - y;

    // feedback rendering to determine a depth
    GLfloat feedbuffer[4];
    glFeedbackBuffer(4, GL_3D, feedbuffer);
    (void) glRenderMode(GL_FEEDBACK);

    // Fake rendering of point (0,0,0)
    glBegin(GL_POINTS);
    glVertex3f(0, 0, lookatdistance);
    glEnd();

    // dummy vars
	double bx, by, ax, ay, bz, az;

    // we can make the un-projection if we got the 4 values we need :
    if (glRenderMode(GL_RENDER) == 4) {
		gluUnProject((GLdouble) (x - dx), (GLdouble) (y - dy),
				feedbuffer[3], modelview, projection, viewport, &bx, &by, &bz);
		gluUnProject((GLdouble) x, (GLdouble) y, feedbuffer[3],
				modelview, projection, viewport, &ax, &ay, &az);

    }
    // otherwise compute with a depth of 1.0 (this not correct but should never happen)
    else {
		gluUnProject((GLdouble) (x - dx), (GLdouble) (y - dy),
				1.0, modelview, projection, viewport, &bx, &by, &bz);
		gluUnProject((GLdouble) x, (GLdouble) y, 1.0,
				modelview, projection, viewport, &ax, &ay, &az);
    }

	// apply panning
	setPanning(getPanningX() + ax - bx, getPanningY() + ay - by, getPanningZ() + az - bz);

	// adjust the looking distance when panning in the Z axis (diagonal)
	lookatdistance = CLAMP( lookatdistance + az - bz, MIN_LOOKAT, MAX_LOOKAT);

}


