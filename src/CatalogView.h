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
	bool isInside(const QPoint &pos);
    bool mousePressEvent(QMouseEvent *event);
    bool mouseMoveEvent(QMouseEvent *event);
    bool mouseReleaseEvent ( QMouseEvent * event );
    bool mouseDoubleClickEvent ( QMouseEvent * event );
    bool wheelEvent ( QWheelEvent * event );

	// Specific implementation
	void setVisible(bool on);
	bool visible() { return _visible;}
	void setTransparent(bool on) { _alpha = on ? 0.5 : 1.0; }
	typedef enum { SMALL = 0, MEDIUM = 1, LARGE = 2 } catalogSize;
	void setSize(catalogSize s);
	catalogSize getSize() { return _currentSize; }

	// drawing
	void drawSource(Source *s, int index);


private:
	bool _visible;
	double _size[3], _iconSize[3], _largeIconSize[3];
	catalogSize _currentSize;
	double _width, _height, h_unit, v_unit;
	float _alpha;
	int first_index, last_index;
	double _clicX, _clicY;
};

#endif /* SELECTIONVIEW_H_ */
