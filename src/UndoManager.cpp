#include "UndoManager.moc"

#include "Source.h"
#include "RenderingManager.h"

// static members
UndoManager *UndoManager::_instance = 0;



UndoManager *UndoManager::getInstance() {

        if (_instance == 0) {
                _instance = new UndoManager;
                Q_CHECK_PTR(_instance);
        }

        return _instance;
}

UndoManager::UndoManager() : QObject(), _suspended(false), _maximumSize(100), _counter(0), _index(0)
{

}

UndoManager::~UndoManager() {

}


void UndoManager::clear()
{

}


void UndoManager::undo()
{
    int index = _index > 0 ? _index - 1  : 0;
    restore(index);
}


void UndoManager::redo()
{
    int index = _index < _counter ? _index + 1  : _counter;
    restore(index);
}


void UndoManager::restore(int i)
{
    // set index
    _index = qBound(0, i, _counter);

    // get status at index
    QDomElement root = _history.firstChildElement(QString("%1").arg(_counter));
    if ( !root.isNull()) {
        qDebug() << root.text();
    }
}

void UndoManager::store()
{
    _suspended = false;

    // go back to index
    _counter = _index;
    // increment counter
    _counter++;
    // TODO : max


    // set index to current counter
    _index = _counter;


    // get the configuration
    QDomElement renderConfig = RenderingManager::getInstance()->getConfiguration(_history);
    if (!renderConfig.isNull()) {
        QDomElement root = _history.createElement( QString("%1").arg(_counter));
        root.appendChild(renderConfig);
        _history.appendChild(root);

        fprintf(stderr, "stored status %d\n", _counter);
    }

    // todo : get view config?? other config ??


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
    _suspended = true;
}


void UndoManager::store(QString signature, QVariantPair, QVariantPair, QVariantPair, QVariantPair, QVariantPair, QVariantPair, QVariantPair)
{
    if (_suspended)
        return;

//    fprintf(stderr, "store %s\n", qPrintable(signature));

    QObject *sender_object = sender();
    if (sender_object) {
        if (_previousSender.isEmpty() || _previousSignature.isEmpty()) {
            // no previous sender or previous signature; its a new event!
            store();
        }
        // we have a previous sender and a previous signature
        else {
            // if the new sender is not the same as previous
            // OR if the method called is different from previous
            if (sender_object->objectName() != _previousSender || signature != _previousSignature ) {
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
