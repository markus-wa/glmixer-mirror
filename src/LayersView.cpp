/*
 * LayersView.cpp
 *
 *  Created on: Feb 26, 2010
 *      Author: bh
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

LayersView::LayersView(): lookatdistance(DEFAULT_LOOKAT), currentSourceDisplacement(0) {

	zoom = DEFAULTZOOM;
	minzoom = MINZOOM;
	maxzoom = MAX_DEPTH_LAYER;
	maxpanx = lookatdistance;
	maxpany = lookatdistance;
	maxpanz = lookatdistance;
	zoomReset();

	icon.load(QString::fromUtf8(":/glmixer/icons/depth.png"));
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
    // First the background stuff
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glBlendEquation(GL_FUNC_ADD);
    glCallList(ViewRenderWidget::layerbg);

	glPushMatrix();
	glScalef(OutputRenderWindow::getInstance()->getAspectRatio() / SOURCE_UNIT, 1.0 / SOURCE_UNIT, 1.0 / SOURCE_UNIT);
    glCallList(ViewRenderWidget::quad_black);
    glCallList(ViewRenderWidget::frame_screen);
	glPopMatrix();

    // Second the icons of the sources (reversed depth order)
    // render in the depth order
    glEnable(GL_TEXTURE_2D);
    bool first = true;
	for(SourceSet::iterator  its = RenderingManager::getInstance()->getBegin(); its != RenderingManager::getInstance()->getEnd(); its++) {

		//
		// 1. Render it into current view
		//

		glPushMatrix();

		if ((*its)->isActive()) {
			// animated displacement
			if (currentSourceDisplacement < MAXDISPLACEMENT)
				currentSourceDisplacement += ( MAXDISPLACEMENT + 0.1 - currentSourceDisplacement) * 10.f / RenderingManager::getRenderingWidget()->getFPS();
			glTranslatef( currentSourceDisplacement, 0.0,  1.0 +(*its)->getDepth());

		} else
			glTranslatef( 0.0, 0.0,  1.0 +(*its)->getDepth());

        glScalef((*its)->getAspectRatio(), 1.0, 1.0);

        // draw border if active
        if ((*its)->isActive())
            glCallList(ViewRenderWidget::border_large_shadow);
        else
            glCallList(ViewRenderWidget::border_thin_shadow);

	    // Blending Function For mixing like in the rendering window
        (*its)->beginEffectsSection();
		// bind the source texture and update its content
		(*its)->update();

		// draw surface (do not set blending from source)
		(*its)->blend();
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


    // the source dropping icon
    Source *s = RenderingManager::getInstance()->getSourceBasketTop();
    if ( s ){
    	double depth = 0.0, dumm = 0.0;
    	unProjectDepth(lastClicPos.x(), lastClicPos.y(), 0.0, 0.0, &depth, &dumm);

		glPushMatrix();
		currentSourceDisplacement = MAXDISPLACEMENT;
		glTranslated( currentSourceDisplacement, 0.0, 1.0 + depth);
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

	if (w > 0 && h > 0) {
		viewport[2] = w;
		viewport[3] = h;
	}
	glViewport(0, 0, viewport[2], viewport[3]);

    // Setup specific projection and view for this window
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(50.0f, (float)  viewport[2] / (float)  viewport[3], 0.1f, lookatdistance * 10.0f);

	glGetDoublev(GL_PROJECTION_MATRIX, projection);

}


void LayersView::setAction(actionType a){

	currentAction = a;

	switch(a) {
	case OVER:
		RenderingManager::getRenderingWidget()->setMouseCursor(ViewRenderWidget::MOUSE_HAND_OPEN);
		break;
	case GRAB:
		RenderingManager::getRenderingWidget()->setMouseCursor(ViewRenderWidget::MOUSE_HAND_CLOSED);
		break;
	default:
		RenderingManager::getRenderingWidget()->setMouseCursor(ViewRenderWidget::MOUSE_ARROW);
	}
}

bool LayersView::mousePressEvent(QMouseEvent *event)
{
	lastClicPos = event->pos();

	// MIDDLE BUTTON ; panning cursor
	if (event->buttons() & Qt::MidButton) {
		RenderingManager::getRenderingWidget()->setMouseCursor(ViewRenderWidget::MOUSE_SIZEALL);
	}
	// DRoP MODE ; explicitly do nothing
	else if ( RenderingManager::getInstance()->getSourceBasketTop() ) {
		RenderingManager::getRenderingWidget()->setMouseCursor(ViewRenderWidget::MOUSE_QUESTION);
		// don't interpret other mouse events in drop mode
		return false;
	}
	// if at least one source icon was clicked
	else if ( getSourcesAtCoordinates(event->x(), viewport[3] - event->y()) ) {

    	// get the top most clicked source
    	SourceSet::iterator clicked = clickedSources.begin();

    	// for LEFT button clic : manipulate only the top most or the newly clicked
    	if (event->buttons() & Qt::LeftButton) {

			//  make the top most source clicked now the newly current one
			if (RenderingManager::getInstance()->setCurrentSource( (*clicked)->getId() )) {
				// now manipulate the current one.
				currentSourceDisplacement = 0;
				// ready for grabbing the current source
				setAction(GRAB);
			}

    	}
    	// for RIGHT button clic : switch the currently active source to the one bellow, if exists
    	else if (event->buttons() & Qt::RightButton) {


    	}
    } else {
		// set current to none (end of list)
		RenderingManager::getInstance()->setCurrentSource( RenderingManager::getInstance()->getEnd() );
		setAction(NONE);
    }

	return true;
}

bool LayersView::mouseMoveEvent(QMouseEvent *event)
{
    int dx = event->x() - lastClicPos.x();
    int dy = lastClicPos.y() - event->y();
    lastClicPos = event->pos();

	// MIDDLE button ; rotation
	if (event->buttons() & Qt::MidButton) {
		// move the view
		panningBy(event->x(), event->y(), dx, dy);
	}
	// DROP MODE : avoid other actions
	else if ( RenderingManager::getInstance()->getSourceBasketTop() ) {

		RenderingManager::getRenderingWidget()->setMouseCursor(ViewRenderWidget::MOUSE_QUESTION);
		// don't interpret mouse events in drop mode
		return false;
	}
	// LEFT BUTTON : grab
	else if (event->buttons() & Qt::LeftButton) {

		// keep the iterator of the current source under the shoulder ; it will be used
		SourceSet::iterator cs = RenderingManager::getInstance()->getCurrentSource();
		if ( RenderingManager::getInstance()->notAtEnd(cs)) {
			// move the source in depth
			grabSource(cs, event->x(), event->y(), dx, dy);
			// ready for grabbing the current source
			setAction(GRAB);
		}
	} else  { // mouse over (no buttons)

		if ( getSourcesAtCoordinates(event->x(), viewport[3] - event->y()) )
			setAction(OVER);
		else
			setAction(NONE);
	}

	return true;
}

bool LayersView::mouseReleaseEvent ( QMouseEvent * event ){

	if ( RenderingManager::getInstance()->getSourceBasketTop() )
			RenderingManager::getRenderingWidget()->setMouseCursor(ViewRenderWidget::MOUSE_QUESTION);
	else if (currentAction == GRAB )
		setAction(OVER);
	else
		setAction(currentAction);

	return true;
}

bool LayersView::wheelEvent ( QWheelEvent * event ){

    int dx = event->x() - lastClicPos.x();
    int dy = lastClicPos.y() - event->y();
    lastClicPos = event->pos();

	float previous = zoom;
	setZoom (zoom - ((float) event->delta() * zoom * minzoom) / (120.0 * maxzoom) );

	if (currentAction == GRAB) {
		deltazoom = zoom - previous;
		// simulate a grab with no mouse movement but a deltazoom :
		SourceSet::iterator cs = RenderingManager::getInstance()->getCurrentSource();
		if ( RenderingManager::getInstance()->notAtEnd(cs))
			grabSource(cs, event->x(), event->y(), dx, dy);
		// reset deltazoom
		deltazoom = 0;
	}

	return true;
}

void LayersView::zoomReset() {

	lookatdistance = DEFAULT_LOOKAT;
	setZoom(DEFAULTZOOM);
	setPanningX(-2.0);
	setPanningY(0.0);
	setPanningZ(0.0);

}

void LayersView::zoomBestFit() {

	// nothing to do if there is no source
	if (RenderingManager::getInstance()->getBegin() == RenderingManager::getInstance()->getEnd()){
		zoomReset();
		return;
	}

	// Compute bounding depths of every sources
    double z_min = 10000, z_max = -10000;
	for(SourceSet::iterator  its = RenderingManager::getInstance()->getBegin(); its != RenderingManager::getInstance()->getEnd(); its++) {
		z_min = MINI (z_min, (*its)->getDepth());
		z_max = MAXI (z_max, (*its)->getDepth());
	}

	setZoom	( z_max );

	// TODO : LayersView::zoomBestFit() also adjust panning
}


bool LayersView::keyPressEvent ( QKeyEvent * event ){


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

bool LayersView::getSourcesAtCoordinates(int mouseX, int mouseY) {

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
        glTranslatef((*its)->isActive() ? currentSourceDisplacement : 0.0, 0.0,  1.0 +(*its)->getDepth());
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

    while (hits != 0) {
    	clickedSources.insert( *(RenderingManager::getInstance()->getById (selectBuf[ (hits-1) * 4 + 3])) );
    	hits--;
    }

    return !clickedSources.empty();
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

    	qDebug("beurk");
		gluUnProject((GLdouble) (x - dx), (GLdouble) (y - dy),
				1.0, modelview, projection, viewport, &bx, &by, depthBeforeDelta);
		gluUnProject((GLdouble) x, (GLdouble) y, 1.0,
				modelview, projection, viewport, &ax, &ay, depth);
    }

}

/**
 *
 **/
void LayersView::grabSource(SourceSet::iterator currentSource, int x, int y, int dx, int dy) {

	double bz = 0.0; // depth before delta movement
	double az = 0.0; // depth at current x and y

	unProjectDepth(x, y, dx, dy, &az, &bz);

    // (az-bz) is the depth change caused by the mouse mouvement
    // deltazoom is the depth change due to zooming in/out while grabbing
    double newdepth =  (*currentSource)->getDepth() +  az - bz  +  deltazoom;
	currentSource = RenderingManager::getInstance()->changeDepth(currentSource, newdepth > 0 ? newdepth : 0.0);

	// we need to set current again
	RenderingManager::getInstance()->setCurrentSource(currentSource);

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
	setPanningX(getPanningX() + ax - bx);
	setPanningY(getPanningY() + ay - by);
	setPanningZ(getPanningZ() + az - bz);

	// adjust the looking distance when panning in the Z axis (diagonal)
	lookatdistance = CLAMP( lookatdistance + az - bz, MIN_LOOKAT, MAX_LOOKAT);

}


