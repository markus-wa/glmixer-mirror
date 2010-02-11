/*
 * RenderingManager.cpp
 *
 *  Created on: 3 nov. 2009
 *      Author: herbelin
 */

#include "RenderingManager.moc"

#include "common.h"
#include "glRenderWidget.h"
#include "View.h"
#include "MixerView.h"
#include "GeometryView.h"
#include "VideoSource.h"
#ifdef OPEN_CV
#include "OpencvSource.h"
#endif

#include <algorithm>


// static members
RenderingManager *RenderingManager::_instance = 0;

GLuint RenderingManager::border_thin_shadow = 0, RenderingManager::border_large_shadow = 0;
GLuint RenderingManager::border_thin = 0, RenderingManager::border_large = 0, RenderingManager::border_scale = 0;
GLuint RenderingManager::quad_texured = 0, RenderingManager::quad_half_textured = 0, RenderingManager::quad_black = 0;
GLuint RenderingManager::frame_selection = 0, RenderingManager::frame_screen = 0;
GLuint RenderingManager::circle_mixing = 0;

class RenderWidget: public glRenderWidget {

	friend class RenderingManager;

public:
	RenderWidget() :glRenderWidget() {

		setMouseTracking(true);
		setFocusPolicy(Qt::ClickFocus);

		noView = new View;
		mixingManipulationView = new MixerView;
		geometryManipulationView = new GeometryView;
		currentManipulationView = noView;
	}

	~RenderWidget() {
	}

	// QGLWidget rendering
	void paintGL();
	void initializeGL();
	void resizeGL(int w, int h);
    void mousePressEvent(QMouseEvent *event);
    void mouseMoveEvent(QMouseEvent *event);
    void mouseReleaseEvent ( QMouseEvent * event );
    void wheelEvent ( QWheelEvent * event );
    void keyPressEvent ( QKeyEvent * event );

    // keep updating even if hidden
    void hideEvent ( QHideEvent * event ) { QGLWidget::hideEvent(event); }


protected:
	// utility
    GLuint buildHalfList();
    GLuint buildSelectList();
    GLuint buildLineList();
    GLuint buildQuadList();
    GLuint buildCircleList();
    GLuint buildFrameList();
    GLuint buildBlackList();
    GLuint buildBordersList();

	View *currentManipulationView, *noView;
	MixerView *mixingManipulationView;
	GeometryView *geometryManipulationView;
};


void RenderWidget::initializeGL()
{
    glRenderWidget::initializeGL();

	setBackgroundColor( QColor(52,52,52) );


    if ( !RenderingManager::border_thin_shadow ) {
    	RenderingManager::border_thin_shadow = buildLineList();
    	RenderingManager::border_large_shadow = RenderingManager::border_thin_shadow + 1;
    }
	if (!RenderingManager::quad_texured)
		RenderingManager::quad_texured = buildQuadList();
	if (!RenderingManager::quad_half_textured)
		RenderingManager::quad_half_textured = buildHalfList();
	if (!RenderingManager::frame_selection)
		RenderingManager::frame_selection = buildSelectList();
	if (!RenderingManager::circle_mixing)
		RenderingManager::circle_mixing = buildCircleList();
	if (!RenderingManager::quad_black)
		RenderingManager::quad_black = buildBlackList();
	if (!RenderingManager::frame_screen)
		RenderingManager::frame_screen = buildFrameList();
	if (!RenderingManager::border_thin) {
		RenderingManager::border_thin = buildBordersList();
		RenderingManager::border_large = RenderingManager::border_thin + 1;
		RenderingManager::border_scale = RenderingManager::border_thin + 2;
	}

	 //TODO use glRectf instead of lame LINELOOP  http://linux.die.net/man/3/glrectf

}

/**
 *  REDIRECT every calls to the current view implementation
 */

void RenderWidget::resizeGL(int w, int h){
	currentManipulationView->resize(w,h);
}
void RenderWidget::paintGL(){
    glRenderWidget::paintGL();
	currentManipulationView->reset();
	currentManipulationView->paint();
}
void RenderWidget::mousePressEvent(QMouseEvent *event){
	makeCurrent();
	currentManipulationView->mousePressEvent(event);
	event->accept();
}
void RenderWidget::mouseMoveEvent(QMouseEvent *event){
	makeCurrent();
	currentManipulationView->mouseMoveEvent(event);
	event->accept();
}
void RenderWidget::mouseReleaseEvent ( QMouseEvent * event ){
	makeCurrent();
	currentManipulationView->mouseReleaseEvent(event);
	event->accept();
}
void RenderWidget::wheelEvent ( QWheelEvent * event ){
	makeCurrent();
	currentManipulationView->wheelEvent(event);
	event->accept();
}
void RenderWidget::keyPressEvent ( QKeyEvent * event ){
	currentManipulationView->keyPressEvent(event);
	event->accept();
}

void RenderingManager::setViewMode(viewMode mode){

	switch (mode) {
	case MIXING:
		_renderwidget->currentManipulationView = (View *) _renderwidget->mixingManipulationView;
		break;
	case GEOMETRY:
		_renderwidget->currentManipulationView = (View *) _renderwidget->geometryManipulationView;
		break;
	case NONE:
	default:
		_renderwidget->currentManipulationView = _renderwidget->noView;
	}

	// update view to match with the changes in modelview and projection matrices (e.g. resized widget)
	_renderwidget->makeCurrent();
	_renderwidget->currentManipulationView->resize(_renderwidget->width(), _renderwidget->height());

}


QPixmap RenderingManager::getViewIcon(){

	return _renderwidget->currentManipulationView->getIcon();
}

QGLWidget *RenderingManager::getQGLWidget() {

	return (QGLWidget *) getInstance()->_renderwidget;
}

RenderingManager *RenderingManager::getInstance() {

	if (_instance == 0) {
		_instance = new RenderingManager;

		if (!QGLFramebufferObject::hasOpenGLFramebufferObjects())
			qWarning("Frame Buffer Objects not supported on this graphics hardware");
	}

	return _instance;
}


RenderingManager::RenderingManager() :
	QObject(), _fbo(NULL) {

	_renderwidget = new RenderWidget;

	setFrameBufferResolution(DEFAULT_WIDTH, DEFAULT_HEIGHT);

	currentSource = getEnd();
}

RenderingManager::~RenderingManager() {

	if (_renderwidget != 0)
		delete _renderwidget;
	clearSourceSet();
}


void RenderingManager::setFrameBufferResolution(int width, int height){

	if (_fbo)
		delete _fbo;

	_fbo = new QGLFramebufferObject(width,height);

}

float RenderingManager::getFrameBufferAspectRatio(){

	return ( (float) _fbo->width() / (float) _fbo->height() );
}


void RenderingManager::bindFrameBuffer(){
	_fbo->bind();
}
void RenderingManager::releaseFrameBuffer(){
	_fbo->release();
}
GLuint RenderingManager::getFrameBufferTexture(){
	return _fbo->texture();
}
int RenderingManager::getFrameBufferWidth(){
	return _fbo->width();
}
int RenderingManager::getFrameBufferHeight(){
	return _fbo->height();
}

void RenderingManager::zoomIn() {
	_renderwidget->currentManipulationView->zoomIn();
}
void RenderingManager::zoomOut() {
	_renderwidget->currentManipulationView->zoomOut();
}
void RenderingManager::zoomReset() {
	_renderwidget->currentManipulationView->zoomReset();
}
void RenderingManager::zoomBestFit() {
	_renderwidget->currentManipulationView->zoomBestFit();
}

void RenderingManager::addSource(VideoFile *vf) {

	double d = (_sources.empty()) ? 0.0 : (*_sources.rbegin())->getDepth() + 1.0;
	// create a source appropriate for this videofile
	VideoSource *s = new VideoSource(vf, (QGLWidget *) _renderwidget, d);
	// ensure we display first frame (not done automatically by signal as it should...)
	s->updateFrame(-1);
	// set the last created source to be current
	setCurrentSource(_sources.insert((Source *) s));

}

#ifdef OPEN_CV
void RenderingManager::addSource(int opencvIndex) {

	double d = (_sources.empty()) ? 0.0 : (*_sources.rbegin())->getDepth() + 1.0;
	OpencvSource *s =
			new OpencvSource(opencvIndex, (QGLWidget *) _renderwidget, d);

	// set the last created source to be current
	setCurrentSource(_sources.insert((Source *) s));

}
#endif

void RenderingManager::removeSource(SourceSet::iterator itsource) {

	if (itsource != _sources.end()) {
		delete (*itsource);
		_sources.erase(itsource);
	}

}

void RenderingManager::clearSourceSet() {
	// TODO does it work?
	for (SourceSet::iterator its = _sources.begin(); its != _sources.end(); its++)
		removeSource(its);
}

bool RenderingManager::notAtEnd(SourceSet::iterator itsource) {
	return (itsource != _sources.end());
}

bool RenderingManager::isValid(SourceSet::iterator itsource) {

	if (notAtEnd(itsource))
		return (_sources.find(*itsource) != _sources.end());
	else
		return false;
}

void RenderingManager::setCurrentSource(SourceSet::iterator si) {

	if (si != currentSource) {
		if (notAtEnd(currentSource))
			(*currentSource)->activate(false);

		currentSource = si;
		emit
		currentSourceChanged(currentSource);

		if (notAtEnd(currentSource))
			(*currentSource)->activate(true);
	}
}


void RenderingManager::setCurrentSource(GLuint name) {
	setCurrentSource( getById(name) );
}

SourceSet::iterator RenderingManager::changeDepth(SourceSet::iterator itsource,
		double newdepth) {
	// TODO : implement

	if (itsource != _sources.end()) {
		//        Source *tmp = new Source(*itsource, newdepth);
		//
		//        // sort again the set by depth: this is done by removing the element and adding a clone
		//        _sources->erase(itsource);
		//        return (_sources->insert(tmp));
	}

	return itsource;
}

SourceSet::iterator RenderingManager::getById(GLuint name) {

	return std::find_if(_sources.begin(), _sources.end(), hasName(name));
}



// code to select rendering texture target:

// glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, fbo);  <- fbo is GLuint QGLFramebufferObject::handle ()
// glGenTextures(1, &texture);
//glBindTexture(target, texture);
//glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, size.width(), size.height(), 0,
//             GL_RGBA, GL_UNSIGNED_BYTE, NULL);
//
//glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
//glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
//glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
//glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
//
//glFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT0_EXT,
//		GL_TEXTURE_2D, texture, 0);
//
//or
//
//glFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT1_EXT,
//		GL_TEXTURE_2D, texture, 0);

// etc.. till GL_COLOR_ATTACHMENT15_EXT




/**
 * Build a display list of a textured QUAD and returns its id
 **/
GLuint RenderWidget::buildHalfList() {

    GLuint id = glGenLists(1);
    glNewList(id, GL_COMPILE);


    glBegin(GL_QUADS); // begin drawing a grid

    glColor4f(1.0, 1.0, 1.0, 1.0);
    // Front Face (note that the texture's corners have to match the quad's corners)
    glNormal3f(0.0f, 0.0f, 1.0f); // front face points out of the screen on z.

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

//    glBegin(GL_TRIANGLES); // begin drawing a triangle
//
//    glColor4f(1.0, 1.0, 1.0, 1.0);
//    // Front Face (note that the texture's corners have to match the quad's corners)
//    glNormal3f(0.0f, 0.0f, 1.0f); // front face points out of the screen on z.
//
//    glTexCoord2f(0.0f, 1.0f);
//    glVertex3d(-1.0, -1.0, 0.0); // Bottom Left
//    glTexCoord2f(1.0f, 1.0f);
//    glVertex3d(1.0, -1.0, 0.0); // Bottom Right
//    glTexCoord2f(1.0f, 0.0f);
//    glVertex3d(1.0, 1.0, 0.0); // Top Right
//
//    glEnd();

    glEndList();
    return id;
}

/**
 * Build a display lists for the line borders and returns its id
 **/
GLuint RenderWidget::buildSelectList() {

    GLuint base = glGenLists(1);

    // selected
    glNewList(base, GL_COMPILE);

    glPushAttrib(GL_ENABLE_BIT | GL_CURRENT_BIT | GL_COLOR_BUFFER_BIT);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDisable(GL_TEXTURE_2D);

    glLineWidth(3.0);
    glColor4f(0.2, 0.80, 0.2, 1.0);

    glBegin(GL_LINE_LOOP); // begin drawing a square
    glVertex3d(-1.1 , -1.1 , 0.0); // Bottom Left
    glVertex3d(1.1 , -1.1 , 0.0); // Bottom Right
    glVertex3d(1.1 , 1.1 , 0.0); // Top Right
    glVertex3d(-1.1 , 1.1 , 0.0); // Top Left
    glEnd();

    glPopAttrib();

    glEndList();

    return base;
}


/**
 * Build a display list of a textured QUAD and returns its id
 **/
GLuint RenderWidget::buildQuadList() {

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
GLuint RenderWidget::buildLineList() {

    GLuint texid = bindTexture(QPixmap(QString::fromUtf8(":/glmixer/textures/shadow_corner.png")), GL_TEXTURE_2D);

    GLuint base = glGenLists(2);
    glListBase(base);

    // default
    glNewList(base, GL_COMPILE);

    glPushAttrib(GL_ENABLE_BIT | GL_CURRENT_BIT | GL_COLOR_BUFFER_BIT);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glEnable(GL_TEXTURE_2D);
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

    glPopAttrib();
    glEndList();

    // over
    glNewList(base + 1, GL_COMPILE);

    glPushAttrib(GL_ENABLE_BIT | GL_CURRENT_BIT | GL_COLOR_BUFFER_BIT);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glEnable(GL_TEXTURE_2D);
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

    glPopAttrib();

    glEndList();

    return base;
}




GLuint RenderWidget::buildCircleList() {

    GLuint id = glGenLists(1);
    GLUquadricObj *quadObj = gluNewQuadric();

    GLuint texid = bindTexture(QPixmap(QString::fromUtf8(":/glmixer/textures/circle.png")), GL_TEXTURE_2D);

    glNewList(id, GL_COMPILE);

    glPushAttrib(GL_ENABLE_BIT | GL_CURRENT_BIT);

    glPushMatrix();
    glTranslatef(0.0, 0.0, MAX_DEPTH_LAYER - 1.0);

    glDisable(GL_TEXTURE_2D);
    glColor4f(0.7, 0.7, 0.7, 1.0);
    gluDisk(quadObj, 0.01  * SOURCE_UNIT, (CIRCLE_SIZE + 0.09) * SOURCE_UNIT, 40, 40);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, texid); // 2d texture (x and y size)

    glTranslatef(0.0, 0.0, 1.0);
    glBegin(GL_QUADS); // begin drawing a square
        glTexCoord2f(0.0f, 0.0f);
        glVertex3d(-CIRCLE_SIZE * SOURCE_UNIT, -CIRCLE_SIZE * SOURCE_UNIT, 0.0); // Bottom Left
        glTexCoord2f(1.0f, 0.0f);
        glVertex3d(CIRCLE_SIZE * SOURCE_UNIT, -CIRCLE_SIZE * SOURCE_UNIT, 0.0f); // Bottom Right
        glTexCoord2f(1.0f, 1.0f);
        glVertex3d(CIRCLE_SIZE * SOURCE_UNIT, CIRCLE_SIZE * SOURCE_UNIT, 0.0f); // Top Right
        glTexCoord2f(0.0f, 1.0f);
        glVertex3d(-CIRCLE_SIZE * SOURCE_UNIT, CIRCLE_SIZE * SOURCE_UNIT, 0.0f); // Top Left
    glEnd();

    glPopMatrix();
    glPopAttrib();
    glEndList();

    return id;
}


/**
 * Build a display list of a black QUAD and returns its id
 **/
GLuint RenderWidget::buildBlackList() {

    GLuint texid = bindTexture(QPixmap(QString::fromUtf8(":/glmixer/textures/shadow.png")), GL_TEXTURE_2D);

    GLuint id = glGenLists(1);
    glNewList(id, GL_COMPILE);

    glPushAttrib(GL_ENABLE_BIT | GL_CURRENT_BIT | GL_COLOR_BUFFER_BIT);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, texid); // 2d texture (x and y size)

    glPushMatrix();
        glTranslatef(0.1 * SOURCE_UNIT, -0.2 * SOURCE_UNIT, 0.1);
        glScalef(1.2 * SOURCE_UNIT, 1.2 * SOURCE_UNIT, 1.0);
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

    glPopAttrib();

    glEndList();
    return id;
}

/**
 * Build a display list of the front line border of the render area and returns its id
 **/
GLuint RenderWidget::buildFrameList() {

    GLuint base = glGenLists(1);
    glListBase(base);

    // default
    glNewList(base, GL_COMPILE);

    glPushAttrib(GL_ENABLE_BIT | GL_CURRENT_BIT  | GL_COLOR_BUFFER_BIT);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glDisable(GL_TEXTURE_2D);
    glLineWidth(5.0);
    glColor4f(0.9, 0.2, 0.9, 0.7);

    glBegin(GL_LINE_LOOP); // begin drawing a square
    glVertex3f(-1.01f* SOURCE_UNIT, -1.01f* SOURCE_UNIT, 0.0f); // Bottom Left
    glVertex3f(1.01f* SOURCE_UNIT, -1.01f* SOURCE_UNIT, 0.0f); // Bottom Right
    glVertex3f(1.01f* SOURCE_UNIT, 1.01f* SOURCE_UNIT, 0.0f); // Top Right
    glVertex3f(-1.01f* SOURCE_UNIT, 1.01f* SOURCE_UNIT, 0.0f); // Top Left
    glEnd();

    glPopAttrib();
    glEndList();

    return base;
}

/**
 * Build 3 display lists for the line borders of sources and returns the base id
 **/
GLuint RenderWidget::buildBordersList() {

    GLuint base = glGenLists(3);
    glListBase(base);

    // default
    glNewList(base, GL_COMPILE);

    glPushAttrib(GL_ENABLE_BIT | GL_CURRENT_BIT | GL_COLOR_BUFFER_BIT);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glDisable(GL_TEXTURE_2D);
    glLineWidth(1.0);
    glColor4f(0.9, 0.9, 0.0, 0.7);

    glBegin(GL_LINE_LOOP); // begin drawing a square
    glVertex3f(-1.0f, -1.0f, 0.0f); // Bottom Left
    glVertex3f(1.0f, -1.0f, 0.0f); // Bottom Right
    glVertex3f(1.0f, 1.0f, 0.0f); // Top Right
    glVertex3f(-1.0f, 1.0f, 0.0f); // Top Left
    glEnd();

    glPopAttrib();
    glEndList();

    // over
    glNewList(base+1, GL_COMPILE);

    glPushAttrib(GL_ENABLE_BIT | GL_CURRENT_BIT | GL_COLOR_BUFFER_BIT);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

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

    glPopAttrib();

    glEndList();

    // selected
    glNewList(base+2, GL_COMPILE);

    glPushAttrib(GL_ENABLE_BIT | GL_CURRENT_BIT | GL_COLOR_BUFFER_BIT);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDisable(GL_TEXTURE_2D);

    glLineWidth(3.0);
    glColor4f(0.9, 0.0, 0.0, 1.0);

    glBegin(GL_LINE_LOOP); // begin drawing a square
    glVertex3f(-1.1f, -1.1f, 0.0f); // Bottom Left
    glVertex3f(1.1f, -1.1f, 0.0f); // Bottom Right
    glVertex3f(1.1f, 1.1f, 0.0f); // Top Right
    glVertex3f(-1.1f, 1.1f, 0.0f); // Top Left
    glEnd();

    glPopAttrib();

    glEndList();

    return base;
}
