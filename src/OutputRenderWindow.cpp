/*
 * OutputRenderWindow.cpp
 *
 *  Created on: Feb 10, 2010
 *      Author: bh
 */

#include "OutputRenderWindow.moc"
#include "RenderingManager.h"

#include <QGLFramebufferObject>


OutputRenderWindow::OutputRenderWindow() : glRenderWidget(0, RenderingManager::getQGLWidget(), Qt::Window | Qt::CustomizeWindowHint | Qt::WindowTitleHint | Qt::WindowMinimizeButtonHint), 
	_aspectRatio(1.0), _useAspectRatio(true) {

	if (!QGLFramebufferObject::hasOpenGLFramebufferObjects())
		qCritical("Frame Buffer Objects not supported on this graphics hardware; ");

}



OutputRenderWindow::~OutputRenderWindow() {

}


OutputRenderWindow *OutputRenderWindow::_instance = 0;

OutputRenderWindow *OutputRenderWindow::getInstance() {

	if (_instance == 0) {
		_instance = new OutputRenderWindow;
	}

	return _instance;
}

void OutputRenderWindow::initializeGL()
{
    // Enables smooth color shading
    glShadeModel(GL_FLAT);

    // disable depth and lighting by default
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_LIGHTING);

    // Enables texturing
    glEnable(GL_TEXTURE_2D);
    // Pure texture color (no lighting)
    glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);

    // Turn blending off
    glDisable(GL_BLEND);

    // setup default background color to black
    glClearColor(0.0, 0.0, 0.0, 1.0f);
	
}

void OutputRenderWindow::resizeGL(int w, int h)
{
    glViewport(0, 0, w, h);
	_aspectRatio = (float) w / (float) h;

    // Setup specific projection and view for this window
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluOrtho2D(-1.0, 1.0, 1.0, -1.0);

    glMatrixMode(GL_MODELVIEW);

	update();
}


void OutputRenderWindow::paintGL() {

	glRenderWidget::paintGL();
	
#if QT_VERSION >= 0x040600
	if (QGLFramebufferObject::hasOpenGLFramebufferBlit () )
	// use the accelerated GL_EXT_framebuffer_blit if available
	{
		if (_useAspectRatio) {
			float renderingAspectRatio = RenderingManager::getInstance()->getFrameBufferAspectRatio();
			if (_aspectRatio < renderingAspectRatio)
				QGLFramebufferObject::blitFramebuffer ( 0, QRect( QPoint( 0, (height() - (int) ( (float) width() / renderingAspectRatio)) / 2 ), QSize(width(), (int) ( (float) width() / renderingAspectRatio))), 
				RenderingManager::getInstance()->_fbo, QRect(QPoint(0,0), RenderingManager::getInstance()->_fbo->size() ) ) ;
	
			else
				QGLFramebufferObject::blitFramebuffer ( 0, QRect(QPoint( (width() - (int) ( (float) height() * renderingAspectRatio )) /2,0), QSize((int) ( (float) height() * renderingAspectRatio), height())), 
				RenderingManager::getInstance()->_fbo, QRect(QPoint(0,0), RenderingManager::getInstance()->_fbo->size() ) ) ;
		}
		else
			QGLFramebufferObject::blitFramebuffer ( 0, QRect(QPoint(0,0), size()), RenderingManager::getInstance()->_fbo, QRect(QPoint(0,0), RenderingManager::getInstance()->_fbo->size() ) ) ;

	}
	else 
#endif
	// 	Draw quad with fbo texture in a more basic OpenGL way
	{

		if (_useAspectRatio) {
			float renderingAspectRatio = RenderingManager::getInstance()->getFrameBufferAspectRatio();
			if (_aspectRatio < renderingAspectRatio)
				glScalef(1.f, _aspectRatio / renderingAspectRatio, 1.f);
			else
				glScalef(renderingAspectRatio / _aspectRatio, 1.f, 1.f);
		}

		glBindTexture(GL_TEXTURE_2D, RenderingManager::getInstance()->getFrameBufferTexture());
		glCallList(RenderingManager::quad_texured);
	}


    displayFPS();
}



void OutputRenderWindow::setFullScreen(bool on) {

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

void OutputRenderWindow::mouseDoubleClickEvent(QMouseEvent * event) {

	// switch fullscreen / window
	if (windowFlags() & Qt::Window) {
		setWindowState(windowState() ^ Qt::WindowFullScreen);
		update();
	}
}

void OutputRenderWindow::keyPressEvent(QKeyEvent * event) {

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

void OutputRenderWindow::closeEvent(QCloseEvent * event) {

	emit windowClosed();
	event->accept();

}


void OutputRenderWindow::useRenderingAspectRatio(bool on) {

	_useAspectRatio = on;

}

float OutputRenderWindow::getAspectRatio(){

	if (_useAspectRatio)
		return RenderingManager::getInstance()->getFrameBufferAspectRatio();
	else
		return _aspectRatio;
}
