#include <QtGlobal>
#include <QDebug>

#include "Source.h"
#include "RenderingManager.h"
#include "ViewRenderWidget.h"

#include "HistoryManager.moc"


//#define DEBUG_HISTORY

HistoryManager::Event::Event(QObject *o, QMetaMethod m, QVector< QVariantPair > args) : _object(o), _method(m), _iskey(false)
{
    // convert the list of QVariant pairs into list of History Arguments
    foreach ( const QVariantPair &a, args) {
        // create pair of History Arguments as a Vector
        QVector<SourceArgument> argument;
        // set value only for valid QVariant
        if (a.first.isValid()) {
            argument.append( SourceArgument(a.first) );
            argument.append( SourceArgument(a.second) );
        } else {
            // else create empty arguments
            argument.append( SourceArgument() );
            argument.append( SourceArgument() );
        }
        // append the history argument to the list (in order)
        _arguments.append(argument);
    }

#ifdef DEBUG_HISTORY
    qDebug() << "New Event " << _method.signature() << _arguments.size();
#endif
}

void HistoryManager::Event::invoke(HistoryManager::Direction dir) {

#ifdef DEBUG_HISTORY
    qDebug() << "Invoke " << _object->objectName() <<  _method.signature() << dir ;
    qDebug() << _arguments;
#endif

    // invoke the method with all arguments (including empty arguments)
    _method.invoke(_object, Qt::QueuedConnection, _arguments[0][dir].argument(), _arguments[1][dir].argument(), _arguments[2][dir].argument(), _arguments[3][dir].argument(), _arguments[4][dir].argument(), _arguments[5][dir].argument(), _arguments[6][dir].argument() );

}

QString HistoryManager::Event::signature() const {

    return _method.signature();
}

QString HistoryManager::Event::objectName() const {

    return _object->objectName();
}


QString HistoryManager::Event::arguments(Direction dir) const {

    QString args("");

    foreach (QVector<SourceArgument> a, _arguments) {
        args += " " + a[dir].string();
    }

    return args;
}


HistoryManager::Event & HistoryManager::Event::operator = (const Event & other )
{
    if (this != &other) // protect against invalid self-assignment
    {
        _object = other._object;
        _method = other._method;
        _arguments = other._arguments;
        _iskey = other._iskey;
    }
    return *this;
}

bool HistoryManager::Event::operator == ( const HistoryManager::Event & other ) const
{
    return ( _method.methodIndex() == other._method.methodIndex() && _object == other._object);
}

bool HistoryManager::Event::operator != ( const HistoryManager::Event & other ) const
{
    return ( _method.methodIndex() != other._method.methodIndex() || _object != other._object);
}


HistoryManager::HistoryManager(QObject *parent) : QObject(parent),
    _direction(FORWARD), _play(false), _loop(true), _reverse(true), _maximumSize(10000)
{
    _current = _history.begin();
}


void HistoryManager::clear()
{
    // empty the list
    _history.clear();

    // go to end
    _current = _history.begin();
    _currentTime = 0;

    // inform that history changed
    emit changed();

#ifdef DEBUG_HISTORY
    qDebug() << "HistoryManager " << _history.size() << " elements";
#endif
}


// append history
void HistoryManager::appendEvents(EventMap history)
{
    // append events in multimap
    _history += history;

    _current = _history.begin();
    _currentTime = _current.key();

    // inform that history changed
    emit changed();
}



double HistoryManager::duration() const
{
    QList<qint64> keys = _history.keys();
    if (keys.empty())
        return 0.0;
    else
        return (double) keys.back() / 1000.0;
}


void HistoryManager::updateCursor()
{

    if (_direction == FORWARD)
    {
        _currentTime += _timer.restart();

        // execute all steps until the elapsed time
        while (_current.key() < _currentTime && _current != _history.end()) {
            _current.value().invoke(_direction);
            _current++;
        }

        // loop
        if (_current == _history.end()) {

            if (!_loop)
                play(false);

            if (_reverse) {
                _direction = _direction==FORWARD ? BACKWARD : FORWARD;
                _current--;
            } else {
                _current = _history.begin();
                _currentTime = _current.key();
            }
        }
    }
    else
    {
        _currentTime -= _timer.restart();

        // execute all steps until the elapsed time
        while (_current.key() > _currentTime && _current != _history.begin()) {
            _current.value().invoke(_direction);
            _current--;
        }

        // loop
        if (_current == _history.begin()) {

            if (!_loop)
                play(false);

            if (_reverse) {
                _direction = _direction==FORWARD ? BACKWARD : FORWARD;
            } else {
                _current = _history.end() -1;
                _currentTime = _current.key();
            }
        }
    }

}

void HistoryManager::play(bool on)
{
    // set play mode
    _play = on;

    if (_play) {
        // connect to ticking clock
        connect(RenderingManager::getRenderingWidget(), SIGNAL(tick()), SLOT(updateCursor()));
        _timer.start();
    }
    else {
        // disconnect ticking clock
        RenderingManager::getRenderingWidget()->disconnect(this);
    }

    emit playing(_play);
}

void HistoryManager::rewind()
{
    _current = _history.begin();
    _currentTime = _current.key();
    _direction = FORWARD;

    QList<Event> events = _history.values(_currentTime);
    foreach(Event e, events)
        e.invoke(FORWARD);
}


qint64 HistoryManager::cursorPosition() const
{
    return _current.key();
}

void HistoryManager::setCursorPosition(qint64 t)
{
//    if (t == 0) {
//        _current = _history.begin();
//        _currentTime = _current.key();
//    }
//    else {

//    }
//    _current.value().invoke(_direction);
}

void HistoryManager::setCursorNextPositionForward()
{
    // ignore if no event
    if (_history.empty())
        return;
    if ( _current != _history.end() ) {

        // invoke the previous event
        _current.value().invoke(HistoryManager::FORWARD);

        // move the cursor to the previous event
        _current++;
    }
    // inform that history changed
    emit changed();
}

void HistoryManager::setCursorNextPositionBackward()
{
    // ignore if no event
    if (_history.empty())
        return;
    if ( _current == _history.end() )
        _current--;

    if ( _current != _history.begin() ) {

        // invoke the previous event
        _current.value().invoke(HistoryManager::BACKWARD);

        // move the cursor to the previous event
        _current--;
    }
    // inform that history changed
    emit changed();
}

void HistoryManager::setCursorNextPosition(HistoryManager::Direction dir)
{
    // Backward in time
    if (dir == HistoryManager::BACKWARD) {
        setCursorNextPositionBackward();
    }
    // Forward in time
    else {
        setCursorNextPositionForward();
    }

}

int HistoryManager::maximumSize() const
{
    return _maximumSize;
}

void HistoryManager::setMaximumSize(int max)
{
    _maximumSize = max;
}

//void HistoryManager::undoAll()
//{

////    QMap<qint64, SourceSlotEvent *> temp(_sourceSlotEvents);
////    _sourceSlotEvents.clear();

////    QMap<qint64, SourceSlotEvent *>::const_iterator i = _sourceSlotEvents.constEnd();
////    --i;
////    while (i != _sourceSlotEvents.constBegin()) {

////        SourceSlotEvent *e = i.value();

////        e->invoke();

////        --i;
////    }


//    QMapIterator<qint64, SourceMethodCall *> i(_sourceSlotEvents);
//    i.toBack();
//    while (i.hasPrevious()) {
//        i.previous();
//        i.value()->invoke();
//    }

//    clearAll();
//}

//void HistoryManager::executeAll()
//{

////    QMap<qint64, SourceSlotEvent *> temp(_sourceSlotEvents);
////    _sourceSlotEvents.clear();

//    QMapIterator<qint64, SourceMethodCall *> i(_sourceSlotEvents);
//    while (i.hasNext()) {
//        i.next();
//        i.value()->invoke();
//    }

////    clearAll();
//}


// remove the top key event
//        qint64 k = _keyEvents.pop();

//        // find the action at the undo key
//        QMultiMap<qint64, Event *>::iterator i = _eventHistory.find(k);

//        // do the action of the given key
//        qDebug() << "undo " << i.key() << i.value()->signature() << i.value()->arguments(HistoryManager::BACKWARD);

//        i.value()->invoke(HistoryManager::BACKWARD);

//        // increment the iterator
//        _currentEvent = i++;

//        // remove the remaining events in the  map
//        for (; i != _eventHistory.end(); i = _eventHistory.erase(i)) {

//            qDebug() << "undo delete " << i.key() << i.value()->signature();
//            delete i.value();
//        }



//        if ( i.key() == k) {
//            qDebug() << "undo " << i.key() << i.value()->signature();
//            i.value()->invoke(HistoryManager::BACKWARD);
//        }

//        else {
//            // in the multimap, get the list of all events at the current (past) key
//            QList<Event *> keyCalls = _eventHistory.values( _currentKey );
//            foreach ( Event *call, keyCalls) {
//                // if the new call is different from one of the previous events at the current key
//                if ( *call != *newcall ) {
//                    // declare a new key event
//                    _currentKey = t;
//                    newcall->setKey(true);
//                    break;
//                }
//            }
//        }


