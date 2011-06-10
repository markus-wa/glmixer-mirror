/*
 * View.h
 *
 *  Created on: Jan 23, 2010
 *      Author: bh
 *
 *  This file is part of GLMixer.
 *
 *   GLMixer is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation, either version 3 of the License, or
 *   (at your option) any later version.
 *
 *   GLMixer is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with GLMixer.  If not, see <http://www.gnu.org/licenses/>.
 *
 *   Copyright 2009, 2010 Bruno Herbelin
 *
 */

#ifndef VIEW_H_
#define VIEW_H_

#include "common.h"
#include "SourceSet.h"

#include <QDomElement>

class CatalogView;

class View {

	friend class ViewRenderWidget;
	friend class RenderingManager;

public:
	/**
	 * View default constructor ; initialize variables.
	 */
	View() :
		zoom(0), minzoom(0), maxzoom(0), deltazoom(0), panx(0), maxpanx(0),
		pany(0), maxpany(0), panz(0), maxpanz(0), modified(true) {
		viewport[0] = 0;
		viewport[1] = 0;
		viewport[2] = 0;
		viewport[3] = 0;
	}
	virtual ~View() {};
	/**
	 * Apply the Modelview matrix
	 */
	virtual void setModelview() {
		glMatrixMode(GL_MODELVIEW);
		glLoadIdentity();
		modified = false;
	}
	/**
	 *
	 */
	bool isModified() { return modified; }
	/**
	 *
	 */
	virtual void paint() {
	}
	/**
	 *
	 */
	virtual void resize(int w, int h) {
		modified = true;
		if ( w > 0 && h > 0) {
			viewport[2] = w;
			viewport[3] = h;
		}
	}
	/**
	 *
	 * @return True if the event was processed and used.
	 */
	virtual bool mousePressEvent(QMouseEvent *event) {
		return false;
	}
	/**
	 *
	 * @return True if the event was processed and used.
	 */
	virtual bool mouseMoveEvent(QMouseEvent *event) {
		return false;
	}
	/**
	 *
	 * @return True if the event was processed and used.
	 */
	virtual bool mouseReleaseEvent(QMouseEvent * event) {
		return false;
	}
	/**
	 *
	 * @return True if the event was processed and used.
	 */
	virtual bool mouseDoubleClickEvent(QMouseEvent * event) {
		return false;
	}
	/**
	 *
	 * @return True if the event was processed and used.
	 */
	virtual bool wheelEvent(QWheelEvent * event) {
		return false;
	}
	/**
	 *
	 * @return True if the event was processed and used.
	 */
	virtual bool keyPressEvent(QKeyEvent * event) ;
	/**
	 *
	 * @return True if the event was processed and used.
	 */
	virtual bool keyReleaseEvent(QKeyEvent * event) ;

	virtual void coordinatesFromMouse(int mouseX, int mouseY, double *X, double *Y);

    typedef enum {NONE = 0, OVER, GRAB, TOOL, SELECT, PANNING, DROP } actionType;
    void setAction(actionType a);
	/**
	 *
	 */
	virtual void zoomIn() {
		setZoom(zoom + (2.f * zoom * minzoom) / maxzoom);
	}
	/**
	 *
	 */
	virtual void zoomOut() {
		setZoom(zoom - (2.f * zoom * minzoom) / maxzoom);
	}
	/**
	 *
	 */
	virtual void zoomReset() {
	}
	/**
	 *
	 */
	virtual void zoomBestFit( bool onlyClickedSource = false ) {
	}
	/**
	 *
	 */
	inline void setZoom(float z) {
		zoom = CLAMP(z, minzoom, maxzoom);
		setModelview();
		modified = true;
	}
	/**
	 *
	 */
	inline float getZoom() {
		return ( zoom );
	}
	/**
	 *
	 */
	inline float getZoomPercent() {
		return ( (zoom - minzoom) * 100.f / (maxzoom - minzoom) );
	}
	/**
	 *
	 */
	inline void setPanningX(float x) {
		panx = CLAMP(x, - maxpanx, maxpanx);
		setModelview();
		modified = true;
	}
	/**
	 *
	 */
	inline float getPanningX() {
		return panx;
	}
	/**
	 *
	 */
	inline void setPanningY(float y) {
		pany = CLAMP(y, - maxpany, maxpany);
		setModelview();
		modified = true;
	}
	/**
	 *
	 */
	inline float getPanningY() {
		return pany;
	}
	/**
	 *
	 */
	inline void setPanningZ(float z) {
		panz = CLAMP(z, - maxpanz, maxpanz);
		setModelview();
		modified = true;
	}
	/**
	 *
	 */
	inline float getPanningZ() {
		return panz;
	}
	/**
	 *
	 */
	inline QPixmap getIcon() {
		return icon;
	}
	/**
	 *
	 */
	inline QString getTitle() {
		return title;
	}
	/**
	 *
	 */
	virtual void clear() {
		zoomReset();
		clickedSources.clear();
	}
	/**
	 *
	 */
	bool sourceClicked() { return !clickedSources.empty(); }


	/**
	 * SELECTION (static)
	 */
	static void select(Source *s);
	static void deselect(Source *s);
	static void select(SourceList l);
	static void deselect(SourceList l);
	static void clearSelection();
	static bool isInSelection(Source *s);
	static bool hasSelection() { return !_selectedSources.empty(); }
	static void setSelection(SourceList l);
	static SourceList::iterator selectionBegin() { return _selectedSources.begin(); }
	static SourceList::iterator selectionEnd() { return _selectedSources.end(); }
	static SourceList copySelection() { return SourceList (_selectedSources); }
	static Source *selectionSource() { return _selectionSource; }
	static void updateSelectionSource();

	/**
	 * CONFIGURATION
	 */
	virtual QDomElement getConfiguration(QDomDocument &doc);
	virtual void setConfiguration(QDomElement xmlconfig);

protected:
	float zoom, minzoom, maxzoom, deltazoom;
	float panx, maxpanx, pany, maxpany, panz, maxpanz;
	bool modified;

	GLint viewport[4];
	GLdouble projection[16];
	GLdouble modelview[16];


	reverseSourceSet clickedSources;
	QPoint lastClicPos;
	QPixmap icon;
	QString title;

    actionType currentAction, previousAction;

    static void computeBoundingBox(const SourceList &l, double bbox[][2]);

private:
	static SourceList _selectedSources;
	static Source *_selectionSource;

};

#endif /* VIEW_H_ */
