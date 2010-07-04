/*
 * MixerView.cpp
 *
 *  Created on: Nov 9, 2009
 *      Author: bh
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
    // First the background stuff
    glCallList(ViewRenderWidget::circle_mixing);

    // and the selection connection lines
    glDisable(GL_TEXTURE_2D);
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
    glEnable(GL_TEXTURE_2D);
    bool first = true;
	for(SourceSet::iterator  its = RenderingManager::getInstance()->getBegin(); its != RenderingManager::getInstance()->getEnd(); its++) {

		//
		// 1. Render it into current view
		//
		glPushMatrix();
		glTranslated((*its)->getAlphaX(), (*its)->getAlphaY(), (*its)->getDepth());
		glScalef( SOURCE_UNIT * (*its)->getAspectRatio(),  SOURCE_UNIT, 1.f);

    	// standard transparency blending
    	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    	glBlendEquation(GL_FUNC_ADD);

		if ((*its)->isActive())
			glCallList(ViewRenderWidget::border_large_shadow);
		else
			glCallList(ViewRenderWidget::border_thin_shadow);

	    // Blending Function For mixing like in the rendering window
        (*its)->beginEffectsSection();

		// bind the source texture and update its content
		(*its)->update();

		// draw surface
		(*its)->draw();

		// draw stippled version of the source on top
		glCallList(ViewRenderWidget::quad_half_textured);

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

    // Then the selection outlines
    for(SourceList::iterator  its = selectedSources.begin(); its != selectedSources.end(); its++) {
        glPushMatrix();
        glTranslated((*its)->getAlphaX(), (*its)->getAlphaY(), (*its)->getDepth());
        glScalef( SOURCE_UNIT * (*its)->getAspectRatio(), SOURCE_UNIT, 1.f);
		glCallList(ViewRenderWidget::frame_selection);
        glPopMatrix();

    }

	// The rectangle for selection
    if ( currentAction == RECTANGLE) {
		glDisable(GL_TEXTURE_2D);
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
		glEnable(GL_TEXTURE_2D);
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
			glTranslated( SOURCE_UNIT * s->getAspectRatio() + 1.0, - SOURCE_UNIT + 1.0, 0.0);
			for (int i = 1; i < RenderingManager::getInstance()->getSourceBasketSize(); ++i ) {
				glTranslated(  2.1, 0.0, 0.0);
				glCallList(ViewRenderWidget::border_thin);
			}
			glPopMatrix();
		glScalef( SOURCE_UNIT * s->getAspectRatio(), SOURCE_UNIT, 1.f);
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
	case OVER:
		RenderingManager::getRenderingWidget()->setMouseCursor(ViewRenderWidget::MOUSE_HAND_OPEN);
		break;
	case TOOL:
		RenderingManager::getRenderingWidget()->setMouseCursor(ViewRenderWidget::MOUSE_HAND_CLOSED);
		break;
	case SELECT:
	case RECTANGLE:
		RenderingManager::getRenderingWidget()->setMouseCursor(ViewRenderWidget::MOUSE_HAND_INDEX);
		break;
	default:
		RenderingManager::getRenderingWidget()->setMouseCursor(ViewRenderWidget::MOUSE_ARROW);
	}
}

bool MixerView::mousePressEvent(QMouseEvent *event)
{
	lastClicPos = event->pos();

	// MIDDLE BUTTON ; panning cursor
	if (event->buttons() & Qt::MidButton) {
		// priority to panning of the view (even in drop mode)
		RenderingManager::getRenderingWidget()->setMouseCursor(ViewRenderWidget::MOUSE_SIZEALL);
	}
	// DRoP MODE ; explicitly do nothing
	else if ( RenderingManager::getInstance()->getSourceBasketTop() ) {
		RenderingManager::getRenderingWidget()->setMouseCursor(ViewRenderWidget::MOUSE_QUESTION);
		// don't interpret other mouse events in drop mode
		return false;
	}
	// LEFT BUTTON ; initiate action
//	else  if (event->buttons() & Qt::LeftButton) {
	else if ( getSourcesAtCoordinates(event->x(), viewport[3] - event->y()) ) { // if at least one source icon was clicked

    	// get the top most clicked source
    	Source *clicked = *clickedSources.begin();

    	// if a source icon was cliked
        if ( clicked ) {

        	// if CTRL button modifier pressed, add clicked to selection
			if ( currentAction != TOOL && QApplication::keyboardModifiers () == Qt::ControlModifier) {
				setAction(SELECT);

	        	if ( !isInAGroup(clicked) ) {
					if ( selectedSources.count(clicked) > 0)
						selectedSources.erase( clicked );
					else
						selectedSources.insert( clicked );
	        	}

			}
			else // not in selection (SELECT) action mode, then just set the current active source
			{
				RenderingManager::getInstance()->setCurrentSource( clicked->getId() );
				// ready for grabbing the current source
				setAction(TOOL);
			}
		}

    } else // clic in background
    {
		// set current to none (end of list)
		RenderingManager::getInstance()->setCurrentSource( RenderingManager::getInstance()->getEnd() );
		// clear selection
		selectedSources.clear();
		setAction(NONE);
		// remember coordinates of clic
		double dumm;
	    gluUnProject((GLdouble) event->x(), (GLdouble) viewport[3] - event->y(), 0.0, modelview, projection, viewport, rectangleStart, rectangleStart+1, &dumm);

	}

	return true;
}

bool MixerView::mouseDoubleClickEvent ( QMouseEvent * event ){

	// for LEFT double button clic alrernate group / selection
	if ( (event->buttons() & Qt::LeftButton) /*&& getSourceAtCoordinates(event->x(), viewport[3] - event->y()) */) {

		if ( getSourcesAtCoordinates(event->x(), viewport[3] - event->y()) ) {

	    	// get the top most clicked source
	    	Source *clicked = *clickedSources.begin();

        	SourceListArray::iterator itss = groupSources.begin();
            for(; itss != groupSources.end(); itss++) {
            	if ( (*itss).count(clicked) > 0 )
            		break;
            }
        	if ( itss != groupSources.end() ) {
        		selectedSources = SourceList(*itss);
        		// erase group and its color
        		groupColor.remove(itss);
        		groupSources.erase(itss);
        	} else {
				// if the clicked source is in the selection
				if ( selectedSources.count(clicked) > 0 && selectedSources.size() > 1 ) {
					//  create a group from the selection
					groupSources.push_front(SourceList(selectedSources));
					groupColor[groupSources.begin()] = QColor::fromHsv ( rand()%180 + 179, 250, 250);
					selectedSources.clear();
				}
				// else add it to the selection
				else
					selectedSources.insert( clicked );
        	}
		}
//		else  // double clic in background
//			zoomBestFit();

	}

	return true;
}


bool MixerView::mouseMoveEvent(QMouseEvent *event)
{
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
	if (event->buttons() & Qt::MidButton) {

		panningBy(event->x(), viewport[3] - event->y(), dx, dy);
		return false;
	}
	// DROP MODE : avoid other actions
	else if ( RenderingManager::getInstance()->getSourceBasketTop() ) {

		RenderingManager::getRenderingWidget()->setMouseCursor(ViewRenderWidget::MOUSE_QUESTION);
		// don't interpret mouse events in drop mode
		return false;
	}
	// LEFT BUTTON : grab or draw a selection rectangle
	else if (event->buttons() & Qt::LeftButton) {

        if ( clicked && currentAction == TOOL )
        {
        	SourceListArray::iterator itss;
            for(itss = groupSources.begin(); itss != groupSources.end(); itss++) {
            	if ( (*itss).count(clicked) > 0 )
            		break;
            }
        	if ( itss != groupSources.end() ) {
				for(SourceList::iterator  its = (*itss).begin(); its != (*itss).end(); its++) {
					grabSource( *its, event->x(), viewport[3] - event->y(), dx, dy);
				}
        	} else if ( selectedSources.count(clicked) > 0 ){
				for(SourceList::iterator  its = selectedSources.begin(); its != selectedSources.end(); its++) {
					grabSource( *its, event->x(), viewport[3] - event->y(), dx, dy);
				}
			}
			else
				grabSource(clicked, event->x(), viewport[3] - event->y(), dx, dy);

        } else {

        	setAction(RECTANGLE);
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
        }
		return true;

    }
	// RIGHT BUTTON : special (non-selection) modification
	else if (event->buttons() & Qt::RightButton) {

    	// RIGHT clic on a source ; change its alpha, but do not make it current
        if ( clicked ) {
        	//  move it individually, even if in a group
        	setAction(TOOL);
			grabSource(clicked, event->x(), viewport[3] - event->y(), dx, dy);

			return true;
        }

    }
	// NO BUTTON : show a mouse-over cursor
	else  {
		if ( getSourcesAtCoordinates(event->x(), viewport[3] - event->y()) )
			// selection mode with CTRL modifier
			if (QApplication::keyboardModifiers () == Qt::ControlModifier)
				setAction(SELECT);
			else
				setAction(OVER);
		else
			setAction(NONE);

    }

	return false;
}

bool MixerView::mouseReleaseEvent ( QMouseEvent * event )
{
	if ( RenderingManager::getInstance()->getSourceBasketTop() )
		RenderingManager::getRenderingWidget()->setMouseCursor(ViewRenderWidget::MOUSE_QUESTION);
	else if (currentAction == TOOL )
		setAction(OVER);
	else if (currentAction == RECTANGLE ){

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
		for(SourceListArray::iterator itss = groupSources.begin(); itss != groupSources.end();) {
			result.erase (result.begin (), result.end ());
			std::set_intersection(rectSources.begin(), rectSources.end(), (*itss).begin(), (*itss).end(), std::inserter(result, result.begin()));
			// if the group is fully inside the rectangle selection
			if ( (*itss).size() == result.size() )
				itss = groupSources.erase( itss );
			else
				itss++;
		}

		setAction(NONE);
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

	if (currentAction == TOOL ) {
		deltazoom = 1.0 - (zoom / previous);
		// simulate a grab with no mouse movement but a deltazoom :
		SourceSet::iterator cs = RenderingManager::getInstance()->getCurrentSource();
		if ( RenderingManager::getInstance()->notAtEnd(cs))
			grabSource(*cs, event->x(), (viewport[3] - event->y()), 0, 0);
		// reset deltazoom
		deltazoom = 0;
	}
	else
		setAction(NONE);

	return true;
}

void MixerView::zoomReset()
{
	setZoom(DEFAULTZOOM);
	setPanningX(0); setPanningY(0);
}

void MixerView::zoomBestFit()
{
	// nothing to do if there is no source
	if (RenderingManager::getInstance()->getBegin() == RenderingManager::getInstance()->getEnd()){
		zoomReset();
		return;
	}

	// 1. compute bounding box of every sources
    double x_min = 10000, x_max = -10000, y_min = 10000, y_max = -10000;
	for(SourceSet::iterator  its = RenderingManager::getInstance()->getBegin(); its != RenderingManager::getInstance()->getEnd(); its++) {
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
    // depending on the axis having the largest extend
    if ( ABS(x_max-x_min) > ABS(y_max-y_min))
    	scale *= ABS(URcorner[0]-LLcorner[0]) / ABS(x_max-x_min);
    else
    	scale *= ABS(URcorner[1]-LLcorner[1]) / ABS(y_max-y_min);
    // apply the scaling
	setZoom( zoom * scale );
}


//bool MixerView::keyPressEvent ( QKeyEvent * event ){
//
//	if (currentAction == OVER )
//		setAction(SELECT);
//	else
//		setAction(currentAction);
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

bool MixerView::getSourcesAtCoordinates(int mouseX, int mouseY) {

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
        glTranslatef( (*its)->getAlphaX(), (*its)->getAlphaY(), (*its)->getDepth());
        glScalef( SOURCE_UNIT * (*its)->getAspectRatio(),  SOURCE_UNIT, 1.f);
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
