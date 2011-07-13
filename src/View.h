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
		zoom(0), minzoom(0), maxzoom(0), deltax(0), deltay(0), panx(0), maxpanx(0),
		pany(0), maxpany(0), panz(0), maxpanz(0), modified(true), individual(false),
		currentAction(NONE), previousAction(NONE){
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

    typedef enum {
    	NONE = 0,
    	OVER,
    	GRAB,
    	TOOL,
    	SELECT,
    	PANNING,
    	DROP
    } ActionType;
    void setAction(ActionType a);
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
	void setPanning(float x, float y, float z = -1.f);
	/**
	 *
	 */
	inline float getPanningX() {
		return panx;
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
	 * CONFIGURATION
	 */
	virtual QDomElement getConfiguration(QDomDocument &doc);
	virtual void setConfiguration(QDomElement xmlconfig);

	/**
	 * User input actions preferences (buttons and modifier keys)
	 */
	typedef enum {
		INPUT_TOOL = 0,
		INPUT_TOOL_INDIVIDUAL,
		INPUT_NAVIGATE,
		INPUT_DRAG,
		INPUT_SELECT,
		INPUT_CONTEXT_MENU,
		INPUT_ZOOM,
		INPUT_NONE
	} UserInput;

	static QString userInputLabel(UserInput ab, bool verbose = false);
    static QString userInputDescription(View::UserInput ab, QMap<View::UserInput,Qt::MouseButtons> bm = View::_buttonmap, QMap<View::UserInput,Qt::KeyboardModifiers> mm = View::_modifiermap);
	static Qt::MouseButtons qtMouseButtons(View::UserInput ab) { return View::_buttonmap[ab]; }
	static Qt::KeyboardModifiers qtMouseModifiers(View::UserInput ab) { return View::_modifiermap[ab]; }
	static QMap<View::UserInput,Qt::MouseButtons> defaultMouseButtonsMap();
    static QMap<View::UserInput,Qt::KeyboardModifiers> defaultModifiersMap();
	static QMap<int,int> getMouseButtonsMap(QMap<View::UserInput,Qt::MouseButtons> from = View::_buttonmap) ;
    static QMap<int,int> getMouseModifiersMap(QMap<View::UserInput,Qt::KeyboardModifiers> from = View::_modifiermap) ;
	static void setMouseButtonsMap(QMap<int, int> m) ;
    static void setMouseModifiersMap(QMap<int, int> m) ;
    static bool isUserInput(QMouseEvent *event, UserInput ab);
	static void setZoomSpeed(float zs);
	static float zoomSpeed();
	static void setZoomCentered(bool on);
	static bool zoomCentered();


	/**
	 * Selection management
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


protected:
	float zoom, minzoom, maxzoom, deltax, deltay;
	float panx, maxpanx, pany, maxpany, panz, maxpanz;
	bool modified, individual;

	GLint viewport[4];
	GLdouble projection[16];
	GLdouble modelview[16];

	reverseSourceSet clickedSources;
	QPoint lastClicPos;
	QPixmap icon;
	QString title;

    ActionType currentAction, previousAction;

    static void computeBoundingBox(const SourceList &l, double bbox[2][2]);

private:
	static SourceList _selectedSources;
	static Source *_selectionSource;
	static float zoomspeed;
	static bool zoomcentered;

    static QMap<View::UserInput,Qt::MouseButtons> _buttonmap;
    static QMap<View::UserInput,Qt::KeyboardModifiers> _modifiermap;
};

#endif /* VIEW_H_ */
