/*
 * SelectionManager.cpp
 *
 *  Created on: Feb 10, 2013
 *      Author: bh
 */

#include "SelectionManager.moc"

#include "RenderingManager.h"
#include "ViewRenderWidget.h"
#include "MixerView.h"
#include "GeometryView.h"
#include "WorkspaceManager.h"


// static members
SelectionManager *SelectionManager::_instance = 0;



SelectionManager *SelectionManager::getInstance() {

    if (_instance == 0) {
        _instance = new SelectionManager;
        Q_CHECK_PTR(_instance);
    }

    return _instance;
}


SelectionManager::SelectionManager() : QObject() {

    // dummy source to interact with selection
    _selectionSource = new Source();
}

SelectionManager::~SelectionManager() {

    delete _selectionSource;
}



void SelectionManager::clearSelection() {
    _selectedSources.clear();
    // update the selection source for geometry view
    updateSelectionSource();

}

void SelectionManager::select(Source *s) {

    // Do not consider the _selectionSource in a selection
    if (!s || s == _selectionSource )
        return;

    // do not select source from other workspace
    if ( WorkspaceManager::getInstance()->current() != s->getWorkspace() )
        return;

    if ( _selectedSources.count(s) > 0)
        _selectedSources.erase(s);
    else
        _selectedSources.insert(s);

    // update the selection source for geometry view
    updateSelectionSource();
}

void SelectionManager::deselect(Source *s) {
    if ( _selectedSources.count(s) > 0)
        _selectedSources.erase( s );
    // update the selection source for geometry view
    updateSelectionSource();
}

void SelectionManager::select(SourceList l)
{
    if (l.empty())
        return;
    // Do not consider the _selectionSource in a selection
    l.erase(SelectionManager::_selectionSource);
    // generate new set as union of current selection and give list
    SourceList result;
    std::set_union(_selectedSources.begin(), _selectedSources.end(), l.begin(), l.end(), std::inserter(result, result.begin()) );
    // set new selection
    _selectedSources = SourceList(result);
    // update the selection source for geometry view
    updateSelectionSource();
}

void SelectionManager::deselect(SourceList l)
{
    if (l.empty())
        return;
    // Do not consider the _selectionSource in a selection
    l.erase(SelectionManager::_selectionSource);
    // generate new set as difference of current selection and give list
    SourceList result;
    std::set_difference(_selectedSources.begin(), _selectedSources.end(), l.begin(), l.end(), std::inserter(result, result.begin()) );
    // set new selection
    _selectedSources = SourceList(result);
    // update the selection source for geometry view
    updateSelectionSource();
}


void SelectionManager::setSelection(SourceList l)
{
    if (l.empty())
        clearSelection();
    else {
        // Do not consider the _selectionSource in a selection
        l.erase(SelectionManager::_selectionSource);
        // set new selection
        _selectedSources = SourceList(l);
        // update the selection source for geometry view
        updateSelectionSource();
    }
}

bool SelectionManager::isInSelection(Source *s) const
{
    return _selectedSources.count(s) > 0;
}


void SelectionManager::updateSelectionSource()
{
    // do not add sources which are not in current workspace
    for ( SourceSet::const_iterator sit = _selectedSources.begin(); sit != _selectedSources.end(); ) {
        if ( WorkspaceManager::getInstance()->current() != (*sit)->getWorkspace() )
            _selectedSources.erase(sit++);
        else
            ++sit;
    }

    // inform of the status of the selection
    emit selectionChanged( !_selectedSources.empty());

    // ignore if no selected sources
    if (_selectedSources.empty())
        return;

    // get the bounding box of the selection
    QRectF bbox = GeometryView::getBoundingBox(_selectedSources);

    // setup the selection source to size and position of the bbox
    _selectionSource->setScaleX( bbox.width() / 2.0 );
    _selectionSource->setScaleY( bbox.height() / 2.0 );
    _selectionSource->setX( bbox.center().x() );
    _selectionSource->setY( bbox.center().y() );
    _selectionSource->setRotationAngle( 0 );

    // setup the selection source alpha coordinates
    QPointF gc = MixerView::getGravityCenter(_selectedSources);
    _selectionSource->setAlphaCoordinates(gc.x(), gc.y());

}


void SelectionManager::selectAll()
{
    SourceList sl;
    SourceSet::iterator sit = RenderingManager::getInstance()->getBegin();

    for ( ; RenderingManager::getInstance()->notAtEnd(sit); sit++)
        sl.insert(*sit);
    select(sl);
}

void SelectionManager::invertSelection()
{
    SourceSet::iterator sit = RenderingManager::getInstance()->getBegin();

    for ( ; RenderingManager::getInstance()->notAtEnd(sit); sit++)
        select(*sit);
}

void SelectionManager::selectCurrentSource()
{
    SourceSet::iterator sit = RenderingManager::getInstance()->getCurrentSource();
    if ( RenderingManager::getInstance()->isValid(sit)) {
        if (isInSelection(*sit))
            deselect(*sit);
        else
            select(*sit);
    }
}
