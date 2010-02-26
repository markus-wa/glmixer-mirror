/*
 * LayersView.cpp
 *
 *  Created on: Feb 26, 2010
 *      Author: bh
 */

#include "LayersView.h"

#include "common.h"
#include "RenderingManager.h"
#include "OutputRenderWindow.h"

#define MINZOOM 0.5
#define MAXZOOM 3.0
#define DEFAULTZOOM 1.0

LayersView::LayersView(): lookatdistance(6.0) {

	zoom = DEFAULTZOOM;
	minzoom = MINZOOM;
	maxzoom = MAXZOOM;
	maxpanx = SOURCE_UNIT*MAXZOOM*20.0;
	maxpany = SOURCE_UNIT*MAXZOOM*20.0;
	maxpanz = SOURCE_UNIT*MAXZOOM*20.0;

	icon.load(QString::fromUtf8(":/glmixer/icons/depth.png"));
}


void LayersView::paint()
{
    // First the background stuff
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
        glTranslatef(0.0, 0.0,  1.0 +(*its)->getDepth());
        glScalef((*its)->getAspectRatio(), 1.0, 1.0);

		// bind the source texture and update its content
		(*its)->update();

		// draw surface (do not set blending from source)
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

    // Then the foreground


    RenderingManager::getInstance()->updatePreviousFrame();
}


void LayersView::reset()
{
    gluLookAt(lookatdistance, lookatdistance, lookatdistance+5.0, 0.0, 0.0, 5.0, 0.0, 1.0, 0.0);
    glScalef(zoom, zoom, zoom);
    glTranslatef(getPanningX(), getPanningY(), getPanningZ());

}


void LayersView::resize(int w, int h)
{
    glViewport(0, 0, w, h);
    viewport[2] = w;
    viewport[3] = h;

    // Setup specific projection and view for this window
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(50.0f, (float)  viewport[2] / (float)  viewport[3], 0.1f, lookatdistance * 3.0f);

    refreshMatrices();
}



void LayersView::mousePressEvent(QMouseEvent *event)
{
	lastClicPos = event->pos();


	if (event->buttons() & Qt::MidButton) {
		RenderingManager::getRenderingWidget()->setCursor(Qt::SizeAllCursor);
	}
	// if at least one source icon was clicked
	else if ( getSourcesAtCoordinates(event->x(), viewport[3] - event->y()) ) {

    	// get the top most clicked source
    	SourceSet::iterator clicked = selection.begin();

    	// for LEFT button clic : manipulate only the top most or the newly clicked
    	if (event->buttons() & Qt::LeftButton) {

			//  make the top most source clicked now the newly current one
			RenderingManager::getInstance()->setCurrentSource( (*clicked)->getId() );

			// now manipulate the current one.

    	}
    	// for RIGHT button clic : switch the currently active source to the one bellow, if exists
    	else if (event->buttons() & Qt::RightButton) {


    	}
    } else
		// set current to none (end of list)
		RenderingManager::getInstance()->setCurrentSource( RenderingManager::getInstance()->getEnd() );

}

void LayersView::mouseMoveEvent(QMouseEvent *event)
{
    int dx = event->x() - lastClicPos.x();
    int dy = lastClicPos.y() - event->y();
    lastClicPos = event->pos();

	// MIDDLE button ; rotation
	if (event->buttons() & Qt::MidButton) {

		panningBy(event->x(), viewport[3] - event->y(), dx, dy);

	}
}

void LayersView::mouseReleaseEvent ( QMouseEvent * event ){

	RenderingManager::getRenderingWidget()->setCursor(Qt::ArrowCursor);

}

void LayersView::wheelEvent ( QWheelEvent * event ){

	setZoom (zoom + ((float) event->delta() * zoom * minzoom) / (120.0 * maxzoom) );

}

void LayersView::zoomReset() {
	setZoom(DEFAULTZOOM);
}

void LayersView::zoomBestFit() {



}


void LayersView::keyPressEvent ( QKeyEvent * event ){


	switch (event->key()) {
		case Qt::Key_Left:
		 break;
		case Qt::Key_Right:
		 break;
		case Qt::Key_Down:
		 break;
		case Qt::Key_Up:
		 break;
	}
}

bool LayersView::getSourcesAtCoordinates(int mouseX, int mouseY) {

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

	for(SourceSet::iterator  its = RenderingManager::getInstance()->getBegin(); its != RenderingManager::getInstance()->getEnd(); its++) {
        glPushMatrix();
        // place and scale
        glTranslatef(0.0, 0.0,  1.0 +(*its)->getDepth());
        glScalef((*its)->getAspectRatio(), 1.0, 1.0);
        (*its)->draw(false, GL_SELECT);
        glPopMatrix();
        qDebug ("draw %d ", (*its)->getId());
    }

    // compute picking . return to rendering mode
    hits = glRenderMode(GL_RENDER);

//    qDebug ("%d hits @ (%d,%d) vp (%d, %d, %d, %d)", hits, mouseX, mouseY, viewport[0], viewport[1], viewport[2], viewport[3]);

    // set the matrices back
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);

    while (hits != 0) {
        qDebug ("hit  %d ", selectBuf[ (hits-1) * 4 + 3]);
    	selection.insert( *(RenderingManager::getInstance()->getById (selectBuf[ (hits-1) * 4 + 3])) );
    	hits--;
    }

    return !selection.empty();
}

/**
 *
 **/
void LayersView::panningBy(int x, int y, int dx, int dy) {

	// in a perspective, we need to know the pseudo depth of the object of interest in order
	// to use gluUnproject ; this is obtained by a quick pseudo rendering in FEEDBACK mode

    // feedback rendering to determine a depth
    GLfloat feedbuffer[4];
    glFeedbackBuffer(4, GL_3D, feedbuffer);
    (void) glRenderMode(GL_FEEDBACK);

    // Fake rendering of point (0,0,0)
    glBegin(GL_POINTS);
    glVertex3f(0.0, 0.0, 0.0);
    glEnd();

    // we can make the un-projection if we got the 4 values we need :
    if (glRenderMode(GL_RENDER) == 4) {
        double bx, by, bz; // before movement
        double ax, ay, az; // after  movement

		gluUnProject((GLdouble) (x - dx), (GLdouble) (y - dy),
				feedbuffer[3], modelview, projection, viewport, &bx, &by, &bz);
		gluUnProject((GLdouble) x, (GLdouble) y, feedbuffer[3],
				modelview, projection, viewport, &ax, &ay, &az);

		// apply panning
		setPanningX(getPanningX() + ax - bx);
		setPanningY(getPanningY() + ay - by);
		setPanningZ(getPanningZ() + az - bz);
    }
}


