/*
 * MixRenderWidget.cpp
 *
 *  Created on: 3 nov. 2009
 *      Author: herbelin
 */

#include "MainRenderWidget.moc"

#include "OpencvSource.h"
#include <algorithm>


// static members
MainRenderWidget *MainRenderWidget::_instance = 0;


const QGLWidget *MainRenderWidget::getQGLWidget() {

    return (QGLWidget *) getInstance();
}

MainRenderWidget *MainRenderWidget::getInstance() {

    if (_instance == 0) {
        _instance = new MainRenderWidget;
    }

    return _instance;
}

void MainRenderWidget::deleteInstance(){

    if (_instance != 0)
        delete _instance;
}

MainRenderWidget::MainRenderWidget(QWidget *parent): glRenderWidget(parent), _fbo(NULL), _useAspectRatio(true) {

	if (!QGLFramebufferObject::hasOpenGLFramebufferObjects ())
	  qWarning("Frame Buffer Objects not supported on this graphics hardware");

	setRenderingResolution(DEFAULT_WIDTH, DEFAULT_HEIGHT);

	this->startTimer(16);

	currentSource = getEnd();
}

void MainRenderWidget::setRenderingResolution(int w, int h) {

	_aspectRatio = (float) w / (float) h;

    makeCurrent();

    if (_fbo)
    	delete _fbo;

    _fbo = new QGLFramebufferObject(w, h);

}

MainRenderWidget::~MainRenderWidget() {

	clearSourceSet();
}

void MainRenderWidget::paintGL()
{
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


	for(SourceSet::iterator its = _sources.begin(); its != _sources.end(); its++) {
		// steps 1 and 2 :
		(*its)->update();

		// step 3


		// step 4
        glPushMatrix();
        if (_useAspectRatio) {
			float windowaspectratio = (float) width() / (float) height();
			if ( windowaspectratio < _aspectRatio)
				glScalef( 1.f, windowaspectratio / _aspectRatio, 1.f);
			else
				glScalef( _aspectRatio / windowaspectratio,  1.f, 1.f);
        }


        glTranslated((*its)->getX(), (*its)->getY(), 0.0);
		glScaled( (*its)->getScaleX(), (*its)->getScaleY(), 1.f);
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
}

void MainRenderWidget::addSource(VideoFile *vf) {

	// create a source appropriate for this videofile
	VideoSource *s = new VideoSource(vf, (QGLWidget *) this);
	// set the last created source to be current
	setCurrentSource( _sources.insert( (Source *) s) );
//	_sources.insert( (Source *) s);
}


#ifdef OPEN_CV
void MainRenderWidget::addSource(int opencvIndex) {

	OpencvSource *s = new OpencvSource(opencvIndex, (QGLWidget *) this);

	// set the last created source to be current
	setCurrentSource( _sources.insert( (Source *) s) );

}
#endif

void MainRenderWidget::removeSource(SourceSet::iterator itsource){

    if (itsource != _sources.end()) {
        delete (*itsource);
        _sources.erase(itsource);
    }

}

void MainRenderWidget::clearSourceSet() {
    // TODO does it work?
	for(SourceSet::iterator its = _sources.begin(); its != _sources.end(); its++)
		removeSource(its);
}


bool MainRenderWidget::notAtEnd(SourceSet::iterator itsource){
    return (itsource != _sources.end());
}


bool MainRenderWidget::isValid(SourceSet::iterator itsource){

	if (notAtEnd(itsource))
		return ( _sources.find(*itsource) != _sources.end() );
	else
		return false;
}

void MainRenderWidget::setCurrentSource(SourceSet::iterator  si){

	if (si != currentSource) {
		if ( notAtEnd(currentSource) )
			 (*currentSource)->activate(false);

		currentSource = si;
		emit currentSourceChanged(currentSource);

		if ( notAtEnd(currentSource) )
			(*currentSource)->activate(true);
	}
}

SourceSet::iterator  MainRenderWidget::changeDepth(SourceSet::iterator itsource, double newdepth){
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

    return std::find_if( _sources.begin(), _sources.end(), hasName(name) );
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

