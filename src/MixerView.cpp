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
#include <algorithm>

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
    glColor4f(0.2, 0.8, 0.2, 1.0);
    glBegin(GL_LINE_LOOP);
    for(SourceSet::iterator  its = selectedSources.begin(); its != selectedSources.end(); its++) {
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

			if ((*its)->isActive())
				glCallList(ViewRenderWidget::border_large_shadow);
			else
				glCallList(ViewRenderWidget::border_thin_shadow);

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
    for(SourceList::iterator  its = selectedSources.begin(); its != selectedSources.end(); its++) {
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
    if ( currentAction == View::RECTANGLE) {
		glColor4f(0.3, 0.8, 0.3, 0.1);
		glRectdv(rectangleStart, rectangleEnd);
		glLineWidth(0.5);
		glColor4f(0.3, 0.8, 0.3, 0.5);
	    glBegin(GL_LINE_LOOP);
		glVertex3d(rectangleStart[0], rectangleStart[1], 0.0);
		glVertex3d(rectangleEnd[0], rectangleStart[1], 0.0);
		glVertex3d(rectangleEnd[0], rectangleEnd[1], 0.0);
		glVertex3d(rectangleStart[0], rectangleEnd[1], 0.0);
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

	View::setAction(a);

	switch(a) {
	case View::OVER:
		RenderingManager::getRenderingWidget()->setMouseCursor(ViewRenderWidget::MOUSE_HAND_OPEN);
		break;
	case View::GRAB:
		RenderingManager::getRenderingWidget()->setMouseCursor(ViewRenderWidget::MOUSE_HAND_CLOSED);
		break;
	case View::SELECT:
	case View::RECTANGLE:
		RenderingManager::getRenderingWidget()->setMouseCursor(ViewRenderWidget::MOUSE_HAND_INDEX);
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
	if ((event->buttons() & Qt::MidButton) || ( (event->buttons() & Qt::LeftButton) && QApplication::keyboardModifiers () == Qt::ShiftModifier) ) {
		// priority to panning of the view (even in drop mode)
		RenderingManager::getRenderingWidget()->setMouseCursor(ViewRenderWidget::MOUSE_SIZEALL);
		return false;
	}
	// DRoP MODE ; explicitly do nothing
	if ( RenderingManager::getInstance()->getSourceBasketTop() ) {
		RenderingManager::getRenderingWidget()->setMouseCursor(ViewRenderWidget::MOUSE_QUESTION);
		// don't interpret other mouse events in drop mode
		return false;
	}
	// OTHER BUTTON ; initiate action
//	else  if (event->buttons() & Qt::LeftButton) {
	if ( getSourcesAtCoordinates(event->x(), viewport[3] - event->y()) ) { // if at least one source icon was clicked

    	// get the top most clicked source
    	Source *clicked = *clickedSources.begin();

    	// should be always true
        if ( clicked ) {

        	// if CTRL button modifier pressed, add clicked to selection
			if ( currentAction != View::GRAB && QApplication::keyboardModifiers () == Qt::ControlModifier) {
				setAction(SELECT);
				// test if source is in a group
        		SourceListArray::iterator itss = groupSources.begin();
				for(; itss != groupSources.end(); itss++) {
					if ( (*itss).count(clicked) > 0 )
						break;
				}
				// NOT in a source : add individual item clicked
	        	if ( itss == groupSources.end()  ) {
					if ( selectedSources.count(clicked) > 0)
						selectedSources.erase( clicked );
					else
						selectedSources.insert( clicked );
	        	}
	        	// add the full group attached to the clicked item
	        	else {
	        		SourceList result;
					if ( selectedSources.count(clicked) > 0)
		        		std::set_difference(selectedSources.begin(), selectedSources.end(), (*itss).begin(), (*itss).end(), std::inserter(result, result.begin()) );
					else
						std::set_union(selectedSources.begin(), selectedSources.end(), (*itss).begin(), (*itss).end(), std::inserter(result, result.begin()) );
					selectedSources = SourceList(result);
	        	}
			}
			else // not in selection (SELECT) action mode, then just set the current active source
			{
				RenderingManager::getInstance()->setCurrentSource( clicked->getId() );
				// ready for grabbing the current source
				setAction(View::GRAB);
			}
		}
    	return true;
    }

	// click in background

	// set current to none (end of list)
	RenderingManager::getInstance()->setCurrentSource( RenderingManager::getInstance()->getEnd() );
	// clear selection
	selectedSources.clear();
	setAction(View::NONE);
	// remember coordinates of clic
	double dumm;
	gluUnProject((GLdouble) event->x(), (GLdouble) viewport[3] - event->y(), 0.0, modelview, projection, viewport, rectangleStart, rectangleStart+1, &dumm);

	return false;
}

bool MixerView::mouseDoubleClickEvent ( QMouseEvent * event )
{
	if (!event)
		return false;

	// for LEFT double button
	if ( event->buttons() & Qt::LeftButton ) {

		// left double click on a source : change the group / selection
		if ( getSourcesAtCoordinates(event->x(), viewport[3] - event->y()) ) {

			// get the top most clicked source
			Source *clicked = *clickedSources.begin();

			// SHIFT + double click = zoom best fit on clicked source
			if (QApplication::keyboardModifiers() == Qt::ShiftModifier) {
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
        		selectedSources = SourceList(*itss);
        		// erase group and its color
        		groupColor.remove(itss);
        		groupSources.erase(itss);
        	}
        	// if double clic NOT on a group ; convert selection into group
        	else {
				// if the clicked source is in the selection
				if ( selectedSources.count(clicked) > 0 && selectedSources.size() > 1 ) {
					//  create a group from the selection
					groupSources.push_front(SourceList(selectedSources));
					groupColor[groupSources.begin()] = QColor::fromHsv ( rand()%180 + 179, 250, 250);
					selectedSources.clear();
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

	// get the top most clicked source, if there is
	static Source *clicked = 0;
	if (!clickedSources.empty())
		clicked = *clickedSources.begin();
	else
		clicked = 0;

    // MIDDLE button ; panning
	if ((event->buttons() & Qt::MidButton) || ( (event->buttons() & Qt::LeftButton) && QApplication::keyboardModifiers () == Qt::ShiftModifier) ) {

			panningBy(event->x(), viewport[3] - event->y(), dx, dy);
			return false;
	}
	// DROP MODE : avoid other actions
	if ( RenderingManager::getInstance()->getSourceBasketTop() ) {

		RenderingManager::getRenderingWidget()->setMouseCursor(ViewRenderWidget::MOUSE_QUESTION);
		// don't interpret mouse events in drop mode
		return false;
	}
	// LEFT BUTTON : grab or draw a selection rectangle
	if (event->buttons() & Qt::LeftButton) {

        if ( clicked && currentAction == View::GRAB )
        {
        	// find if the source is in a group
        	SourceListArray::iterator itss;
            for(itss = groupSources.begin(); itss != groupSources.end(); itss++) {
            	if ( (*itss).count(clicked) > 0 )
            		break;
            }
            // if the source is in the selection, move the selection
        	if ( selectedSources.count(clicked) > 0 ){
				for(SourceList::iterator  its = selectedSources.begin(); its != selectedSources.end(); its++) {
					grabSource( *its, event->x(), viewport[3] - event->y(), dx, dy);
				}
			}
        	// else, if it is in a group, move the group
        	else if ( itss != groupSources.end() ) {
				for(SourceList::iterator  its = (*itss).begin(); its != (*itss).end(); its++) {
					grabSource( *its, event->x(), viewport[3] - event->y(), dx, dy);
				}
        	}
        	// nothing special, move the source individually
			else
				grabSource(clicked, event->x(), viewport[3] - event->y(), dx, dy);

    		return true;

        } else {

        	if (clicked)
        		return false;

        	setAction(View::RECTANGLE);
			// set coordinate of end of rectangle selection
			double dumm;
		    gluUnProject((GLdouble) event->x(), (GLdouble) viewport[3] - event->y(), 0.0, modelview, projection, viewport, rectangleEnd, rectangleEnd+1, &dumm);

		    // loop over every sources to check if it is in the rectangle area
		    SourceList rectSources;
		    for(SourceSet::iterator  its = RenderingManager::getInstance()->getBegin(); its != RenderingManager::getInstance()->getEnd(); its++)
		    {
		    	if ((*its)->getAlphaX() > MINI(rectangleStart[0], rectangleEnd[0]) &&
		    		(*its)->getAlphaX() < MAXI(rectangleStart[0], rectangleEnd[0]) &&
		    		(*its)->getAlphaY() > MINI(rectangleStart[1], rectangleEnd[1]) &&
		    		(*its)->getAlphaY() < MAXI(rectangleStart[1], rectangleEnd[1]) ){
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

			selectedSources = SourceList(rectSources);
			return false;
        }

    }
	// RIGHT BUTTON : special (non-selection) modification
	if (event->buttons() & Qt::RightButton) {

    	// RIGHT clic on a source ; change its alpha, but do not make it current
        if ( clicked ) {
        	//  move it individually, even if in a group
        	setAction(View::GRAB);

        	// find if the source is in a group
        	SourceListArray::iterator itss;
            for(itss = groupSources.begin(); itss != groupSources.end(); itss++) {
            	if ( (*itss).count(clicked) > 0 )
            		break;
            }

            // if the source is in the selection AND in a group, then move the group
			if ( itss != groupSources.end() && selectedSources.count(clicked) > 0 ){
				for(SourceList::iterator  its = (*itss).begin(); its != (*itss).end(); its++) {
					grabSource( *its, event->x(), viewport[3] - event->y(), dx, dy);
				}
        	}
			else
				// move the source individually
				grabSource(clicked, event->x(), viewport[3] - event->y(), dx, dy);

			return true;
        }

    }
	// NO BUTTON : show a mouse-over cursor
	if ( getSourcesAtCoordinates(event->x(), viewport[3] - event->y(), false) ) {
		// selection mode with CTRL modifier
		if (QApplication::keyboardModifiers () == Qt::ControlModifier)
			setAction(View::SELECT);
		else
			setAction(View::OVER);
	}
	else
		setAction(View::NONE);

	return false;
}

bool MixerView::mouseReleaseEvent ( QMouseEvent * event )
{
	if ( RenderingManager::getInstance()->getSourceBasketTop() )
		RenderingManager::getRenderingWidget()->setMouseCursor(ViewRenderWidget::MOUSE_QUESTION);
	else if (currentAction == View::GRAB )
		setAction(OVER);
	else if (currentAction == View::RECTANGLE ){

		// set coordinate of end of rectangle selection
		double dumm;
	    gluUnProject((GLdouble) event->x(), (GLdouble) viewport[3] - event->y(), 0.0, modelview, projection, viewport, rectangleEnd, rectangleEnd+1, &dumm);

	    // loop over every sources to check if it is in the rectangle area
	    SourceList rectSources;
	    for(SourceSet::iterator  its = RenderingManager::getInstance()->getBegin(); its != RenderingManager::getInstance()->getEnd(); its++)
	    {
	    	if ((*its)->getAlphaX() > MINI(rectangleStart[0], rectangleEnd[0]) &&
	    		(*its)->getAlphaX() < MAXI(rectangleStart[0], rectangleEnd[0]) &&
	    		(*its)->getAlphaY() > MINI(rectangleStart[1], rectangleEnd[1]) &&
	    		(*its)->getAlphaY() < MAXI(rectangleStart[1], rectangleEnd[1]) ){
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
				// if the group is fully inside the rectangle selection
//				if ( (*itss).size() == result.size() )
//					itss = groupSources.erase( itss );
//				else
					itss++;
			}
	    }

		setAction(View::NONE);
	} else
		setAction(currentAction);

	return true;
}

bool MixerView::wheelEvent ( QWheelEvent * event )
{
	float previous = zoom;

	if (QApplication::keyboardModifiers () == Qt::ControlModifier)
		setZoom (zoom + ((float) event->delta() * zoom * minzoom) / (30.0 * maxzoom) );
	else
		setZoom (zoom + ((float) event->delta() * zoom * minzoom) / (120.0 * maxzoom) );

	if (currentAction == View::GRAB ) {
		deltazoom = 1.0 - (zoom / previous);
		// simulate a grab with no mouse movement but a deltazoom :
		SourceSet::iterator cs = RenderingManager::getInstance()->getCurrentSource();
		if ( RenderingManager::getInstance()->notAtEnd(cs))
			grabSource(*cs, event->x(), (viewport[3] - event->y()), 0, 0);
		// reset deltazoom
		deltazoom = 0;
	}
	else
		setAction(View::NONE);

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
    double scale = 0.95;
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

	if (currentAction == OVER && event->modifiers() == Qt::ControlModifier)
		setAction(SELECT);
	else
		setAction(currentAction);

	SourceSet::iterator its = RenderingManager::getInstance()->getCurrentSource();

	if (its != RenderingManager::getInstance()->getEnd()) {
	    int dx =0, dy = 0, factor = 1;
	    if (event->modifiers() & Qt::ControlModifier)
	    	factor *= 10;
	    if (event->modifiers() & Qt::ShiftModifier)
	    	factor *= 2;
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

void MixerView::removeFromSelection(Source *s)
{
	View::removeFromSelection(s);

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

		return !clickedSources.empty();
	} else
		return hits != 0;

}

/**
 *
 **/
void MixerView::grabSource(Source *s, int x, int y, int dx, int dy) {

	if (!s)
		return;

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


void MixerView::alphaCoordinatesFromMouse(int mouseX, int mouseY, double *alphaX, double *alphaY){

	double dum;

	gluUnProject((GLdouble) mouseX, (GLdouble) (viewport[3] - mouseY), 0.0,
	            modelview, projection, viewport, alphaX, alphaY, &dum);

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



