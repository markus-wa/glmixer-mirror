/*
 * MixerView.cpp
 *
 *  Created on: Nov 9, 2009
 *      Author: bh
 */

#include "MixerView.h"

#include "common.h"
#include "RenderingManager.h"
#include "ViewRenderWidget.h"

#define MINZOOM 0.04
#define MAXZOOM 1.0
#define DEFAULTZOOM 0.1

MixerView::MixerView() : View(), currentAction(NONE)
{
	zoom = DEFAULTZOOM;
	minzoom = MINZOOM;
	maxzoom = MAXZOOM;
	maxpanx = SOURCE_UNIT*MAXZOOM*CIRCLE_SIZE;
	maxpany = SOURCE_UNIT*MAXZOOM*CIRCLE_SIZE;

    icon.load(QString::fromUtf8(":/glmixer/icons/mixer.png"));
}

void MixerView::paint()
{

    // First the circles and other background stuff
    glCallList(ViewRenderWidget::circle_mixing);

    // and the selection connection lines
    glDisable(GL_TEXTURE_2D);
    glLineWidth(3.0);
    glColor4f(0.2, 0.80, 0.2, 1.0);
    glBegin(GL_LINE_LOOP);
    for(SourceSet::iterator  its = selectedSources.begin(); its != selectedSources.end(); its++) {
        glVertex3d((*its)->getAlphaX(), (*its)->getAlphaY(), 0.0);
    }
    glEnd();

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

		// bind the source texture and update its content
		(*its)->update();

		// draw surface (do not set blending from source)
		(*its)->draw();

		// draw stippled version of the source on top
		glCallList(ViewRenderWidget::quad_half_textured);

		glPopMatrix();


		//
		// 2. Render it into FBO
		//
        RenderingManager::getInstance()->renderToFrameBuffer(its, first);
        first = false;

	}

	if (first)
		RenderingManager::getInstance()->clearFrameBuffer();

    // Then the selection outlines
    for(SourceSet::iterator  its = selectedSources.begin(); its != selectedSources.end(); its++) {
        glPushMatrix();
        glTranslated((*its)->getAlphaX(), (*its)->getAlphaY(), (*its)->getDepth());
        glScalef( SOURCE_UNIT * (*its)->getAspectRatio(), SOURCE_UNIT, 1.f);
		glCallList(ViewRenderWidget::frame_selection);
        glPopMatrix();

    }


    RenderingManager::getInstance()->updatePreviousFrame();
}


void MixerView::reset()
{
    glScalef(zoom, zoom, zoom);
    glTranslatef(getPanningX(), getPanningY(), 0.0);
}


void MixerView::resize(int w, int h)
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



void MixerView::setAction(actionType a){

	currentAction = a;

	switch(a) {
	case OVER:
		RenderingManager::getRenderingWidget()->setCursor(Qt::OpenHandCursor);
		break;
	case GRAB:
		RenderingManager::getRenderingWidget()->setCursor(Qt::ClosedHandCursor);
		break;
	case SELECT:
		RenderingManager::getRenderingWidget()->setCursor(Qt::PointingHandCursor);
		break;
	default:
		RenderingManager::getRenderingWidget()->setCursor(Qt::ArrowCursor);
	}
}

bool MixerView::mousePressEvent(QMouseEvent *event)
{
	lastClicPos = event->pos();

	if (event->buttons() & Qt::MidButton) {
		RenderingManager::getRenderingWidget()->setCursor(Qt::SizeAllCursor);
	}
	// if at least one source icon was clicked
	else if (event->buttons() & Qt::LeftButton) {
    	// What was cliked ?
    	SourceSet::iterator cliked = getSourceAtCoordinates(event->x(), viewport[3] - event->y());

    	// if a source icon was cliked
        if ( RenderingManager::getInstance()->notAtEnd(cliked) ) {

        	// if CTRL button modifier pressed, add clicked to selection
			if ( currentAction != GRAB && QApplication::keyboardModifiers () == Qt::ControlModifier) {
				setAction(SELECT);

				if ( selectedSources.count(*cliked) > 0)
					selectedSources.erase( *cliked );
				else
					selectedSources.insert( *cliked );

			}
			else // not in selection (SELECT) action mode, then just set the current active source
			{
				RenderingManager::getInstance()->setCurrentSource( cliked );
				// ready for grabbing the current source
				setAction(GRAB);
			}
		} else {
			// set current to none (end of list)
			RenderingManager::getInstance()->setCurrentSource( RenderingManager::getInstance()->getEnd() );
			setAction(NONE);
		}

    } else if (event->buttons() & Qt::RightButton) {
    	// TODO context menu

	    double LLcorner[3];
	    gluUnProject(event->x(), viewport[3] - event->y(), 1, modelview, projection, viewport, LLcorner, LLcorner+1, LLcorner+2);

		qDebug (" %f %f  ", LLcorner[0], LLcorner[1]);



    } else if (event->buttons() & Qt::MidButton) {
    	// almost a wheel action ; middle mouse = best zoom fit
        zoomBestFit();
    }

	return true;
}

bool MixerView::mouseDoubleClickEvent ( QMouseEvent * event ){


	// for LEFT double button clic : pan the view to zoom on this source
	if ( (event->buttons() & Qt::LeftButton) /*&& getSourceAtCoordinates(event->x(), viewport[3] - event->y()) */) {

		SourceSet::iterator cliked = getSourceAtCoordinates(event->x(), viewport[3] - event->y());
		if ( RenderingManager::getInstance()->notAtEnd(cliked) ) {
			// reset the source

		} else
			zoomBestFit();

	}

	return true;
}


bool MixerView::mouseMoveEvent(QMouseEvent *event)
{
    int dx = event->x() - lastClicPos.x();
    int dy = lastClicPos.y() - event->y();
    lastClicPos = event->pos();

	if (event->buttons() & Qt::MidButton) {

		panningBy(event->x(), viewport[3] - event->y(), dx, dy);

	} else if (event->buttons() & Qt::LeftButton) {
    	SourceSet::iterator cs = RenderingManager::getInstance()->getCurrentSource();
        if ( RenderingManager::getInstance()->notAtEnd(cs)) {

        	setAction(GRAB);
			if ( selectedSources.count(*cs) > 0 ){
                for(SourceSet::iterator  its = selectedSources.begin(); its != selectedSources.end(); its++) {
                    grabSource(its, event->x(), viewport[3] - event->y(), dx, dy);
                }
            }
            else
                grabSource(cs, event->x(), viewport[3] - event->y(), dx, dy);

			return true;
        }
//    } else if (event->buttons() & Qt::RightButton) {

    } else  { // mouse over (no buttons)

    	SourceSet::iterator over = getSourceAtCoordinates(event->x(), viewport[3] - event->y());
		 // selection mode with CTRL modifier
		if ( RenderingManager::getInstance()->notAtEnd(over))
			if (QApplication::keyboardModifiers () == Qt::ControlModifier)
				setAction(SELECT);
			else
				setAction(OVER);
		else
			setAction(NONE);

    }

	return false;
}

bool MixerView::mouseReleaseEvent ( QMouseEvent * event ){

	if (currentAction == GRAB )
		setAction(OVER);
	else
		setAction(currentAction);

	return true;
}

bool MixerView::wheelEvent ( QWheelEvent * event ){


	float previous = zoom;
	setZoom (zoom + ((float) event->delta() * zoom * minzoom) / (120.0 * maxzoom) );

	if (currentAction == GRAB ) {
		deltazoom = 1.0 - (zoom / previous);
		// simulate a grab with no mouse movement but a deltazoom :
		SourceSet::iterator cs = RenderingManager::getInstance()->getCurrentSource();
		if ( RenderingManager::getInstance()->notAtEnd(cs))
			grabSource(cs, event->x(), (viewport[3] - event->y()), 0, 0);
		// reset deltazoom
		deltazoom = 0;
	}
	else
		setAction(NONE);

	return true;
}

void MixerView::zoomReset() {
	setZoom(DEFAULTZOOM);
	setPanningX(0); setPanningY(0);
}

void MixerView::zoomBestFit() {

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


bool MixerView::keyPressEvent ( QKeyEvent * event ){

	if (currentAction == OVER )
		setAction(SELECT);
	else
		setAction(currentAction);

	switch (event->key()) {
		case Qt::Key_Left:
			return true;
		case Qt::Key_Right:
			return true;
		case Qt::Key_Down:
			return true;
		case Qt::Key_Up:
			return true;
		default:
			return false;
	}
}

SourceSet::iterator  MixerView::getSourceAtCoordinates(int mouseX, int mouseY) {

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

    if (hits != 0) {
        // select the top most
        return RenderingManager::getInstance()->getById (selectBuf[ (hits-1) * 4 + 3]);
    } else {
        return RenderingManager::getInstance()->getEnd();
    }

}

/**
 *
 **/
void MixerView::grabSource(SourceSet::iterator s, int x, int y, int dx, int dy) {

    double bx, by, bz; // before movement
    double ax, ay, az; // after  movement

    gluUnProject((GLdouble) (x - dx), (GLdouble) (y - dy),
            0.0, modelview, projection, viewport, &bx, &by, &bz);
    gluUnProject((GLdouble) x, (GLdouble) y, 0.0,
            modelview, projection, viewport, &ax, &ay, &az);

    ax += (ax + getPanningX()) * deltazoom;
    ay += (ay + getPanningY()) * deltazoom;

    double ix = (*s)->getAlphaX() + ax - bx;
    double iy = (*s)->getAlphaY() + ay - by;

    // move icon
    (*s)->setAlphaCoordinates( ix, iy );

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



