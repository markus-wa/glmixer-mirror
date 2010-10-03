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
#include "VideoSource.h"
#include "AlgorithmSource.h"
#include "CaptureSource.h"

#include <QGLFramebufferObject>


OutputRenderWindow *OutputRenderWindow::_instance = 0;

OutputRenderWidget::OutputRenderWidget(QWidget *parent, const QGLWidget * shareWidget, Qt::WindowFlags f) : glRenderWidget(parent, shareWidget, f),
		useAspectRatio(true), useWindowAspectRatio(true), preferedDuration(1000), currentAlpha(1.0), backgroundSource(0),
		transition_type(TRANSITION_NONE), customTransitionColor(QColor()), customTransitionVideoSource(0) {

	rx = 0;
	ry = 0;
	rw = width();
	rh = height();

	animationAlpha = new QPropertyAnimation(this, "alpha");
	animationAlpha->setDuration(0);
	animationAlpha->setEasingCurve(QEasingCurve::InOutQuad);
    QObject::connect(animationAlpha, SIGNAL(finished()), this, SIGNAL(animationFinished() ) );

}

void OutputRenderWidget::setTransitionCurve(int curveType)
{
	animationAlpha->setEasingCurve( (QEasingCurve::Type) qBound( (int) QEasingCurve::Linear, curveType, (int) QEasingCurve::OutInBounce));
}

int OutputRenderWidget::transitionCurve() const
{
	return (int) animationAlpha->easingCurve().type();
}

void OutputRenderWidget::setTransitionDuration(int duration)
{
	preferedDuration = duration;

}

int OutputRenderWidget::transitionDuration() const
{
	return preferedDuration;
}

float OutputRenderWidget::getAspectRatio() const{

	if (useAspectRatio)
		return RenderingManager::getInstance()->getFrameBufferAspectRatio();
	else
		return aspectRatio;
}

void OutputRenderWidget::initializeGL() {

	glRenderWidget::initializeGL();

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
//	else
	// do this computation always ; rendering with polygon may be used during transition
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

	update();
}


void OutputRenderWidget::paintGL()
{
	glRenderWidget::paintGL();

	if ( currentAlpha > 1.0 && RenderingManager::blit_fbo_extension )
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
		if ( backgroundSource != NULL ) {
			backgroundSource->update();
			glPushMatrix();
			glScalef(1.0, -1.0, 1.0);
			QColor texcolor = backgroundSource->getColor();
			glColor4f(texcolor.redF(), texcolor.greenF(), texcolor.blueF(), 1.0 - currentAlpha);
			glCallList(ViewRenderWidget::quad_texured);
			glPopMatrix();
		}

		// apply the texture of the frame buffer
		glBindTexture(GL_TEXTURE_2D, RenderingManager::getInstance()->getFrameBufferTexture());

		// apply clutch transparency
		glColor4f(1.0, 1.0, 1.0, currentAlpha);

		// draw the polygon with texture
		glCallList(ViewRenderWidget::quad_texured);

	}
}

void OutputRenderWidget::setTransitionSource(Source *s)
{
	if (backgroundSource) {
		if (backgroundSource == (Source*) customTransitionVideoSource)
			customTransitionVideoSource->play(false);
		else
			delete backgroundSource;
	}

	backgroundSource = s;
}


void OutputRenderWidget::setTransitionMedia(QString filename)
{
	if (customTransitionVideoSource)
		delete customTransitionVideoSource;

	customTransitionVideoSource = NULL;

    VideoFile *newSourceVideoFile = NULL;
	if ( glSupportsExtension("GL_EXT_texture_non_power_of_two") || glSupportsExtension("GL_ARB_texture_non_power_of_two")  )
		newSourceVideoFile = new VideoFile(this);
	else
		newSourceVideoFile = new VideoFile(this, true, SWS_POINT);

    Q_CHECK_PTR(newSourceVideoFile);

	if ( newSourceVideoFile->open( filename ) ) {
		newSourceVideoFile->setOptionRestartToMarkIn(true);
		// create new video source
		GLuint textureIndex;
		makeCurrent();
		glGenTextures(1, &textureIndex);
		customTransitionVideoSource = new VideoSource(newSourceVideoFile, textureIndex, 0.0);
	} else {
		qCritical( qPrintable( tr("The file %1 could not be loaded.").arg(filename)) );
		delete newSourceVideoFile;
	}

}


QString OutputRenderWidget::transitionMedia() const
{
	if (customTransitionVideoSource )
		return customTransitionVideoSource->getVideoFile()->getFileName();

	return QString();
}

void OutputRenderWidget::smoothAlphaTransition(bool visible, transitionType transition){

	if (animationAlpha->state() == QAbstractAnimation::Running )
		animationAlpha->stop();

	QObject::disconnect(animationAlpha, SIGNAL(finished()), this, SLOT(setTransitionSource()) );

	switch (transition) {
		case TRANSITION_BACKGROUND:
			setTransitionSource(NULL);
			animationAlpha->setDuration(preferedDuration);
			break;
		case TRANSITION_LAST_FRAME:
			// special case ; don't behave identically for fade in than out
			if (visible) {
				QObject::connect(animationAlpha, SIGNAL(finished()), this, SLOT(setTransitionSource()) );
				animationAlpha->setDuration(preferedDuration);
			} else {
				// capture screen and use it immediately as transition source
				QImage capture = RenderingManager::getInstance()->captureFrameBuffer();
				capture = capture.convertToFormat(QImage::Format_RGB32);
				GLuint textureIndex;
				makeCurrent();
				glGenTextures(1, &textureIndex);
				setTransitionSource( (Source *) new CaptureSource(capture, textureIndex, 0.0));
				animationAlpha->setDuration(0);
			}
			break;
		case TRANSITION_CUSTOM_COLOR:
			if (visible) {
				QObject::connect(animationAlpha, SIGNAL(finished()), this, SLOT(setTransitionSource()) );
			} else {
				GLuint textureIndex;
				makeCurrent();
				glGenTextures(1, &textureIndex);
				Source *s = (Source *) new AlgorithmSource(0, textureIndex, 0.0, 4, 4, 0, 0);
				s->setColor(customTransitionColor);
				setTransitionSource(s);
			}
			animationAlpha->setDuration(preferedDuration);
			break;
		case TRANSITION_CUSTOM_MEDIA:
			if (customTransitionVideoSource) {
				if (visible) {
					QObject::connect(animationAlpha, SIGNAL(finished()), this, SLOT(setTransitionSource()) );
				} else {
					setTransitionSource(customTransitionVideoSource);
					customTransitionVideoSource->play(true);
				}
			}
			animationAlpha->setDuration(preferedDuration);
			break;
		default:
		case TRANSITION_NONE:
			setTransitionSource(NULL);
			animationAlpha->setDuration(0);
	}

	animationAlpha->setStartValue( currentAlpha );
	animationAlpha->setEndValue( visible ? 1.1 : 0.0 );
	animationAlpha->start();

}

OutputRenderWindow::OutputRenderWindow() : OutputRenderWidget(0, (QGLWidget *)RenderingManager::getRenderingWidget(), Qt::Window | Qt::CustomizeWindowHint | Qt::WindowTitleHint | Qt::WindowMinimizeButtonHint)
{
	// this is not a windet, but a window
	useWindowAspectRatio = false;
	setCursor(Qt::BlankCursor);

	// default transition of 1 second
	setTransitionDuration(1000);
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
	glRenderWidget::initializeGL();

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


