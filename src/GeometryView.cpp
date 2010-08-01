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

GeometryView::GeometryView() : View(), quadrant(0), currentTool(SCALE)
{
	zoom = DEFAULTZOOM;
	minzoom = MINZOOM;
	maxzoom = MAXZOOM;
	maxpanx = SOURCE_UNIT*MAXZOOM*2.0;
	maxpany = SOURCE_UNIT*MAXZOOM*2.0;
	currentAction = View::NONE;

	borderType = ViewRenderWidget::border_large;

    icon.load(QString::fromUtf8(":/glmixer/icons/manipulation.png"));
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
	static QColor clearColor;

    // first the black background (as the rendering black clear color) with shadow
	glPushMatrix();
    glScalef( OutputRenderWindow::getInstance()->getAspectRatio(), 1.0, 1.0);
    glCallList(ViewRenderWidget::quad_window[RenderingManager::getInstance()->clearToWhite()?1:0]);
    glPopMatrix();

    bool first = true;
    // then the icons of the sources (reversed depth order)
	for(SourceSet::iterator  its = RenderingManager::getInstance()->getBegin(); its != RenderingManager::getInstance()->getEnd(); its++) {
		//
		// 1. Render it into current view
		//
        // place and scale
        glPushMatrix();
        glTranslated((*its)->getX(), (*its)->getY(), (*its)->getDepth());
        glRotated((*its)->getRotationAngle(), 0.0, 0.0, 1.0);
        glScaled((*its)->getScaleX(), (*its)->getScaleY(), 1.f);

        // draw border and handles if active
		if ((*its)->isActive())
	        glCallList(borderType);
		else
			glCallList(ViewRenderWidget::border_thin);

	    // Blending Function For mixing like in the rendering window
        (*its)->beginEffectsSection();
		// bind the source texture and update its content
		(*its)->update();
		// test for culling
        (*its)->testCulling();
        // Draw it !
		(*its)->blend();
        (*its)->draw();

        glPopMatrix();

		//
		// 2. Render it into FBO
		//
        RenderingManager::getInstance()->renderToFrameBuffer(*its, first);
        first = false;

    }
	// if no source was rendered, clear anyway
	if (first)
		RenderingManager::getInstance()->renderToFrameBuffer(0, first);
	else
		// fill-in the loopback buffer
		RenderingManager::getInstance()->updatePreviousFrame();

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

//    if (currentAction == View::TOOL) {
//		glDisable(GL_TEXTURE_2D);
//		glPointSize(10.0);
//		glColor3d(1.0, 0.0, 0.0);
//		glBegin(GL_LINES);
//		glVertex2d(tmp[0], tmp[1]);
//		glVertex2d(tmp[2], tmp[3]);
//		glEnd();
//    }
}


void GeometryView::resize(int w, int h)
{
	View::resize(w, h);
	glViewport(0, 0, viewport[2], viewport[3]);

    // Setup specific projection and view for this window
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();

    if (w > h)
         glOrtho(-SOURCE_UNIT* (double) viewport[2] / (double) viewport[3], SOURCE_UNIT*(double) viewport[2] / (double) viewport[3], -SOURCE_UNIT, SOURCE_UNIT, -MAX_DEPTH_LAYER, 10.0);
     else
         glOrtho(-SOURCE_UNIT, SOURCE_UNIT, -SOURCE_UNIT*(double) viewport[3] / (double) viewport[2], SOURCE_UNIT*(double) viewport[3] / (double) viewport[2], -MAX_DEPTH_LAYER, 10.0);

	glGetDoublev(GL_PROJECTION_MATRIX, projection);
}


bool GeometryView::mousePressEvent(QMouseEvent *event)
{
	lastClicPos = event->pos();

	if (event->buttons() & Qt::MidButton) {
		RenderingManager::getRenderingWidget()->setMouseCursor(ViewRenderWidget::MOUSE_SIZEALL);
	}
	else if ( RenderingManager::getInstance()->getSourceBasketTop() ) {
		RenderingManager::getRenderingWidget()->setMouseCursor(ViewRenderWidget::MOUSE_QUESTION);
		// don't interpret other mouse events in drop mode
		return false;
	}
	else if ( getSourcesAtCoordinates(event->x(), viewport[3] - event->y()) ) { // if at least one source icon was clicked

    	// get the top most clicked source
    	SourceSet::iterator clicked = clickedSources.begin();

    	// for LEFT button clic : manipulate only the top most or the newly clicked
    	if (event->buttons() & Qt::LeftButton) {

    		// if there was no current source, its simple : just take the top most source clicked now
    		// OR
			// if the currently active source is NOT in the set of clicked sources,
			if ( RenderingManager::getInstance()->getCurrentSource() == RenderingManager::getInstance()->getEnd()
				|| clickedSources.count(*RenderingManager::getInstance()->getCurrentSource() ) == 0 )
    			//  make the top most source clicked now the newly current one
    			RenderingManager::getInstance()->setCurrentSource( (*clicked)->getId() );

			// now manipulate the current one ; the action depends on the quadrant clicked (4 corners).
			if(quadrant == 0 || currentTool == MOVE) {
				setAction(View::GRAB);
				RenderingManager::getRenderingWidget()->setMouseCursor(ViewRenderWidget::MOUSE_HAND_CLOSED);
			} else {
				setAction(View::TOOL);
			}

    	}
    	// for RIGHT button clic : switch the currently active source to the one bellow, if exists
    	else if (event->buttons() & Qt::RightButton) {

    		// if there is no source selected, select the top most
    		if ( RenderingManager::getInstance()->getCurrentSource() == RenderingManager::getInstance()->getEnd() )
    			RenderingManager::getInstance()->setCurrentSource( (*clicked)->getId() );
    		// else, try to take another one bellow it
    		else {
    			// find where the current source is in the clickedSources
    			clicked = clickedSources.find(*RenderingManager::getInstance()->getCurrentSource()) ;
    			// decrement the clicked iterator forward in the clickedSources (and jump back to end when at begining)
    			if ( clicked == clickedSources.begin() )
    				clicked = clickedSources.end();
				clicked--;

				// set this newly clicked source as the current one
    			RenderingManager::getInstance()->setCurrentSource( (*clicked)->getId() );
    			// update quadrant to match newly current source
    			quadrant = getSourceQuadrant(RenderingManager::getInstance()->getCurrentSource(), event->x(), viewport[3] - event->y());
    		}
    	}
    } else
		// set current to none (end of list)
		RenderingManager::getInstance()->setCurrentSource( RenderingManager::getInstance()->getEnd() );

	return true;
}

bool GeometryView::mouseMoveEvent(QMouseEvent *event)
{

    int dx = event->x() - lastClicPos.x();
    int dy = lastClicPos.y() - event->y();
    lastClicPos = event->pos();

	// MIDDLE button ; panning
	if (event->buttons() & Qt::MidButton) {

		panningBy(event->x(), viewport[3] - event->y(), dx, dy);

	}
	// DROP MODE ; show a question mark cursor and avoid other actions
	else if ( RenderingManager::getInstance()->getSourceBasketTop() ) {

		RenderingManager::getRenderingWidget()->setMouseCursor(ViewRenderWidget::MOUSE_QUESTION);
		// don't interpret mouse events in drop mode
		return false;

	}
	// LEFT button : use TOOL on the current source
	else if (event->buttons() & Qt::LeftButton) {
		// keep the iterator of the current source under the shoulder ; it will be used
		SourceSet::iterator cs = RenderingManager::getInstance()->getCurrentSource();
		if ( RenderingManager::getInstance()->notAtEnd(cs)) {

			if (currentAction == View::TOOL) {
				if (currentTool == MOVE)
					grabSource(cs, event->x(), viewport[3] - event->y(), dx, dy);
				else if (currentTool == SCALE)
					scaleSource(cs, event->x(), viewport[3] - event->y(), dx, dy, QApplication::keyboardModifiers () == Qt::ShiftModifier);
				else if (currentTool == ROTATE) {
					rotateSource(cs, event->x(), viewport[3] - event->y(), dx, dy, QApplication::keyboardModifiers () == Qt::ShiftModifier);
					setTool(currentTool);
				}
			} else if (currentAction == View::GRAB) {
				grabSource(cs, event->x(), viewport[3] - event->y(), dx, dy);
			}
		}
		return true;

	} else if (event->buttons() & Qt::RightButton) {

		// TODO : implement right-move = action on a single element of the selection group

	} else  { // mouse over (no buttons)

		if ( getSourcesAtCoordinates(event->x(), viewport[3] - event->y()) ) {

    		// if there was no current source
    		// OR
			// if the currently active source is NOT in the set of sources under the cursor,
			// THEN
			// set quadrant to 0 (grab)
			// ELSE
			// use the current source for quadrant computation
			if ( RenderingManager::getInstance()->getCurrentSource() == RenderingManager::getInstance()->getEnd()
				|| clickedSources.count(*RenderingManager::getInstance()->getCurrentSource() ) == 0 )
//				quadrant = getSourceQuadrant(clickedSources.begin(), event->x(), viewport[3] - event->y());
				quadrant = 0;
			else
				quadrant = getSourceQuadrant(RenderingManager::getInstance()->getCurrentSource(), event->x(), viewport[3] - event->y());

			if(quadrant == 0 || currentTool == MOVE)
				borderType = ViewRenderWidget::border_large;
			else
				borderType = ViewRenderWidget::border_scale;

			setAction(View::OVER);

		} else
			setAction(View::NONE);

	}

	return false;
}

bool GeometryView::mouseReleaseEvent ( QMouseEvent * event ){

	if ( RenderingManager::getInstance()->getSourceBasketTop() )
		RenderingManager::getRenderingWidget()->setMouseCursor(ViewRenderWidget::MOUSE_QUESTION);
	else {
		if ( getSourcesAtCoordinates(event->x(), viewport[3] - event->y()) )
			setAction(View::OVER);
		else
			setAction(View::NONE);
	}

    // enforces minimal size ; check that the rescaling did not go bellow the limits and fix it
	if ( RenderingManager::getInstance()->notAtEnd( RenderingManager::getInstance()->getCurrentSource()) ) {
		(*RenderingManager::getInstance()->getCurrentSource())->clampScale();
	}

	return true;
}

bool GeometryView::wheelEvent ( QWheelEvent * event ){


	float previous = zoom;
	if (QApplication::keyboardModifiers () == Qt::ControlModifier)
		setZoom (zoom + ((float) event->delta() * zoom * minzoom) / (30.0 * maxzoom) );
	else
		setZoom (zoom + ((float) event->delta() * zoom * minzoom) / (120.0 * maxzoom) );

	if (currentAction == View::GRAB || currentAction == View::TOOL ){
		deltazoom = 1.0 - (zoom / previous);
		// keep the iterator of the current source under the shoulder ; it will be used
		SourceSet::iterator cs = RenderingManager::getInstance()->getCurrentSource();
		if ( RenderingManager::getInstance()->notAtEnd(cs)) {
			// manipulate the current source according to the operation detected when clicking
			if (currentTool == MOVE || currentAction == View::GRAB)
				grabSource(cs, event->x(), viewport[3] - event->y(), 0, 0);
			else if (currentTool == SCALE)
				scaleSource(cs, event->x(), viewport[3] - event->y(), 0, 0, QApplication::keyboardModifiers () == Qt::ShiftModifier);
			else if (currentTool == ROTATE)
				rotateSource(cs, event->x(), viewport[3] - event->y(), 0, 0, QApplication::keyboardModifiers () == Qt::ShiftModifier);

		}
		// reset deltazoom
		deltazoom = 0;
	} else {
		// do not show action indication (as it is likely to become invalid with view change)
		borderType = ViewRenderWidget::border_large;
		setAction(View::NONE);
	}



	return true;
}


bool GeometryView::mouseDoubleClickEvent ( QMouseEvent * event ){


	// TODO :  for LEFT double button clic alrernate group / selection
	// for LEFT double button clic : expand the current source to the rendering area
	if ( (event->buttons() & Qt::LeftButton) && getSourcesAtCoordinates(event->x(), viewport[3] - event->y()) ) {

		if ( RenderingManager::getInstance()->getCurrentSource() != RenderingManager::getInstance()->getEnd()){
			(*RenderingManager::getInstance()->getCurrentSource())->resetScale();
		} else
			zoomBestFit();

	}

	return true;
}


//bool GeometryView::keyPressEvent ( QKeyEvent * event ){
//
//	switch (event->key()) {
//		case Qt::Key_Left:
//			return true;
//		case Qt::Key_Right:
//			return true;
//		case Qt::Key_Down:
//			return true;
//		case Qt::Key_Up:
//			return true;
//		default:
//			return false;
//	}
//}

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
		// TODO implement crop
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
		RenderingManager::getRenderingWidget()->setMouseCursor(ViewRenderWidget::MOUSE_HAND_OPEN);
		break;
	case View::OVER:
	case View::TOOL:
		setTool(currentTool);
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

void GeometryView::zoomBestFit() {

	// nothing to do if there is no source
	if (RenderingManager::getInstance()->getBegin() == RenderingManager::getInstance()->getEnd()){
		zoomReset();
		return;
	}

	// 1. compute bounding box of every sources
    double x_min = 10000, x_max = -10000, y_min = 10000, y_max = -10000;
	for(SourceSet::iterator  its = RenderingManager::getInstance()->getBegin(); its != RenderingManager::getInstance()->getEnd(); its++) {
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
    double scale = 0.95;
    // depending on the axis having the largest extend
    if ( ABS(x_max-x_min) > ABS(y_max-y_min))
    	scale *= ABS(URcorner[0]-LLcorner[0]) / ABS(x_max-x_min);
    else
    	scale *= ABS(URcorner[1]-LLcorner[1]) / ABS(y_max-y_min);
    // apply the scaling
	setZoom( zoom * scale );

}


bool GeometryView::getSourcesAtCoordinates(int mouseX, int mouseY) {

	// prepare variables
	clickedSources.clear();
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

    while (hits != 0) {
    	clickedSources.insert( *(RenderingManager::getInstance()->getById (selectBuf[ (hits-1) * 4 + 3])) );
    	hits--;
    }

    return !clickedSources.empty();
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


void GeometryView::coordinatesFromMouse(int mouseX, int mouseY, double *X, double *Y){

	double dum;

	gluUnProject((GLdouble) mouseX, (GLdouble) (viewport[3] - mouseY), 0.0,
	            modelview, projection, viewport, X, Y, &dum);

}


/**
 *
 **/
void GeometryView::grabSource(SourceSet::iterator currentSource, int x, int y, int dx, int dy) {

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

    ax = (*currentSource)->getX() + (ax - bx);
    ay = (*currentSource)->getY() + (ay - by);

    // move source
    (*currentSource)->moveTo(ax, ay);

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
void GeometryView::scaleSource(SourceSet::iterator currentSource, int X, int Y, int dx, int dy, bool option) {

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

    double w = ((*currentSource)->getScaleX());
    double x = (*currentSource)->getX();
    double h = ((*currentSource)->getScaleY());
    double y = (*currentSource)->getY();
    double cosa = cos(-(*currentSource)->getRotationAngle() / 180.0 * M_PI);
    double sina = sin(-(*currentSource)->getRotationAngle() / 180.0 * M_PI);

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

    // reverse rotation to apply scaling and shift in the world reference
    cosa = cos((*currentSource)->getRotationAngle() / 180.0 * M_PI);
    sina = sin((*currentSource)->getRotationAngle() / 180.0 * M_PI);

	dum = ax * cosa - ay * sina;
	ay = ay  * cosa + ax * sina;
	ax = dum;

    (*currentSource)->scaleBy(sx, sy);
    (*currentSource)->moveTo(x + ax, y + ay);
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
void GeometryView::rotateSource(SourceSet::iterator currentSource, int X, int Y, int dx, int dy, bool option) {

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
    double x = (*currentSource)->getX();
    double y = (*currentSource)->getY();
	ax -= x; ay -= y;
	bx -= x; by -= y;

	// scale (center scaling) if option is OFF
	if (!option) {
		// compute scaling according to distances change
		dum = sqrt(ax * ax + ay * ay) / sqrt(bx * bx + by * by);
		// Scaling
	    (*currentSource)->scaleBy(dum, dum);
	}

	// compute angle between before and after
	ax = atan(ax/ay) * 180.0 / M_PI;
	bx = atan(bx/by) * 180.0 / M_PI;
	// special case of opposing angles around 180
	dum = (bx * ax) > 0 ? bx - ax : SIGN(ax) * (bx + ax);

	// incremental relative rotation
	dum += (*currentSource)->getRotationAngle() + 360.0;
	// modulo 360
	dum -= (double)( (int) dum / 360 ) * 360.0;

	(*currentSource)->setRotationAngle( ABS(dum) );

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
 *
 **/
char GeometryView::getSourceQuadrant(SourceSet::iterator currentSource, int X, int Y) {
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
    ax -= (*currentSource)->getX();
    ay -= (*currentSource)->getY();
    // quadrant is relative to source orientation
    double cosa = cos(-(*currentSource)->getRotationAngle() / 180.0 * M_PI);
    double sina = sin(-(*currentSource)->getRotationAngle() / 180.0 * M_PI);
	double x = ax * cosa - ay * sina;
	double y = ay * cosa + ax * sina;

	double w = ((*currentSource)->getScaleX());
	double h = ((*currentSource)->getScaleY());
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


