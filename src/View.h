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

class CatalogView;

class View {

public:
	View() :
		zoom(0), minzoom(0), maxzoom(0), deltazoom(0), panx(0), maxpanx(0),
		pany(0), maxpany(0), panz(0), maxpanz(0), modified(true) {
		viewport[0] = 0;
		viewport[1] = 0;
		viewport[2] = 0;
		viewport[3] = 0;
	}
	virtual ~View() {
	}
	virtual void setModelview() {
		glMatrixMode(GL_MODELVIEW);
		glLoadIdentity();
		modified = false;
	}
	virtual void paint() {
	}
	virtual void resize(int w, int h) {
		modified = true;
	}
	virtual bool mousePressEvent(QMouseEvent *event) {
		return false;
	}
	virtual bool mouseMoveEvent(QMouseEvent *event) {
		return false;
	}
	virtual bool mouseReleaseEvent(QMouseEvent * event) {
		return false;
	}
	virtual bool mouseDoubleClickEvent(QMouseEvent * event) {
		return false;
	}
	virtual bool wheelEvent(QWheelEvent * event) {
		return false;
	}
	virtual bool keyPressEvent(QKeyEvent * event) {
		return false;
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
		setModelview();
		modified = true;
	}
	inline float getZoom() {
		return ( zoom );
	}
	inline float getZoomPercent() {
		return ( (zoom - minzoom) * 100.f / (maxzoom - minzoom) );
	}

	inline void setPanningX(float x) {
		panx = CLAMP(x, - maxpanx, maxpanx);
		setModelview();
		modified = true;
	}
	inline float getPanningX() {
		return panx;
	}
	inline void setPanningY(float y) {
		pany = CLAMP(y, - maxpany, maxpany);
		setModelview();
		modified = true;
	}
	inline float getPanningY() {
		return pany;
	}
	inline void setPanningZ(float z) {
		panz = CLAMP(z, - maxpanz, maxpanz);
		setModelview();
		modified = true;
	}
	inline float getPanningZ() {
		return panz;
	}

	inline QPixmap getIcon() {
		return icon;
	}

	virtual void clear() {
		zoomReset();
		selectedSources.clear();
		clickedSources.clear();
	}

	bool isModified() { return modified; }

	bool noSourceClicked() { return clickedSources.empty(); }

protected:
	float zoom, minzoom, maxzoom, deltazoom;
	float panx, maxpanx, pany, maxpany, panz, maxpanz;
	bool modified;

	GLint viewport[4];
	GLdouble projection[16];
	GLdouble modelview[16];

	SourceList selectedSources;
	reverseSourceSet clickedSources;
	QPoint lastClicPos;
	QPixmap icon;

};

#endif /* VIEW_H_ */
