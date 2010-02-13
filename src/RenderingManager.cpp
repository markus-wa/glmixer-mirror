/*
 * RenderingManager.cpp
 *
 *  Created on: 3 nov. 2009
 *      Author: herbelin
 */

#include "RenderingManager.moc"

#include "common.h"
//#include "glRenderWidget.h"
//#include "View.h"
//#include "MixerView.h"
//#include "GeometryView.h"
#include "VideoSource.h"
#ifdef OPEN_CV
#include "OpencvSource.h"
#endif

#include <algorithm>


// static members
RenderingManager *RenderingManager::_instance = 0;




ViewRenderWidget *RenderingManager::getRenderingWidget() {

	return getInstance()->_renderwidget;
}

RenderingManager *RenderingManager::getInstance() {

	if (_instance == 0){
		_instance = new RenderingManager;
	    Q_CHECK_PTR(_instance);
	}

	return _instance;
}


RenderingManager::RenderingManager() :
	QObject(), _fbo(NULL) {

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

	if (!QGLFramebufferObject::hasOpenGLFramebufferObjects())
		qCritical("Frame Buffer Objects not supported on this graphics hardware");
	else {
		if (_fbo)
			delete _fbo;

		_renderwidget->makeCurrent();
		_fbo = new QGLFramebufferObject(width,height);
	    Q_CHECK_PTR(_fbo);
	}
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
GLuint RenderingManager::getFrameBufferHandle(){
	return _fbo->handle();
}
int RenderingManager::getFrameBufferWidth(){
	return _fbo->width();
}
int RenderingManager::getFrameBufferHeight(){
	return _fbo->height();
}

void RenderingManager::addSource(VideoFile *vf) {

	double d = (_sources.empty()) ? 0.0 : (*_sources.rbegin())->getDepth() + 1.0;
	// create a source appropriate for this videofile
	VideoSource *s = new VideoSource(vf, (QGLWidget *) _renderwidget, d);
    Q_CHECK_PTR(s);
	// ensure we display first frame (not done automatically by signal as it should...)
	s->updateFrame(-1);

	// set the last created source to be current
	setCurrentSource(_sources.insert((Source *) s));
}

#ifdef OPEN_CV
void RenderingManager::addSource(int opencvIndex) {

	double d = (_sources.empty()) ? 0.0 : (*_sources.rbegin())->getDepth() + 1.0;
	OpencvSource *s = new OpencvSource(opencvIndex, (QGLWidget *) _renderwidget, d);
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


