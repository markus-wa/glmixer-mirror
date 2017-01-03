#include "UndoManager.moc"

#include "Source.h"
#include "RenderingManager.h"
#include "SourcePropertyBrowser.h"

// static members
UndoManager *UndoManager::_instance = 0;

UndoManager *UndoManager::getInstance() {

    if (_instance == 0) {
        _instance = new UndoManager;
        Q_CHECK_PTR(_instance);
    }

    return _instance;
}

UndoManager::UndoManager() : QObject(), _status(ACTIVE), _firstIndex(-1), _lastIndex(-1), _currentIndex(-1), _maximumSize(100)
{

}

UndoManager::~UndoManager() {

    clear();
}

void UndoManager::setMaximumSize(int m)
{
    _status = m > 1 ? ACTIVE : DISABLED;
    _maximumSize = m;
}

void UndoManager::clear()
{
    fprintf(stderr, "CLEAR \n");

    //clear history
    _history.clear();
    _firstIndex = -1;
    _currentIndex = -1;
    _lastIndex = -1;

    _status = ACTIVE;
    _previousSender = QString();
    _previousSignature = QString();

}


void UndoManager::undo()
{
    if (_status > DISABLED) {

        // store status in case an action is pending
        // i.e. undo action marks the end of previous actions
        if (_status < ACTIVE) {
            store();
        }

        // if there is an index before current
        if (_currentIndex > _firstIndex) {
            // undo
            restore(_currentIndex  - 1);
        }

    }
}


void UndoManager::redo()
{
    if (_status > DISABLED) {

        // if there is an index after current
        if (_currentIndex < _lastIndex)
            // redo
            restore(_currentIndex + 1);

    }
}


void UndoManager::restore(long int i)
{
    fprintf(stderr, "restore %ld ?", i);

    // nothing to do
    if (_status == DISABLED || _currentIndex == i )
        return;

    // set index
    _currentIndex = qBound(_firstIndex, i, _lastIndex);

    fprintf(stderr, " restoring %ld [%ld %ld] ", _currentIndex, _firstIndex, _lastIndex);

    // TODO create a list of existing sources

    // get status at index
    QDomElement root = _history.firstChildElement(QString("%1").arg(_currentIndex));
    if ( !root.isNull()) {

        // TODO . use setConfiguration of Rendering Manager

        QDomElement renderConfig = root.firstChildElement("SourceList");
        if ( !renderConfig.isNull()) {
            QDomElement child = renderConfig.firstChildElement("Source");
            while (!child.isNull()) {

                QString sourcename = child.attribute("name");
                SourceSet::iterator sit = RenderingManager::getInstance()->getByName(sourcename);
                if ( RenderingManager::getInstance()->isValid(sit) ) {
                    if ( !(*sit)->setConfiguration(child) )
                        qDebug() << "failed";
                }


                fprintf(stderr, " %s ", qPrintable(sourcename));
                // TODO remove source in list of existing

                // read next source
                child = child.nextSiblingElement("Source");
            }
        }
        else
            qDebug() << "sourcelists is empty";
    }
    else
        qDebug() << "root is null";

    // TODO delete sources which do not exist anymore (remain in list of existing).


    _previousSender = QString();
    _previousSignature = QString();

    fprintf(stderr, "  DONE !\n");

    _status = ACTIVE;

    RenderingManager::getInstance()->refreshCurrentSource();
}

void UndoManager::store()
{
    // nothing to do
    if (_status == DISABLED)
        return;

    fprintf(stderr, "  store ? {%d}\n", _status);

    // store only if not idle
    if (_status > IDLE) {

        _currentIndex++;

        addHistory(_currentIndex);

        // remove old event if counter exceed maximum
        if (_lastIndex-_firstIndex > _maximumSize) {
            QDomElement elem = _history.firstChildElement(QString("%1").arg(_firstIndex));
            _history.removeChild(elem);
            fprintf(stderr, "remove old index %ld\n", _firstIndex);
            _firstIndex++;
        }

        if (_firstIndex < 0)
            _firstIndex = 0;

        fprintf(stderr, "=>stored index %ld [%ld %ld]\n", _currentIndex, _firstIndex, _lastIndex);
    }

    // if suspended, do not store next event
    if (_status == PENDING) {
        _status = IDLE;
    }

}


void UndoManager::store(QString signature)
{
    // nothing to do
    if (_status == DISABLED)
        return;

    // Check the event and determine if this event should be stored
    QObject *sender_object = sender();
    if (sender_object) {

        // if no previous sender or no previous signature; its a new event!
        if (_previousSender.isEmpty() || _previousSignature.isEmpty()) {
            fprintf(stderr, "  store %s  {%d} ", qPrintable(signature), _status);
            store();
        }
        // we have a previous sender and a previous signature
        else {
            // if the new sender is not the same as previous
            // OR if the method called is different from previous
            if (sender_object->objectName() != _previousSender
                    || signature != _previousSignature ) {
                // this event is different from previous
                fprintf(stderr, "  store %s  {%d} ", qPrintable(signature), _status);
                store();
            }
            // do not store event if was same previous sender and signature
        }

        // remember sender
        _previousSender = sender_object->objectName();
        // remember signature
        _previousSignature = signature;
    }


}



void UndoManager::addHistory(long int index)
{
    // remove all history after current index
    for (long int i = _lastIndex; i >= index; --i) {
        QDomElement elem = _history.firstChildElement(QString("%1").arg(i));
        _history.removeChild(elem);
        fprintf(stderr, "remove future index %ld\n", i);
    }

    // add the configuration
    QDomElement renderConfig = RenderingManager::getInstance()->getConfiguration(_history);
    if (!renderConfig.isNull()) {
        QDomElement root = _history.createElement( QString("%1").arg(index));
        root.appendChild(renderConfig);
        _history.appendChild(root);
    }

    // remember last added index
    _lastIndex = index;
}


void UndoManager::suspend()
{
    _status = PENDING;

    fprintf(stderr, "  suspend  {%d}\n", _status);
}


void UndoManager::unsuspend()
{
    _status = READY;

    _previousSender = QString();
    _previousSignature = QString();

    fprintf(stderr, "  unsuspend  {%d}\n", _status);
}
