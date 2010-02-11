/*
 * MixerView.cpp
 *
 *  Created on: Nov 9, 2009
 *      Author: bh
 */

#include "MixerView.h"

#include "common.h"
#include "RenderingManager.h"

#define MINZOOM 0.04
#define MAXZOOM 1.0
#define DEFAULTZOOM 0.1

MixerView::MixerView() : View(), currentAction(NONE)
{
	zoom = DEFAULTZOOM;
	minzoom = MINZOOM;
	maxzoom = MAXZOOM;

    icon.load(QString::fromUtf8(":/glmixer/icons/mixer.png"));
}

MixerView::~MixerView() {

}

void MixerView::paint()
{

    // First the circles and other background stuff
    glCallList(RenderingManager::circle_mixing);

    // and the selection connection lines
    glDisable(GL_TEXTURE_2D);
    glLineWidth(3.0);
    glColor4f(0.2, 0.80, 0.2, 1.0);
    glBegin(GL_LINE_LOOP);
    for(SourceSet::iterator  its = selection.begin(); its != selection.end(); its++) {
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

		if ((*its)->isActive())
			glCallList(RenderingManager::border_large_shadow);
		else
			glCallList(RenderingManager::border_thin_shadow);

		// bind the source texture and update its content
		(*its)->update();
		// draw surface
		(*its)->draw();
		glCallList(RenderingManager::quad_half_textured);
		glPopMatrix();


		//
		// 2. Render it into FBO
		//
		glPushAttrib(GL_ALL_ATTRIB_BITS);
		glViewport(0, 0, RenderingManager::getInstance()->getFrameBufferWidth(), RenderingManager::getInstance()->getFrameBufferHeight());

		glMatrixMode(GL_PROJECTION);
		glPushMatrix();
		glLoadIdentity();
		gluOrtho2D(-SOURCE_UNIT, SOURCE_UNIT, -SOURCE_UNIT, SOURCE_UNIT);

		glMatrixMode(GL_MODELVIEW);
		glPushMatrix();
		glLoadIdentity();

		// render to the framebuffer object
		RenderingManager::getInstance()->bindFrameBuffer();
		{
			if (first) {
			    glClearColor(0.0, 0.0, 0.0, 1.0f);
				glClear(GL_COLOR_BUFFER_BIT);
				first = false;
			}

		    // Blending Function For transparency Based On Source Alpha Value
		    glBlendFunc(GL_SRC_ALPHA, GL_ONE);

			glTranslated((*its)->getX(), (*its)->getY(), 0.0);
			glScaled((*its)->getScaleX(), (*its)->getScaleY(), 1.f);

	        (*its)->draw();
		}
		RenderingManager::getInstance()->releaseFrameBuffer();

		// pop the projection matrix and GL state back for rendering the current view
		// to the actual widget
		glPopAttrib();
		glMatrixMode(GL_PROJECTION);
		glPopMatrix();
		glMatrixMode(GL_MODELVIEW);
		glPopMatrix();

	}

    // Then the selection outlines
    for(SourceSet::iterator  its = selection.begin(); its != selection.end(); its++) {
        glPushMatrix();
        glTranslated((*its)->getAlphaX(), (*its)->getAlphaY(), (*its)->getDepth());
        glScalef( SOURCE_UNIT * (*its)->getAspectRatio(), SOURCE_UNIT, 1.f);
		glCallList(RenderingManager::frame_selection);
        glPopMatrix();

    }
}


void MixerView::reset()
{

    glScalef(zoom, zoom, zoom);

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
         glOrtho(-SOURCE_UNIT* (double) w / (double) h, SOURCE_UNIT*(double) w / (double) h, -SOURCE_UNIT, SOURCE_UNIT, -1.0, 100.0);
     else
         glOrtho(-SOURCE_UNIT, SOURCE_UNIT, -SOURCE_UNIT*(double) h / (double) w, SOURCE_UNIT*(double) h / (double) w, -1.0, 100.0);

    refreshMatrices();
}



void MixerView::setAction(actionType a){

	currentAction = a;

	switch(a) {
	case OVER:
		RenderingManager::getQGLWidget()->setCursor(Qt::OpenHandCursor);
		break;
	case GRAB:
		RenderingManager::getQGLWidget()->setCursor(Qt::ClosedHandCursor);
		break;
	case SELECT:
		RenderingManager::getQGLWidget()->setCursor(Qt::PointingHandCursor);
		break;
	default:
		RenderingManager::getQGLWidget()->setCursor(Qt::ArrowCursor);
	}
}

void MixerView::mousePressEvent(QMouseEvent *event)
{
	lastClicPos = event->pos();

    if (event->buttons() & Qt::LeftButton) {
    	// What was cliked ?
    	SourceSet::iterator cliked = getSourceAtCoordinates(event->x(), viewport[3] - event->y());

    	// if a source icon was cliked
        if ( RenderingManager::getInstance()->notAtEnd(cliked) ) {

        	// if CTRL button modifier pressed, add clicked to selection
			if ( currentAction != GRAB && QApplication::keyboardModifiers () == Qt::ControlModifier) {
				setAction(SELECT);

				if ( selection.count(*cliked) > 0)
					selection.erase( *cliked );
				else
					selection.insert( *cliked );

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
    }

}

void MixerView::mouseMoveEvent(QMouseEvent *event)
{
    int dx = event->x() - lastClicPos.x();
    int dy = lastClicPos.y() - event->y();
    lastClicPos = event->pos();

    if (event->buttons() & Qt::LeftButton) {
    	SourceSet::iterator cs = RenderingManager::getInstance()->getCurrentSource();
        if ( RenderingManager::getInstance()->notAtEnd(cs)) {

        	setAction(GRAB);
			//            if ( find_if( selection.begin(), selection.end(), hasName( (*cs)->getId()) ) != selection.end() ){
			if ( selection.count(*cs) > 0 ){
                for(SourceSet::iterator  its = selection.begin(); its != selection.end(); its++) {
                    grabSource(its, event->x(), viewport[3] - event->y(), dx, dy);
                }
            }
            else
                grabSource(cs, event->x(), viewport[3] - event->y(), dx, dy);
        }
    } else if (event->buttons() & Qt::RightButton) {

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

}

void MixerView::mouseReleaseEvent ( QMouseEvent * event ){

	if (currentAction == GRAB )
		setAction(OVER);
	else
		setAction(currentAction);

}

void MixerView::wheelEvent ( QWheelEvent * event ){

	setAction(NONE);
	setZoom (zoom + ((float) event->delta() * zoom * minzoom) / (120.0 * maxzoom) );

}

void MixerView::zoomReset() {setZoom(DEFAULTZOOM);}
void MixerView::zoomBestFit() {}


void MixerView::keyPressEvent ( QKeyEvent * event ){

	if (currentAction == OVER )
		setAction(SELECT);
	else
		setAction(currentAction);

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

    double ix = (*s)->getAlphaX() + ax - bx;
    double iy = (*s)->getAlphaY() + ay - by;

    // move icon
    (*s)->setAlphaCoordinates( ix, iy, CIRCLE_SIZE);

}



