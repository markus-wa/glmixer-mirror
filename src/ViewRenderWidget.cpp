/*
 * ViewRenderWidget.cpp
 *
 *  Created on: Feb 13, 2010
 *      Author: bh
 */

#include "ViewRenderWidget.moc"

#include "View.h"
#include "MixerView.h"
#include "GeometryView.h"
#include "LayersView.h"
#include "RenderingManager.h"

GLuint ViewRenderWidget::border_thin_shadow = 0, ViewRenderWidget::border_large_shadow = 0;
GLuint ViewRenderWidget::border_thin = 0, ViewRenderWidget::border_large = 0, ViewRenderWidget::border_scale = 0;
GLuint ViewRenderWidget::quad_texured = 0, ViewRenderWidget::quad_black = 0;
GLuint ViewRenderWidget::frame_selection = 0, ViewRenderWidget::frame_screen = 0;
GLuint ViewRenderWidget::circle_mixing = 0, ViewRenderWidget::layerbg = 0;
GLuint ViewRenderWidget::quad_half_textured = 0, ViewRenderWidget::quad_stipped_textured[] = {0,0,0,0};

ViewRenderWidget::ViewRenderWidget() :glRenderWidget() {

	setMouseTracking(true);
	setFocusPolicy(Qt::ClickFocus);

	noView = new View;
    Q_CHECK_PTR(noView);
	mixingManipulationView = new MixerView;
    Q_CHECK_PTR(mixingManipulationView);
	geometryManipulationView = new GeometryView;
    Q_CHECK_PTR(geometryManipulationView);
	layersManipulationView = new LayersView;
    Q_CHECK_PTR(layersManipulationView);
	currentManipulationView = noView;

	displayMessage = false;
	connect(&messageTimer, SIGNAL(timeout()), SLOT(hideMessage()));
	messageTimer.setSingleShot(true);
}

ViewRenderWidget::~ViewRenderWidget() {
	// TODO Auto-generated destructor stub
}


void ViewRenderWidget::initializeGL()
{
    glRenderWidget::initializeGL();

	setBackgroundColor( QColor(52,52,52) );


    if ( !border_thin_shadow ) {
    	border_thin_shadow = buildLineList();
    	border_large_shadow = border_thin_shadow + 1;
    }
	if (!quad_texured)
		quad_texured = buildQuadList();
	if (!quad_half_textured){
		quad_stipped_textured[0] = buildHalfList_fine();
		quad_stipped_textured[1] = buildHalfList_gross();
		quad_stipped_textured[2] = buildHalfList_checkerboard();
		quad_stipped_textured[3] = buildHalfList_triangle();
		quad_half_textured = quad_stipped_textured[0];
	}
	if (!frame_selection)
		frame_selection = buildSelectList();
	if (!circle_mixing)
		circle_mixing = buildCircleList();
	if (!layerbg)
		layerbg = buildLayerbgList();
	if (!quad_black)
		quad_black = buildBlackList();
	if (!frame_screen)
		frame_screen = buildFrameList();
	if (!border_thin) {
		border_thin = buildBordersList();
		border_large = border_thin + 1;
		border_scale = border_thin + 2;
	}

}

void ViewRenderWidget::setViewMode(viewMode mode){

	switch (mode) {
	case MIXING:
		currentManipulationView = (View *) mixingManipulationView;
		showMessage("Mixing View");
		break;
	case GEOMETRY:
		currentManipulationView = (View *) geometryManipulationView;
		showMessage("Geometry View");
		break;
	case LAYER:
		currentManipulationView = (View *) layersManipulationView;
		showMessage("Layers View");
		break;
	case NONE:
	default:
		currentManipulationView = noView;
	}

	// update view to match with the changes in modelview and projection matrices (e.g. resized widget)
	makeCurrent();
	currentManipulationView->resize(width(), height());

}


QPixmap ViewRenderWidget::getViewIcon(){

	return currentManipulationView->getIcon();
}

/**
 *  REDIRECT every calls to the current view implementation
 */

void ViewRenderWidget::resizeGL(int w, int h){
	currentManipulationView->resize(w,h);
}

void ViewRenderWidget::paintGL(){
    glRenderWidget::paintGL();
	currentManipulationView->reset();
	currentManipulationView->paint();

	if (displayMessage){
	    glColor4f(0.8, 0.80, 0.2, 1.0);
		renderText(20, int(3.5*((QApplication::font().pixelSize()>0)?QApplication::font().pixelSize():QApplication::font().pointSize())), message, QFont());
	}
}

void ViewRenderWidget::mousePressEvent(QMouseEvent *event){
	makeCurrent();
	if (!currentManipulationView->mousePressEvent(event))
		QWidget::mousePressEvent(event);
}

void ViewRenderWidget::mouseMoveEvent(QMouseEvent *event){
	makeCurrent();
	if (!currentManipulationView->mouseMoveEvent(event))
		QWidget::mouseMoveEvent(event);
	else {
		if ( currentManipulationView == mixingManipulationView )
			emit sourceMixingModified();
		else if ( currentManipulationView == geometryManipulationView )
			emit sourceGeometryModified();
		else if ( currentManipulationView == layersManipulationView )
			emit sourceLayerModified();

	}
}

void ViewRenderWidget::mouseReleaseEvent ( QMouseEvent * event ){
	makeCurrent();
	if (!currentManipulationView->mouseReleaseEvent(event) )
		QWidget::mouseReleaseEvent(event);
}

void ViewRenderWidget::mouseDoubleClickEvent ( QMouseEvent * event ){
	makeCurrent();
	if(!currentManipulationView->mouseDoubleClickEvent(event))
		QWidget::mouseDoubleClickEvent(event);
//	else
//		emit sourceModified( RenderingManager::getInstance()->getCurrentSource() );
}

void ViewRenderWidget::wheelEvent ( QWheelEvent * event ){
	makeCurrent();
	if(!currentManipulationView->wheelEvent(event))
		QWidget::wheelEvent(event);
}

void ViewRenderWidget::keyPressEvent ( QKeyEvent * event ){
	makeCurrent();
	if (!currentManipulationView->keyPressEvent(event))
		QWidget::keyPressEvent(event);
}

void ViewRenderWidget::zoomIn() {
	makeCurrent();
	currentManipulationView->zoomIn();
	showMessage(QString("%1 \%").arg(currentManipulationView->getZoom(), 0, 'f', 1));
}

void ViewRenderWidget::zoomOut() {
	makeCurrent();
	currentManipulationView->zoomOut();
	showMessage(QString("%1 \%").arg(currentManipulationView->getZoom(), 0, 'f', 1));
}

void ViewRenderWidget::zoomReset() {
	makeCurrent();
	currentManipulationView->zoomReset();
	showMessage(QString("%1 \%").arg(currentManipulationView->getZoom(), 0, 'f', 1));
}

void ViewRenderWidget::zoomBestFit() {
	makeCurrent();
	currentManipulationView->zoomBestFit();
	showMessage(QString("%1 \%").arg(currentManipulationView->getZoom(), 0, 'f', 1));
}

void ViewRenderWidget::showMessage(QString s) {
	if (displayMessage)
		messageTimer.stop();
	message = s;
	messageTimer.start(1000);
	displayMessage = true;
}

/**
 * Build a display list of a textured QUAD and returns its id
 **/
GLuint ViewRenderWidget::buildHalfList_fine() {

    GLubyte halftone[] = {
    	    0xAA, 0xAA, 0xAA, 0xAA, 0x55, 0x55, 0x55, 0x55,
    	    0xAA, 0xAA, 0xAA, 0xAA, 0x55, 0x55, 0x55, 0x55,
    	    0xAA, 0xAA, 0xAA, 0xAA, 0x55, 0x55, 0x55, 0x55,
    	    0xAA, 0xAA, 0xAA, 0xAA, 0x55, 0x55, 0x55, 0x55,
    	    0xAA, 0xAA, 0xAA, 0xAA, 0x55, 0x55, 0x55, 0x55,
    	    0xAA, 0xAA, 0xAA, 0xAA, 0x55, 0x55, 0x55, 0x55,
    	    0xAA, 0xAA, 0xAA, 0xAA, 0x55, 0x55, 0x55, 0x55,
    	    0xAA, 0xAA, 0xAA, 0xAA, 0x55, 0x55, 0x55, 0x55,
    	    0xAA, 0xAA, 0xAA, 0xAA, 0x55, 0x55, 0x55, 0x55,
    	    0xAA, 0xAA, 0xAA, 0xAA, 0x55, 0x55, 0x55, 0x55,
    	    0xAA, 0xAA, 0xAA, 0xAA, 0x55, 0x55, 0x55, 0x55,
    	    0xAA, 0xAA, 0xAA, 0xAA, 0x55, 0x55, 0x55, 0x55,
    	    0xAA, 0xAA, 0xAA, 0xAA, 0x55, 0x55, 0x55, 0x55,
    	    0xAA, 0xAA, 0xAA, 0xAA, 0x55, 0x55, 0x55, 0x55,
    	    0xAA, 0xAA, 0xAA, 0xAA, 0x55, 0x55, 0x55, 0x55,
    	    0xAA, 0xAA, 0xAA, 0xAA, 0x55, 0x55, 0x55, 0x55};

    GLuint id = glGenLists(1);
    glNewList(id, GL_COMPILE);

    glEnable (GL_POLYGON_STIPPLE);
    glPolygonStipple (halftone);

	glColor4f(1.0, 1.0, 1.0, 1.0);

    glBegin(GL_QUADS); // begin drawing a square

    // Front Face (note that the texture's corners have to match the quad's corners)
    glNormal3f(0.0f, 0.0f, 1.0f); // front face points out of the screen on z.

    glTexCoord2f(0.0f, 1.0f);
    glVertex3f(-1.0f, -1.0f, 0.0f); // Bottom Left
    glTexCoord2f(1.0f, 1.0f);
    glVertex3f(1.0f, -1.0f, 0.0f); // Bottom Right
    glTexCoord2f(1.0f, 0.0f);
    glVertex3f(1.0f, 1.0f, 0.0f); // Top Right
    glTexCoord2f(0.0f, 0.0f);
    glVertex3f(-1.0f, 1.0f, 0.0f); // Top Left

    glEnd();

    glDisable (GL_POLYGON_STIPPLE);

    glEndList();
    return id;
}

GLuint ViewRenderWidget::buildHalfList_gross() {

    GLubyte halftone[] = {
    	    0xCC, 0xCC, 0xCC, 0xCC, 0x33, 0x33, 0x33, 0x33,
    	    0x33, 0x33, 0x33, 0x33, 0xCC, 0xCC, 0xCC, 0xCC,
    	    0xCC, 0xCC, 0xCC, 0xCC, 0x33, 0x33, 0x33, 0x33,
    	    0x33, 0x33, 0x33, 0x33, 0xCC, 0xCC, 0xCC, 0xCC,
    	    0xCC, 0xCC, 0xCC, 0xCC, 0x33, 0x33, 0x33, 0x33,
    	    0x33, 0x33, 0x33, 0x33, 0xCC, 0xCC, 0xCC, 0xCC,
    	    0xCC, 0xCC, 0xCC, 0xCC, 0x33, 0x33, 0x33, 0x33,
    	    0x33, 0x33, 0x33, 0x33, 0xCC, 0xCC, 0xCC, 0xCC,
    	    0xCC, 0xCC, 0xCC, 0xCC, 0x33, 0x33, 0x33, 0x33,
    	    0x33, 0x33, 0x33, 0x33, 0xCC, 0xCC, 0xCC, 0xCC,
    	    0xCC, 0xCC, 0xCC, 0xCC, 0x33, 0x33, 0x33, 0x33,
    	    0x33, 0x33, 0x33, 0x33, 0xCC, 0xCC, 0xCC, 0xCC,
    	    0xCC, 0xCC, 0xCC, 0xCC, 0x33, 0x33, 0x33, 0x33,
    	    0x33, 0x33, 0x33, 0x33, 0xCC, 0xCC, 0xCC, 0xCC,
    	    0xCC, 0xCC, 0xCC, 0xCC, 0x33, 0x33, 0x33, 0x33,
    	    0x33, 0x33, 0x33, 0x33, 0xCC, 0xCC, 0xCC, 0xCC};

    GLuint id = glGenLists(1);
    glNewList(id, GL_COMPILE);

    glEnable (GL_POLYGON_STIPPLE);
    glPolygonStipple (halftone);


	glColor4f(1.0, 1.0, 1.0, 1.0);

    glBegin(GL_QUADS); // begin drawing a square

    // Front Face (note that the texture's corners have to match the quad's corners)
    glNormal3f(0.0f, 0.0f, 1.0f); // front face points out of the screen on z.

    glTexCoord2f(0.0f, 1.0f);
    glVertex3f(-1.0f, -1.0f, 0.0f); // Bottom Left
    glTexCoord2f(1.0f, 1.0f);
    glVertex3f(1.0f, -1.0f, 0.0f); // Bottom Right
    glTexCoord2f(1.0f, 0.0f);
    glVertex3f(1.0f, 1.0f, 0.0f); // Top Right
    glTexCoord2f(0.0f, 0.0f);
    glVertex3f(-1.0f, 1.0f, 0.0f); // Top Left

    glEnd();

    glDisable (GL_POLYGON_STIPPLE);

    glEndList();
    return id;
}


GLuint ViewRenderWidget::buildHalfList_checkerboard() {

    GLuint id = glGenLists(1);
    glNewList(id, GL_COMPILE);

    // NON transparent
	glColor4f(1.0, 1.0, 1.0, 1.0);

    // Front Face (note that the texture's corners have to match the quad's corners)
    glNormal3f(0.0f, 0.0f, 1.0f); // front face points out of the screen on z.

    glBegin(GL_QUADS); // begin drawing a grid

    int I = 8, J = 6;
    float x = 0.f, y = 0.f, u = 0.f, v = 0.f;
    float dx = 2.f / (float)I, dy = 2.f / (float)J, du = 1.f / (float)I, dv = 1.f / (float)J;
    for (int i = 0; i < I; ++i)
    	for (int j = 0; j < J; ++j)
    		if ( (i+j)%2 == 0 ) {
    			u = (float) i * du;
    			v = 1.f - (float) j * dv;
    			x = (float) i * dx -1.0;
    			y = (float) j * dy -1.0;
    		    glTexCoord2f(u, v);
    		    glVertex3f(x, y, 0.0f); // Bottom Left
    		    glTexCoord2f(u + du, v);
    		    glVertex3f(x + dx, y, 0.0f); // Bottom Right
    		    glTexCoord2f(u + du, v - dv);
    		    glVertex3f(x + dx, y + dy, 0.0f); // Top Right
    		    glTexCoord2f(u, v - dv);
    		    glVertex3f(x, y + dy, 0.0f); // Top Left
    	    }

    glEnd();

    glEndList();
    return id;
}

GLuint ViewRenderWidget::buildHalfList_triangle() {

    GLuint id = glGenLists(1);
    glNewList(id, GL_COMPILE);

	glColor4f(1.0, 1.0, 1.0, 1.0);

    glBegin(GL_TRIANGLES); // begin drawing a triangle

    glColor4f(1.0, 1.0, 1.0, 1.0);
    // Front Face (note that the texture's corners have to match the quad's corners)
    glNormal3f(0.0f, 0.0f, 1.0f); // front face points out of the screen on z.

    glTexCoord2f(0.0f, 1.0f);
    glVertex3d(-1.0, -1.0, 0.0); // Bottom Left
    glTexCoord2f(1.0f, 1.0f);
    glVertex3d(1.0, -1.0, 0.0); // Bottom Right
    glTexCoord2f(1.0f, 0.0f);
    glVertex3d(1.0, 1.0, 0.0); // Top Right

    glEnd();

    glEndList();
    return id;
}


/**
 * Build a display lists for the line borders and returns its id
 **/
GLuint ViewRenderWidget::buildSelectList() {

    GLuint base = glGenLists(1);

    // selected
    glNewList(base, GL_COMPILE);

//    glPushAttrib(GL_ENABLE_BIT | GL_CURRENT_BIT | GL_COLOR_BUFFER_BIT);

//    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glBlendEquation(GL_FUNC_ADD);
    glDisable(GL_TEXTURE_2D);

    glLineWidth(3.0);
    glColor4f(0.2, 0.80, 0.2, 1.0);

    glBegin(GL_LINE_LOOP); // begin drawing a square
    glVertex3d(-1.1 , -1.1 , 0.0); // Bottom Left
    glVertex3d(1.1 , -1.1 , 0.0); // Bottom Right
    glVertex3d(1.1 , 1.1 , 0.0); // Top Right
    glVertex3d(-1.1 , 1.1 , 0.0); // Top Left
    glEnd();

//    glPopAttrib();

    glEnable(GL_TEXTURE_2D);

    glEndList();

    return base;
}


/**
 * Build a display list of a textured QUAD and returns its id
 **/
GLuint ViewRenderWidget::buildQuadList() {

    GLuint id = glGenLists(1);
    glNewList(id, GL_COMPILE);

    glBegin(GL_QUADS); // begin drawing a square

    // Front Face (note that the texture's corners have to match the quad's corners)
    glNormal3f(0.0f, 0.0f, 1.0f); // front face points out of the screen on z.

    glTexCoord2f(0.0f, 1.0f);
    glVertex3f(-1.0f, -1.0f, 0.0f); // Bottom Left
    glTexCoord2f(1.0f, 1.0f);
    glVertex3f(1.0f, -1.0f, 0.0f); // Bottom Right
    glTexCoord2f(1.0f, 0.0f);
    glVertex3f(1.0f, 1.0f, 0.0f); // Top Right
    glTexCoord2f(0.0f, 0.0f);
    glVertex3f(-1.0f, 1.0f, 0.0f); // Top Left

    glEnd();

    glEndList();
    return id;
}

/**
 * Build 2 display lists for the line borders and shadows
 **/
GLuint ViewRenderWidget::buildLineList() {

    GLuint texid = bindTexture(QPixmap(QString::fromUtf8(":/glmixer/textures/shadow_corner.png")), GL_TEXTURE_2D);

	GLclampf highpriority = 1.0;
	glPrioritizeTextures(1, &texid, &highpriority);

    GLuint base = glGenLists(2);
    glListBase(base);

    // default
    glNewList(base, GL_COMPILE);

    glBindTexture(GL_TEXTURE_2D, texid); // 2d texture (x and y size)

    glPushMatrix();
    glTranslatef(0.05, -0.05, 0.1);
    glScalef(1.2, 1.2, 1.0);
    glBegin(GL_QUADS); // begin drawing a square
    glTexCoord2f(0.0f, 0.0f);
    glVertex3f(-1.f, -1.f, 0.0f); // Bottom Left
    glTexCoord2f(1.0f, 0.0f);
    glVertex3f(1.f, -1.f, 0.0f); // Bottom Right
    glTexCoord2f(1.0f, 1.0f);
    glVertex3f(1.f, 1.f, 0.0f); // Top Right
    glTexCoord2f(0.0f, 1.0f);
    glVertex3f(-1.f, 1.f, 0.0f); // Top Left
    glEnd();
    glPopMatrix();

    glDisable(GL_TEXTURE_2D);
    glLineWidth(1.0);
    glColor4f(0.9, 0.9, 0.0, 0.7);

    glBegin(GL_LINE_LOOP); // begin drawing a square
    glVertex3f(-1.05f, -1.05f, 0.0f); // Bottom Left
    glVertex3f(1.05f, -1.05f, 0.0f); // Bottom Right
    glVertex3f(1.05f, 1.05f, 0.0f); // Top Right
    glVertex3f(-1.05f, 1.05f, 0.0f); // Top Left
    glEnd();

    glEnable(GL_TEXTURE_2D);

    glEndList();

    // over
    glNewList(base + 1, GL_COMPILE);


    glBindTexture(GL_TEXTURE_2D, texid); // 2d texture (x and y size)

    glPushMatrix();
    glTranslatef(0.15, -0.13, 0.1);
    glScalef(1.15, 1.15, 1.0);
    glBegin(GL_QUADS); // begin drawing a square
    glTexCoord2f(0.0f, 0.0f);
    glVertex3f(-1.f, -1.f, 0.0f); // Bottom Left
    glTexCoord2f(1.0f, 0.0f);
    glVertex3f(1.f, -1.f, 0.0f); // Bottom Right
    glTexCoord2f(1.0f, 1.0f);
    glVertex3f(1.f, 1.f, 0.0f); // Top Right
    glTexCoord2f(0.0f, 1.0f);
    glVertex3f(-1.f, 1.f, 0.0f); // Top Left
    glEnd();
    glPopMatrix();

    glDisable(GL_TEXTURE_2D);
    glLineWidth(3.0);
    glColor4f(0.9, 0.9, 0.0, 0.7);

    glBegin(GL_LINE_LOOP); // begin drawing a square
    glVertex3f(-1.05f, -1.05f, 0.0f); // Bottom Left
    glVertex3f(1.05f, -1.05f, 0.0f); // Bottom Right
    glVertex3f(1.05f, 1.05f, 0.0f); // Top Right
    glVertex3f(-1.05f, 1.05f, 0.0f); // Top Left
    glEnd();

    glEnable(GL_TEXTURE_2D);

    glEndList();

    return base;
}




GLuint ViewRenderWidget::buildCircleList() {

    GLuint id = glGenLists(1);
    GLUquadricObj *quadObj = gluNewQuadric();

    GLuint texid = 0; //bindTexture(QPixmap(QString::fromUtf8(":/glmixer/textures/circle.png")), GL_TEXTURE_2D);
	glGenTextures(1, &texid);
	glBindTexture(GL_TEXTURE_2D, texid);
	QImage p( ":/glmixer/textures/circle.png" );
	gluBuild2DMipmaps(GL_TEXTURE_2D, GL_COMPRESSED_RGBA, p.width(), p. height(), GL_RGBA, GL_UNSIGNED_BYTE, p.bits());
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR );
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR_MIPMAP_LINEAR );
	GLclampf highpriority = 1.0;
	glPrioritizeTextures(1, &texid, &highpriority);

    glNewList(id, GL_COMPILE);

    glPushMatrix();
    glTranslatef(0.0, 0.0, - 1.0);

    glDisable(GL_BLEND);
    glBindTexture(GL_TEXTURE_2D, texid); // 2d texture (x and y size)

    glColor4f(1.0, 1.0, 1.0, 1.0);
    gluQuadricTexture(quadObj, GL_TRUE);
    gluDisk(quadObj, 0.0, CIRCLE_SIZE * SOURCE_UNIT, 50, 3);

    glDisable(GL_TEXTURE_2D);

    // blended antialiasing
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glBlendEquation(GL_FUNC_ADD);

    glColor4f(0.6, 0.6, 0.6, 1.0);
    glLineWidth(5.0);

    glBegin(GL_LINE_LOOP);
    for (float i = 0; i < 2.0 * M_PI; i+= 0.07)
    	glVertex3f(CIRCLE_SIZE * SOURCE_UNIT * cos(i), CIRCLE_SIZE  * SOURCE_UNIT * sin(i),0);
    glEnd();

    glEnable(GL_TEXTURE_2D);

    glPopMatrix();
    glEndList();

    return id;
}



GLuint ViewRenderWidget::buildLayerbgList() {

    GLuint id = glGenLists(1);

//    GLuint texid = bindTexture(QPixmap(QString::fromUtf8(":/glmixer/textures/layerbg.png")), GL_TEXTURE_2D);

    glNewList(id, GL_COMPILE);

//    glBlendFunc(GL_SRC_ALPHA, GL_ONE);
//    glBlendEquation(GL_FUNC_ADD);
//
//    glBindTexture(GL_TEXTURE_2D, texid); // 2d texture (x and y size)
//
//    glColor3f(1.0, 1.0, 1.0);
//    glBegin(GL_QUADS); // begin drawing a square
//        glTexCoord2f(0.0f, 0.0f);
//        glVertex3d(-5.0, 0.0, - 30.0); // Bottom Left
//        glTexCoord2f(1.0f, 0.0f);
//        glVertex3d( 5.0, 0.0, - 30.0); // Bottom Right
//        glTexCoord2f(1.0f, 1.0f);
//        glVertex3d( 5.0,0.0,   30.0); // Top Right
//        glTexCoord2f(0.0f, 1.0f);
//        glVertex3d( -5.0, 0.0, 30.0); // Top Left
//    glEnd();

    glDisable(GL_TEXTURE_2D);
    glColor4f(0.6, 0.6, 0.6, 1.0);
    glLineWidth(0.7);
    glBegin(GL_LINES);
    for (float i = -4.0; i < 6.0; i += CLAMP( ABS(i)/2.f , 0.01, 5.0)) {
    	glVertex3f(i - 1.3, -1.1 + exp(-10 * (i+0.2)), 0.0);
    	glVertex3f(i - 1.3, -1.1 + exp(-10 * (i+0.2)), 31.0);
    }
    glEnd();

    glEndList();

    return id;
}


/**
 * Build a display list of a black QUAD and returns its id
 **/
GLuint ViewRenderWidget::buildBlackList() {

    GLuint texid = 0; // bindTexture(QPixmap(QString::fromUtf8(":/glmixer/textures/shadow.png")), GL_TEXTURE_2D);

    // generate the texture with optimal performance ;
	glGenTextures(1, &texid);
	glBindTexture(GL_TEXTURE_2D, texid);
	QImage p( ":/glmixer/textures/shadow.png" );
	gluBuild2DMipmaps(GL_TEXTURE_2D, GL_COMPRESSED_RGBA, p.width(), p. height(), GL_RGBA, GL_UNSIGNED_BYTE, p.bits());
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR );
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR_MIPMAP_LINEAR );
	GLclampf highpriority = 1.0;
	glPrioritizeTextures(1, &texid, &highpriority);

    GLuint id = glGenLists(1);
    glNewList(id, GL_COMPILE);

    glDisable(GL_TEXTURE_2D);
    glDisable(GL_BLEND);
    glColor4f(0.f, 0.f, 0.f, 1.f);
    glBegin(GL_QUADS); // begin drawing a square
    // Front Face
    glNormal3f(0.0f, 0.0f, 1.0f); // front face points out of the screen on z.
        glVertex3f(-1.0f* SOURCE_UNIT, -1.0f* SOURCE_UNIT, 0.0f); // Bottom Left
        glVertex3f(1.0f* SOURCE_UNIT, -1.0f* SOURCE_UNIT, 0.0f); // Bottom Right
        glVertex3f(1.0f* SOURCE_UNIT, 1.0f* SOURCE_UNIT, 0.0f); // Top Right
        glVertex3f(-1.0f* SOURCE_UNIT, 1.0f* SOURCE_UNIT, 0.0f); // Top Left
    glEnd();

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glBlendEquation(GL_FUNC_ADD);
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, texid); // 2d texture (x and y size)

    glColor4f(0.f, 0.f, 0.f, 0.8f);

    glPushMatrix();
        glTranslatef(0.02 * SOURCE_UNIT, -0.1 * SOURCE_UNIT, 0.1);
        glScalef(1.5 * SOURCE_UNIT, 1.5 * SOURCE_UNIT, 1.0);
        glBegin(GL_QUADS); // begin drawing a square
            glTexCoord2f(0.0f, 1.0f);
            glVertex3f(-1.f, -1.f, 0.0f); // Bottom Left
            glTexCoord2f(1.0f, 1.0f);
            glVertex3f(1.f, -1.f, 0.0f);  // Bottom Right
            glTexCoord2f(1.0f, 0.0f);
            glVertex3f(1.f, 1.f, 0.0f);   // Top Right
            glTexCoord2f(0.0f, 0.0f);
            glVertex3f(-1.f, 1.f, 0.0f);  // Top Left
        glEnd();
    glPopMatrix();


//    glPopAttrib();

    glEndList();
    return id;
}

/**
 * Build a display list of the front line border of the render area and returns its id
 **/
GLuint ViewRenderWidget::buildFrameList() {

    GLuint base = glGenLists(1);
    glListBase(base);

    // default
    glNewList(base, GL_COMPILE);

    // blended antialiasing
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glBlendEquation(GL_FUNC_ADD);

    glDisable(GL_TEXTURE_2D);
    glLineWidth(5.0);
    glColor4f(0.85, 0.15, 0.85, 1.0);

    glBegin(GL_LINE_LOOP); // begin drawing the frame (with marks on axis)

    glVertex3f(-1.01f* SOURCE_UNIT, -1.01f* SOURCE_UNIT, 0.0f); // Bottom Left
    glVertex3f(0.0f, -1.01f* SOURCE_UNIT, 0.0f);
    glVertex3f(0.0f, -1.05f* SOURCE_UNIT, 0.0f);
    glVertex3f(0.0f, -1.01f* SOURCE_UNIT, 0.0f);
    glVertex3f(1.01f* SOURCE_UNIT, -1.01f* SOURCE_UNIT, 0.0f); // Bottom Right
    glVertex3f(1.01f* SOURCE_UNIT, 0.0f, 0.0f);
    glVertex3f(1.05f* SOURCE_UNIT, 0.0f, 0.0f);
    glVertex3f(1.01f* SOURCE_UNIT, 0.0f, 0.0f);
    glVertex3f(1.01f* SOURCE_UNIT, 1.01f* SOURCE_UNIT, 0.0f); // Top Right
    glVertex3f(0.0f, 1.01f* SOURCE_UNIT, 0.0f);
    glVertex3f(0.0f, 1.05f* SOURCE_UNIT, 0.0f);
    glVertex3f(0.0f, 1.01f* SOURCE_UNIT, 0.0f);
    glVertex3f(-1.01f* SOURCE_UNIT, 1.01f* SOURCE_UNIT, 0.0f); // Top Left
    glVertex3f(-1.01f* SOURCE_UNIT, 0.0f, 0.0f);
    glVertex3f(-1.05f* SOURCE_UNIT, 0.0f, 0.0f);
    glVertex3f(-1.01f* SOURCE_UNIT, 0.0f, 0.0f);

    glEnd();

    glEnable(GL_TEXTURE_2D);
    glEndList();

    return base;
}

/**
 * Build 3 display lists for the line borders of sources and returns the base id
 **/
GLuint ViewRenderWidget::buildBordersList() {

    GLuint base = glGenLists(3);
    glListBase(base);

    // default
    glNewList(base, GL_COMPILE);

//    glPushAttrib(GL_ENABLE_BIT | GL_CURRENT_BIT | GL_COLOR_BUFFER_BIT);

//    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glBlendEquation(GL_FUNC_ADD);

    glDisable(GL_TEXTURE_2D);
    glLineWidth(1.0);
    glColor4f(0.9, 0.9, 0.0, 0.7);

    glBegin(GL_LINE_LOOP); // begin drawing a square
    glVertex3f(-1.0f, -1.0f, 0.0f); // Bottom Left
    glVertex3f(1.0f, -1.0f, 0.0f); // Bottom Right
    glVertex3f(1.0f, 1.0f, 0.0f); // Top Right
    glVertex3f(-1.0f, 1.0f, 0.0f); // Top Left
    glEnd();

//    glPopAttrib();

    glEnable(GL_TEXTURE_2D);
    glEndList();

    // over
    glNewList(base+1, GL_COMPILE);

//    glPushAttrib(GL_ENABLE_BIT | GL_CURRENT_BIT | GL_COLOR_BUFFER_BIT);

//    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glBlendEquation(GL_FUNC_ADD);

    glDisable(GL_TEXTURE_2D);
    glColor4f(0.9, 0.9, 0.0, 0.7);

    glLineWidth(3.0);
    glBegin(GL_LINE_LOOP); // begin drawing a square
    glVertex3f(-1.0f, -1.0f, 0.0f); // Bottom Left
    glVertex3f(1.0f, -1.0f, 0.0f); // Bottom Right
    glVertex3f(1.0f, 1.0f, 0.0f); // Top Right
    glVertex3f(-1.0f, 1.0f, 0.0f); // Top Left
    glEnd();

    glLineWidth(1.0);
    glBegin(GL_LINES); // begin drawing a square
//    glVertex3f(-1.0f, -1.0f, 0.0f); // Bottom Left
    glVertex3f(-0.80f, -1.0f, 0.0f);
    glVertex3f(-0.80f, -0.80f, 0.0f);
    glVertex3f(-0.80f, -0.80f, 0.0f);
    glVertex3f(-1.0f, -0.80f, 0.0f);

//    glVertex3f(1.0f, -1.0f, 0.0f); // Bottom Right
    glVertex3f(1.0f, -0.80f, 0.0f);
    glVertex3f(0.80f, -0.80f, 0.0f);
    glVertex3f(0.80f, -0.80f, 0.0f);
    glVertex3f(0.80f, -1.0f, 0.0f);

//    glVertex3f(1.0f, 1.0f, 0.0f); // Top Right
    glVertex3f(0.80f, 1.0f, 0.0f);
    glVertex3f(0.80f, 0.80f, 0.0f);
    glVertex3f(0.80f, 0.80f, 0.0f);
    glVertex3f(1.0f, 0.80f, 0.0f);

//    glVertex3f(-1.0f, 1.0f, 0.0f); // Top Left
    glVertex3f(-0.80f, 1.0f, 0.0f);
    glVertex3f(-0.80f, 0.80f, 0.0f);
    glVertex3f(-0.80f, 0.80f, 0.0f);
    glVertex3f(-1.0f, 0.80f, 0.0f);

    glEnd();

    glEnable(GL_TEXTURE_2D);
//    glPopAttrib();

    glEndList();

    // selected
    glNewList(base+2, GL_COMPILE);

//    glPushAttrib(GL_ENABLE_BIT | GL_CURRENT_BIT | GL_COLOR_BUFFER_BIT);

//    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glBlendEquation(GL_FUNC_ADD);
    glDisable(GL_TEXTURE_2D);

    glLineWidth(3.0);
    glColor4f(0.9, 0.0, 0.0, 1.0);

    glBegin(GL_LINE_LOOP); // begin drawing a square
    glVertex3f(-1.1f, -1.1f, 0.0f); // Bottom Left
    glVertex3f(1.1f, -1.1f, 0.0f); // Bottom Right
    glVertex3f(1.1f, 1.1f, 0.0f); // Top Right
    glVertex3f(-1.1f, 1.1f, 0.0f); // Top Left
    glEnd();

    glEnable(GL_TEXTURE_2D);
//    glPopAttrib();

    glEndList();

    return base;
}
