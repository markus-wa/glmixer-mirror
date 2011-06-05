/*
 * View.cpp
 *
 *  Created on: May 21, 2011
 *      Author: bh
 */

#include "View.h"

// list of sources in the selection
SourceList View::_selectedSources;
// dummy source to interact with selection
Source *View::_selectionSource = new Source();

QDomElement View::getConfiguration(QDomDocument &doc) {
	QDomElement viewelem = doc.createElement("View");
	QDomElement z = doc.createElement("Zoom");
	z.setAttribute("value", getZoom());
	viewelem.appendChild(z);
	QDomElement pos = doc.createElement("Panning");
	pos.setAttribute("X", getPanningX());
	pos.setAttribute("Y", getPanningY());
	viewelem.appendChild(pos);

	return viewelem;
}

void View::setConfiguration(QDomElement xmlconfig) {
	setZoom(xmlconfig.firstChildElement("Zoom").attribute("value").toFloat());
	setPanningX(xmlconfig.firstChildElement("Panning").attribute("X").toFloat());
	setPanningY(xmlconfig.firstChildElement("Panning").attribute("Y").toFloat());
}


void View::clearSelection() {
	_selectedSources.clear();
	updateSelectionSource();
}

void View::select(Source *s) {

	if ( View::_selectedSources.count(s) > 0)
		View::_selectedSources.erase(s);
	else
		View::_selectedSources.insert(s);

	updateSelectionSource();
}

void View::deselect(Source *s) {
	if ( _selectedSources.count(s) > 0)
		_selectedSources.erase( s );
	updateSelectionSource();
}

void View::select(SourceList l)
{
	SourceList result;

	std::set_union(View::_selectedSources.begin(), View::_selectedSources.end(), l.begin(), l.end(), std::inserter(result, result.begin()) );

	View::_selectedSources = SourceList(result);
	updateSelectionSource();
}

void View::deselect(SourceList l)
{
	SourceList result;

	std::set_difference(View::_selectedSources.begin(), View::_selectedSources.end(), l.begin(), l.end(), std::inserter(result, result.begin()) );

	View::_selectedSources = SourceList(result);
	updateSelectionSource();
}


void View::setSelection(SourceList l)
{
	View::_selectedSources = SourceList(l);
	updateSelectionSource();
}

bool View::isInSelection(Source *s)
{
	return View::_selectedSources.count(s) > 0;
}


void View::updateSelectionSource()
{
	// prepare vars
	GLdouble point[2], center[2], bbox[2][2];
	GLdouble cosa, sina, d;
	d = 0;

	// compute center of all sources in the list
	center[0] = 0;
	center[1] = 0;
	for(SourceList::iterator  its = _selectedSources.begin(); its != _selectedSources.end(); its++) {
		center[0] += (*its)->getX();
		center[1] += (*its)->getY();
	}
	center[0] /= _selectedSources.size();
	center[1] /= _selectedSources.size();

	// init bbox to max size
	bbox[0][0] = SOURCE_UNIT * 5.0;
	bbox[0][1] = SOURCE_UNIT * 5.0;
	bbox[1][0] = -SOURCE_UNIT * 5.0;
	bbox[1][1] = -SOURCE_UNIT * 5.0;
	// compute bounding box of all sources in the list
	for(SourceList::iterator  its = _selectedSources.begin(); its != _selectedSources.end(); its++) {
		cosa = cos((*its)->getRotationAngle() / 180.0 * M_PI);
		sina = sin((*its)->getRotationAngle() / 180.0 * M_PI);
		for (GLdouble i = -1.0; i < 2.0; i += 2.0)
			for (GLdouble j = -1.0; j < 2.0; j += 2.0) {
				// corner with apply rotation
				point[0] = i * (*its)->getScaleX() * cosa - j * (*its)->getScaleY() * sina + (*its)->getX();
				point[1] = j * (*its)->getScaleY() * cosa + i * (*its)->getScaleX() * sina + (*its)->getY();
				// keep max and min
				bbox[0][0] = qMin( (point[0] - center[0]), bbox[0][0]);
				bbox[0][1] = qMin( (point[1] - center[1]), bbox[0][1]);
				bbox[1][0] = qMax( (point[0] - center[0]), bbox[1][0]);
				bbox[1][1] = qMax( (point[1] - center[1]), bbox[1][1]);
			}
	}

	View::selectionSource()->setScaleX( qMax( ABS(bbox[0][0]), ABS(bbox[1][0]) ));
	View::selectionSource()->setScaleY( qMax( ABS(bbox[0][1]), ABS(bbox[1][1]) ));

	View::selectionSource()->setX( center[0] );
	View::selectionSource()->setY( center[1] );

	View::selectionSource()->setRotationAngle( 0 );
}


bool View::keyPressEvent(QKeyEvent * event) {

	return false;
}

bool View::keyReleaseEvent(QKeyEvent * event) {

	return false;
}

void View::setAction(actionType a){

	previousAction = currentAction;
	currentAction = a;
}


void View::coordinatesFromMouse(int mouseX, int mouseY, double *X, double *Y){

	double dum;
	gluUnProject((GLdouble) mouseX, (GLdouble) (viewport[3] - mouseY), 0.0,
	            modelview, projection, viewport, X, Y, &dum);

}
