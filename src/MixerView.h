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

    void paint();
    void setModelview();
    void resize(int w = -1, int h = -1);
    bool mousePressEvent(QMouseEvent *event);
    bool mouseMoveEvent(QMouseEvent *event);
    bool mouseReleaseEvent ( QMouseEvent * event );
    bool mouseDoubleClickEvent ( QMouseEvent * event );
    bool wheelEvent ( QWheelEvent * event );
//    bool keyPressEvent ( QKeyEvent * event );

    // TODO void tabletEvent ( QTabletEvent * event ); // handling of tablet features like pressure and rotation

	void clear();
	void zoomReset();
	void zoomBestFit();

	void alphaCoordinatesFromMouse(int mouseX, int mouseY, double *alphaX, double *alphaY);

private:

    bool getSourcesAtCoordinates(int mouseX, int mouseY);
    void grabSource(Source *s, int x, int y, int dx, int dy);
    void panningBy(int x, int y, int dx, int dy) ;

    void setAction(actionType a);

    // creation of groups from set of selection
	SourceListArray groupSources;
    bool isInAGroup(Source *);
	QMap<SourceListArray::iterator, QColor> groupColor;
};

#endif /* MIXERVIEWWIDGET_H_ */
