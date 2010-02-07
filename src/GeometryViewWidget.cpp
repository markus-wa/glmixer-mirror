/*
 * GeometryViewWidget.cpp
 *
 *  Created on: Jan 31, 2010
 *      Author: bh
 */

#include "GeometryViewWidget.moc"

#include "MainRenderWidget.h"
#include <algorithm>

#define MINZOOM 0.1
#define MAXZOOM 3.0
#define DEFAULTZOOM 0.5


GeometryViewWidget::GeometryViewWidget(QWidget * parent, const QGLWidget * shareWidget)
	: glRenderWidget(parent, shareWidget), quadrant(0), currentAction(NONE)
{
	zoom = DEFAULTZOOM;
	minzoom = MINZOOM;
	maxzoom = MAXZOOM;
	maxpanx = SOURCE_UNIT*MAXZOOM*2.0;
	maxpany = SOURCE_UNIT*MAXZOOM*2.0;

	setMouseTracking(true);
}


GeometryViewWidget::~GeometryViewWidget() {
	// TODO Auto-generated destructor stub
}


void GeometryViewWidget::paintGL()
{
	glRenderWidget::paintGL();

    glScalef(zoom * MainRenderWidget::getInstance()->getRenderingAspectRatio(), zoom, zoom);
    glTranslatef(getPanningX(), getPanningY(), 0.0);

    // first the black background (as the rendering black clear color) with shadow
    glCallList(MainRenderWidget::quad_black);

    // then the icons of the sources (reversed depth order)
	for(SourceSet::iterator  its = MainRenderWidget::getInstance()->getBegin(); its != MainRenderWidget::getInstance()->getEnd(); its++) {

        glPushMatrix();
        // place and scale
        glTranslated((*its)->getX(), (*its)->getY(), (*its)->getDepth());
        glScaled((*its)->getScaleX(), (*its)->getScaleY(), 1.f);
        // draw border if active
        if ((*its)->isActive())
            glCallList(MainRenderWidget::border_large);
        else
            glCallList(MainRenderWidget::border_thin);

        (*its)->draw();

        glPopMatrix();

    }

    // last the frame thing
    glCallList(MainRenderWidget::frame_screen);

}


void GeometryViewWidget::initializeGL()
{
    glRenderWidget::initializeGL();

    viewport[0] = 0;
    viewport[1] = 0;
    viewport[2] = this->width();
    viewport[3] = this->height();

	setBackgroundColor( QColor(52,52,52) );
}


void GeometryViewWidget::resizeGL(int w, int h)
{
    glViewport(0, 0, w, h);
    viewport[2] = w;
    viewport[3] = h;

    // Setup specific projection and view for this window
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();

    if (w > h)
         glOrtho(-SOURCE_UNIT* (double) w / (double) h, SOURCE_UNIT*(double) w / (double) h, -SOURCE_UNIT, SOURCE_UNIT, -1.0, 100.0);
     else
         glOrtho(-SOURCE_UNIT, SOURCE_UNIT, -SOURCE_UNIT*(double) h / (double) w, SOURCE_UNIT*(double) h / (double) w, -1.0, 100.0);

    glGetDoublev(GL_PROJECTION_MATRIX, projection);

    glMatrixMode(GL_MODELVIEW);
    glGetDoublev(GL_MODELVIEW_MATRIX, modelview);
}


void GeometryViewWidget::mousePressEvent(QMouseEvent *event)
{
	lastClicPos = event->pos();


	if (event->buttons() & Qt::MidButton) {
		setCursor(Qt::SizeAllCursor);
	}
	// if at least one source icon was clicked (and fill-in the selection of sources under mouse)
	else if ( getSourcesAtCoordinates(event->x(), viewport[3] - event->y()) ) {

    	// get the top most clicked source
    	SourceSet::iterator clicked = selection.begin();

    	// for LEFT button clic : manipulate only the top most or the newly clicked
    	if (event->buttons() & Qt::LeftButton) {

    		// if there was no current source, its simple : just take the top most source clicked now
    		// OR
			// if the currently active source is NOT in the set of clicked sources,
			if ( MainRenderWidget::getInstance()->getCurrentSource() == MainRenderWidget::getInstance()->getEnd()
					|| std::find_if(selection.begin(), selection.end(), hasName((*MainRenderWidget::getInstance()->getCurrentSource())->getId())) ==  selection.end())
    			//  make the top most source clicked now the newly current one
    			MainRenderWidget::getInstance()->setCurrentSource( (*clicked)->getId() );

			// now manipulate the current one.
    		quadrant = getSourceQuadrant(MainRenderWidget::getInstance()->getCurrentSource(), event->x(), viewport[3] - event->y());
			if (quadrant > 0) {
				currentAction = GeometryViewWidget::SCALE;
				if ( quadrant % 2 )
					setCursor(Qt::SizeFDiagCursor);
				else
					setCursor(Qt::SizeBDiagCursor);
			} else  {
				currentAction = GeometryViewWidget::MOVE;
				setCursor(Qt::ClosedHandCursor);
			}
    	}
    	// for RIGHT button clic : switch the currently active source to the one bellow, if exists
    	else if (event->buttons() & Qt::RightButton) {

    		// if there is no source selected, select the top most
    		if ( MainRenderWidget::getInstance()->getCurrentSource() == MainRenderWidget::getInstance()->getEnd() )
    			MainRenderWidget::getInstance()->setCurrentSource( (*clicked)->getId() );
    		// else, try to take another one bellow it
    		else {
    			// find where the current source is in the selection
    			clicked = std::find_if(selection.begin(), selection.end(), hasName((*MainRenderWidget::getInstance()->getCurrentSource())->getId()));
    			// decrement the clicked iterator forward in the selection (and jump back to end when at begining)
    			if ( clicked == selection.begin() )
    				clicked = selection.end();
				clicked--;
				// set this newly clicked source as the current one
    			MainRenderWidget::getInstance()->setCurrentSource( (*clicked)->getId() );
    		}
    	}
    } else
		// set current to none (end of list)
		MainRenderWidget::getInstance()->setCurrentSource( MainRenderWidget::getInstance()->getEnd() );

	event->accept();
}

void GeometryViewWidget::mouseMoveEvent(QMouseEvent *event)
{
    int dx = event->x() - lastClicPos.x();
    int dy = lastClicPos.y() - event->y();
    lastClicPos = event->pos();

//    // if mouse not out of the window
//    if (lastClicPos.x()>viewport[0] && lastClicPos.x()<viewport[2] && lastClicPos.y()>viewport[1] && lastClicPos.y()<viewport[3]  ) {
		// MIDDLE button ; panning
		if (event->buttons() & Qt::MidButton) {

			panningBy(event->x(), viewport[3] - event->y(), dx, dy);

		}
		// LEFT button : MOVE or SCALE the current source
		else if (event->buttons() & Qt::LeftButton) {
			// keep the iterator of the current source under the shoulder ; it will be used
			SourceSet::iterator cs = MainRenderWidget::getInstance()->getCurrentSource();
			if ( MainRenderWidget::getInstance()->notAtEnd(cs)) {
				// manipulate the current source according to the operation detected when clicking
				if (currentAction == GeometryViewWidget::SCALE)
					scaleSource(cs, event->x(), viewport[3] - event->y(), dx, dy);
				else if (currentAction == GeometryViewWidget::MOVE)
					grabSource(cs, event->x(), viewport[3] - event->y(), dx, dy);

			}
		} else if (event->buttons() & Qt::RightButton) {

		} else  { // mouse over (no buttons)

		//	SourceSet::iterator over = getSourceAtCoordinates(event->x(), viewport[3] - event->y());


		}

//    }
	event->accept();
}

void GeometryViewWidget::mouseReleaseEvent ( QMouseEvent * event ){

	setCursor(Qt::ArrowCursor);
	event->accept();
}

void GeometryViewWidget::wheelEvent ( QWheelEvent * event ){

	setZoom (zoom + ((float) event->delta() * zoom * minzoom) / (120.0 * maxzoom) );


    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    glScalef(zoom * MainRenderWidget::getInstance()->getRenderingAspectRatio(), zoom, zoom);
    glTranslatef(getPanningX(), getPanningY(), 0.0);

    glGetDoublev(GL_MODELVIEW_MATRIX, modelview);

	event->accept();
}

void GeometryViewWidget::zoomIn() {setZoom(zoom + ( 2.f * zoom * minzoom) / maxzoom);}
void GeometryViewWidget::zoomOut() {setZoom(zoom -  ( 2.f * zoom * minzoom) / maxzoom);}
void GeometryViewWidget::zoomReset() {setZoom(DEFAULTZOOM); setPanningX(0); setPanningY(0);}
void GeometryViewWidget::zoomBestFit() {}


void GeometryViewWidget::keyPressEvent ( QKeyEvent * event ){



}


bool GeometryViewWidget::getSourcesAtCoordinates(int mouseX, int mouseY) {

	// TODO : really needed?
	makeCurrent();

	// prepare variables
	selection.clear();
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
    glGetDoublev(GL_MODELVIEW_MATRIX, modelview);

	for(SourceSet::iterator  its = MainRenderWidget::getInstance()->getBegin(); its != MainRenderWidget::getInstance()->getEnd(); its++) {
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
    	selection.insert( *(MainRenderWidget::getInstance()->getById (selectBuf[ (hits-1) * 4 + 3])) );
    	hits--;
    }

    return !selection.empty();
}


/**
 *
 **/
void GeometryViewWidget::panningBy(int x, int y, int dx, int dy) {

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
void GeometryViewWidget::grabSource(SourceSet::iterator currentSource, int x, int y, int dx, int dy) {

    double bx, by, bz; // before movement
    double ax, ay, az; // after  movement

    gluUnProject((GLdouble) (x - dx), (GLdouble) (y - dy),
            0.0, modelview, projection, viewport, &bx, &by, &bz);
    gluUnProject((GLdouble) x, (GLdouble) y, 0.0,
            modelview, projection, viewport, &ax, &ay, &az);

    double ix = (*currentSource)->getX() + ax - bx;
    double iy = (*currentSource)->getY() + ay - by;

    // move source
    (*currentSource)->moveTo(ix, iy);

}

/**
 *
 **/
void GeometryViewWidget::scaleSource(SourceSet::iterator currentSource, int X, int Y, int dx, int dy) {

    double bx, by, bz; // before movement
    double ax, ay, az; // after  movement

    // make proportionnal scaling

    gluUnProject((GLdouble) (X - dx), (GLdouble) (Y - dy),
            0.0, modelview, projection, viewport, &bx, &by, &bz);
    gluUnProject((GLdouble) X, (GLdouble) Y, 0.0,
            modelview, projection, viewport, &ax, &ay, &az);


    double w = (*currentSource)->getScaleX();
    double x = (*currentSource)->getX();
    double h = (*currentSource)->getScaleY();
    double y = (*currentSource)->getY();
    double sx = 1.0, sy = 1.0;
    double xp = x, yp = y;

    if ( quadrant == 2 || quadrant == 3) {  // RIGHT
        if ( ax > x ) { // prevent change of quadrant
            sx = (ax - x + w) / ( bx - x + w);
            xp = x + w * (sx - 1.0);
        }
    } else {                                // LEFT
        if ( ax < x ) { // prevent change of quadrant
            sx = (ax - x - w) / ( bx - x - w);
            xp = x - w * (sx - 1.0);
        }
    }
    // enforces minimal size
    if (w < (MIN_SCALE * (*currentSource)->getAspectRatio()) && sx < 1.0){ sx=1.0; xp=x; }

    if ( quadrant < 3 ){                    // TOP
        if ( ay > y ) { // prevent change of quadrant
            sy = (ay - y + h) / ( by - y + h);
            yp = y + h * (sy - 1.0);
        }
    } else {                                // BOTTOM
        if ( ay < y ) { // prevent change of quadrant
            sy = (ay - y - h) / ( by - y - h);
            yp = y - h * (sy - 1.0);
        }
    }
    // enforces minimal size
    if (h < MIN_SCALE && sy < 1.0){ sy=1.0; yp=y; }

    (*currentSource)->scaleBy(sx, sy);
    (*currentSource)->moveTo(xp, yp);


}


/**
 *
 **/
char GeometryViewWidget::getSourceQuadrant(SourceSet::iterator currentSource, int X, int Y) {
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

    double w = (*currentSource)->getScaleX();
    double x = (*currentSource)->getX();
    double h = (*currentSource)->getScaleY();
    double y = (*currentSource)->getY();

    if (( x > ax + 0.8*w ) && ( y < ay - 0.8*h) ) // RIGHT BOTTOM
        quadrant = 1;
    else if  (( x > ax + 0.8*w) && ( y > ay + 0.8*h ) ) // RIGHT TOP
        quadrant = 4;
    else if  (( x < ax - 0.8*w) && ( y < ay - 0.8*h) ) // LEFT BOTTOM
        quadrant = 2;
    else if  (( x < ax - 0.8*w) && ( y > ay + 0.8*h ) ) // LEFT TOP
        quadrant = 3;

//    if ( ax > x) { // RIGHT
//        if (ay < y) // BOTTOM
//            quadrant = 3;
//        else
//            quadrant = 2;
//    } else {
//        if (ay < y) // BOTTOM
//            quadrant = 4;
//        else
//            quadrant = 1;
//    }

    return quadrant;
}


