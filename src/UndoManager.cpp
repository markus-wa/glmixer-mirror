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

UndoManager::UndoManager() : QObject(), _status(ACTIVE), _maximumSize(100), _firstIndex(1), _lastIndex(0), _currentIndex(0)
{

}

UndoManager::~UndoManager() {

}


void UndoManager::clear()
{

    fprintf(stderr, "CLEAR \n");

    //clear history
    _history.clear();
    _firstIndex = 1;
    _currentIndex = 0;
    _lastIndex = 0;

    _status = ACTIVE;
    _previousSender = QString();
    _previousSignature = QString();

    // store initial status
    store();
}


void UndoManager::undo()
{
    if (_currentIndex > _firstIndex)
        restore(_currentIndex - 1);
}


void UndoManager::redo()
{
    if (_currentIndex < _lastIndex)
        restore(_currentIndex + 1);
}


void UndoManager::restore(int i)
{
    fprintf(stderr, "restore %d ?", i);

    // nothing to do
    if (_currentIndex == i )
        return;

    // set index
    _currentIndex = qBound(_firstIndex, i, _lastIndex);

    fprintf(stderr, " restoring %d [%d %d] ", _currentIndex, _firstIndex, _lastIndex);


//    if (true) {
//        QFile file("/home/bh/testhistory.xml");
//        if (!file.open(QFile::WriteOnly | QFile::Text) ) {
//            return;
//        }
//        QTextStream out(&file);
//        _history.save(out, 4);
//        file.close();
//    }

    // create a list of existing sources

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
                // mark source as updated

                // read next source
                child = child.nextSiblingElement("Source");
            }
        }
        else
            qDebug() << "sourcelists is empty";
    }
    else
        qDebug() << "root is null";


//    RenderingManager::getPropertyBrowserWidget()->showProperties(RenderingManager::getInstance()->getCurrentSource());

    _previousSender = QString();
    _previousSignature = QString();

    fprintf(stderr, "  DONE !\n");
}

void UndoManager::store()
{
    fprintf(stderr, "  store ? {%d}\n", _status);

    if (_status != IDLE) {

        // remove all stored status after _index
        for (int i = _currentIndex + 1; i <=_lastIndex; ++i) {
            QDomElement elem = _history.firstChildElement(QString("%1").arg(i));
            _history.removeChild(elem);
//            fprintf(stderr, "remove future index %d\n", i);
        }

        // go back to index
        _lastIndex = _currentIndex;

        // increment counter
        _lastIndex++;

        // get the configuration
        QDomElement renderConfig = RenderingManager::getInstance()->getConfiguration(_history);
        if (!renderConfig.isNull()) {
            QDomElement root = _history.createElement( QString("%1").arg(_lastIndex));
            root.appendChild(renderConfig);
            _history.appendChild(root);
        }

        // remove old event if counter exceed maximum
        if (_lastIndex-_firstIndex > _maximumSize) {
            QDomElement elem = _history.firstChildElement(QString("%1").arg(_firstIndex));
            _history.removeChild(elem);
//            fprintf(stderr, "remove old index %d\n", _firstIndex);
            _firstIndex++;
        }

        // set index to current counter
        _currentIndex = _lastIndex;

        fprintf(stderr, "=>stored index %d [%d %d]\n", _currentIndex, _firstIndex, _lastIndex);
    }

    // reactivate the undo manager
    _status = ACTIVE;

//    if (_counter%10) {
//        QFile file("/home/bh/testhistory.xml");
//        if (!file.open(QFile::WriteOnly | QFile::Text) ) {
//            return;
//        }
//        QTextStream out(&file);
//        _history.save(out, 4);
//        file.close();
//    }
}


void UndoManager::suspend()
{
    _status = IDLE;

    fprintf(stderr, "  suspend  {%d}\n", _status);
}


void UndoManager::store(QString signature)
{

    fprintf(stderr, "  store %s ? {%d} ", qPrintable(signature), _status);

    // do nothing if manager is suspended
    // but remember that something happened
    if (_status != ACTIVE) {
        _status = PENDING;

        fprintf(stderr, "  NO !\n");
        return;
    }

    fprintf(stderr, "  YES !\n");

    // Check the event and determine if this event should be stored
    QObject *sender_object = sender();
    if (sender_object) {

        // if no previous sender or no previous signature; its a new event!
        if (_previousSender.isEmpty() || _previousSignature.isEmpty()) {
            store();
        }
        // we have a previous sender and a previous signature
        else {
            // if the new sender is not the same as previous
            // OR if the method called is different from previous
            if (sender_object->objectName() != _previousSender
                    || signature != _previousSignature ) {
                // this event is different from previous
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
