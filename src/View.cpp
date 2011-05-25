/*
 * View.cpp
 *
 *  Created on: May 21, 2011
 *      Author: bh
 */

#include "View.h"

SourceList View::selectedSources;

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


void View::removeFromSelection(Source *s) {
	if ( selectedSources.count(s) > 0)
		selectedSources.erase( s );
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
