/*
 * SessionSwitcher.h
 *
 *  Created on: Mar 27, 2011
 *      Author: bh
 */

#ifndef SESSIONSWITCHER_H_
#define SESSIONSWITCHER_H_

#include <QColor>
#include <QPropertyAnimation>

class Source;

class SessionSwitcher: public QObject {

	Q_OBJECT
    Q_PROPERTY(float alpha READ alpha WRITE setAlpha)

	friend class SessionSwitcherWidget;

public:
	SessionSwitcher(QObject *parent = 0);
	virtual ~SessionSwitcher();

	void render();

	void setAlpha(float a) { overlayAlpha = a; }
	float alpha() const { return overlayAlpha; }

	int transitionDuration() const;
	int transitionCurve() const ;

	void setTransitionColor(QColor c) { customTransitionColor = c; }
	QColor transitionColor() const { return customTransitionColor; }

	void setTransitionMedia(QString filename);
	QString transitionMedia() const ;

	typedef enum {
		TRANSITION_NONE = 0,
		TRANSITION_BACKGROUND = 1,
		TRANSITION_CUSTOM_COLOR = 2,
		TRANSITION_LAST_FRAME = 3,
		TRANSITION_CUSTOM_MEDIA = 4
	} transitionType;
	void setTransitionType(transitionType t);
	transitionType getTransitionType() const {return transition_type;}

public Q_SLOTS:
	// initiate the transition animation
	void startTransition(bool sceneVisible, bool instanteneous = false);
	// end the transition
	void endTransition();
	// choose the transition source (for custom transition types)
	void setTransitionSource(Source *s = NULL);
	// set the duration of fading
	void setTransitionDuration(int ms);
	// set the profile of fading
	void setTransitionCurve(int curveType);
	// instantaneous set transparency of overlay
	void setTransparency(int alpha);

Q_SIGNALS:
	void animationFinished();
	void transitionSourceChanged(Source *s);

private:
	bool manual_mode;
	int duration;
	float overlayAlpha;
	Source *overlaySource;
	QPropertyAnimation *animationAlpha;
	transitionType transition_type;
    QColor customTransitionColor;
    class VideoSource *customTransitionVideoSource;
};

#endif /* SESSIONSWITCHER_H_ */