/*
 * OutputRenderWindow.cpp
 *
 *  Created on: Feb 10, 2010
 *      Author: bh
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

#include "common.h"
#include "OutputRenderWindow.moc"
#include "RenderingManager.h"
#include "ViewRenderWidget.h"

#include <QGLFramebufferObject>


OutputRenderWindow *OutputRenderWindow::_instance = 0;

OutputRenderWidget::OutputRenderWidget(QWidget *parent, const QGLWidget * shareWidget, Qt::WindowFlags f) : glRenderWidget(parent, shareWidget, f),
		useAspectRatio(true), useWindowAspectRatio(true) {

	setCursor(Qt::BlankCursor);
	rx = 0;
	ry = 0;
	rw = width();
	rh = height();
}


float OutputRenderWidget::getAspectRatio() const{

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
	glActiveTexture(GL_TEXTURE0);
    glEnable(GL_TEXTURE_2D);

    // This hint can improve the speed of texturing when perspective-correct texture coordinate interpolation isn't needed
    glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_FASTEST);
    // Pure texture color (no lighting)
    glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);

    // Turn blending off
    glDisable(GL_BLEND);

	setBackgroundColor(palette().color(QPalette::Window));
}

void OutputRenderWidget::resizeGL(int w, int h)
{
	if (w == 0 || h == 0) {
		w = width();
		h = height();
	}

	// generic widget resize (also computes aspectRatio)
	glRenderWidget::resizeGL(w, h);

	if ( RenderingManager::blit_fbo_extension ) {
		// respect the aspect ratio of the rendering manager
		if ( useAspectRatio ) {
			float renderingAspectRatio = RenderingManager::getInstance()->getFrameBufferAspectRatio();
			if (aspectRatio < renderingAspectRatio) {
				int nh = (int)( float(w) / renderingAspectRatio);
				rx = 0;
				ry = (h - nh) / 2;
				rw = w;
				rh = (h + nh) / 2;

			} else {
				int nw = (int)( float(h) * renderingAspectRatio );
				rx = (w - nw) / 2;
				ry = 0;
				rw = (w + nw) / 2;
				rh = h;
			}
		}
		// the option 'free aspect ratio' is on ; use the window dimensions
		// (only valid for widget, not window)
		else if ( useWindowAspectRatio ) {
			float windowAspectRatio = OutputRenderWindow::getInstance()->aspectRatio;
			if ( aspectRatio < windowAspectRatio) {
				int nh = (int)( float(w) / windowAspectRatio);
				rx = 0;
				ry = (h - nh) / 2;
				rw = w;
				rh = (h + nh) / 2;

			} else {
				int nw = (int)( float(h) * windowAspectRatio );
				rx = (w - nw) / 2;
				ry = 0;
				rw = (w + nw) / 2;
				rh = h;
			}
		} else {
			rx = 0;
			ry = 0;
			rw = w;
			rh = h;
		}
	}
	else
	{
		glLoadIdentity();

		if (useAspectRatio) {
			float renderingAspectRatio = RenderingManager::getInstance()->getFrameBufferAspectRatio();
			if (aspectRatio < renderingAspectRatio)
				glScalef(1.f, -aspectRatio / renderingAspectRatio, 1.f);
			else
				glScalef(renderingAspectRatio / aspectRatio, -1.f, 1.f);
		} else if (useWindowAspectRatio) { // (only valid for widget, not window)
			float windowAspectRatio = OutputRenderWindow::getInstance()->aspectRatio;
			if (aspectRatio < windowAspectRatio)
				glScalef(1.f, -aspectRatio / windowAspectRatio, 1.f);
			else
				glScalef(windowAspectRatio / aspectRatio, -1.f, 1.f);
		} else {
			glScalef(1.f, -1.0, 1.f);
		}
	}
}


void OutputRenderWindow::resizeGL(int w, int h)
{
	OutputRenderWidget::resizeGL(w, h);
	emit resized();
}

void OutputRenderWidget::useFreeAspectRatio(bool on)
{
	useAspectRatio = !on;
	refresh();
}

void OutputRenderWidget::refresh()
{
	makeCurrent();
	resizeGL();
	paintGL();
}


void OutputRenderWidget::paintGL()
{
	glRenderWidget::paintGL();

	if ( RenderingManager::blit_fbo_extension )
	// use the accelerated GL_EXT_framebuffer_blit if available
	{
		// select FIRST texture attachment as source
	    glBindFramebuffer(GL_READ_FRAMEBUFFER, RenderingManager::getInstance()->getFrameBufferHandle());
		glReadBuffer(GL_COLOR_ATTACHMENT0);

		glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
		glBlitFramebuffer(0, 0, RenderingManager::getInstance()->getFrameBufferWidth(), RenderingManager::getInstance()->getFrameBufferHeight(),
									 rx, ry, rw, rh,
									 GL_COLOR_BUFFER_BIT, GL_NEAREST);
	} else
	// 	Draw quad with fbo texture in a more basic OpenGL way
	{
		glBindTexture(GL_TEXTURE_2D, RenderingManager::getInstance()->getFrameBufferTexture());
		glCallList(ViewRenderWidget::quad_texured);
	}
}



OutputRenderWindow::OutputRenderWindow() : OutputRenderWidget(0, (QGLWidget *)RenderingManager::getRenderingWidget(), Qt::Window | Qt::CustomizeWindowHint | Qt::WindowTitleHint | Qt::WindowMinimizeButtonHint) {

	useWindowAspectRatio = false;
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
//
//void OutputRenderWindow::closeEvent(QCloseEvent * event) {
//
//	emit windowClosed();
//	event->accept();
//
//}

