/*
 * SessionSwitcher.cpp
 *
 *  Created on: Mar 27, 2011
 *      Author: bh
 */

#include "SessionSwitcher.moc"

#include "ViewRenderWidget.h"
#include "RenderingManager.h"
#include "VideoSource.h"
#include "AlgorithmSource.h"
#include "CaptureSource.h"

#include <QGLWidget>

SessionSwitcher::SessionSwitcher(QObject *parent)  : QObject(parent), duration(1000), overlayAlpha(0.0), overlaySource(0),
transition_type(TRANSITION_NONE), customTransitionColor(QColor()), customTransitionVideoSource(0) {

	animationAlpha = new QPropertyAnimation(this, "alpha");
	animationAlpha->setDuration(0);
	animationAlpha->setEasingCurve(QEasingCurve::InOutQuad);

    QObject::connect(animationAlpha, SIGNAL(finished()), this, SIGNAL(animationFinished() ) );
    QObject::connect(animationAlpha, SIGNAL(finished()), this, SLOT(endTransition() ) );

	// default transition of 1 second
	setTransitionDuration(1000);
}

SessionSwitcher::~SessionSwitcher() {

	if (overlaySource)
		delete overlaySource;
}

void SessionSwitcher::render() {

	// if we shall render the overlay, do it !
	if ( overlaySource ) {
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		glBlendEquation(GL_FUNC_ADD);

		glColor4f(overlaySource->getColor().redF(), overlaySource->getColor().greenF(), overlaySource->getColor().blueF(), overlayAlpha);
		glActiveTexture(GL_TEXTURE0);
		glEnable(GL_TEXTURE_2D);
		overlaySource->update();
		glScaled( RenderingManager::getInstance()->getFrameBufferAspectRatio() * overlaySource->getScaleX(), overlaySource->getScaleY(), 1.f);
		glCallList(ViewRenderWidget::quad_texured);
		glDisable(GL_TEXTURE_2D);
	}

}


void SessionSwitcher::setTransitionCurve(int curveType)
{
	animationAlpha->setEasingCurve( (QEasingCurve::Type) qBound( (int) QEasingCurve::Linear, curveType, (int) QEasingCurve::OutInBounce));
}

int SessionSwitcher::transitionCurve() const
{
	return (int) animationAlpha->easingCurve().type();
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
}


void SessionSwitcher::setTransitionMedia(QString filename)
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
		newSourceVideoFile->play(true);
		// create new video source
		customTransitionVideoSource = (VideoSource*) RenderingManager::getInstance()->newMediaSource(newSourceVideoFile);

	} else {
		qCritical( "The file %s could not be loaded.", qPrintable(filename) );
		delete newSourceVideoFile;
	}

	if (transition_type == TRANSITION_CUSTOM_MEDIA)
		overlaySource = customTransitionVideoSource;


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
			s =  RenderingManager::getInstance()->newAlgorithmSource(0, 4, 4, 0, 0) ;
			s->setColor(customTransitionColor);
			break;
		case TRANSITION_BACKGROUND:
			s =  RenderingManager::getInstance()->newAlgorithmSource(0, 4, 4, 0, 0) ;
			s->setColor(QColor(Qt::black)); // TODO what if white background ?
			break;
		case TRANSITION_CUSTOM_MEDIA:
			// did we load a transition source?
			if (customTransitionVideoSource) {
				s = (Source *) customTransitionVideoSource;
				break;
			}
		default:
			s = NULL;
	}

	setTransitionSource(s);

}

void SessionSwitcher::endTransition()
{
	RenderingManager::getRenderingWidget()->setFaded(false);
}


void SessionSwitcher::startTransition(bool sceneVisible, bool instanteneous){

	if (animationAlpha->state() == QAbstractAnimation::Running )
		animationAlpha->stop();

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
			}
		case TRANSITION_NONE:
			instanteneous = true;
			break;
		case TRANSITION_CUSTOM_MEDIA:
			// did we load a transition source?
			if (customTransitionVideoSource)
				customTransitionVideoSource->play(true);
		default:
			break;
	}

	animationAlpha->setCurrentTime(0);
	animationAlpha->setDuration( instanteneous ? 0 : duration );
	animationAlpha->setStartValue( overlayAlpha );
	animationAlpha->setEndValue( sceneVisible ? 0.0 : 1.0 );
	// do not do "animationAlpha->start();" immediately ; there is a delay in rendering
	// so, we also delay a little the start of the transition to make sure it is fully applied
	QTimer::singleShot(100, animationAlpha, SLOT(start()));

	RenderingManager::getRenderingWidget()->setFaded(!instanteneous);

}

