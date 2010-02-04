/*
 * View.h
 *
 *  Created on: Jan 23, 2010
 *      Author: bh
 */

#ifndef VIEW_H_
#define VIEW_H_

#include "common.h"
#include "SourceSet.h"


class View {
public:
	View(): zoom(0), minzoom(0), maxzoom(0), panx(0), maxpanx(0), pany(0), maxpany(0) {}
	virtual ~View() {}

    inline void setZoom(float z) { zoom = CLAMP(z, minzoom, maxzoom);}
    inline float getZoom() { return zoom; }
    inline void setPanningX(float x) { panx = CLAMP(x, -maxpanx, maxpanx);}
    inline float getPanningX() { return panx; }
    inline void setPanningY(float y) { pany = CLAMP(y, -maxpany, maxpany);}
    inline float getPanningY() { return pany; }

protected:
    float zoom, minzoom, maxzoom;
    float panx, maxpanx, pany, maxpany;

    GLdouble projection[16];
    GLdouble modelview[16];
    GLint viewport[4];

    SourceSet selection;
};

#endif /* VIEW_H_ */
