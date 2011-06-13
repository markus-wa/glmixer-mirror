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
 *   Copyright 2009, 2010 Bruno Herbelin
 *
 */

#include "MixerView.h"

#include "common.h"
#include "RenderingManager.h"
#include "ViewRenderWidget.h"
#include "CatalogView.h"

#define MINZOOM 0.04
#define MAXZOOM 1.0
#define DEFAULTZOOM 0.1

MixerView::MixerView() : View()
{
	zoom = DEFAULTZOOM;
	minzoom = MINZOOM;
	maxzoom = MAXZOOM;
	maxpanx = 2.0*SOURCE_UNIT*MAXZOOM*CIRCLE_SIZE;
	maxpany = 2.0*SOURCE_UNIT*MAXZOOM*CIRCLE_SIZE;
	currentAction = View::NONE;

	drawRectangle = false;
    selectionRectangleStart[0] = selectionRectangleEnd[0] = -maxpanx;
    selectionRectangleStart[1] = selectionRectangleEnd[1] = -maxpanx;

    icon.load(QString::fromUtf8(":/glmixer/icons/mixer.png"));
    title = " Mixing view";
}

void MixerView::setModelview()
{
	View::setModelview();
	glScalef(zoom, zoom, zoom);
	glTranslatef(getPanningX(), getPanningY(), 0.0);
	glGetDoublev(GL_MODELVIEW_MATRIX, modelview);
}

void MixerView::paint()
{
	static double renderingAspectRatio = 1.0;
	static bool first = true, last = false;

    // First the background stuff
    glCallList(ViewRenderWidget::circle_mixing);

    // and the selection connection lines
    glLineStipple(1, 0x9999);
    glEnable(GL_LINE_STIPPLE);
    glLineWidth(2.0);
	glColor4ub(COLOR_SELECTION, 255);
    glBegin(GL_LINE_LOOP);
    for(SourceSet::iterator  its = View::selectionBegin(); its != View::selectionEnd(); its++) {
        glVertex3d((*its)->getAlphaX(), (*its)->getAlphaY(), 0.0);
    }
    glEnd();
    glDisable(GL_LINE_STIPPLE);

    // the groups
    glLineWidth(3.0);
    for(SourceListArray::iterator itss = groupSources.begin(); itss != groupSources.end(); itss++) {
    	glColor4f(groupColor[itss].redF(), groupColor[itss].greenF(),groupColor[itss].blueF(), 0.9);
        glBegin(GL_LINE_LOOP);
        for(SourceList::iterator  its = (*itss).begin(); its != (*itss).end(); its++) {
            glVertex3d((*its)->getAlphaX(), (*its)->getAlphaY(), 0.0);
        }
        glEnd();
    }

    // Second the icons of the sources (reversed depth order)
    // render in the depth order
    if (ViewRenderWidget::program->bind()) {
		first = true;
		last = false;
		for(SourceSet::iterator  its = RenderingManager::getInstance()->getBegin(); its != RenderingManager::getInstance()->getEnd(); its++) {

			//
			// 1. Render it into current view
			//
			glPushMatrix();
			glTranslated((*its)->getAlphaX(), (*its)->getAlphaY(), (*its)->getDepth());

			renderingAspectRatio = (*its)->getScaleX() / (*its)->getScaleY();
			if ( ABS(renderingAspectRatio) > 1.0)
				glScaled(SOURCE_UNIT, SOURCE_UNIT / renderingAspectRatio,  1.0);
			else
				glScaled(SOURCE_UNIT * renderingAspectRatio, SOURCE_UNIT,  1.0);

			// standard transparency blending
			glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
			glBlendEquation(GL_FUNC_ADD);

			// Blending Function For mixing like in the rendering window
			(*its)->beginEffectsSection();

			// bind the source texture and update its content
			ViewRenderWidget::setSourceDrawingMode(!(*its)->isStandby());
			(*its)->update();

			// draw surface
			(*its)->draw();

			if (!(*its)->isStandby()) {

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
			ViewRenderWidget::setSourceDrawingMode(false);

			if (RenderingManager::getInstance()->isCurrentSource(its))
				glCallList(ViewRenderWidget::border_large_shadow + ((*its)->isModifiable() ? 0 :2) );
			else
				glCallList(ViewRenderWidget::border_thin_shadow + ((*its)->isModifiable() ? 0 :2) );

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
    for(SourceList::iterator  its = View::selectionBegin(); its != View::selectionEnd(); its++) {
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

	// The rectangle for selection
    if ( drawRectangle ) {
		glColor4ub(COLOR_SELECTION_AREA, 25);
		glRectdv(selectionRectangleStart, selectionRectangleEnd);
		glLineWidth(0.5);
		glColor4ub(COLOR_SELECTION_AREA, 125);
	    glBegin(GL_LINE_LOOP);
		glVertex3d(selectionRectangleStart[0], selectionRectangleStart[1], 0.0);
		glVertex3d(selectionRectangleEnd[0], selectionRectangleStart[1], 0.0);
		glVertex3d(selectionRectangleEnd[0], selectionRectangleEnd[1], 0.0);
		glVertex3d(selectionRectangleStart[0], selectionRectangleEnd[1], 0.0);
	    glEnd();
    }

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

}



void MixerView::clear()
{
	View::clear();

	groupSources.clear();
	groupColor.clear();

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

}



void MixerView::setAction(actionType a){

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
	}
}

bool MixerView::mousePressEvent(QMouseEvent *event)
{
	if (!event)
		return false;

	lastClicPos = event->pos();

	// MIDDLE BUTTON ; panning cursor
	if ((event->button() == Qt::MidButton) || ( (event->button() == Qt::LeftButton) && (event->modifiers() & Qt::MetaModifier) ) ) {
		// priority to panning of the view (even in drop mode)
		setAction(View::PANNING);
		return false;
	}

	// DRoP MODE ; explicitly do nothing
	if ( RenderingManager::getInstance()->getSourceBasketTop() ) {
		setAction(View::DROP);
		// don't interpret other mouse events in drop mode
		return false;
	}


	// OTHER BUTTON ; initiate action
	if ( getSourcesAtCoordinates(event->x(), viewport[3] - event->y()) ) { // if at least one source icon was clicked

    	// get the top most clicked source
    	Source *clicked = *clickedSources.begin();
    	if (!clicked )
    		return false;


		// SELECT MODE : add/remove from selection
		if ( currentAction == View::SELECT && event->button() == Qt::LeftButton ) {

			// test if source is in a group
			SourceListArray::iterator itss = groupSources.begin();
			for(; itss != groupSources.end(); itss++) {
				if ( (*itss).count(clicked) > 0 )
					break;
			}
			// NOT in a source : add individual item clicked
			if ( itss == groupSources.end() )
				View::select(clicked);
			// else add the full group attached to the clicked item
			else {
				if ( View::isInSelection(clicked) )
					View::deselect(*itss);
				else
					View::select(*itss);
			}
		}
		else  // not in selection (SELECT) action mode, then just set the current active source
		{
			RenderingManager::getInstance()->setCurrentSource( clicked->getId() );

			// ready for grabbing the current source
			if ( clicked->isModifiable() )
				setAction(View::GRAB);
		}

		return true;
    }

	// click in background

	// set current to none (end of list)
	RenderingManager::getInstance()->unsetCurrentSource();

	// back to no action
	if ( currentAction != View::SELECT )
		setAction(View::NONE);

	// remember coordinates of clic
	double dumm;
	gluUnProject((GLdouble) event->x(), (GLdouble) viewport[3] - event->y(), 0.0, modelview, projection, viewport, selectionRectangleStart, selectionRectangleStart+1, &dumm);

	return false;
}

bool MixerView::mouseDoubleClickEvent ( QMouseEvent * event )
{
	if (!event)
		return false;

	if (currentAction == View::DROP)
		return false;

	// for LEFT double button
	if ( event->button() == Qt::LeftButton ) {

		// left double click on a source : change the group / selection
		if ( getSourcesAtCoordinates(event->x(), viewport[3] - event->y()) ) {

			// get the top most clicked source
			Source *clicked = *clickedSources.begin();

			// Meta + double click = zoom best fit on clicked source
			if (event->modifiers () & Qt::MetaModifier) {
				RenderingManager::getInstance()->setCurrentSource( clicked->getId() );
				zoomBestFit(true);
				return false;
			}

        	SourceListArray::iterator itss = groupSources.begin();
            for(; itss != groupSources.end(); itss++) {
            	if ( (*itss).count(clicked) > 0 )
            		break;
            }
            // if double clic on a group ; convert group into selection
        	if ( itss != groupSources.end() ) {
        		View::setSelection(*itss);
        		// erase group and its color
        		groupColor.remove(itss);
        		groupSources.erase(itss);
        	}
        	// if double clic NOT on a group ; convert selection into group
        	else {
        		SourceList selection = View::copySelection();
				// if the clicked source is in the selection
				if ( selection.count(clicked)>0 && selection.size()>1 ) {
					//  create a group from the selection
					groupSources.push_front(selection);
					groupColor[groupSources.begin()] = QColor::fromHsv ( rand()%180 + 179, 250, 250);
					View::clearSelection();
				}
        	}
        	return true;
		}
		// default action ; zoom best fit on whole screen
		else
			zoomBestFit(false);
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
		// SHIFT ?
		if ( QApplication::keyboardModifiers () & Qt::ShiftModifier ) {
			// special move ; move the sources in the opposite
			for(SourceSet::iterator  its = RenderingManager::getInstance()->getBegin(); its != RenderingManager::getInstance()->getEnd(); its++) {
				grabSource( *its, event->x(), viewport[3] - event->y(), -dx, -dy);
			}
		}
		// panning background
		panningBy(event->x(), viewport[3] - event->y(), dx, dy);
		return true;
	}

	if ( event->buttons() & Qt::LeftButton ) {

		// get the top most clicked source, if there is one
		static Source *clicked = 0;
		if (sourceClicked())
			clicked = *clickedSources.begin();
		else
			clicked = 0;

		// SELECT AREA (no clicked source)
		if ( !clicked ) {

			drawRectangle = true;

			// set coordinate of end of rectangle selection
			double dumm;
			gluUnProject((GLdouble) event->x(), (GLdouble) viewport[3] - event->y(), 0.0, modelview, projection, viewport, selectionRectangleEnd, selectionRectangleEnd+1, &dumm);

			// loop over every sources to check if it is in the rectangle area
			SourceList rectSources;
			for(SourceSet::iterator  its = RenderingManager::getInstance()->getBegin(); its != RenderingManager::getInstance()->getEnd(); its++)
			{
				if ((*its)->getAlphaX() > MINI(selectionRectangleStart[0], selectionRectangleEnd[0]) &&
					(*its)->getAlphaX() < MAXI(selectionRectangleStart[0], selectionRectangleEnd[0]) &&
					(*its)->getAlphaY() > MINI(selectionRectangleStart[1], selectionRectangleEnd[1]) &&
					(*its)->getAlphaY() < MAXI(selectionRectangleStart[1], selectionRectangleEnd[1]) ){
					rectSources.insert(*its);
				}
			}

			SourceList result;
			for(SourceListArray::iterator itss = groupSources.begin(); itss != groupSources.end(); itss++) {
				result.erase (result.begin (), result.end ());
				std::set_intersection(rectSources.begin(), rectSources.end(), (*itss).begin(), (*itss).end(), std::inserter(result, result.begin()));
				// if the group is fully inside the rectangle selection
				if ( (*itss).size() != result.size() ) {
					// ensure none of the group source remain in the selection
					result.erase (result.begin (), result.end ());
					std::set_difference(rectSources.begin(), rectSources.end(), (*itss).begin(), (*itss).end(), std::inserter(result, result.begin()) );
					rectSources = SourceList(result);
				}
			}

			if ( currentAction == View::SELECT )  // extend selection
				View::select(rectSources);
			else  // new selection
				View::setSelection(rectSources);

			return false;
		}


		// OTHER BUTTON & clicked : grab
		if (currentAction == View::GRAB )
		{
			grabSources(clicked, event->x(), viewport[3] - event->y(), dx, dy);
			return true;
		}
	}

	// SELECT MODE
	if ( currentAction == View::SELECT ) {
		return false;
	}

	// NO BUTTON : show a mouse-over cursor
	if ( getSourcesAtCoordinates(event->x(), viewport[3] - event->y(), false) )
		setAction(View::OVER);
	else
		setAction(View::NONE);

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
	else if (drawRectangle){

		// set coordinate of end of rectangle selection
		double dumm;
	    gluUnProject((GLdouble) event->x(), (GLdouble) viewport[3] - event->y(), 0.0, modelview, projection, viewport, selectionRectangleEnd, selectionRectangleEnd+1, &dumm);

	    // loop over every sources to check if it is in the rectangle area
	    SourceList rectSources;
	    for(SourceSet::iterator  its = RenderingManager::getInstance()->getBegin(); its != RenderingManager::getInstance()->getEnd(); its++)
	    {
	    	if ((*its)->getAlphaX() > MINI(selectionRectangleStart[0], selectionRectangleEnd[0]) &&
	    		(*its)->getAlphaX() < MAXI(selectionRectangleStart[0], selectionRectangleEnd[0]) &&
	    		(*its)->getAlphaY() > MINI(selectionRectangleStart[1], selectionRectangleEnd[1]) &&
	    		(*its)->getAlphaY() < MAXI(selectionRectangleStart[1], selectionRectangleEnd[1]) ){
	    		rectSources.insert(*its);
	    	}
	    }

	    if (rectSources.size() > 0) {

			// set the last selected as current
			RenderingManager::getInstance()->setCurrentSource( (*rectSources.begin())->getId() );

			SourceList result;
			for(SourceListArray::iterator itss = groupSources.begin(); itss != groupSources.end();) {
				result.erase (result.begin (), result.end ());
				std::set_intersection(rectSources.begin(), rectSources.end(), (*itss).begin(), (*itss).end(), std::inserter(result, result.begin()));
					itss++;
			}
	    }

	} else if ( currentAction == View::SELECT && !getSourcesAtCoordinates(event->x(), viewport[3] - event->y()))
		View::clearSelection();

	drawRectangle = false;

	return true;
}

bool MixerView::wheelEvent ( QWheelEvent * event )
{
	float previous = zoom;

	if ( event->modifiers () & Qt::MetaModifier )
		setZoom (zoom + ((float) event->delta() * zoom * minzoom) / (30.0 * maxzoom) );
	else
		setZoom (zoom + ((float) event->delta() * zoom * minzoom) / (120.0 * maxzoom) );

	if (currentAction == View::GRAB ) {
		deltazoom = 1.0 - (zoom / previous);
		// simulate a grab with no mouse movement but a deltazoom :
		SourceSet::iterator cs = RenderingManager::getInstance()->getCurrentSource();
		if ( RenderingManager::getInstance()->notAtEnd(cs))
			grabSources(*cs, event->x(), (viewport[3] - event->y()), 0, 0);
		// reset deltazoom
		deltazoom = 0;
	}

	return true;
}

void MixerView::zoomReset()
{
	setZoom(DEFAULTZOOM);
	setPanningX(0); setPanningY(0);
}

void MixerView::zoomBestFit( bool onlyClickedSource )
{
	// nothing to do if there is no source
	if (RenderingManager::getInstance()->getBegin() == RenderingManager::getInstance()->getEnd()){
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
	setPanningX	( -( x_min + ABS(x_max - x_min)/ 2.0 ) );
	setPanningY	( -( y_min + ABS(y_max - y_min)/ 2.0 )  );

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

	// detect CTRL to enter select mode
	if (event->modifiers() & Qt::ControlModifier){
		setAction(View::SELECT);
		return true;
	}

	// else move a source
	SourceSet::iterator its = RenderingManager::getInstance()->getCurrentSource();
	if (its != RenderingManager::getInstance()->getEnd()) {
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
		grabSource(*its, 0, 0, dx, dy);

		return true;
	}

	return false;
}

bool MixerView::keyReleaseEvent(QKeyEvent * event) {

	if ( currentAction == View::SELECT )
		setAction(previousAction);

	return false;
}

void MixerView::removeFromGroup(Source *s)
{

	// find the group containing the source to delete
	SourceListArray::iterator itss;
	for(itss = groupSources.begin(); itss != groupSources.end(); itss++) {
		if ( (*itss).count(s) > 0 )
			break;
	}

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
	} else {
		int s = 0;
		while (hits != 0) {
			if ( (*(RenderingManager::getInstance()->getById (selectBuf[ (hits-1) * 4 + 3])))->isModifiable() )
				s++;
			hits--;
		}
		return s > 0;
	}

}

/**
 *
 **/
void MixerView::grabSources(Source *s, int x, int y, int dx, int dy) {

	if (!s) return;

	// find if the source is in a group
	SourceListArray::iterator itss;
	for(itss = groupSources.begin(); itss != groupSources.end(); itss++) {
		if ( (*itss).count(s) > 0 )
			break;
	}

	// SHIFT : special (non-selection) modification
	if ( QApplication::keyboardModifiers () & Qt::ShiftModifier ){
		// if the source is in the selection AND in a group, then move the group individually
		if ( itss != groupSources.end() && View::isInSelection(s) ){
			for(SourceList::iterator  its = (*itss).begin(); its != (*itss).end(); its++) {
				grabSource( *its, x, y, dx, dy);
			}
		}
		else
			// move the source individually
			grabSource(s, x, y, dx, dy);
	}
	// NORMAL : move selection or group
	else {
		// if the source is in the selection, move the selection
		if ( View::isInSelection(s) ){
			for(SourceList::iterator  its = View::selectionBegin(); its != View::selectionEnd(); its++) {
				grabSource( *its, x, y, dx, dy);
			}
		}
		// else, if it is in a group, move the group
		else if ( itss != groupSources.end() ) {
			for(SourceList::iterator  its = (*itss).begin(); its != (*itss).end(); its++) {
				grabSource( *its, x, y, dx, dy);
			}
		}
		// nothing special, move the source individually
		else
			grabSource(s, x, y, dx, dy);
	}

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

    ax += (ax + getPanningX()) * deltazoom;
    ay += (ay + getPanningY()) * deltazoom;

    double ix = s->getAlphaX() + ax - bx;
    double iy = s->getAlphaY() + ay - by;

    // move icon
    s->setAlphaCoordinates( ix, iy );
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
    setPanningX(getPanningX() + ax - bx);
    setPanningY(getPanningY() + ay - by);
}



bool MixerView::isInAGroup(Source *s){

	SourceListArray::iterator itss = groupSources.begin();
    for(; itss != groupSources.end(); itss++) {
    	if ( (*itss).count(s) > 0 )
    		break;
    }
	return ( itss != groupSources.end() );
}

QDomElement MixerView::getConfiguration(QDomDocument &doc){

	QDomElement mixviewelem = View::getConfiguration(doc);

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



