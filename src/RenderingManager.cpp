/*
 * RenderingManager.cpp
 *
 *  Created on: 3 nov. 2009
 *      Author: herbelin
 *
 *  This file is part of GLMixer.
 *
 *   GLMixer is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation, either version 3 of the License, or
 *   (at your option) any later version.
 *
 *   GLMixer is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with GLMixer.  If not, see <http://www.gnu.org/licenses/>.
 *
 *   Copyright 2009, 2010 Bruno Herbelin
 *
 */

#include "RenderingManager.moc"

#include "common.h"

#include "AlgorithmSource.h"
#include "VideoFile.h"
#include "VideoSource.h"
#ifdef OPEN_CV
#include "OpencvSource.h"
#endif
#include "CaptureSource.h"
#include "SvgSource.h"
#include "RenderingSource.h"
Source::RTTI RenderingSource::type = Source::RENDERING_SOURCE;
#include "CloneSource.h"
Source::RTTI CloneSource::type = Source::CLONE_SOURCE;

#include "ViewRenderWidget.h"
#include "CatalogView.h"
#include "RenderingEncoder.h"
#include "SourcePropertyBrowser.h"
#include "SessionSwitcher.h"


#include <map>
#include <algorithm>
#include <QGLFramebufferObject>
#include <QProgressDialog>

// static members
RenderingManager *RenderingManager::_instance = 0;
bool RenderingManager::blit_fbo_extension = true;
QSize RenderingManager::sizeOfFrameBuffer[ASPECT_RATIO_FREE][QUALITY_UNSUPPORTED] = { { QSize(640,480), QSize(768,576), QSize(800,600), QSize(1024,768), QSize(1600,1200) },
																		   { QSize(720,480), QSize(864,576), QSize(900,600), QSize(1152,768), QSize(1440,960) },
																           { QSize(800,480), QSize(912,570), QSize(960,600), QSize(1280,800), QSize(1920,1200) },
																           { QSize(848,480), QSize(1024,576), QSize(1088,612), QSize(1280,720), QSize(1920,1080) }};

ViewRenderWidget *RenderingManager::getRenderingWidget() {

	return getInstance()->_renderwidget;
}

SourcePropertyBrowser *RenderingManager::getPropertyBrowserWidget() {

	return getInstance()->_propertyBrowser;
}

RenderingEncoder *RenderingManager::getRecorder() {

	return getInstance()->_recorder;
}

SessionSwitcher *RenderingManager::getSessionSwitcher() {

	return getInstance()->_switcher;
}

void RenderingManager::setUseFboBlitExtension(bool on){

       if (glSupportsExtension("GL_EXT_framebuffer_blit"))
               RenderingManager::blit_fbo_extension = on;
       else {
               // if extension not supported but it is requested, show warning
               if (on) {
            	   qWarning() << "RenderingManager|" << tr("OpenGL extension GL_EXT_framebuffer_blit is not supported on this graphics hardware. Rendering speed be sub-optimal but all should work properly.");
            	   qCritical() << "RenderingManager|" << tr("Cannot use Blit framebuffer extension.");
               }
               RenderingManager::blit_fbo_extension = false;
       }
}

RenderingManager *RenderingManager::getInstance() {

	if (_instance == 0) {

		if (!QGLFramebufferObject::hasOpenGLFramebufferObjects())
			qFatal( "%s", qPrintable( tr("OpenGL Frame Buffer Objects are not supported on this graphics hardware."
					"\n\nThe program cannot operate properly.\n\nExiting...") ));

		if (!glSupportsExtension("GL_ARB_vertex_program") || !glSupportsExtension("GL_ARB_fragment_program"))
			qFatal( "%s", qPrintable( tr("OpenGL GLSL programming is not supported on this graphics hardware."
					"\n\nThe program cannot operate properly.\n\nExiting...")));

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
	QObject(), _fbo(NULL), _fboCatalogTexture(0), previousframe_fbo(NULL), countRenderingSource(0),
			previousframe_index(0), previousframe_delay(1), clearWhite(false),
			gammaShift(1.f), _scalingMode(Source::SCALE_CROP), _playOnDrop(true), paused(false), _showProgressBar(true) {

	// 1. Create the view rendering widget and its catalog view
	_renderwidget = new ViewRenderWidget;
	Q_CHECK_PTR(_renderwidget);

	_propertyBrowser = new SourcePropertyBrowser;
	Q_CHECK_PTR(_propertyBrowser);

	// create recorder and session switcher
	_recorder = new RenderingEncoder(this);
	_switcher = new SessionSwitcher(this);

	// 2. Connect the above view holders to events
    QObject::connect(this, SIGNAL(currentSourceChanged(SourceSet::iterator)), _propertyBrowser, SLOT(showProperties(SourceSet::iterator) ) );

    QObject::connect(_renderwidget, SIGNAL(sourceMixingModified()), _propertyBrowser, SLOT(updateMixingProperties() ) );
    QObject::connect(_renderwidget, SIGNAL(sourceGeometryModified()), _propertyBrowser, SLOT(updateGeometryProperties() ) );
    QObject::connect(_renderwidget, SIGNAL(sourceLayerModified()), _propertyBrowser, SLOT(updateLayerProperties() ) );

    QObject::connect(_renderwidget, SIGNAL(sourceMixingDrop(double,double)), this, SLOT(dropSourceWithAlpha(double, double) ) );
    QObject::connect(_renderwidget, SIGNAL(sourceGeometryDrop(double,double)), this, SLOT(dropSourceWithCoordinates(double, double)) );
    QObject::connect(_renderwidget, SIGNAL(sourceLayerDrop(double)), this, SLOT(dropSourceWithDepth(double)) );

    // 3. Initialize the frame buffer
    renderingQuality = QUALITY_VGA;
    renderingAspectRatio = ASPECT_RATIO_4_3;
	setFrameBufferResolution( sizeOfFrameBuffer[renderingAspectRatio][renderingQuality] );

	_currentSource = getEnd();

	// 4. Setup the default default values ! :)
    _defaultSource = new Source;
}

RenderingManager::~RenderingManager() {

	clearSourceSet();

	if (_renderwidget)
		delete _renderwidget;

	if (_fbo)
		delete _fbo;

	delete _propertyBrowser;
	delete _recorder;
	delete _switcher;
	delete _defaultSource;
}


void RenderingManager::setRenderingQuality(frameBufferQuality q)
{
	if ( q == QUALITY_UNSUPPORTED )
		q = QUALITY_VGA;

	// ignore if nothing changes
	if (q == renderingQuality)
		return;

	// quality changed ; change resolution
	renderingQuality = q;
	setFrameBufferResolution( sizeOfFrameBuffer[renderingAspectRatio][renderingQuality]);
}


void RenderingManager::setRenderingAspectRatio(standardAspectRatio ar)
{
	// ignore if nothing changes
	if ( ar == renderingAspectRatio )
		return;

	renderingAspectRatio = ar;

	// by default, free windows are rendered with a 4:3 aspect ratio frame bufer
	if (ar == ASPECT_RATIO_FREE)
		ar = ASPECT_RATIO_4_3;
	setFrameBufferResolution( sizeOfFrameBuffer[ar][renderingQuality]);
}

void RenderingManager::setFrameBufferResolution(QSize size) {

	_renderwidget->makeCurrent();

	if (_fbo) {
		delete _fbo;
		glDeleteTextures(1, &_fboCatalogTexture);
	}

	// create an fbo (with internal automatic first texture attachment)
	_fbo = new QGLFramebufferObject(size);
	Q_CHECK_PTR(_fbo);

	if (_fbo->bind()) {
		// default draw target
		glDrawBuffer(GL_COLOR_ATTACHMENT0);

		// create second draw target texture for this FBO (for catalog)
		glGenTextures(1, &_fboCatalogTexture);
		glBindTexture(GL_TEXTURE_2D, _fboCatalogTexture);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, _fbo->width(), _fbo->height(), 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);

		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

		glFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, _fboCatalogTexture, 0);
		glBindTexture(GL_TEXTURE_2D, 0);
		_fbo->release();

		// store viewport info
		_renderwidget->_renderView->viewport[0] = 0;
		_renderwidget->_renderView->viewport[1] = 0;
		_renderwidget->_renderView->viewport[2] = _fbo->width();
		_renderwidget->_renderView->viewport[3] = _fbo->height();

		_renderwidget->_catalogView->viewport[1] = 0;
		_renderwidget->_catalogView->viewport[3] = _fbo->height();
	}
	else
		qFatal( "%s", qPrintable( tr("OpenGL Frame Buffer Objects is not working on this hardware."
							"\n\nThe program cannot operate properly.\n\nExiting...")));
}


double RenderingManager::getFrameBufferAspectRatio() const{

	return ((double) _fbo->width() / (double) _fbo->height());
}

void RenderingManager::postRenderToFrameBuffer() {

	// skip loop back if disabled
	if (previousframe_fbo) {

		// frame delay
		previousframe_index++;

		if (previousframe_index % previousframe_delay)
			return;

		previousframe_index = 0;

		if (RenderingManager::blit_fbo_extension)
		// use the accelerated GL_EXT_framebuffer_blit if available
		{
			glBindFramebuffer(GL_READ_FRAMEBUFFER, _fbo->handle());
			// TODO : Can we draw in different texture buffer so we can keep an history of
			// several frames, and each loopback source could use a different one
			glBindFramebuffer(GL_DRAW_FRAMEBUFFER, previousframe_fbo->handle());

			glBlitFramebuffer(0, _fbo->height(), _fbo->width(), 0, 0, 0,
					previousframe_fbo->width(), previousframe_fbo->height(),
					GL_COLOR_BUFFER_BIT, GL_NEAREST);

			glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
		} else
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
			if (previousframe_fbo->bind())
			{

				glClearColor(0.f, 0.f, 0.f, 1.f);
				glClear(GL_COLOR_BUFFER_BIT);

				glEnable(GL_TEXTURE_2D);
				glBindTexture(GL_TEXTURE_2D, _fbo->texture());
				glCallList(ViewRenderWidget::quad_texured);

				previousframe_fbo->release();
			}

			// pop the projection matrix and GL state back for rendering the current view
			// to the actual widget
			glPopAttrib();
			glMatrixMode(GL_PROJECTION);
			glPopMatrix();
			glMatrixMode(GL_MODELVIEW);
			glPopMatrix();
		}
	}

	// save the frame to file (the recorder knows if it is active or not)
	_recorder->addFrame();

}


void RenderingManager::renderToFrameBuffer(Source *source, bool first, bool last) {

	glPushAttrib(GL_COLOR_BUFFER_BIT | GL_VIEWPORT_BIT);

	glViewport(0, 0, _renderwidget->_renderView->viewport[2], _renderwidget->_renderView->viewport[3]);

	glMatrixMode(GL_PROJECTION);
	glPushMatrix();
	glLoadMatrixd(_renderwidget->_renderView->projection);

	glMatrixMode(GL_MODELVIEW);
	glPushMatrix();
	glLoadIdentity();


	// render to the frame buffer object
	if (_fbo->bind())
	{
		//
		// 1. Draw into first texture attachment; the final output rendering
		//
		if (first) {
			if (clearWhite)
				glClearColor(1.f, 1.f, 1.f, 1.f);
			else
				glClearColor(0.f, 0.f, 0.f, 1.f);
			glClear(GL_COLOR_BUFFER_BIT);
		}

		if (source) {
			// draw the source only if not culled and alpha not null
			if (!source->isStandby() && !source->isCulled() && source->getAlpha() > 0.0) {
				glTranslated(source->getX(), source->getY(), 0.0);
		        glRotated(source->getRotationAngle(), 0.0, 0.0, 1.0);
				glScaled(source->getScaleX(), source->getScaleY(), 1.f);

				// gamma shift
				ViewRenderWidget::program->setUniformValue("gamma", source->getGamma() * gammaShift);

				source->blend();
				source->draw();
			}
			// in any case, always end the effect
			source->endEffectsSection();

		}
		//
		// 2. Draw into second texture  attachment ; the catalog (if visible)
		//
		if (_renderwidget->_catalogView->visible()) {
			glDrawBuffer(GL_COLOR_ATTACHMENT1);

			// clear Modelview
			glLoadIdentity();
			// draw without effect
			ViewRenderWidget::setSourceDrawingMode(false);

			static int indexSource = 0;
			if (first) {
				// Clear Catalog view
				_renderwidget->_catalogView->clear();
				indexSource = 0;
			}
			// Draw this source into the catalog
			_renderwidget->_catalogView->drawSource( source, indexSource++);

			glDrawBuffer(GL_COLOR_ATTACHMENT0);
		}

		// render the transition layer on top after the last frame
		if (last) {

			ViewRenderWidget::setSourceDrawingMode(true);
			_switcher->render();
		}

		_fbo->release();
	}
	else
		qFatal( "%s", qPrintable( tr("OpenGL Frame Buffer Objects is not accessible."
			"\n\nThe program cannot operate properly.\n\nExiting...")));

	// pop the projection matrix and GL state back for rendering the current view
	// to the actual widget
	glPopAttrib();
	glMatrixMode(GL_PROJECTION);
	glPopMatrix();
	glMatrixMode(GL_MODELVIEW);
	glPopMatrix();
}

Source *RenderingManager::newRenderingSource(double depth) {

	_renderwidget->makeCurrent();
	// create the previous frame (frame buffer object) if needed
	if (!previousframe_fbo) {
		previousframe_fbo = new QGLFramebufferObject(_fbo->width(), _fbo->height());
	}

	// create a source appropriate
	RenderingSource *s = new RenderingSource(previousframe_fbo->texture(), getAvailableDepthFrom(depth));
	if (s) s->setName( _defaultSource->getName() + "Render");

	return ( (Source *) s );
}

QImage RenderingManager::captureFrameBuffer() {

	_renderwidget->makeCurrent();
	return _fbo->toImage();
}


Source *RenderingManager::newSvgSource(QSvgRenderer *svg, double depth){

	// create the texture for this source
	GLuint textureIndex;
	_renderwidget->makeCurrent();
	glGenTextures(1, &textureIndex);
	// high priority means low variability
	GLclampf highpriority = 1.0;
	glPrioritizeTextures(1, &textureIndex, &highpriority);

	// create a source appropriate
	SvgSource *s = new SvgSource(svg, textureIndex, getAvailableDepthFrom(depth));
	if (s) s->setName( _defaultSource->getName() + "Svg");

	return ( (Source *) s );
}

Source *RenderingManager::newCaptureSource(QImage img, double depth) {

	// create the texture for this source
	GLuint textureIndex;
	_renderwidget->makeCurrent();
	glGenTextures(1, &textureIndex);
	// high priority means low variability
	GLclampf highpriority = 1.0;
	glPrioritizeTextures(1, &textureIndex, &highpriority);

	// create a source appropriate
	CaptureSource *s = new CaptureSource(img, textureIndex, getAvailableDepthFrom(depth));
	if (s) s->setName( _defaultSource->getName() + "Capture");

	return ( (Source *) s );
}

Source *RenderingManager::newMediaSource(VideoFile *vf, double depth) {


	// create the texture for this source
	GLuint textureIndex;
	_renderwidget->makeCurrent();
	glGenTextures(1, &textureIndex);
	// low priority means high variability
	GLclampf lowpriority = 0.1;
	glPrioritizeTextures(1, &textureIndex, &lowpriority);

	// create a source appropriate for this videofile
	VideoSource *s = new VideoSource(vf, textureIndex, getAvailableDepthFrom(depth) );
	if (s) s->setName( _defaultSource->getName() + QDir(vf->getFileName()).dirName().split(".").first());

	return ( (Source *) s );
}

#ifdef OPEN_CV
Source *RenderingManager::newOpencvSource(int opencvIndex, double depth) {

	GLuint textureIndex;
	OpencvSource *s = 0;

	// try to create the OpenCV source
	try {
		// create the texture for this source
		_renderwidget->makeCurrent();
		glGenTextures(1, &textureIndex);
		GLclampf lowpriority = 0.1;

		glPrioritizeTextures(1, &textureIndex, &lowpriority);

		// try to create the opencv source
		s = new OpencvSource(opencvIndex, textureIndex, getAvailableDepthFrom(depth));
		s->setName( _defaultSource->getName() + "Opencv");

	} catch (NoCameraIndexException){

		// free the OpenGL texture
		glDeleteTextures(1, &textureIndex);
		// return an invalid pointer
		s = 0;
	}

	return ( (Source *) s );
}
#endif

Source *RenderingManager::newAlgorithmSource(int type, int w, int h, double v,
		int p, double depth) {

	// create the texture for this source
	GLuint textureIndex;
	_renderwidget->makeCurrent();
	glGenTextures(1, &textureIndex);
	GLclampf lowpriority = 0.1;
	glPrioritizeTextures(1, &textureIndex, &lowpriority);

	// create a source appropriate
	AlgorithmSource *s = new AlgorithmSource(type, textureIndex, getAvailableDepthFrom(depth), w, h, v, p);
	if (s) s->setName( _defaultSource->getName() + "Algo");

	return ( (Source *) s );
}

Source *RenderingManager::newCloneSource(SourceSet::iterator sit, double depth) {

	// create a source appropriate for this videofile
	CloneSource *s = new CloneSource(sit, getAvailableDepthFrom(depth));
	if (s) s->setName( _defaultSource->getName() + "Clone");

	return ( (Source *) s );
}

bool RenderingManager::insertSource(Source *s)
{
	if (s) {
		// replace the source name by another available one based on the original name
		s->setName( getAvailableNameFrom(s->getName()) );

		if (_sources.size() < MAX_SOURCE_COUNT) {
			//insert the source to the list
			if (_sources.insert(s).second)
				// inform of success
				return true;
			else
				qWarning() << tr("RenderingManager|Not enough memory to insert the source into the stack.");
		}
		else
			qWarning() << tr("RenderingManager|You have reached the maximum amount of source supported (%1).").arg(MAX_SOURCE_COUNT);
	}

	return false;
}

void RenderingManager::addSourceToBasket(Source *s)
{
	// add the source into the basket
	dropBasket.insert(s);
	// apply default parameters
	s->copyPropertiesFrom(_defaultSource);
	// scale the source to match the preferences
	s->resetScale(_scalingMode);
	// select no source
	setCurrentSource( getEnd() );
}

void RenderingManager::clearBasket()
{
	for (SourceList::iterator sit = dropBasket.begin(); sit != dropBasket.end(); sit = dropBasket.begin()) {
		dropBasket.erase(sit);
		delete (*sit);
	}
}

void RenderingManager::resetSource(SourceSet::iterator sit){

	// apply default parameters
	(*sit)->copyPropertiesFrom(_defaultSource);
	// scale the source to match the preferences
	(*sit)->resetScale(_scalingMode);

}


void RenderingManager::selectCurrentSource(GLuint name){

	setCurrentSource(name);
}


void RenderingManager::toggleMofifiableCurrentSource(){

	if(isValid(_currentSource)) {
		(*_currentSource)->setModifiable( ! (*_currentSource)->isModifiable() );
		_propertyBrowser->showProperties(_currentSource);
	}
}

void RenderingManager::resetCurrentSource(){

	if(isValid(_currentSource)) {
		resetSource(_currentSource);
		_propertyBrowser->showProperties(_currentSource);
	}
}

int RenderingManager::getSourceBasketSize() const{

	return (int) (dropBasket.size());
}

Source *RenderingManager::getSourceBasketTop() const{

	if (dropBasket.empty())
		return 0;
	else
		return (*dropBasket.begin());
}

void RenderingManager::dropSourceWithAlpha(double alphax, double alphay){

	// nothing to drop ?
	if (dropBasket.empty())
		return;
	// get the pointer to the source at the top of the list
	Source *top = *dropBasket.begin();
	// remove from the basket
	dropBasket.erase(top);
	// insert the source
	if ( insertSource(top) ) {
		// make it current
		setCurrentSource(top->getId());
		// apply the modifications
		top->setAlphaCoordinates(alphax, alphay);
		// start playing (according to preference)
		top->play(_playOnDrop);
	} else
		delete top;

}

void RenderingManager::dropSourceWithCoordinates(double x, double y){

	// nothing to drop ?
	if (dropBasket.empty())
		return;

	// get the pointer to the source at the top of the list
	Source *top = *dropBasket.begin();
	// remove from the basket
	dropBasket.erase(top);
	// insert the source
	if ( insertSource(top) ) {
		// make it current
		setCurrentSource(top->getId());
		// apply the modifications
		top->setX(x);
		top->setY(y);
		// start playing (according to preference)
		top->play(_playOnDrop);
	} else
		delete top;
}

void RenderingManager::dropSourceWithDepth(double depth){

	// nothing to drop ?
	if (dropBasket.empty())
		return;

	// get the pointer to the source at the top of the list
	Source *top = *dropBasket.begin();
	// remove from the basket
	dropBasket.erase(top);
	// insert the source
	if ( insertSource(top) ) {
		// make it current
		setCurrentSource(top->getId());
		// apply the modifications
		top->setDepth(depth);
		// start playing (according to preference)
		top->play(_playOnDrop);
	} else
		delete top;

}

void RenderingManager::removeSource(const GLuint idsource){

	removeSource(getById(idsource));

}

void RenderingManager::removeSource(SourceSet::iterator itsource) {

	// remove from all selections
	_renderwidget->removeFromSelections(*itsource);

	// if we are removing the current source, ensure it is not the current one anymore
	if (itsource == _currentSource) {
		_currentSource = _sources.end();
		emit currentSourceChanged(_currentSource);
	}

	if (itsource != _sources.end()) {
		// if this is not a clone
		if ((*itsource)->rtti() != Source::CLONE_SOURCE)
			// remove every clone of the source to be removed
			for (SourceList::iterator clone = (*itsource)->getClones()->begin(); clone != (*itsource)->getClones()->end(); clone = (*itsource)->getClones()->begin()) {
				removeSource((*clone)->getId());
			}
		// then remove the source itself
		qDebug() << (*itsource)->getName() << tr("|Source deleted.");
		_sources.erase(itsource);
		delete (*itsource);
	}

	// is there no more rendering source ?
	if (countRenderingSource == 0) {
		// no more rendering source; we can disable update of previous frame
		if (previousframe_fbo)
			delete previousframe_fbo;
		previousframe_fbo = NULL;
	}
}

void RenderingManager::clearSourceSet() {

	// clear the list of sources
	for (SourceSet::iterator its = _sources.begin(); its != _sources.end(); its = _sources.begin())
		removeSource(its);

	// reset the id counter
	Source::lastid = 1;

	qDebug("RenderingManager|--------------- clear --------------------");
}

bool RenderingManager::notAtEnd(SourceSet::iterator itsource)  const{
	return (itsource != _sources.end());
}

bool RenderingManager::isValid(SourceSet::iterator itsource)  const{

	if (notAtEnd(itsource))
		return (_sources.find(*itsource) != _sources.end());
	else
		return false;
}


bool RenderingManager::isCurrentSource(Source *s){
	if (_currentSource != _sources.end())
		return ( s == *_currentSource );
	else
		return false;
}

bool RenderingManager::isCurrentSource(SourceSet::iterator si){

	return ( si == _currentSource );

}

bool RenderingManager::setCurrentSource(SourceSet::iterator si) {

	if (si != _currentSource) {

		_currentSource = si;
		emit currentSourceChanged(_currentSource);

		return true;
	}
	return false;
}

bool RenderingManager::setCurrentSource(GLuint id) {

	return setCurrentSource(getById(id));
}


bool RenderingManager::setCurrentNext(){

	if (_sources.empty() )
		return false;

	if (_currentSource != _sources.end()) {
		// increment to next source
		_currentSource++;
		// loop to begin if at end
		if (_currentSource == _sources.end())
			_currentSource = _sources.begin();
	} else
		_currentSource = _sources.begin();

	emit currentSourceChanged(_currentSource);
	return true;
}

bool RenderingManager::setCurrentPrevious(){

	if (_sources.empty() )
		return false;

	if (_currentSource != _sources.end()) {

		// if at the beginning, go to the end
		if (_currentSource == _sources.begin())
			_currentSource = _sources.end();
	}

	// decrement to previous source
	_currentSource--;
	emit currentSourceChanged(_currentSource);
	return true;
}


QString RenderingManager::getAvailableNameFrom(QString name) const{

	// start with a tentative name and assume it is NOT ok
	QString tentativeName = name;
	bool isok = false;
	int countbad = 2;
	// try to find the name in the list; it is still not ok if it exists
	while (!isok) {
		if ( isValid( getByName(tentativeName) ) ){
			// modify the tentative name and keep trying
			tentativeName = name + QString("-%1").arg(countbad++);
		} else
			isok = true;
	}
	// finally the tentative name is ok
	return tentativeName;
}

double RenderingManager::getAvailableDepthFrom(double depth) const {

	double tentativeDepth = depth;

	// place it at the front if no depth is provided (default argument = -1)
	if (tentativeDepth < 0)
		tentativeDepth  = (_sources.empty()) ? 0.0 : (*_sources.rbegin())->getDepth() + 1.0;

	tentativeDepth += dropBasket.size();

	// try to find a source at this depth in the list; it is not ok if it exists
	bool isok = false;
	while (!isok) {
		if ( isValid( std::find_if(_sources.begin(), _sources.end(), isCloseTo(tentativeDepth)) ) ){
			tentativeDepth += DEPTH_EPSILON;
		} else
			isok = true;
	}
	// finally the tentative depth is ok
	return tentativeDepth;
}

SourceSet::iterator RenderingManager::changeDepth(SourceSet::iterator itsource,
		double newdepth) {

	if (itsource != _sources.end()) {
		// verify that the depth value is not already taken, or too close to, and adjust in case.
		SourceSet::iterator sb, se;
		double depthinc = 0.0;
		if (newdepth < (*itsource)->getDepth()) {
			sb = _sources.begin();
			se = itsource;
			depthinc = -DEPTH_EPSILON;
		} else {
			sb = itsource;
			sb++;
			se = _sources.end();
			depthinc = DEPTH_EPSILON;
		}
		while (std::find_if(sb, se, isCloseTo(newdepth)) != se) {
			newdepth += depthinc;
		}

		// remember pointer to the source
		Source *tmp = (*itsource);
		// sort again the set by depth: this is done by removing the element and adding it again after changing its depth
		_sources.erase(itsource);
		// change the source internal depth value
		tmp->setDepth(newdepth);

		if (newdepth < 0) {
			// if request to place the source in a negative depth, shift all sources forward
			for (SourceSet::iterator it = _sources.begin(); it
					!= _sources.end(); it++)
				(*it)->setDepth((*it)->getDepth() - newdepth);
		}

		// re-insert the source into the sorted list ; it will be placed according to its new depth
		std::pair<SourceSet::iterator, bool> ret;
		ret = _sources.insert(tmp);
		if (ret.second)
			return (ret.first);
		else
			return (_sources.end());
	}

	return _sources.end();
}

SourceSet::iterator RenderingManager::getById(const GLuint id) const {

	return std::find_if(_sources.begin(), _sources.end(), hasId(id));
}

SourceSet::iterator RenderingManager::getByName(const QString name) const{

	return std::find_if(_sources.begin(), _sources.end(), hasName(name));
}
/**
 * save and load configuration
 */
QDomElement RenderingManager::getConfiguration(QDomDocument &doc, QDir current) {

	QDomElement config = doc.createElement("SourceList");

	for (SourceSet::iterator its = _sources.begin(); its != _sources.end(); its++) {

		QDomElement sourceElem = doc.createElement("Source");
		sourceElem.setAttribute("name", (*its)->getName());
		sourceElem.setAttribute("playing", (*its)->isPlaying());
		sourceElem.setAttribute("modifiable", (*its)->isModifiable());

		QDomElement pos = doc.createElement("Position");
		pos.setAttribute("X", (*its)->getX());
		pos.setAttribute("Y", (*its)->getY());
		sourceElem.appendChild(pos);

		QDomElement rot = doc.createElement("Center");
		rot.setAttribute("X", (*its)->getCenterX());
		rot.setAttribute("Y", (*its)->getCenterY());
		sourceElem.appendChild(rot);

		QDomElement a = doc.createElement("Angle");
		a.setAttribute("A", (*its)->getRotationAngle());
		sourceElem.appendChild(a);

		QDomElement scale = doc.createElement("Scale");
		scale.setAttribute("X", (*its)->getScaleX());
		scale.setAttribute("Y", (*its)->getScaleY());
		sourceElem.appendChild(scale);

		QDomElement crop = doc.createElement("Crop");
		crop.setAttribute("X", (*its)->getTextureCoordinates().x());
		crop.setAttribute("Y", (*its)->getTextureCoordinates().y());
		crop.setAttribute("W", (*its)->getTextureCoordinates().width());
		crop.setAttribute("H", (*its)->getTextureCoordinates().height());
		sourceElem.appendChild(crop);

		QDomElement d = doc.createElement("Depth");
		d.setAttribute("Z", (*its)->getDepth());
		sourceElem.appendChild(d);

		QDomElement alpha = doc.createElement("Alpha");
		alpha.setAttribute("X", (*its)->getAlphaX());
		alpha.setAttribute("Y", (*its)->getAlphaY());
		sourceElem.appendChild(alpha);

		QDomElement color = doc.createElement("Color");
		color.setAttribute("R", (*its)->getColor().red());
		color.setAttribute("G", (*its)->getColor().green());
		color.setAttribute("B", (*its)->getColor().blue());
		sourceElem.appendChild(color);

		QDomElement blend = doc.createElement("Blending");
		blend.setAttribute("Function", (*its)->getBlendFuncDestination());
		blend.setAttribute("Equation", (*its)->getBlendEquation());
		blend.setAttribute("Mask", (*its)->getMask());
		sourceElem.appendChild(blend);

		QDomElement filter = doc.createElement("Filter");
		filter.setAttribute("Filtered", (*its)->isFiltered());
		filter.setAttribute("Pixelated", (*its)->isPixelated());
		filter.setAttribute("InvertMode", (*its)->getInvertMode());
		filter.setAttribute("Filter", (*its)->getFilter());
		sourceElem.appendChild(filter);

		QDomElement Coloring = doc.createElement("Coloring");
		Coloring.setAttribute("Brightness", (*its)->getBrightness());
		Coloring.setAttribute("Contrast", (*its)->getContrast());
		Coloring.setAttribute("Saturation", (*its)->getSaturation());
		Coloring.setAttribute("Hueshift", (*its)->getHueShift());
		Coloring.setAttribute("luminanceThreshold", (*its)->getLuminanceThreshold());
		Coloring.setAttribute("numberOfColors", (*its)->getNumberOfColors());
		sourceElem.appendChild(Coloring);

		QDomElement Chromakey = doc.createElement("Chromakey");
		Chromakey.setAttribute("on", (*its)->getChromaKey());
		Chromakey.setAttribute("R", (*its)->getChromaKeyColor().red());
		Chromakey.setAttribute("G", (*its)->getChromaKeyColor().green());
		Chromakey.setAttribute("B", (*its)->getChromaKeyColor().blue());
		Chromakey.setAttribute("Tolerance", (*its)->getChromaKeyTolerance());
		sourceElem.appendChild(Chromakey);

		QDomElement Gamma = doc.createElement("Gamma");
		Gamma.setAttribute("value", (*its)->getGamma());
		Gamma.setAttribute("minInput", (*its)->getGammaMinInput());
		Gamma.setAttribute("maxInput", (*its)->getGammaMaxInput());
		Gamma.setAttribute("minOutput", (*its)->getGammaMinOuput());
		Gamma.setAttribute("maxOutput", (*its)->getGammaMaxOutput());
		sourceElem.appendChild(Gamma);

		QDomElement specific = doc.createElement("TypeSpecific");
		specific.setAttribute("type", (*its)->rtti());

		if ((*its)->rtti() == Source::VIDEO_SOURCE) {
			VideoSource *vs = dynamic_cast<VideoSource *> (*its);
			VideoFile *vf = vs->getVideoFile();
			// Necessary information for re-creating this video source:
			// filename, marks, saturation
			QDomElement f = doc.createElement("Filename");
			f.setAttribute("PowerOfTwo", (int) vf->getPowerOfTwoConversion());
			f.setAttribute("IgnoreAlpha", (int) vf->ignoresAlphaChannel());
			f.setAttribute("Relative", current.relativeFilePath(vf->getFileName()) );
			QDomText filename = doc.createTextNode(vf->getFileName());
			f.appendChild(filename);
			specific.appendChild(f);

			QDomElement m = doc.createElement("Marks");
			m.setAttribute("In", (uint) vf->getMarkIn());
			m.setAttribute("Out", (uint) vf->getMarkOut());
			specific.appendChild(m);

			QDomElement p = doc.createElement("Play");
			p.setAttribute("Speed", vf->getPlaySpeed());
			p.setAttribute("Loop", vf->isLoop());
			specific.appendChild(p);

			QDomElement o = doc.createElement("Options");
			o.setAttribute("AllowDirtySeek", vf->getOptionAllowDirtySeek());
			o.setAttribute("RestartToMarkIn", vf->getOptionRestartToMarkIn());
			o.setAttribute("RevertToBlackWhenStop", vf->getOptionRevertToBlackWhenStop());
			specific.appendChild(o);
#ifdef OPEN_CV
		}
		else if ((*its)->rtti() == Source::CAMERA_SOURCE) {
			OpencvSource *cs = dynamic_cast<OpencvSource *> (*its);

			QDomElement f = doc.createElement("CameraIndex");
			QDomText id = doc.createTextNode(QString::number(cs->getOpencvCameraIndex()));
			f.appendChild(id);
			specific.appendChild(f);
#endif
		}
		else if ((*its)->rtti() == Source::ALGORITHM_SOURCE) {
			AlgorithmSource *as = dynamic_cast<AlgorithmSource *> (*its);

			QDomElement f = doc.createElement("Algorithm");
			QDomText algo = doc.createTextNode(QString::number(as->getAlgorithmType()));
			f.appendChild(algo);
			specific.appendChild(f);

			// get size
			QDomElement s = doc.createElement("Frame");
			s.setAttribute("Width", as->getFrameWidth());
			s.setAttribute("Height", as->getFrameHeight());
			specific.appendChild(s);

			QDomElement x = doc.createElement("Update");
			x.setAttribute("Periodicity", QString::number(as->getPeriodicity()) );
			x.setAttribute("Variability", as->getVariability());
			specific.appendChild(x);
		}
		else if ((*its)->rtti() == Source::CAPTURE_SOURCE) {
			CaptureSource *cs = dynamic_cast<CaptureSource *> (*its);

			QByteArray ba;
			QBuffer buffer(&ba);
			buffer.open(QIODevice::WriteOnly);
			cs->image().save(&buffer, "JPG");
			buffer.close();

			QDomElement f = doc.createElement("Image");
			QDomText img = doc.createTextNode( QString::fromLatin1(ba.constData(), ba.size()) );

			f.appendChild(img);
			specific.appendChild(f);
		}
		else if ((*its)->rtti() == Source::CLONE_SOURCE) {
			CloneSource *cs = dynamic_cast<CloneSource *> (*its);

			QDomElement f = doc.createElement("CloneOf");
			QDomText name = doc.createTextNode(cs->getOriginalName());
			f.appendChild(name);
			specific.appendChild(f);
		}
		else if ((*its)->rtti() == Source::SVG_SOURCE) {
			SvgSource *svgs = dynamic_cast<SvgSource *> (*its);
			QByteArray ba = svgs->getDescription();

			QDomElement f = doc.createElement("Svg");
			QDomText name = doc.createTextNode( QString::fromLatin1(ba.constData(), ba.size()) );
			f.appendChild(name);
			specific.appendChild(f);
		}
		sourceElem.appendChild(specific);

		config.appendChild(sourceElem);
	}

	return config;
}

void applySourceConfig(Source *newsource, QDomElement child) {

	QDomElement tmp;
	newsource->setName( child.attribute("name") );
	newsource->play( child.attribute("playing", "1").toInt() );
	newsource->setModifiable( child.attribute("modifiable", "1").toInt() );

	newsource->setX( child.firstChildElement("Position").attribute("X", "0").toDouble() );
	newsource->setY( child.firstChildElement("Position").attribute("Y", "0").toDouble() );
	newsource->setCenterX( child.firstChildElement("Center").attribute("X", "0").toDouble() );
	newsource->setCenterY( child.firstChildElement("Center").attribute("Y", "0").toDouble() );
	newsource->setRotationAngle( child.firstChildElement("Angle").attribute("A", "0").toDouble() );
	newsource->setScaleX( child.firstChildElement("Scale").attribute("X", "1").toDouble() );
	newsource->setScaleY( child.firstChildElement("Scale").attribute("Y", "1").toDouble() );

	tmp = child.firstChildElement("Alpha");
	newsource->setAlphaCoordinates( tmp.attribute("X", "0").toDouble(), tmp.attribute("Y", "0").toDouble() );

	tmp = child.firstChildElement("Color");
	newsource->setColor( QColor( tmp.attribute("R", "255").toInt(),tmp.attribute("G", "255").toInt(), tmp.attribute("B", "255").toInt() ) );

	tmp = child.firstChildElement("Crop");
	newsource->setTextureCoordinates( QRectF( tmp.attribute("X", "0").toDouble(), tmp.attribute("Y", "0").toDouble(),tmp.attribute("W", "1").toDouble(),tmp.attribute("H", "1").toDouble() ) );

	tmp = child.firstChildElement("Blending");
	newsource->setBlendEquation( (GLenum) tmp.attribute("Equation", "32774").toInt()  );
	newsource->setBlendFunc( GL_SRC_ALPHA, (GLenum) tmp.attribute("Function", "1").toInt() );
	newsource->setMask( (Source::maskType) tmp.attribute("Mask", "0").toInt() );

	tmp = child.firstChildElement("Filter");
	newsource->setFiltered( tmp.attribute("Filtered", "0").toInt() );
	newsource->setPixelated( tmp.attribute("Pixelated", "0").toInt() );
	newsource->setInvertMode( (Source::invertModeType) tmp.attribute("InvertMode", "0").toInt() );
	newsource->setFilter( (Source::filterType) tmp.attribute("Filter", "0").toInt() );

	tmp = child.firstChildElement("Coloring");
	newsource->setBrightness( tmp.attribute("Brightness", "0").toInt() );
	newsource->setContrast( tmp.attribute("Contrast", "0").toInt() );
	newsource->setSaturation( tmp.attribute("Saturation", "0").toInt() );
	newsource->setHueShift( tmp.attribute("Hueshift", "0").toInt() );
	newsource->setLuminanceThreshold( tmp.attribute("luminanceThreshold", "0").toInt() );
	newsource->setNumberOfColors( tmp.attribute("numberOfColors", "0").toInt() );

	tmp = child.firstChildElement("Chromakey");
	newsource->setChromaKey( tmp.attribute("on", "0").toInt() );
	newsource->setChromaKeyColor( QColor( tmp.attribute("R", "255").toInt(),tmp.attribute("G", "0").toInt(), tmp.attribute("B", "0").toInt() ) );
	newsource->setChromaKeyTolerance( tmp.attribute("Tolerance", "7").toInt() );

	tmp = child.firstChildElement("Gamma");
	newsource->setGamma( tmp.attribute("value", "1").toFloat(),
			tmp.attribute("minInput", "0").toFloat(),
			tmp.attribute("maxInput", "1").toFloat(),
			tmp.attribute("minOutput", "0").toFloat(),
			tmp.attribute("maxOutput", "1").toFloat());

}

int RenderingManager::addConfiguration(QDomElement xmlconfig, QDir current) {

	QList<QDomElement> clones;
    int errors = 0;

	int count = 0;
    QProgressDialog *progress = 0;
    if (_showProgressBar) {
    	progress = new QProgressDialog ("Loading sources...", "Abort", 1, xmlconfig.childNodes().count());
		progress->setWindowModality(Qt::NonModal);
		progress->setMinimumDuration( 500 );
    }

    // start loop of sources to create
	QDomElement child = xmlconfig.firstChildElement("Source");
	while (!child.isNull()) {

		if (progress) {
			progress->setValue(++count);

			if (progress->wasCanceled()) {
				delete progress;
				break;
			}
		}

		// pointer for new source
		Source *newsource = 0;
		// read the depth where the source should be created
		double depth = child.firstChildElement("Depth").attribute("Z", "0").toDouble();

		// get the type of the source to create
		QDomElement t = child.firstChildElement("TypeSpecific");
		Source::RTTI type = (Source::RTTI) t.attribute("type").toInt();

		// create the source according to its specific type information
		if (type == Source::VIDEO_SOURCE ){
			// read the tags specific for a video source
			QDomElement Filename = t.firstChildElement("Filename");
			QDomElement marks = t.firstChildElement("Marks");

			// create the video file
			VideoFile *newSourceVideoFile = NULL;
			if ( !Filename.attribute("PowerOfTwo","0").toInt() && (glSupportsExtension("GL_EXT_texture_non_power_of_two") || glSupportsExtension("GL_ARB_texture_non_power_of_two") ) )
				newSourceVideoFile = new VideoFile(this);
			else
				newSourceVideoFile = new VideoFile(this, true, SWS_FAST_BILINEAR);
			// if the video file was created successfully
			if (newSourceVideoFile){
				// first reads with the absolute file name
				QString fileNameToOpen = Filename.text();
				// if there is no such file, try generate a file name from the relative file name
				if (!QFileInfo(fileNameToOpen).exists())
					fileNameToOpen = current.absoluteFilePath( Filename.attribute("Relative", "") );
				// if there is such a file
				if (QFileInfo(fileNameToOpen).exists()) {
					// can we open this existing file ?
					if ( newSourceVideoFile->open( fileNameToOpen, marks.attribute("In").toUInt(), marks.attribute("Out").toUInt(), Filename.attribute("IgnoreAlpha").toInt() ) ) {
						// create the source as it is a valid video file (this also set it to be the current source)
						newsource = RenderingManager::getInstance()->newMediaSource(newSourceVideoFile, depth);
						if (newsource){
							// all is good ! we can apply specific parameters to the video file
							QDomElement play = t.firstChildElement("Play");
							newSourceVideoFile->setPlaySpeed(play.attribute("Speed","3").toInt());
							newSourceVideoFile->setLoop(play.attribute("Loop","1").toInt());
							QDomElement options = t.firstChildElement("Options");
							newSourceVideoFile->setOptionAllowDirtySeek(options.attribute("AllowDirtySeek","0").toInt());
							newSourceVideoFile->setOptionRestartToMarkIn(options.attribute("RestartToMarkIn","0").toInt());
							newSourceVideoFile->setOptionRevertToBlackWhenStop(options.attribute("RevertToBlackWhenStop","0").toInt());

							qDebug() << child.attribute("name") << tr("|Media source created with ") << QFileInfo(fileNameToOpen).fileName();
						}
						else {
							qWarning() << child.attribute("name") << tr("|Could not be created.");
							errors++;
						}
					}
					else {
						qWarning() << child.attribute("name") << tr("|Could not load ")<< fileNameToOpen;
						errors++;
					}
				}
				else {
					qWarning() << child.attribute("name") << tr("|No file named %1 or %2.").arg(Filename.text()).arg(fileNameToOpen);
					errors++;
				}

				// if one of the above failed, remove the video file object from memory
				if (!newsource)
					delete newSourceVideoFile;

			}
			else {
				qWarning() << child.attribute("name") << tr("|Could not allocate memory.");
				errors++;
			}

#ifdef OPEN_CV
		} else if ( type == Source::CAMERA_SOURCE ) {
			QDomElement camera = t.firstChildElement("CameraIndex");

			newsource = RenderingManager::getInstance()->newOpencvSource( camera.text().toInt(), depth);
			if (!newsource) {
				qWarning() << child.attribute("name") <<  tr("|Could not open OpenCV device index %2.").arg(camera.text());
				errors ++;
			} else
				qDebug() << child.attribute("name") <<  tr("|OpenCV source created (device index %2).").arg(camera.text());
#endif
		}
		else if ( type == Source::ALGORITHM_SOURCE) {
			// read the tags specific for an algorithm source
			QDomElement Algorithm = t.firstChildElement("Algorithm");
			QDomElement Frame = t.firstChildElement("Frame");
			QDomElement Update = t.firstChildElement("Update");

			newsource = RenderingManager::getInstance()->newAlgorithmSource(Algorithm.text().toInt(),
					Frame.attribute("Width").toInt(), Frame.attribute("Height").toInt(),
					Update.attribute("Variability").toDouble(), Update.attribute("Periodicity").toInt(), depth);
			if (!newsource) {
				qWarning() << child.attribute("name") << tr("|Could not create algorithm source.");
		        errors++;
			} else
				qDebug() << child.attribute("name") << tr("|Algorithm source created (")<< AlgorithmSource::getAlgorithmDescription(Algorithm.text().toInt()) << ").";
		}
		else if ( type == Source::RENDERING_SOURCE) {
			// no tags specific for a rendering source
			newsource = RenderingManager::getInstance()->newRenderingSource(depth);
			if (!newsource) {
				qWarning() << child.attribute("name") << tr("|Could not create rendering loop-back source.");
				errors++;
			} else
				qDebug() << child.attribute("name") << tr("|Rendering loop-back source created.");
		}
		else if ( type == Source::CAPTURE_SOURCE) {

			QDomElement img = t.firstChildElement("Image");
			QImage image;
			QByteArray data =  img.text().toLatin1();

			if ( image.loadFromData( reinterpret_cast<const uchar *>(data.data()), data.size(), "JPG") )
				newsource = RenderingManager::getInstance()->newCaptureSource(image, depth);

			if (!newsource) {
				qWarning() << child.attribute("name") << tr("|Could not create capture source.");
		        errors++;
			} else
				qDebug() << child.attribute("name") << tr("|Capture source created.");
		}
		else if ( type == Source::SVG_SOURCE) {

			QDomElement svg = t.firstChildElement("Svg");
			QByteArray data =  svg.text().toLatin1();

			QSvgRenderer *rendersvg = new QSvgRenderer(data);
			if ( rendersvg )
				newsource = RenderingManager::getInstance()->newSvgSource(rendersvg, depth);

			if (!newsource) {
				qWarning() << child.attribute("name") << tr("|Could not create vector graphics source.");
		        errors++;
			} else
				qDebug() << child.attribute("name") << tr("|Vector graphics source created.");
		}
		else if ( type == Source::CLONE_SOURCE) {
			// remember the node of the sources to clone
			clones.push_back(child);
		}

		if (newsource) {
			// insert the source in the scene
			if ( insertSource(newsource) )
				// Apply parameters to the created source
				applySourceConfig(newsource, child);
			else
				delete newsource;
		}

		child = child.nextSiblingElement();
	}
	// end loop on sources to create
	if (progress) progress->setValue(xmlconfig.childNodes().count());

	// Process the list of clones names ; now that every source exist, we can be sure they can be cloned
    QListIterator<QDomElement> it(clones);
    while (it.hasNext()) {

		Source *clonesource = 0;

    	QDomElement c = it.next();
    	double depth = c.firstChildElement("Depth").attribute("Z").toDouble();
    	QDomElement t = c.firstChildElement("TypeSpecific");
		QDomElement f = t.firstChildElement("CloneOf");
		// find the source which name is f.text()
    	SourceSet::iterator cloneof =  getByName(f.text());
    	if (isValid(cloneof)) {
    		clonesource = RenderingManager::getInstance()->newCloneSource(cloneof, depth);
    		if (clonesource) {
    			// Apply parameters to the created source
    			applySourceConfig(clonesource, c);
    			// insert the source in the scene
    			if ( !insertSource(clonesource) )
    				delete clonesource;
    			else
        			qDebug() << c.attribute("name") << tr("|Clone of source %1 created.").arg(f.text());
    		}else {
    			qWarning() << c.attribute("name") << tr("|Could not create clone source.");
    	        errors++;
    		}
    	} else {
    		qWarning() << c.attribute("name") << tr("|Cannot clone %2 ; no such source.").arg(f.text());
    		errors++;
    	}
    }

	// set current source to none (end of list)
	setCurrentSource( getEnd() );
	if (progress) delete progress;

	return errors;
}

void RenderingManager::setGammaShift(float g)
{
	gammaShift = g;
}

float RenderingManager::getGammaShift() const
{
	return gammaShift;
}

standardAspectRatio doubleToAspectRatio(double ar)
{
	if ( ABS(ar - (4.0 / 3.0) ) < EPSILON )
		return ASPECT_RATIO_4_3;
	else if ( ABS(ar - (16.0 / 9.0) ) < EPSILON )
		return ASPECT_RATIO_16_9;
	else  if ( ABS(ar - (3.0 / 2.0) ) < EPSILON )
		return ASPECT_RATIO_3_2;
	else if ( ABS(ar - (16.0 / 10.0) ) < EPSILON )
		return ASPECT_RATIO_16_10;
	else
		return ASPECT_RATIO_FREE;
}

void RenderingManager::pause(bool on){

	static std::map<Source *, bool> sourcePlayStatus;
	// setup status
	paused = on;

	// for every source in the manager, start/stop it
	for (SourceSet::iterator its = _sources.begin(); its != _sources.end(); its++) {
		// exception for video source which are paused
		if ( (*its)->rtti() == Source::VIDEO_SOURCE ) {
			VideoSource *s = dynamic_cast<VideoSource *>(*its);
			if (on) {
				sourcePlayStatus[s] = s->isPaused();
				s->pause(true);
			} else
				s->pause(sourcePlayStatus[s]);
		} else {
			if (on) {
				sourcePlayStatus[*its] = (*its)->isPlaying();
				(*its)->play(!on);
			} else
				(*its)->play(sourcePlayStatus[*its]);
		}
	}

	if (!on)
		sourcePlayStatus.clear();

	qDebug() << "RenderingManager|" << (on ? tr("All sources paused.") : tr("All source un-paused.") );
}



