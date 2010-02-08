/*
 * MixRenderWidget.cpp
 *
 *  Created on: 3 nov. 2009
 *      Author: herbelin
 */

#include "MainRenderWidget.moc"

#include "common.h"
#include "glRenderWidget.h"
#include "OpencvSource.h"
#include <algorithm>


// static members
MainRenderWidget *MainRenderWidget::_instance = 0;

GLuint MainRenderWidget::border_thin_shadow = 0, MainRenderWidget::border_large_shadow = 0;
GLuint MainRenderWidget::border_thin = 0, MainRenderWidget::border_large = 0, MainRenderWidget::border_scale = 0;
GLuint MainRenderWidget::quad_texured = 0, MainRenderWidget::quad_half_textured = 0, MainRenderWidget::quad_black = 0;
GLuint MainRenderWidget::frame_selection = 0, MainRenderWidget::frame_screen = 0;
GLuint MainRenderWidget::circle_mixing = 0;

class RenderWidget: public glRenderWidget {

public:
	RenderWidget(MainRenderWidget *mainrenderwidget) :
		glRenderWidget(0), mrw(mainrenderwidget), _fbo(NULL), w(0), h(0),
				_aspectRatio(1.0), _useAspectRatio(true) {
		if (!QGLFramebufferObject::hasOpenGLFramebufferObjects())
			qWarning(
					"Frame Buffer Objects not supported on this graphics hardware");
	}

	~RenderWidget() {
	}

	// QGLWidget rendering
	void paintGL();
	void initializeGL();

	MainRenderWidget *mrw;
	// TODO: implement the use of fbo
	QGLFramebufferObject *_fbo;

	int w,h;
	float _aspectRatio;
	bool _useAspectRatio;


	// utility
    GLuint buildHalfList();
    GLuint buildSelectList();
    GLuint buildLineList();
    GLuint buildQuadList();
    GLuint buildCircleList();
    GLuint buildFrameList();
    GLuint buildBlackList();
    GLuint buildBordersList();

};


void RenderWidget::initializeGL()
{
    glRenderWidget::initializeGL();

    if ( !MainRenderWidget::border_thin_shadow ) {
    	MainRenderWidget::border_thin_shadow = buildLineList();
    	MainRenderWidget::border_large_shadow = MainRenderWidget::border_thin_shadow + 1;
    }
	if (!MainRenderWidget::quad_texured)
		MainRenderWidget::quad_texured = buildQuadList();
	if (!MainRenderWidget::quad_half_textured)
		MainRenderWidget::quad_half_textured = buildHalfList();
	if (!MainRenderWidget::frame_selection)
		MainRenderWidget::frame_selection = buildSelectList();
	if (!MainRenderWidget::circle_mixing)
		MainRenderWidget::circle_mixing = buildCircleList();
	if (!MainRenderWidget::quad_black)
		MainRenderWidget::quad_black = buildBlackList();
	if (!MainRenderWidget::frame_screen)
		MainRenderWidget::frame_screen = buildFrameList();
	if (!MainRenderWidget::border_thin) {
		MainRenderWidget::border_thin = buildBordersList();
		MainRenderWidget::border_large = MainRenderWidget::border_thin + 1;
		MainRenderWidget::border_scale = MainRenderWidget::border_thin + 2;
	}

}

void RenderWidget::paintGL() {
	glRenderWidget::paintGL();

	//  loop with only 1 texture bind per source
	// setup rendering projection for mixer view
	// init modelview for render of square texture in full viewport
	// for each source
	//    1 bind texture for this source ; update from videoFile if source has changed
	//    2 render square in fbo (may push/modify/pop the projection matrix)
	//    3 push modelview and apply source transforms
	//	  4 render square in current context
	//	  5 pop modelview


	for (SourceSet::iterator its = mrw->_sources.begin(); its != mrw->_sources.end(); its++) {
		// steps 1 and 2 :
		(*its)->update();

		// step 3


		// step 4
		glPushMatrix();
		if (_useAspectRatio) {
			float windowaspectratio = (float) width() / (float) height();
			if (windowaspectratio < _aspectRatio)
				glScalef(1.f, windowaspectratio / _aspectRatio, 1.f);
			else
				glScalef(_aspectRatio / windowaspectratio, 1.f, 1.f);
		}

		glTranslated((*its)->getX(), (*its)->getY(), 0.0);
		glScaled((*its)->getScaleX(), (*its)->getScaleY(), 1.f);
		(*its)->draw();

		glPopMatrix();

	}

	// TODO; create a specific fbo for the render source.
	// code to render the ouput of the window into fbo

	//        fbo->bind();
	//        glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);
	//        glBindTexture(GL_TEXTURE_2D, _s->textureIndex);
	//		glCallList(VideoSource::squareDisplayList);
	//        fbo->release();


	// TODO: implement the use of fbo for rendered output
}

const QGLWidget *MainRenderWidget::getQGLWidget() {

	return (QGLWidget *) getInstance()->_renderwidget;
}

MainRenderWidget *MainRenderWidget::getInstance() {

	if (_instance == 0) {
		_instance = new MainRenderWidget();
	}

	return _instance;
}

void MainRenderWidget::deleteInstance() {

	if (_instance != 0)
		_instance->close();
}

MainRenderWidget::MainRenderWidget() :
	QWidget(0) {

	_renderwidget = new RenderWidget(this);
	setRenderingResolution(DEFAULT_WIDTH, DEFAULT_HEIGHT);

	QVBoxLayout *verticalLayout = new QVBoxLayout(this);
	verticalLayout->setSpacing(0);
	verticalLayout->setContentsMargins(0, 0, 0, 0);
	verticalLayout->addWidget(_renderwidget);

	_renderwidget->setParent(this);

	currentSource = getEnd();
}

MainRenderWidget::~MainRenderWidget() {

	clearSourceSet();
}


void MainRenderWidget::setRenderingResolution(int width, int height){

	if (_renderwidget) {
		_renderwidget->w = width;
		_renderwidget->h = height;
		_renderwidget->_aspectRatio = (float) width / (float) height;

		_renderwidget->makeCurrent();

		if (_renderwidget->_fbo)
			delete _renderwidget->_fbo;

		_renderwidget->_fbo = new QGLFramebufferObject(_renderwidget->w,_renderwidget->h);
	}
}

float MainRenderWidget::getRenderingAspectRatio(){

	if (_renderwidget && _renderwidget->_useAspectRatio)
		return _renderwidget->_aspectRatio;
	else
		return ( (float) width() / (float) height() );
}

void MainRenderWidget::useRenderingAspectRatio(bool on) {

	if (_renderwidget)
		_renderwidget->_useAspectRatio = on;

}

void MainRenderWidget::addSource(VideoFile *vf) {

	double d = (_sources.empty()) ? 0.0 : (*_sources.rbegin())->getDepth() + 1.0;
	// create a source appropriate for this videofile
	VideoSource *s = new VideoSource(vf, (QGLWidget *) _renderwidget, d);
	// ensure we display first frame (not done automatically by signal as it should...)
	s->updateFrame(-1);
	// set the last created source to be current
	setCurrentSource(_sources.insert((Source *) s));

}

#ifdef OPEN_CV
void MainRenderWidget::addSource(int opencvIndex) {

	double d = (_sources.empty()) ? 0.0 : (*_sources.rbegin())->getDepth() + 1.0;
	OpencvSource *s =
			new OpencvSource(opencvIndex, (QGLWidget *) _renderwidget, d);

	// set the last created source to be current
	setCurrentSource(_sources.insert((Source *) s));

}
#endif

void MainRenderWidget::removeSource(SourceSet::iterator itsource) {

	if (itsource != _sources.end()) {
		delete (*itsource);
		_sources.erase(itsource);
	}

}

void MainRenderWidget::clearSourceSet() {
	// TODO does it work?
	for (SourceSet::iterator its = _sources.begin(); its != _sources.end(); its++)
		removeSource(its);
}

bool MainRenderWidget::notAtEnd(SourceSet::iterator itsource) {
	return (itsource != _sources.end());
}

bool MainRenderWidget::isValid(SourceSet::iterator itsource) {

	if (notAtEnd(itsource))
		return (_sources.find(*itsource) != _sources.end());
	else
		return false;
}

void MainRenderWidget::setCurrentSource(SourceSet::iterator si) {

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


void MainRenderWidget::setCurrentSource(GLuint name) {
	setCurrentSource( getById(name) );
}

SourceSet::iterator MainRenderWidget::changeDepth(SourceSet::iterator itsource,
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

SourceSet::iterator MainRenderWidget::getById(GLuint name) {

	return std::find_if(_sources.begin(), _sources.end(), hasName(name));
}


void MainRenderWidget::setFullScreen(bool on) {

	// this is valid only for WINDOW widgets
	if (windowFlags() & Qt::Window) {

		// if ask fullscreen and already fullscreen
		if (on && (windowState() & Qt::WindowFullScreen))
			return;

		// if ask NOT fullscreen and already NOT fullscreen
		if (!on && !(windowState() & Qt::WindowFullScreen))
			return;

		// other cases ; need to switch fullscreen <-> not fullscreen
		setWindowState(windowState() ^ Qt::WindowFullScreen);
		update();
	}

}

void MainRenderWidget::mouseDoubleClickEvent(QMouseEvent * event) {

	// switch fullscreen / window
	if (windowFlags() & Qt::Window) {
		setWindowState(windowState() ^ Qt::WindowFullScreen);
		update();
	}
}

void MainRenderWidget::keyPressEvent(QKeyEvent * event) {

	switch (event->key()) {
	case Qt::Key_Escape:
		setFullScreen(false);
		break;
	case Qt::Key_Enter:
	case Qt::Key_Space:
		setFullScreen(true);
		break;
	default:
		QWidget::keyPressEvent(event);
	}

	event->accept();
}

void MainRenderWidget::closeEvent(QCloseEvent * event) {

	emit windowClosed();
	event->accept();

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
