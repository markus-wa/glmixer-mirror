/*
 * selectionView.h
 *
 *  Created on: May 12, 2010
 *      Author: bh
 */

#ifndef CATALOGVIEW_H_
#define CATALOGVIEW_H_

#include "View.h"
#include <map>

class CatalogView: public View {

public:
	CatalogView();
	virtual ~CatalogView();

	// View implementation
	void setModelview();
	void clear();
	void paint();
	void resize(int w = -1, int h = -1);
    bool mousePressEvent(QMouseEvent *event);
    bool mouseMoveEvent(QMouseEvent *event);
    bool mouseReleaseEvent ( QMouseEvent * event );
    bool mouseDoubleClickEvent ( QMouseEvent * event );
    bool wheelEvent ( QWheelEvent * event );

	// Specific implementation
	void setVisible(bool on);
	bool visible() { return _visible;}
	void setSize(int s);
	int size() { return _size; }
	void drawSource(Source *s, int index);

//	GLint *getViewport() { return viewport; }

private:
	bool _visible;
	int _size;
	double _height, h_unit, v_unit;
	float _alpha;

};

#endif /* SELECTIONVIEW_H_ */
