/*
 * View.h
 *
 *  Created on: Jan 23, 2010
 *      Author: bh
 */

#ifndef VIEW_H_
#define VIEW_H_

#include "SourceSet.h"

#define CLAMP(x, low, high)  (((x) > (high)) ? (high) : (((x) < (low)) ? (low) : (x)))

class View {
public:
	View(): zoom(0), minzoom(0), maxzoom(0) {}
	virtual ~View() {}

    inline void setZoom(float z) { zoom = CLAMP(z, minzoom, maxzoom);}
    inline float getZoom() { return zoom; }

protected:
    float zoom, minzoom, maxzoom;

    GLdouble projection[16];
    GLdouble modelview[16];
    GLint viewport[4];

    SourceSet selection;
};

#endif /* VIEW_H_ */
