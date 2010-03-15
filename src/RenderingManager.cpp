/*
 * RenderingManager.cpp
 *
 *  Created on: 3 nov. 2009
 *      Author: herbelin
 */

#include "RenderingManager.moc"

#include "common.h"

#include "RenderingSource.h"
RenderingSource::RTTI RenderingSource::type = Source::RENDERING_SOURCE;

#include "CloneSource.h"
CloneSource::RTTI CloneSource::type = Source::CLONE_SOURCE;

#include "ViewRenderWidget.h"
#include "SourcePropertyBrowser.h"
#include "AlgorithmSource.h"
#include "VideoFile.h"
#include "VideoSource.h"
#ifdef OPEN_CV
#include "OpencvSource.h"
#endif

#include <algorithm>
#include <QGLFramebufferObject>


// static members
RenderingManager *RenderingManager::_instance = 0;
bool RenderingManager::blit = false;


ViewRenderWidget *RenderingManager::getRenderingWidget() {

	return getInstance()->_renderwidget;
}


SourcePropertyBrowser *RenderingManager::getPropertyBrowserWidget() {

	return getInstance()->_propertyBrowser;
}

RenderingManager *RenderingManager::getInstance() {

	if (_instance == 0){

		if (!QGLFramebufferObject::hasOpenGLFramebufferObjects())
			qCritical("*** ERROR ***\n\nOpenGL Frame Buffer Objects are not supported on this graphics hardware; the program cannot operate properly.\n\nExiting...");

		if (glSupportsExtension("GL_EXT_framebuffer_blit"))
	    	RenderingManager::blit = true;
	    else
	    	qWarning("** WARNING **\n\nOpenGL extension GL_EXT_framebuffer_blit is not supported on this graphics hardware.\n\nRendering speed be sub-optimal but all should work properly.");

		_instance = new RenderingManager;
	    Q_CHECK_PTR(_instance);
	}

	return _instance;
}

void RenderingManager::deleteInstance() {
	if (_instance != 0)
		delete _instance;
}

RenderingManager::RenderingManager() :
	QObject(), _fbo(NULL), previousframe_fbo(NULL), countRenderingSource(0), previousframe_index(0), previousframe_delay(1) {

	_renderwidget = new ViewRenderWidget;
    Q_CHECK_PTR(_renderwidget);

    _propertyBrowser = new SourcePropertyBrowser;
    Q_CHECK_PTR(_propertyBrowser);

	setFrameBufferResolution(DEFAULT_WIDTH, DEFAULT_HEIGHT);

	_currentSource = getEnd();
}

RenderingManager::~RenderingManager() {

	clearSourceSet();

	if (_renderwidget != 0)
		delete _renderwidget;

	if (_fbo)
		delete _fbo;

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
	    glBindFramebufferEXT(GL_READ_FRAMEBUFFER, _fbo->handle());
		glBindFramebufferEXT(GL_DRAW_FRAMEBUFFER, previousframe_fbo->handle());

		glBlitFramebufferEXT(0, _fbo->height(), _fbo->width(), 0,
							0, 0, previousframe_fbo->width(), previousframe_fbo->height(),
							GL_COLOR_BUFFER_BIT, GL_NEAREST);

	    glBindFramebufferEXT(GL_DRAW_FRAMEBUFFER, 0);
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

void RenderingManager::clearFrameBuffer(){
	_fbo->bind();
	{
		glPushAttrib(GL_COLOR_BUFFER_BIT);
		glClearColor(0.0, 0.0, 0.0, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT);
		glPopAttrib();
	}
	_fbo->release();
}

void RenderingManager::renderToFrameBuffer(SourceSet::iterator itsource, bool clearfirst){

	glPushAttrib(GL_COLOR_BUFFER_BIT | GL_VIEWPORT_BIT);

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

		if ( !(*itsource)->isCulled()  && (*itsource)->getAlpha() > 0.0 ) {
			glTranslated((*itsource)->getX(), (*itsource)->getY(), 0.0);
			glScaled((*itsource)->getScaleX(), (*itsource)->getScaleY(), 1.f);

			(*itsource)->blend();
			(*itsource)->draw();
		}
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
		
	// in QT 4.6 implementation of blitting of FBO, the y is flipped ; avoid this here on the texture
#if QT_VERSION < 0x040600
	GLuint textureIndex = _renderwidget->bindTexture (capture);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
#else
	GLuint textureIndex = _renderwidget->bindTexture (capture, GL_TEXTURE_2D, GL_RGBA, QGLContext::LinearFilteringBindOption | QGLContext::MipmapBindOption);
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR );
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR_MIPMAP_LINEAR );
#endif

	// optimize texture access
	GLclampf highpriority = 1.0;
	glPrioritizeTextures(1, &textureIndex, &highpriority);

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
	GLclampf lowpriority = 0.1;
	glPrioritizeTextures(1, &textureIndex, &lowpriority);

	// place it forward
	double d = (_sources.empty()) ? 0.0 : (*_sources.rbegin())->getDepth() + 1.0;

	// create a source appropriate for this videofile
	VideoSource *s = new VideoSource(vf, textureIndex, d);
    Q_CHECK_PTR(s);

    // create the properties
    _propertyBrowser->createProperty((Source *) s);

	// set the last created source to be current
	setCurrentSource(_sources.insert((Source *) s));
}

#ifdef OPEN_CV
void RenderingManager::addOpencvSource(int opencvIndex) {

	// create the texture for this source
	GLuint textureIndex;
	_renderwidget->makeCurrent();
	glGenTextures(1, &textureIndex);
	GLclampf lowpriority = 0.1;
	glPrioritizeTextures(1, &textureIndex, &lowpriority);

	// place it forward
	double d = (_sources.empty()) ? 0.0 : (*_sources.rbegin())->getDepth() + 1.0;

	// create the OpenCV source
	OpencvSource *s = new OpencvSource(opencvIndex, textureIndex, d);
    Q_CHECK_PTR(s);

	// set the last created source to be current
	setCurrentSource(_sources.insert((Source *) s));
}
#endif


void RenderingManager::addAlgorithmSource(int type, int w, int h, double v, int p) {

	// create the texture for this source
	GLuint textureIndex;
	_renderwidget->makeCurrent();
	glGenTextures(1, &textureIndex);
	GLclampf lowpriority = 0.1;
	glPrioritizeTextures(1, &textureIndex, &lowpriority);

	// place it forward
	double d = (_sources.empty()) ? 0.0 : (*_sources.rbegin())->getDepth() + 1.0;

	// create a source appropriate for this videofile
	AlgorithmSource *s = new AlgorithmSource(type, textureIndex, d, w, h, v, p);
    Q_CHECK_PTR(s);

	// set the last created source to be current
	setCurrentSource(_sources.insert((Source *) s));
}


void RenderingManager::addCloneSource(SourceSet::iterator sit) {

	// place it forward
	double d = (_sources.empty()) ? 0.0 : (*_sources.rbegin())->getDepth() + 1.0;

	// create a source appropriate for this videofile
	CloneSource *clone = new CloneSource(sit, d);
    Q_CHECK_PTR(clone);

	// set the last created source to be current
	setCurrentSource(_sources.insert((Source *) clone));
}

void RenderingManager::removeSource(SourceSet::iterator itsource) {

	// if we are removing the current source, ensure it is not the current one anymore
	if (itsource == _currentSource) {
		_currentSource = _sources.end();
		emit currentSourceChanged(_currentSource);
	}

	if (itsource != _sources.end() ) {
		CloneSource *cs = dynamic_cast<CloneSource *>(*itsource);
		if (cs == NULL)
			// first remove every clone of the source to be removed (if it is not a clone already)
			for (SourceList::iterator clone = (*itsource)->getClones()->begin(); clone < (*itsource)->getClones()->end(); clone = (*itsource)->getClones()->begin()) {
				removeSource( getById( (*clone)->getId() ) );
			}
		// then remove the source itself
		_sources.erase(itsource);
		delete (*itsource);
	}

	// Disable update of previous frame if all the RenderingSources are deleted
	if (countRenderingSource <= 0){
		if (previousframe_fbo)
			delete previousframe_fbo;
		previousframe_fbo = 0;
	}
}

void RenderingManager::clearSourceSet() {

	for (SourceSet::iterator its = _sources.begin(); its != _sources.end(); its = _sources.begin())
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

bool RenderingManager::setCurrentSource(SourceSet::iterator si) {

	if (si != _currentSource) {
		if (notAtEnd(_currentSource))
			(*_currentSource)->activate(false);

		_currentSource = si;
		emit currentSourceChanged(_currentSource);

		if (notAtEnd(_currentSource))
			(*_currentSource)->activate(true);

		return true;
	}
	return false;
}


bool RenderingManager::setCurrentSource(GLuint name) {

	return setCurrentSource( getById(name) );
}

SourceSet::iterator RenderingManager::changeDepth(SourceSet::iterator itsource, double newdepth) {

	if (itsource != _sources.end()) {
		Source *tmp = (*itsource);
		// sort again the set by depth: this is done by removing the element and adding it again after changing its depth
		_sources.erase(itsource);
		tmp->setDepth(newdepth);

		return (_sources.insert(tmp));
	}

	return itsource;
}

SourceSet::iterator RenderingManager::getById(GLuint name) {

	return std::find_if(_sources.begin(), _sources.end(), hasName(name));
}


