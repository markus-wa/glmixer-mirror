/*
 * MixRenderWidget.cpp
 *
 *  Created on: 3 nov. 2009
 *      Author: herbelin
 */

#include "MainRenderWidget.moc"


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

MainRenderWidget::MainRenderWidget(QWidget *parent): glRenderWidget(parent), _s(NULL),
		_aspectRatio(DEFAULT_ASPECT_RATIO), _useAspectRatio(true) {


    makeCurrent();
    fbo = new QGLFramebufferObject(512, 512);

	this->startTimer(20);

}

MainRenderWidget::~MainRenderWidget() {
	if (_s)
		delete _s;
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


	if (_s) {
		// steps 1 and 2 :
		_s->renderTexture();

		// step 3
        fbo->bind();
        glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);
        glBindTexture(GL_TEXTURE_2D, _s->textureIndex);
		glCallList(VideoSource::squareDisplayList);
        fbo->release();

		// step 4
        glPushMatrix();
        if (_useAspectRatio) {
			float windowaspectratio = (float) width() / (float) height();
			if ( windowaspectratio < _aspectRatio)
				glScalef( 1.f, windowaspectratio / _aspectRatio, 1.f);
			else
				glScalef( _aspectRatio / windowaspectratio,  1.f, 1.f);
        }
		glCallList(VideoSource::squareDisplayList);
		glPopMatrix();

	}
}

void MainRenderWidget::createSource(VideoFile *vf) {


//	makeCurrent();
	_s = new VideoSource(vf, (QGLWidget *) this);

}
