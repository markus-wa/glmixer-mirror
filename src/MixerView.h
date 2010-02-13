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

    typedef enum {NONE = 0, OVER, GRAB, SELECT } actionType;

	MixerView();

    virtual void paint();
    virtual void reset();
    virtual void resize(int w, int h);
    void mousePressEvent(QMouseEvent *event);
    void mouseMoveEvent(QMouseEvent *event);
    void mouseReleaseEvent ( QMouseEvent * event );
    void wheelEvent ( QWheelEvent * event );
    void keyPressEvent ( QKeyEvent * event );
    // TODO void tabletEvent ( QTabletEvent * event ); // handling of tablet features like pressure and rotation

	void zoomReset();
	void zoomBestFit();

private:

    SourceSet::iterator  getSourceAtCoordinates(int mouseX, int mouseY);
    void grabSource(SourceSet::iterator s, int x, int y, int dx, int dy);

    actionType currentAction;

    void setAction(actionType a);

};

#endif /* MIXERVIEWWIDGET_H_ */
