/*
 * GeometryView.cpp
 *
 *  Created on: Jan 31, 2010
 *      Author: bh
 */

#include "GeometryView.h"

#include "RenderingManager.h"
#include "OutputRenderWindow.h"
#include <algorithm>

#define MINZOOM 0.1
#define MAXZOOM 3.0
#define DEFAULTZOOM 0.5


GeometryView::GeometryView() : View(), quadrant(0), currentAction(NONE)
{
	zoom = DEFAULTZOOM;
	minzoom = MINZOOM;
	maxzoom = MAXZOOM;
	maxpanx = SOURCE_UNIT*MAXZOOM*2.0;
	maxpany = SOURCE_UNIT*MAXZOOM*2.0;

    icon.load(QString::fromUtf8(":/glmixer/icons/manipulation.png"));
}


void GeometryView::paint()
{
    // first the black background (as the rendering black clear color) with shadow
    glCallList(ViewRenderWidget::quad_black);

    bool first = true;
    // then the icons of the sources (reversed depth order)
	for(SourceSet::iterator  its = RenderingManager::getInstance()->getBegin(); its != RenderingManager::getInstance()->getEnd(); its++) {

		//
		// 1. Render it into current view
		//
        // place and scale
        glPushMatrix();
        glTranslated((*its)->getX(), (*its)->getY(), (*its)->getDepth());
        glScaled((*its)->getScaleX(), (*its)->getScaleY(), 1.f);

		// bind the source texture and update its content
		(*its)->update();

	    // Blending Function For mixing like in the rendering window
        (*its)->blend();
        (*its)->draw();

        // draw border if active
        if ((*its)->isActive())
            glCallList(ViewRenderWidget::border_large);
        else
            glCallList(ViewRenderWidget::border_thin);

        glPopMatrix();

		//
		// 2. Render it into FBO
		//
        RenderingManager::getInstance()->renderToFrameBuffer(its, first);
        first = false;

    }
	if (first)
		RenderingManager::getInstance()->clearFrameBuffer();

    // last the frame thing
    glCallList(ViewRenderWidget::frame_screen);

    RenderingManager::getInstance()->updatePreviousFrame();
}


void GeometryView::reset()
{
    glScalef(zoom * OutputRenderWindow::getInstance()->getAspectRatio(), zoom, zoom);
    glTranslatef(getPanningX(), getPanningY(), 0.0);
}


void GeometryView::resize(int w, int h)
{
    glViewport(0, 0, w, h);
    viewport[2] = w;
    viewport[3] = h;

    // Setup specific projection and view for this window
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();

    if (w > h)
         glOrtho(-SOURCE_UNIT* (double) w / (double) h, SOURCE_UNIT*(double) w / (double) h, -SOURCE_UNIT, SOURCE_UNIT, -MAX_DEPTH_LAYER, 10.0);
     else
         glOrtho(-SOURCE_UNIT, SOURCE_UNIT, -SOURCE_UNIT*(double) h / (double) w, SOURCE_UNIT*(double) h / (double) w, -MAX_DEPTH_LAYER, 10.0);

    refreshMatrices();
}


bool GeometryView::mousePressEvent(QMouseEvent *event)
{
	lastClicPos = event->pos();

	if (event->buttons() & Qt::MidButton) {
		RenderingManager::getRenderingWidget()->setCursor(Qt::SizeAllCursor);
	}
	// if at least one source icon was clicked
	else if ( getSourcesAtCoordinates(event->x(), viewport[3] - event->y()) ) {

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

			// now manipulate the current one.
    		quadrant = getSourceQuadrant(RenderingManager::getInstance()->getCurrentSource(), event->x(), viewport[3] - event->y());
			if (quadrant > 0) {
				currentAction = GeometryView::SCALE;
				if ( quadrant % 2 )
					RenderingManager::getRenderingWidget()->setCursor(Qt::SizeFDiagCursor);
				else
					RenderingManager::getRenderingWidget()->setCursor(Qt::SizeBDiagCursor);
			} else  {
				currentAction = GeometryView::MOVE;
				RenderingManager::getRenderingWidget()->setCursor(Qt::ClosedHandCursor);
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
	// LEFT button : MOVE or SCALE the current source
	else if (event->buttons() & Qt::LeftButton) {
		// keep the iterator of the current source under the shoulder ; it will be used
		SourceSet::iterator cs = RenderingManager::getInstance()->getCurrentSource();
		if ( RenderingManager::getInstance()->notAtEnd(cs)) {
			// manipulate the current source according to the operation detected when clicking
			if (currentAction == GeometryView::SCALE)
				scaleSource(cs, event->x(), viewport[3] - event->y(), dx, dy);
			else if (currentAction == GeometryView::MOVE)
				grabSource(cs, event->x(), viewport[3] - event->y(), dx, dy);

		}
//		} else if (event->buttons() & Qt::RightButton) {


	} else  { // mouse over (no buttons)

	//	SourceSet::iterator over = getSourceAtCoordinates(event->x(), viewport[3] - event->y());


	}

	return true;
}

bool GeometryView::mouseReleaseEvent ( QMouseEvent * event ){

	RenderingManager::getRenderingWidget()->setCursor(Qt::ArrowCursor);

    // enforces minimal size ; check that the rescaling did not go bellow the limits and fix it
	if ( RenderingManager::getInstance()->notAtEnd( RenderingManager::getInstance()->getCurrentSource()) ) {
		(*RenderingManager::getInstance()->getCurrentSource())->clampScale();
	}

	currentAction = GeometryView::NONE;
	return true;
}

bool GeometryView::wheelEvent ( QWheelEvent * event ){


	float previous = zoom;
	setZoom (zoom + ((float) event->delta() * zoom * minzoom) / (120.0 * maxzoom) );

	if (currentAction == GeometryView::SCALE || currentAction == GeometryView::MOVE ){
		deltazoom = 1.0 - (zoom / previous);
		// keep the iterator of the current source under the shoulder ; it will be used
		SourceSet::iterator cs = RenderingManager::getInstance()->getCurrentSource();
		if ( RenderingManager::getInstance()->notAtEnd(cs)) {
			// manipulate the current source according to the operation detected when clicking
			if (currentAction == GeometryView::SCALE)
				scaleSource(cs, event->x(), viewport[3] - event->y(), 0, 0);
			else if (currentAction == GeometryView::MOVE)
				grabSource(cs, event->x(), viewport[3] - event->y(), 0, 0);

		}
		// reset deltazoom
		deltazoom = 0;
	}



	return true;
}


bool GeometryView::mouseDoubleClickEvent ( QMouseEvent * event ){


	// for LEFT double button clic : expand the current source to the rendering area
	if ( (event->buttons() & Qt::LeftButton) && getSourcesAtCoordinates(event->x(), viewport[3] - event->y()) ) {

		if ( RenderingManager::getInstance()->getCurrentSource() != RenderingManager::getInstance()->getEnd()){

			(*RenderingManager::getInstance()->getCurrentSource())->resetScale();
		} else
			zoomBestFit();

	}

	return true;
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
        glScaled( (*its)->getScaleX(), (*its)->getScaleY(), 1.f);
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

/**
 *
 **/
void GeometryView::grabSource(SourceSet::iterator currentSource, int x, int y, int dx, int dy) {

    double bx, by, bz; // before movement
    double ax, ay, az; // after  movement

    gluUnProject((GLdouble) (x - dx), (GLdouble) (y - dy),
            0.0, modelview, projection, viewport, &bx, &by, &bz);
    gluUnProject((GLdouble) x, (GLdouble) y, 0.0,
            modelview, projection, viewport, &ax, &ay, &az);

    ax += (ax + getPanningX()) * deltazoom;
    ay += (ay + getPanningY()) * deltazoom;

    double ix = (*currentSource)->getX() + (ax - bx);
    double iy = (*currentSource)->getY() + (ay - by);

    // move source
    (*currentSource)->moveTo(ix, iy);

}

/**
 *
 **/
void GeometryView::scaleSource(SourceSet::iterator currentSource, int X, int Y, int dx, int dy) {

    double bx, by, bz; // before movement
    double ax, ay, az; // after  movement

    // make proportionnal scaling

    gluUnProject((GLdouble) (X - dx), (GLdouble) (Y - dy),
            1.0, modelview, projection, viewport, &bx, &by, &bz);
    gluUnProject((GLdouble) X, (GLdouble) Y, 1.0,
            modelview, projection, viewport, &ax, &ay, &az);


    double w = ((*currentSource)->getScaleX());
    double x = (*currentSource)->getX();
    double h = ((*currentSource)->getScaleY());
    double y = (*currentSource)->getY();
    double sx = 1.0, sy = 1.0;
    double xp = x, yp = y;

    ax += (ax + getPanningX()) * deltazoom;
    ay += (ay + getPanningY()) * deltazoom;

    if ( quadrant == 2 || quadrant == 3) {  // RIGHT
            sx = (ax - x + w) / ( bx - x + w);
            xp = x + w * (sx - 1.0);
    } else {                                // LEFT
            sx = (ax - x - w) / ( bx - x - w);
            xp = x - w * (sx - 1.0);
    }

    if ( quadrant < 3 ){                    // TOP
            sy = (ay - y + h) / ( by - y + h);
            yp = y + h * (sy - 1.0);
    } else {                                // BOTTOM
            sy = (ay - y - h) / ( by - y - h);
            yp = y - h * (sy - 1.0);
    }

    (*currentSource)->scaleBy(sx, sy);
    (*currentSource)->moveTo(xp, yp);
}


/**
 *
 **/
char GeometryView::getSourceQuadrant(SourceSet::iterator currentSource, int X, int Y) {
    //      ax
    //      ^
    //  ----|----
    //  | 3 | 4 |
    //  ----+---- > ay
    //  | 2 | 1 |
    //  ---------
    char quadrant = 0;
    double ax, ay, az;

    gluUnProject((GLdouble) X, (GLdouble) Y, 0.0,
            modelview, projection, viewport, &ax, &ay, &az);

    double w = ((*currentSource)->getScaleX());
    double x = (*currentSource)->getX();
    double h = ((*currentSource)->getScaleY());
    double y = (*currentSource)->getY();

    if (( x > ax + 0.8 * ABS(w) ) && ( y < ay - 0.8 * ABS(h) ) ) // RIGHT BOTTOM
//        quadrant = 1;
        quadrant = h > 0 ? (w > 0 ? (1) : (2)) : (w > 0 ? (4) : (3));
    else if  (( x > ax + 0.8 * ABS(w)) && ( y > ay + 0.8 * ABS(h) ) ) // RIGHT TOP
//        quadrant = 4;
        quadrant = h > 0 ? (w > 0 ? (4) : (3)) : (w > 0 ? (1) : (2));
    else if  (( x < ax - 0.8 * ABS(w)) && ( y < ay - 0.8 * ABS(h) ) ) // LEFT BOTTOM
//        quadrant = 2;
    	quadrant = h > 0 ? (w > 0 ? (2) : (1)) : (w > 0 ? (3) : (4));
    else if  (( x < ax - 0.8 * ABS(w)) && ( y > ay + 0.8 * ABS(h) ) ) // LEFT TOP
//        quadrant = 3;
    	quadrant = h > 0 ? (w > 0 ? (3) : (4)) : (w > 0 ? (2) : (1));

    return quadrant;
}


