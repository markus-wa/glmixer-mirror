/*
 * RenderingManager.cpp
 *
 *  Created on: 3 nov. 2009
 *      Author: herbelin
 */

#include "RenderingManager.moc"

#include "common.h"

#include "RenderingSource.h"
#include "VideoSource.h"
#ifdef OPEN_CV
#include "OpencvSource.h"
#endif

#include <algorithm>


// static members
RenderingManager *RenderingManager::_instance = 0;
bool RenderingManager::blit = false;


ViewRenderWidget *RenderingManager::getRenderingWidget() {

	return getInstance()->_renderwidget;
}

RenderingManager *RenderingManager::getInstance() {

	if (_instance == 0){

		if (!QGLFramebufferObject::hasOpenGLFramebufferObjects())
			qCritical("*** ERROR ***\n\nOpenGL Frame Buffer Objects are not supported on this graphics hardware; the program cannot operate properly.\n\nExiting...");

	    if (glRenderWidget::glSupportsExtension("GL_EXT_framebuffer_blit"))
	    	RenderingManager::blit = true;
	    else
	    	qWarning("** WARNING **\n\nOpenGL extension GL_EXT_framebuffer_blit is not supported on this graphics hardware.\n\nRendering speed be sub-optimal but all should work properly.");

		_instance = new RenderingManager;
	    Q_CHECK_PTR(_instance);
	}

	return _instance;
}


RenderingManager::RenderingManager() :
	QObject(), _fbo(NULL), previousframe_fbo(NULL), countRenderingSource(0), previousframe_index(0), previousframe_delay(1) {

	_renderwidget = new ViewRenderWidget;
    Q_CHECK_PTR(_renderwidget);

	setFrameBufferResolution(DEFAULT_WIDTH, DEFAULT_HEIGHT);

	currentSource = getEnd();
}

RenderingManager::~RenderingManager() {

	if (_renderwidget != 0)
		delete _renderwidget;

	if (_fbo)
		delete _fbo;

	clearSourceSet();
}


void RenderingManager::setFrameBufferResolution(int width, int height){

	if (_fbo)
		delete _fbo;

	_renderwidget->makeCurrent();
	_fbo = new QGLFramebufferObject(width,height);
	Q_CHECK_PTR(_fbo);

}

float RenderingManager::getFrameBufferAspectRatio(){

	return ( (float) _fbo->width() / (float) _fbo->height() );
}


GLuint RenderingManager::getFrameBufferTexture(){
	return _fbo->texture();
}
GLuint RenderingManager::getFrameBufferHandle(){
	return _fbo->handle();
}
int RenderingManager::getFrameBufferWidth(){
	return _fbo->width();
}
int RenderingManager::getFrameBufferHeight(){
	return _fbo->height();
}


void RenderingManager::updatePreviousFrame(){

	if ( !previousframe_fbo )
		return;

	previousframe_index++;

	if ( previousframe_index % previousframe_delay)
		return;
	else
		previousframe_index = 0;


	if (RenderingManager::blit)
	// use the accelerated GL_EXT_framebuffer_blit if available
	{
	    glBindFramebufferEXT(GL_READ_FRAMEBUFFER_EXT, _fbo->handle());
		glBindFramebufferEXT(GL_DRAW_FRAMEBUFFER_EXT, previousframe_fbo->handle());

		glBlitFramebufferEXT(0, _fbo->height(), _fbo->width(), 0,
							0, 0, previousframe_fbo->width(), previousframe_fbo->height(),
							GL_COLOR_BUFFER_BIT, GL_NEAREST);

	    glBindFramebufferEXT(GL_DRAW_FRAMEBUFFER_EXT, 0);

	}
	else
	// 	Draw quad with fbo texture in a more basic OpenGL way
	{
		glPushAttrib(GL_ALL_ATTRIB_BITS);

		glViewport(0, 0, previousframe_fbo->width(), previousframe_fbo->height());

		glMatrixMode(GL_PROJECTION);
		glPushMatrix();
		glLoadIdentity();
		gluOrtho2D(-1.0, 1.0, -1.0, 1.0);

		glMatrixMode(GL_MODELVIEW);
		glPushMatrix();
		glLoadIdentity();

	    glColor4f(1.0, 1.0, 1.0, 1.0);

		// render to the frame buffer object
		previousframe_fbo->bind();
		glBindTexture(GL_TEXTURE_2D, _fbo->texture());

		glCallList(ViewRenderWidget::quad_texured);
		previousframe_fbo->release();

		// pop the projection matrix and GL state back for rendering the current view
		// to the actual widget
		glPopAttrib();
		glMatrixMode(GL_PROJECTION);
		glPopMatrix();
		glMatrixMode(GL_MODELVIEW);
		glPopMatrix();
	}

}

void RenderingManager::renderToFrameBuffer(SourceSet::iterator itsource, bool clearfirst){

	glPushAttrib(GL_ALL_ATTRIB_BITS);

	glViewport(0, 0, _fbo->width(), _fbo->height());

	glMatrixMode(GL_PROJECTION);
	glPushMatrix();
	glLoadIdentity();
	gluOrtho2D(-SOURCE_UNIT, SOURCE_UNIT, -SOURCE_UNIT, SOURCE_UNIT);

	glMatrixMode(GL_MODELVIEW);
	glPushMatrix();
	glLoadIdentity();

	// render to the frame buffer object
	_fbo->bind();
	{
		if (clearfirst) {
		    glClearColor(0.0, 0.0, 0.0, 1.0f);
			glClear(GL_COLOR_BUFFER_BIT);
		}

	    // Blending Function For transparency Based On Source Alpha Value
	    glBlendFunc(GL_SRC_ALPHA, GL_ONE);

		glTranslated((*itsource)->getX(), (*itsource)->getY(), 0.0);
		glScaled((*itsource)->getScaleX(), (*itsource)->getScaleY(), 1.f);

        (*itsource)->draw();
	}
	_fbo->release();

	// pop the projection matrix and GL state back for rendering the current view
	// to the actual widget
	glPopAttrib();
	glMatrixMode(GL_PROJECTION);
	glPopMatrix();
	glMatrixMode(GL_MODELVIEW);
	glPopMatrix();
}


void RenderingManager::captureFrameBuffer(){

	_renderwidget->makeCurrent();
	capture = _fbo->toImage();
}

void RenderingManager::saveCapturedFrameBuffer(QString filename){

	if (!capture.save(filename))
		qWarning("** Warning **\n\nCould not save file %s.", qPrintable(filename));

}

void RenderingManager::addRenderingSource() {

	_renderwidget->makeCurrent();
	// create the previous frame (frame buffer object) if needed
	if (!previousframe_fbo) {
		previousframe_fbo = new QGLFramebufferObject(_fbo->width(),_fbo->height());
	}

	// place it forward
	double d = (_sources.empty()) ? 0.0 : (*_sources.rbegin())->getDepth() + 1.0;

	// create a source appropriate for this videofile
	RenderingSource *s = new RenderingSource(previousframe_fbo->texture(), d);
    Q_CHECK_PTR(s);

	// set the last created source to be current
	setCurrentSource(_sources.insert((Source *) s));
}

void RenderingManager::addCaptureSource(){

	_renderwidget->makeCurrent();
	// create the texture from the capture
	if (capture.isNull())
		capture = _fbo->toImage();
	GLuint textureIndex = _renderwidget->bindTexture (capture);

	// place it forward
	double d = (_sources.empty()) ? 0.0 : (*_sources.rbegin())->getDepth() + 1.0;

	// create a source appropriate for this videofile
	Source *s = new Source(textureIndex, d);
    Q_CHECK_PTR(s);

    s->setAspectRatio( double(capture.width()) / double(capture.height()) );

	// set the last created source to be current
	setCurrentSource(_sources.insert((Source *) s));

}

void RenderingManager::addMediaSource(VideoFile *vf) {

	// create the texture for this source
	GLuint textureIndex;
	_renderwidget->makeCurrent();
	glGenTextures(1, &textureIndex);
	glBindTexture(GL_TEXTURE_2D, textureIndex);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

	// place it forward
	double d = (_sources.empty()) ? 0.0 : (*_sources.rbegin())->getDepth() + 1.0;

	// create a source appropriate for this videofile
	VideoSource *s = new VideoSource(vf, textureIndex, d);
    Q_CHECK_PTR(s);
	// ensure we display first frame (not done automatically by signal as it should...)
	s->updateFrame(-1);

	// set the last created source to be current
	setCurrentSource(_sources.insert((Source *) s));
}

#ifdef OPEN_CV
void RenderingManager::addOpencvSource(int opencvIndex) {

	// create the texture for this source
	GLuint textureIndex;
	_renderwidget->makeCurrent();
	glGenTextures(1, &textureIndex);
	glBindTexture(GL_TEXTURE_2D, textureIndex);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

	// place it forward
	double d = (_sources.empty()) ? 0.0 : (*_sources.rbegin())->getDepth() + 1.0;

	// create the OpenCV source
	OpencvSource *s = new OpencvSource(opencvIndex, textureIndex, d);
    Q_CHECK_PTR(s);

	// set the last created source to be current
	setCurrentSource(_sources.insert((Source *) s));
}
#endif

void RenderingManager::removeSource(SourceSet::iterator itsource) {

	if (itsource != _sources.end()) {
		delete (*itsource);
		_sources.erase(itsource);
	}

	// Disable update of previous frame if all the RenderingSources are deleted
	if (countRenderingSource <= 0){
		if (previousframe_fbo)
			delete previousframe_fbo;
		previousframe_fbo = 0;
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


