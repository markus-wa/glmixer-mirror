/*
 * MixerView.h
 *
 *  Created on: Nov 9, 2009
 *      Author: bh
 */

#ifndef MIXERVIEWWIDGET_H_
#define MIXERVIEWWIDGET_H_

#include "View.h"

class MixerView: public View {

public:

	MixerView();

    virtual void paint();
    virtual void reset();
    virtual void resize(int w, int h);
    bool mousePressEvent(QMouseEvent *event);
    bool mouseMoveEvent(QMouseEvent *event);
    bool mouseReleaseEvent ( QMouseEvent * event );
    bool mouseDoubleClickEvent ( QMouseEvent * event );
    bool wheelEvent ( QWheelEvent * event );
    bool keyPressEvent ( QKeyEvent * event );
    // TODO void tabletEvent ( QTabletEvent * event ); // handling of tablet features like pressure and rotation

    void clear();
	void zoomReset();
	void zoomBestFit();

private:

	Source *cliked;
    Source *getSourceAtCoordinates(int mouseX, int mouseY);
    void grabSource(Source *s, int x, int y, int dx, int dy);
    void panningBy(int x, int y, int dx, int dy) ;

    typedef enum {NONE = 0, OVER, GRAB, SELECT, RECTANGLE } actionType;
    actionType currentAction;
	GLdouble rectangleStart[2], rectangleEnd[2];
    void setAction(actionType a);

    // creation of groups from set of selection
	SourceListArray groupSources;
    bool isInAGroup(Source *);
	QMap<SourceListArray::iterator, QColor> groupColor;
};

#endif /* MIXERVIEWWIDGET_H_ */
