/*
 * LayersView.h
 *
 *  Created on: Feb 26, 2010
 *      Author: bh
 */

#ifndef LAYERSVIEW_H_
#define LAYERSVIEW_H_

#include "View.h"

class LayersView: public View {
public:
	LayersView();

    virtual void paint();
    virtual void reset();
    virtual void resize(int w, int h);
    bool mousePressEvent(QMouseEvent *event);
    bool mouseMoveEvent(QMouseEvent *event);
    bool mouseReleaseEvent ( QMouseEvent * event );
    bool wheelEvent ( QWheelEvent * event );
    bool keyPressEvent ( QKeyEvent * event );
    // TODO void tabletEvent ( QTabletEvent * event ); // handling of tablet features like pressure and rotation

	void zoomReset();
	void zoomBestFit();


private:
    float lookatdistance;

    bool getSourcesAtCoordinates(int mouseX, int mouseY);
    void panningBy(int x, int y, int dx, int dy);
};

#endif /* LAYERSVIEW_H_ */
