/*
 * OutputRenderWindow.cpp
 *
 *  Created on: Feb 10, 2010
 *      Author: bh
 */

#include "common.h"
#include "OutputRenderWindow.moc"
#include "RenderingManager.h"

#include <QGLFramebufferObject>


OutputRenderWindow *OutputRenderWindow::_instance = 0;

OutputRenderWidget::OutputRenderWidget(QWidget *parent, const QGLWidget * shareWidget, Qt::WindowFlags f) : glRenderWidget(parent, shareWidget, f),
		useAspectRatio(true){

}


float OutputRenderWidget::getAspectRatio(){

	if (useAspectRatio)
		return RenderingManager::getInstance()->getFrameBufferAspectRatio();
	else
		return aspectRatio;
}

void OutputRenderWidget::initializeGL() {

    // Enables smooth color shading
    glShadeModel(GL_FLAT);

    // disable depth and lighting by default
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_LIGHTING);

    // Enables texturing
    glEnable(GL_TEXTURE_2D);
    // This hint can improve the speed of texturing when perspective- correct texture coordinate interpolation isn't needed
    glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_FASTEST);
    // Pure texture color (no lighting)
    glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);

    // Turn blending off
    glDisable(GL_BLEND);

	setBackgroundColor(palette().color(QPalette::Window));
}


void OutputRenderWidget::paintGL()
{
	glRenderWidget::paintGL();

	if ( RenderingManager::blit )
	// use the accelerated GL_EXT_framebuffer_blit if available
	{
	    glBindFramebufferEXT(GL_READ_FRAMEBUFFER, RenderingManager::getInstance()->getFrameBufferHandle());
		glBindFramebufferEXT(GL_DRAW_FRAMEBUFFER, 0);

		if (useAspectRatio) {
			float renderingAspectRatio = RenderingManager::getInstance()->getFrameBufferAspectRatio();
			if (aspectRatio < renderingAspectRatio) {
				int h = int( (float) width() / renderingAspectRatio);
				glBlitFramebufferEXT(0, 0, RenderingManager::getInstance()->getFrameBufferWidth(), RenderingManager::getInstance()->getFrameBufferHeight(),
			                         0, (height() - h) / 2, width(),  (height() + h) / 2,
			                             GL_COLOR_BUFFER_BIT, GL_NEAREST);
			} else {
				int w = int( (float) height() * renderingAspectRatio );
			    glBlitFramebufferEXT(0, 0, RenderingManager::getInstance()->getFrameBufferWidth(), RenderingManager::getInstance()->getFrameBufferHeight(),
			    		             (width() - w) / 2, 0, (width() + w) / 2, height(),
			                         GL_COLOR_BUFFER_BIT, GL_NEAREST);
			}
		}
		else
			glBlitFramebufferEXT(0, 0, RenderingManager::getInstance()->getFrameBufferWidth(), RenderingManager::getInstance()->getFrameBufferHeight(),
			                             0, 0, width(), height(),
			                             GL_COLOR_BUFFER_BIT, GL_NEAREST);
	}
	else
	// 	Draw quad with fbo texture in a more basic OpenGL way
	{
		if (useAspectRatio) {
			float renderingAspectRatio = RenderingManager::getInstance()->getFrameBufferAspectRatio();
			if (aspectRatio < renderingAspectRatio)
				glScalef(1.f, aspectRatio / renderingAspectRatio, 1.f);
			else
				glScalef(renderingAspectRatio / aspectRatio, 1.f, 1.f);
		}

		glBindTexture(GL_TEXTURE_2D, RenderingManager::getInstance()->getFrameBufferTexture());
		glCallList(ViewRenderWidget::quad_texured);
	}
}



OutputRenderWindow::OutputRenderWindow() : OutputRenderWidget(0, (QGLWidget *)RenderingManager::getRenderingWidget(), Qt::Window | Qt::CustomizeWindowHint | Qt::WindowTitleHint | Qt::WindowMinimizeButtonHint) {


}

OutputRenderWindow *OutputRenderWindow::getInstance() {

	if (_instance == 0) {
		_instance = new OutputRenderWindow;
	    Q_CHECK_PTR(_instance);
	}

	return _instance;
}

void OutputRenderWindow::initializeGL()
{
	OutputRenderWidget::initializeGL();

    // setup default background color to black
    glClearColor(0.0, 0.0, 0.0, 1.0f);
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

	useAspectRatio = on;
}

// #if QT_VERSION >= 0x040600
	// if (QGLFramebufferObject::hasOpenGLFramebufferBlit () )
	// use the accelerated GL_EXT_framebuffer_blit if available
	// {
		// if (_useAspectRatio) {
		
			// glClear(GL_COLOR_BUFFER_BIT);
			// float renderingAspectRatio = RenderingManager::getInstance()->getFrameBufferAspectRatio();
			// if (_aspectRatio < renderingAspectRatio)
				// QGLFramebufferObject::blitFramebuffer ( 0, QRect( QPoint( 0, (height() - (int) ( (float) width() / renderingAspectRatio)) / 2 ), QSize(width(), (int) ( (float) width() / renderingAspectRatio))), 
				// RenderingManager::getInstance()->_fbo, QRect(QPoint(0,0), RenderingManager::getInstance()->_fbo->size() ) ) ;
	
			// else
				// QGLFramebufferObject::blitFramebuffer ( 0, QRect(QPoint( (width() - (int) ( (float) height() * renderingAspectRatio )) /2,0), QSize((int) ( (float) height() * renderingAspectRatio), height())), 
				// RenderingManager::getInstance()->_fbo, QRect(QPoint(0,0), RenderingManager::getInstance()->_fbo->size() ) ) ;
		// }
		// else
			// QGLFramebufferObject::blitFramebuffer ( 0, QRect(QPoint(0,0), size()), RenderingManager::getInstance()->_fbo, QRect(QPoint(0,0), RenderingManager::getInstance()->_fbo->size() ) ) ;

	// }
	// else 
// #endif
		// Draw quad with fbo texture in a more basic OpenGL way
	// {
		// glRenderWidget::paintGL();

		// if (_useAspectRatio) {
			// float renderingAspectRatio = RenderingManager::getInstance()->getFrameBufferAspectRatio();
			// if (_aspectRatio < renderingAspectRatio)
				// glScalef(1.f, _aspectRatio / renderingAspectRatio, 1.f);
			// else
				// glScalef(renderingAspectRatio / _aspectRatio, 1.f, 1.f);
		// }

		// glBindTexture(GL_TEXTURE_2D, RenderingManager::getInstance()->getFrameBufferTexture());
		// glCallList(RenderingManager::quad_texured);
	// }

