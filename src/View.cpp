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
// maps for button and modifiers
QMap<View::UserInput,Qt::MouseButtons> View::_buttonmap = View::defaultMouseButtonsMap();
QMap<View::UserInput,Qt::KeyboardModifiers> View::_modifiermap = View::defaultModifiersMap();
bool View::zoomcentered = false;
float View::zoomspeed = 120.0;


#if QT_VERSION > 0x040602
#define QTMIDDLEBUTTON Qt::MiddleButton
#else
#define QTMIDDLEBUTTON Qt::MidButton
#endif

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
	setZoom(xmlconfig.firstChildElement("Zoom").attribute("value", "1").toFloat());
	setPanning(xmlconfig.firstChildElement("Panning").attribute("X", "0").toFloat(), xmlconfig.firstChildElement("Panning").attribute("Y", "0").toFloat());
}


void View::clearSelection() {
	_selectedSources.clear();
	updateSelectionSource();
}

void View::setPanning(float x, float y, float z) {
	panx = CLAMP(x, -maxpanx, maxpanx);
	pany = CLAMP(y, - maxpany, maxpany);
	if (z > 0)
		panz = CLAMP(z, - maxpanz, maxpanz);

	setModelview();
	modified = true;
}

void View::select(Source *s) {

	// Do not consider the _selectionSource in a selection
	if (s == _selectionSource)
		return;

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
	// Do not consider the _selectionSource in a selection
	l.erase(View::_selectionSource);
	// generate new set as union of current selection and give list
	SourceList result;
	std::set_union(View::_selectedSources.begin(), View::_selectedSources.end(), l.begin(), l.end(), std::inserter(result, result.begin()) );
	// set new selection
	View::_selectedSources = SourceList(result);
	updateSelectionSource();
}

void View::deselect(SourceList l)
{
	// Do not consider the _selectionSource in a selection
	l.erase(View::_selectionSource);
	// generate new set as difference of current selection and give list
	SourceList result;
	std::set_difference(View::_selectedSources.begin(), View::_selectedSources.end(), l.begin(), l.end(), std::inserter(result, result.begin()) );
	// set new selection
	View::_selectedSources = SourceList(result);
	updateSelectionSource();
}


void View::setSelection(SourceList l)
{
	// Do not consider the _selectionSource in a selection
	l.erase(View::_selectionSource);
	// set new selection
	View::_selectedSources = SourceList(l);
	updateSelectionSource();
}

bool View::isInSelection(Source *s)
{
	return View::_selectedSources.count(s) > 0;
}


void View::computeBoundingBox(const SourceList &l, double bbox[2][2])
{
	double cosa, sina;
	double point[2];

	// init bbox to max size
	bbox[0][0] = SOURCE_UNIT * 5.0;
	bbox[0][1] = SOURCE_UNIT * 5.0;
	bbox[1][0] = -SOURCE_UNIT * 5.0;
	bbox[1][1] = -SOURCE_UNIT * 5.0;

	// compute Axis aligned bounding box of all sources in the list
	for(SourceList::iterator  its = l.begin(); its != l.end(); its++) {
    	if ((*its)->isStandby())
    		continue;
		cosa = cos((*its)->getRotationAngle() / 180.0 * M_PI);
		sina = sin((*its)->getRotationAngle() / 180.0 * M_PI);
		for (GLdouble i = -1.0; i < 2.0; i += 2.0)
			for (GLdouble j = -1.0; j < 2.0; j += 2.0) {
				// corner with apply rotation
				point[0] = i * (*its)->getScaleX() * cosa - j * (*its)->getScaleY() * sina + (*its)->getX();
				point[1] = j * (*its)->getScaleY() * cosa + i * (*its)->getScaleX() * sina + (*its)->getY();
				// keep max and min
				bbox[0][0] = qMin( point[0], bbox[0][0]);
				bbox[0][1] = qMin( point[1], bbox[0][1]);
				bbox[1][0] = qMax( point[0], bbox[1][0]);
				bbox[1][1] = qMax( point[1], bbox[1][1]);
			}
	}
}

void View::updateSelectionSource()
{
	// prepare vars
	GLdouble point[2], bbox[2][2];

	if (_selectedSources.empty())
		return;

	computeBoundingBox(_selectedSources, bbox);

	point[0] = (bbox[1][0] - bbox[0][0]) / 2.0;
	point[1] = (bbox[1][1] - bbox[0][1]) / 2.0;

	View::selectionSource()->setScaleX( point[0] );
	View::selectionSource()->setScaleY( point[1] );

	View::selectionSource()->setX( bbox[0][0] + point[0] );
	View::selectionSource()->setY( bbox[0][1] + point[1] );

	View::selectionSource()->setRotationAngle( 0 );
}


bool View::keyPressEvent(QKeyEvent * event) {

	return false;
}

bool View::keyReleaseEvent(QKeyEvent * event) {

	return false;
}

void View::setAction(ActionType a){

	if (currentAction != View::SELECT)
		previousAction = currentAction;
	currentAction = a;
}


void View::coordinatesFromMouse(int mouseX, int mouseY, double *X, double *Y){

	double dum;
	gluUnProject((GLdouble) mouseX, (GLdouble) (viewport[3] - mouseY), 0.0,
	            modelview, projection, viewport, X, Y, &dum);

}


QString View::userInputLabel(View::UserInput ab, bool verbose){

	if (verbose) {
		switch(ab) {
		case INPUT_ZOOM:
			return QObject::tr("Zoom in a source or zoom out to get an overview.");
		case INPUT_CONTEXT_MENU:
			return QObject::tr("Show the context menu.");
		case INPUT_NAVIGATE:
			return QObject::tr("Navigate by dragging the background (the sources follow).");
		case INPUT_DRAG:
			return QObject::tr("Drag the background independently from the sources.");
		case INPUT_SELECT:
			return QObject::tr("Add a source to the selection.");
		case INPUT_TOOL:
			return QObject::tr("Use the current tool (grab, scale, etc.) on the whole selection.");
		case INPUT_TOOL_INDIVIDUAL:
			return QObject::tr("Use the current tool (grab, scale, etc.) on one source at a time.");
		default:
			return QObject::tr("Do Nothing");
		}
	} else {
		switch(ab) {
		case INPUT_ZOOM:
			return QObject::tr("Zoom");
		case INPUT_CONTEXT_MENU:
			return QObject::tr("Context\nmenu");
		case INPUT_NAVIGATE:
			return QObject::tr("Navigate");
		case INPUT_DRAG:
			return QObject::tr("Drag");
		case INPUT_SELECT:
			return QObject::tr("Select");
		case INPUT_TOOL:
			return QObject::tr("Tool");
		case INPUT_TOOL_INDIVIDUAL:
			return QObject::tr("Tool\n(individual)");
		default:
			return QObject::tr("Do Nothing");
		}
	}
}


QMap<int, int> View::getMouseButtonsMap(QMap<View::UserInput,Qt::MouseButtons> from)
{
	QMap<int, int> intmap;
	QMapIterator<View::UserInput,Qt::MouseButtons> i(from);
	while ( i.hasNext() ) {
		i.next();
		intmap[ (int) i.key() ] = int( i.value() );
	}

	return intmap;
}

QMap<int, int> View::getMouseModifiersMap(QMap<View::UserInput,Qt::KeyboardModifiers> from)
{
	QMap<int, int> intmap;
	QMapIterator<View::UserInput,Qt::KeyboardModifiers> i(from);
	while ( i.hasNext() ) {
		i.next();
		intmap[ (int) i.key() ] = int( i.value() );
	}

	return intmap;
}

void View::setMouseButtonsMap(QMap<int,int> m)
{
	QMapIterator<int,int> i(m);
	while ( i.hasNext() ) {
		i.next();
		 _buttonmap [ (View::UserInput) i.key() ] = Qt::MouseButtons(i.value());
	}
}

void View::setMouseModifiersMap(QMap<int,int> m)
{
	QMapIterator<int,int> i(m);
	while ( i.hasNext() ) {
		i.next();
	     _modifiermap [ (View::UserInput) i.key() ] = Qt::KeyboardModifiers(i.value());
	}
}

QMap<View::UserInput,Qt::MouseButtons> View::defaultMouseButtonsMap()
{
	QMap<View::UserInput,Qt::MouseButtons> qbuttonmap;
	qbuttonmap[View::INPUT_TOOL] = Qt::LeftButton;
	qbuttonmap[View::INPUT_TOOL_INDIVIDUAL] = Qt::LeftButton;
	qbuttonmap[View::INPUT_NAVIGATE] = QTMIDDLEBUTTON;
	qbuttonmap[View::INPUT_DRAG] = QTMIDDLEBUTTON;
	qbuttonmap[View::INPUT_SELECT] = Qt::LeftButton;
	qbuttonmap[View::INPUT_CONTEXT_MENU] = Qt::RightButton;
	qbuttonmap[View::INPUT_ZOOM] = Qt::NoButton;
	return qbuttonmap;
}

QMap<View::UserInput,Qt::KeyboardModifiers> View::defaultModifiersMap()
{
	QMap<View::UserInput,Qt::KeyboardModifiers> qmodifiermap;
	qmodifiermap[View::INPUT_TOOL] = Qt::NoModifier;
	qmodifiermap[View::INPUT_TOOL_INDIVIDUAL] = Qt::ShiftModifier;
	qmodifiermap[View::INPUT_NAVIGATE] = Qt::NoModifier;
	qmodifiermap[View::INPUT_DRAG] = Qt::ShiftModifier;
	qmodifiermap[View::INPUT_SELECT] = Qt::ControlModifier;
	qmodifiermap[View::INPUT_CONTEXT_MENU] = Qt::NoModifier;
	qmodifiermap[View::INPUT_ZOOM] = Qt::NoModifier;
	return qmodifiermap;
}


bool View::isUserInput(QMouseEvent *event, UserInput ab)
{
	// NOT XOR is true when both are true or both are null
	return (  !(event->buttons() ^ _buttonmap[ab]) && !(QApplication::keyboardModifiers() ^ _modifiermap[ab] ) );
}


QString View::userInputDescription(View::UserInput ab, QMap<View::UserInput,Qt::MouseButtons> bm, QMap<View::UserInput,Qt::KeyboardModifiers> mm){

	QString text;

	if (mm[ab] & Qt::SHIFT)
		text.append("[SHIFT] + ");
	if (mm[ab] & Qt::ALT)
		text.append("[ALT] + ");

#ifdef __APPLE__
	if (mm[ab] & Qt::META)
		text.append("[CTRL] + ");
	if (mm[ab] & Qt::CTRL)
		text.append("[CMD] + ");
#else
	if (mm[ab] & Qt::META)
		text.append("[WIN] + ");
	if (mm[ab] & Qt::CTRL)
		text.append("[CTRL] + ");
#endif

	if (bm[ab] & Qt::LeftButton )
		text.append("Left");
	if (bm[ab] & Qt::RightButton )
		text.append("Right");
	if (bm[ab] & QTMIDDLEBUTTON)
		text.append("Middle");
	if (bm[ab] & Qt::XButton1 )
		text.append("1st extra");
	if (bm[ab] & Qt::XButton2)
		text.append("2nd extra");

	if (text.isEmpty())
		text.append("Not attributed.");
	else
		text.append(" mouse button.");

	return text;
}

void View::setZoomSpeed(float zs){
	zoomspeed = CLAMP(zs, 20.0, 220.0);
}
float View::zoomSpeed(){
	return zoomspeed;
}
void View::setZoomCentered(bool on){
	zoomcentered = on;
}
bool View::zoomCentered(){
	return zoomcentered;
}

