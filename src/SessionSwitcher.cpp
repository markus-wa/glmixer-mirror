/*
 * SessionSwitcher.cpp
 *
 *  Created on: Mar 27, 2011
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
 *   Copyright 2009, 2012 Bruno Herbelin
 *
 */

#include "SessionSwitcher.moc"

#include "ViewRenderWidget.h"
#include "OutputRenderWindow.h"
#include "RenderingManager.h"
#include "VideoSource.h"
#include "AlgorithmSource.h"
#include "CaptureSource.h"

#include <QGLWidget>

SessionSwitcher::SessionSwitcher(QObject *parent)  : QObject(parent), manual_mode(false), duration(1000), currentAlpha(0.0), overlayAlpha(0.0), overlaySource(0),
transition_type(TRANSITION_NONE), customTransitionColor(QColor()), customTransitionVideoSource(0) {

	// alpha opacity mask
	alphaAnimation = new QPropertyAnimation(this, "alpha");
	alphaAnimation->setDuration(500);
	alphaAnimation->setEasingCurve(QEasingCurve::InOutQuad);

	// transition overlay
	overlayAnimation = new QPropertyAnimation(this, "overlay");
	overlayAnimation->setDuration(0);
	overlayAnimation->setEasingCurve(QEasingCurve::InOutQuad);

    QObject::connect(overlayAnimation, SIGNAL(finished()), this, SIGNAL(animationFinished() ) );
    QObject::connect(overlayAnimation, SIGNAL(finished()), this, SLOT(endTransition() ) );

	// default transition of 1 second
	setTransitionDuration(1000);

}

SessionSwitcher::~SessionSwitcher() {

    if (alphaAnimation)
        delete alphaAnimation;
    if (overlayAnimation)
        delete overlayAnimation;
    if (overlaySource)
        delete overlaySource;
}

void SessionSwitcher::render() {

	if (currentAlpha > 0.0) {
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		glBlendEquation(GL_FUNC_ADD);
		glColor4f(0.0, 0.0, 0.0, currentAlpha);
		glScaled( OutputRenderWindow::getInstance()->getAspectRatio() * SOURCE_UNIT, SOURCE_UNIT, 1.0);
		glCallList(ViewRenderWidget::quad_texured);
	}

	// if we shall render the overlay, do it !
	if ( overlayAlpha > 0.0 && overlaySource ) {
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		glBlendEquation(GL_FUNC_ADD);

		glColor4f(overlaySource->getColor().redF(), overlaySource->getColor().greenF(), overlaySource->getColor().blueF(), overlayAlpha);
		glActiveTexture(GL_TEXTURE0);
		glEnable(GL_TEXTURE_2D);
        overlaySource->bind();
		glScaled( OutputRenderWindow::getInstance()->getAspectRatio() * overlaySource->getScaleX(), overlaySource->getScaleY(), 1.0);
		glCallList(ViewRenderWidget::quad_texured);
		glDisable(GL_TEXTURE_2D);
	}

}


bool SessionSwitcher::transitionActive() const
{
	return (overlayAnimation->state() == QAbstractAnimation::Running );
}

void SessionSwitcher::setTransitionCurve(int curveType)
{
	overlayAnimation->setEasingCurve( (QEasingCurve::Type) qBound( (int) QEasingCurve::Linear, curveType, (int) QEasingCurve::OutInBounce));
}

int SessionSwitcher::transitionCurve() const
{
	return (int) overlayAnimation->easingCurve().type();
}

void SessionSwitcher::setTransitionDuration(int ms)
{
	duration = ms;

}

int SessionSwitcher::transitionDuration() const
{
	return duration;
}

void SessionSwitcher::setTransparency(int alpha)
{
	overlayAlpha = 1.f - ((float) alpha / 100.f);
}


void SessionSwitcher::setTransitionSource(Source *s)
{
	if (overlaySource) {
		if (overlaySource == (Source*) customTransitionVideoSource)
			customTransitionVideoSource->play(false);
		else
			delete overlaySource;
	}

	overlaySource = s;
	emit transitionSourceChanged(overlaySource);
}


void SessionSwitcher::setTransitionMedia(QString filename, bool generatePowerOfTwoRequested)
{
	if (customTransitionVideoSource)
		delete customTransitionVideoSource;

	customTransitionVideoSource = NULL;

    VideoFile *newSourceVideoFile = NULL;
    if ( !generatePowerOfTwoRequested && (glSupportsExtension("GL_EXT_texture_non_power_of_two") || glSupportsExtension("GL_ARB_texture_non_power_of_two") ) )
		newSourceVideoFile = new VideoFile(this);
	else
		newSourceVideoFile = new VideoFile(this, true, SWS_POINT);

    Q_CHECK_PTR(newSourceVideoFile);

	if ( newSourceVideoFile->open( filename ) ) {
		newSourceVideoFile->setOptionRestartToMarkIn(true);
		newSourceVideoFile->play(false);
		// create new video source
		customTransitionVideoSource = (VideoSource*) RenderingManager::getInstance()->newMediaSource(newSourceVideoFile);
	} else {
        qCritical() << filename << QChar(124).toLatin1()<< QObject::tr("Session file could not be loaded.");
		delete newSourceVideoFile;
	}

	if (transition_type == TRANSITION_CUSTOM_MEDIA)
		overlaySource = customTransitionVideoSource;

	emit transitionSourceChanged(overlaySource);
}


QString SessionSwitcher::transitionMedia() const
{
	if (customTransitionVideoSource )
		return customTransitionVideoSource->getVideoFile()->getFileName();

	return QString();
}


void SessionSwitcher::setTransitionType(transitionType t) {
	transition_type = t;

	Source *s = NULL;
	switch (transition_type) {
		case TRANSITION_CUSTOM_COLOR:
			s =  RenderingManager::getInstance()->newAlgorithmSource(int(AlgorithmSource::FLAT), 2, 2, 0, 0, true) ;
			s->setColor(customTransitionColor);
			break;
		case TRANSITION_BACKGROUND:
			s =  RenderingManager::getInstance()->newAlgorithmSource(int(AlgorithmSource::FLAT), 2, 2, 0, 0, true) ;
			s->setColor(QColor(Qt::black)); // TODO what if white background ?
			break;
		case TRANSITION_CUSTOM_MEDIA:
			// did we load a transition source?
			s = customTransitionVideoSource ? (Source *) customTransitionVideoSource : NULL;
			break;
		default:
			s = NULL;
			break;
	}

	setTransitionSource(s);
}

void SessionSwitcher::endTransition()
{
	RenderingManager::getRenderingWidget()->setFaded(false);
}


void SessionSwitcher::startTransition(bool sceneVisible, bool instanteneous){

	if (overlayAnimation->state() == QAbstractAnimation::Running )
		overlayAnimation->stop();


	switch (transition_type) {
		case TRANSITION_LAST_FRAME:
			// special case ; don't behave identically for fade in than fade out
			if (sceneVisible)
				break;
			else {
				// capture screen and use it immediately as transition source
				QImage capture = RenderingManager::getInstance()->captureFrameBuffer();
				capture = capture.convertToFormat(QImage::Format_RGB32);
				setTransitionSource( RenderingManager::getInstance()->newCaptureSource(capture) );
				// no break
			}
		case TRANSITION_NONE:
			instanteneous = true;
			break;
		case TRANSITION_CUSTOM_MEDIA:
			// did we load a transition source?
			if (customTransitionVideoSource)
				customTransitionVideoSource->play(true);
			break;
		default:
			break;
	}

	if (manual_mode) {
		instanteneous = true;
		sceneVisible = false;
	}
	overlayAnimation->setCurrentTime(0);
	overlayAnimation->setDuration( instanteneous ? 0 : duration );
	overlayAnimation->setStartValue( overlayAlpha );
	overlayAnimation->setEndValue( sceneVisible ? 0.0 : 1.0 );
	// do not do "animationAlpha->start();" immediately ; there is a delay in rendering
	// so, we also delay a little the start of the transition to make sure it is fully applied
	QTimer::singleShot(100, overlayAnimation, SLOT(start()));

	RenderingManager::getRenderingWidget()->setFaded(!instanteneous);

}


void SessionSwitcher::setAlpha(float a)
{
	currentAlpha = a;
	emit alphaChanged( int( a * 100.0) );
}

void SessionSwitcher::setAlpha(int a)
{
	currentAlpha = float(a) / 100.0;
}

void SessionSwitcher::smoothAlphaTransition(bool visible){

	if (alphaAnimation->state() == QAbstractAnimation::Running )
		alphaAnimation->stop();

	alphaAnimation->setStartValue( currentAlpha );
	alphaAnimation->setEndValue( visible ? 0.0 : 1.1 );
	alphaAnimation->start();

}
