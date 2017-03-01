#include "HistoryRecorder.moc"

#include "RenderingManager.h"

HistoryRecorder::HistoryRecorder(QObject *parent) : QObject(parent)
{
    // connect recorder to all source events
    for(SourceSet::iterator  its = RenderingManager::getInstance()->getBegin(); its != RenderingManager::getInstance()->getEnd(); its++) {

        connect(*its, SIGNAL(methodCalled(QString, QVariantPair, QVariantPair, QVariantPair, QVariantPair, QVariantPair, QVariantPair, QVariantPair)), SLOT(storeEvent(QString, QVariantPair, QVariantPair, QVariantPair, QVariantPair, QVariantPair, QVariantPair, QVariantPair)));
    }

}

HistoryRecorder::~HistoryRecorder()
{
    for(SourceSet::iterator  its = RenderingManager::getInstance()->getBegin(); its != RenderingManager::getInstance()->getEnd(); its++) {

        (*its)->disconnect(this);
    }

    // empty the list
    _history.clear();
}


HistoryManager::EventMap HistoryRecorder::getEvents()
{
    return _history;
}



void HistoryRecorder::storeEvent(QString signature, QVariantPair arg1, QVariantPair arg2, QVariantPair arg3, QVariantPair arg4, QVariantPair arg5, QVariantPair arg6, QVariantPair arg7)
{

    QObject *sender_object = sender();
    if (sender_object) {

        // keep time of the event (only even numbers)
        qint64 t = 0;
        if (_history.empty())
            _timer.start();
        else
            t = _timer.elapsed();
        t += t%2;

        // get meta object of the object sending this slot
        QMetaObject *sender_metaobject = (QMetaObject *) sender_object->metaObject();

        // get slot id
        int methodId = sender_metaobject->indexOfSlot(qPrintable(signature));

        // get the method of this meta class
        QMetaMethod method = sender_metaobject->method( methodId );

        // list of arguments
        QVector< QVariantPair > arguments;
        arguments << arg1 << arg2 << arg3 << arg4 << arg5 << arg6 << arg7;

        // create an object storing this method call
        HistoryManager::Event newcall(sender_object, method, arguments );

        // remember the event in the history
        _history.insert(t, newcall);


#ifdef DEBUG_HISTORY
        qDebug() << "Stored " << t << sender_metaobject->className() << newcall->signature() << newcall->arguments();
#endif

    }
}

