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
	View() :
		zoom(0), minzoom(0), maxzoom(0), panx(0), maxpanx(0), pany(0), maxpany(0), panz(0), maxpanz(0) {
		viewport[0] = 0;
		viewport[1] = 0;
		viewport[2] = 0;
		viewport[3] = 0;
	}
	virtual ~View() {
	}

	virtual void paint() {
	}
	virtual void reset() {
	}
	virtual void resize(int w, int h) {
	}
	virtual void mousePressEvent(QMouseEvent *event) {
	}
	virtual void mouseMoveEvent(QMouseEvent *event) {
	}
	virtual void mouseReleaseEvent(QMouseEvent * event) {
	}
	virtual void mouseDoubleClickEvent(QMouseEvent * event) {
	}
	virtual void wheelEvent(QWheelEvent * event) {
	}
	virtual void keyPressEvent(QKeyEvent * event) {
	}

	virtual void zoomIn() {
		setZoom(zoom + (2.f * zoom * minzoom) / maxzoom);
	}
	;
	virtual void zoomOut() {
		setZoom(zoom - (2.f * zoom * minzoom) / maxzoom);
	}
	virtual void zoomReset() {
	}
	virtual void zoomBestFit() {
	}
	inline void setZoom(float z) {
		zoom = CLAMP(z, minzoom, maxzoom);
		refreshMatrices();
	}
	inline float getZoom() {
		return ( (zoom - minzoom) * 100.f / (maxzoom - minzoom) );
	}

	inline void setPanningX(float x) {
		panx = CLAMP(x, - maxpanx, maxpanx);
		refreshMatrices();
	}
	inline float getPanningX() {
		return panx;
	}
	inline void setPanningY(float y) {
		pany = CLAMP(y, - maxpany, maxpany);
		refreshMatrices();
	}
	inline float getPanningY() {
		return pany;
	}
	inline void setPanningZ(float z) {
		panz = CLAMP(z, - maxpanz, maxpanz);
		refreshMatrices();
	}
	inline float getPanningZ() {
		return panz;
	}

	inline void refreshMatrices() {
		glMatrixMode(GL_PROJECTION);
		glGetDoublev(GL_PROJECTION_MATRIX, projection);
		glMatrixMode(GL_MODELVIEW);
		glLoadIdentity();
		reset();
		glGetDoublev(GL_MODELVIEW_MATRIX, modelview);
	}

	inline QPixmap getIcon() {
		return icon;
	}

protected:
	float zoom, minzoom, maxzoom;
	float panx, maxpanx, pany, maxpany, panz, maxpanz;

	GLint viewport[4];
	GLdouble projection[16];
	GLdouble modelview[16];

	reverseSourceSet selection;
	QPoint lastClicPos;
	QPixmap icon;
};

#endif /* VIEW_H_ */
