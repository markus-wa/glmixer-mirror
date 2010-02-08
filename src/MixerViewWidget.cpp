/*
 * MixerViewWidget.cpp
 *
 *  Created on: Nov 9, 2009
 *      Author: bh
 */

#include "MixerViewWidget.moc"

#include "common.h"
#include "MainRenderWidget.h"

#define MINZOOM 0.04
#define MAXZOOM 1.0
#define DEFAULTZOOM 0.1

MixerViewWidget::MixerViewWidget( QWidget * parent, const QGLWidget * shareWidget)
	: glRenderWidget(parent, shareWidget), currentAction(NONE)
{
	zoom = DEFAULTZOOM;
	minzoom = MINZOOM;
	maxzoom = MAXZOOM;

	setMouseTracking(true);
	setFocusPolicy(Qt::ClickFocus);
}

MixerViewWidget::~MixerViewWidget() {

}

void MixerViewWidget::paintGL()
{
	glRenderWidget::paintGL();

    glScalef(zoom, zoom, zoom);

    // First the circles and other background stuff
    glCallList(MainRenderWidget::circle_mixing);

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
	for(SourceSet::iterator  its = MainRenderWidget::getInstance()->getBegin(); its != MainRenderWidget::getInstance()->getEnd(); its++) {

		glPushMatrix();
		glTranslated((*its)->getAlphaX(), (*its)->getAlphaY(), (*its)->getDepth());
		glScalef( SOURCE_UNIT * (*its)->getAspectRatio(),  SOURCE_UNIT, 1.f);

		if ((*its)->isActive())
			glCallList(MainRenderWidget::border_large_shadow);
		else
			glCallList(MainRenderWidget::border_thin_shadow);

		(*its)->draw(true);
		glCallList(MainRenderWidget::quad_half_textured);
		glPopMatrix();

	}

    // Then the selection outlines
    for(SourceSet::iterator  its = selection.begin(); its != selection.end(); its++) {
        glPushMatrix();
        glTranslated((*its)->getAlphaX(), (*its)->getAlphaY(), (*its)->getDepth());
        glScalef( SOURCE_UNIT * (*its)->getAspectRatio(), SOURCE_UNIT, 1.f);
		glCallList(MainRenderWidget::frame_selection);
        glPopMatrix();

    }
}


void MixerViewWidget::initializeGL()
{
    glRenderWidget::initializeGL();

    viewport[0] = 0;
    viewport[1] = 0;
    viewport[2] = this->width();
    viewport[3] = this->height();

	setBackgroundColor( QColor(52,52,52) );

}


void MixerViewWidget::resizeGL(int w, int h)
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



void MixerViewWidget::setAction(actionType a){

	currentAction = a;

	switch(a) {
	case OVER:
		setCursor(Qt::OpenHandCursor);
		break;
	case GRAB:
		setCursor(Qt::ClosedHandCursor);
		break;
	case SELECT:
		setCursor(Qt::PointingHandCursor);
		break;
	default:
		setCursor(Qt::ArrowCursor);
	}
}

void MixerViewWidget::mousePressEvent(QMouseEvent *event)
{
	lastClicPos = event->pos();

    if (event->buttons() & Qt::LeftButton) {
    	// What was cliked ?
    	SourceSet::iterator cliked = getSourceAtCoordinates(event->x(), viewport[3] - event->y());

    	// if a source icon was cliked
        if ( MainRenderWidget::getInstance()->notAtEnd(cliked) ) {

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
				MainRenderWidget::getInstance()->setCurrentSource( cliked );
				// ready for grabbing the current source
				setAction(GRAB);
			}
		} else {
			// set current to none (end of list)
			MainRenderWidget::getInstance()->setCurrentSource( MainRenderWidget::getInstance()->getEnd() );
			setAction(NONE);
		}

    } else if (event->buttons() & Qt::RightButton) {
    	// TODO context menu
    }

	event->accept();
}

void MixerViewWidget::mouseMoveEvent(QMouseEvent *event)
{
    int dx = event->x() - lastClicPos.x();
    int dy = lastClicPos.y() - event->y();
    lastClicPos = event->pos();

    if (event->buttons() & Qt::LeftButton) {
    	SourceSet::iterator cs = MainRenderWidget::getInstance()->getCurrentSource();
        if ( MainRenderWidget::getInstance()->notAtEnd(cs)) {

        	setAction(GRAB);
            if ( find_if( selection.begin(), selection.end(), hasName( (*cs)->getId()) ) != selection.end() ){
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
		if ( MainRenderWidget::getInstance()->notAtEnd(over))
			if (QApplication::keyboardModifiers () == Qt::ControlModifier)
				setAction(SELECT);
			else
				setAction(OVER);
		else
			setAction(NONE);

    }


	event->accept();
}

void MixerViewWidget::mouseReleaseEvent ( QMouseEvent * event ){

	if (currentAction == GRAB )
		setAction(OVER);
	else
		setAction(currentAction);

	event->accept();
}

void MixerViewWidget::wheelEvent ( QWheelEvent * event ){

	setAction(NONE);
	setZoom (zoom + ((float) event->delta() * zoom * minzoom) / (120.0 * maxzoom) );

	event->accept();
}

void MixerViewWidget::zoomIn() {setZoom(zoom + ( 2.f * zoom * minzoom) / maxzoom);}
void MixerViewWidget::zoomOut() {setZoom(zoom -  ( 2.f * zoom * minzoom) / maxzoom);}
void MixerViewWidget::zoomReset() {setZoom(DEFAULTZOOM);}
void MixerViewWidget::zoomBestFit() {}


void MixerViewWidget::keyPressEvent ( QKeyEvent * event ){

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
		default:
		 QGLWidget::keyPressEvent(event);
	}
}

SourceSet::iterator  MixerViewWidget::getSourceAtCoordinates(int mouseX, int mouseY) {

	// TODO : really needed?
	makeCurrent();

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
        return MainRenderWidget::getInstance()->getById (selectBuf[ (hits-1) * 4 + 3]);
    } else {
        return MainRenderWidget::getInstance()->getEnd();
    }

}

/**
 *
 **/
void MixerViewWidget::grabSource(SourceSet::iterator s, int x, int y, int dx, int dy) {

	// TODO : really needed?
	makeCurrent();

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



