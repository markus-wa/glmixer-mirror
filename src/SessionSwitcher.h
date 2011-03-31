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
		TRANSITION_LAST_FRAME = 2,
		TRANSITION_CUSTOM_COLOR = 3,
		TRANSITION_CUSTOM_MEDIA = 4
	} transitionType;
	void setTransitionType(transitionType t);
	transitionType getTransitionType() const {return transition_type;}

public Q_SLOTS:
	void startTransition(bool sceneVisible, bool instanteneous = false);
	void endTransition();
	void setTransitionSource(Source *s = NULL);
	void setTransitionDuration(int ms);
	void setTransitionCurve(int curveType);
	void setTransparency(int alpha);

Q_SIGNALS:
	void animationFinished();

private:
	int duration;
	float overlayAlpha;
	Source *overlaySource;
	QPropertyAnimation *animationAlpha;
	transitionType transition_type;
    QColor customTransitionColor;
    class VideoSource *customTransitionVideoSource;
};

#endif /* SESSIONSWITCHER_H_ */
