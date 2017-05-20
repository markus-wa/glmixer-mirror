#include "History.h"
#include "Source.h"

History::Event::Event()
{
    _object = NULL;
    _method = QMetaMethod();
}

History::Event::Event(const Event &e)
{
    _object = e._object;
    _method = e._method;
    _arguments = e._arguments;
}

History::Event::Event(QObject *o, QMetaMethod m, QVector< QVariantPair > args) : _object(o), _method(m)
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

void History::Event::invoke(History::Direction dir) {

#ifdef DEBUG_HISTORY
    qDebug() << "Invoke " << _object->objectName() <<  _method.signature() << dir ;
    qDebug() << _arguments;
#endif

    if (_object) {
        // invoke the method with all arguments (including empty arguments)
        _method.invoke(_object, Qt::QueuedConnection,
                       _arguments[0][dir].argument(),
                _arguments[1][dir].argument(),
                _arguments[2][dir].argument(),
                _arguments[3][dir].argument(),
                _arguments[4][dir].argument(),
                _arguments[5][dir].argument(),
                _arguments[6][dir].argument() );
    }

}

QString History::Event::signature() const {

    return _method.signature();
}

QString History::Event::objectName() const {

    if (_object)
        return _object->objectName();
    else
        return "";
}

QString History::Event::arguments(History::Direction dir) const {

    QString args("");

    foreach (QVector<SourceArgument> a, _arguments) {
        args += " " + a[dir].string();
    }

    return args;
}

History::Event & History::Event::operator = (const History::Event & other )
{
    if (this != &other) // protect against invalid self-assignment
    {
        _object = other._object;
        _method = other._method;
        _arguments = other._arguments;
    }
    return *this;
}

bool History::Event::operator == ( const History::Event & other ) const
{
    return ( _object == other._object && _method.methodIndex() == other._method.methodIndex() );
}

bool History::Event::operator != ( const History::Event & other ) const
{
    return ( _object != other._object || _method.methodIndex() != other._method.methodIndex());
}

History::History()
{

}


History & History::operator = (const History & other )
{
    if (this != &other) // protect against invalid self-assignment
    {
        _events = other._events;
        _beginning = other._beginning;
        _ending = other._ending;
    }
    return *this;
}

void History::clear()
{
    _events.clear();
    _beginning.clear();
    _ending.clear();
}

void History::append(qint64 keytime, Event e)
{
    // insert event
    _events.insert(keytime, e);

    // insert unique first key event
    if ( !_beginning.contains( e.objectName()) || _beginning.value(e.objectName()).first > keytime )
        _beginning.insert(e.objectName(), qMakePair(keytime, e) );

    // replace last event
    _ending.insert(e.objectName(), qMakePair(keytime, e) );
}

//void History::append(History h)
//{
//    // insert all event
//    _events += h._events;

//    // insert new beginning key events
//    for (KeyEventMap::iterator i = h._beginning.begin(); i != h._beginning.end(); ++i) {
//        // if this source is not in the list
//        // or if the newly added value is before
//        if ( !_beginning.contains(i.key()) || _beginning.value(i.key()).first > i.value().first)
//            _beginning.insert(i.key(), i.value());
//    }

//    // replace all ending key events
//    for (KeyEventMap::iterator i = h._ending.begin(); i != h._ending.end(); ++i) {
//        _ending.insert(i.key(), i.value());
//    }

//}

double History::duration() const
{
    QList<qint64> keys = _events.keys();
    if (keys.empty())
        return 0.0;
    else
        return (double) keys.back() / 1000.0;
}

History::EventMap::iterator History::begin()
{
    return _events.begin();
}

History::EventMap::iterator History::end()
{
    return _events.end();
}

bool History::empty() const
{
    return _events.empty();
}

int History::size() const
{
    return _events.size();
}

void History::invoke(Direction dir)
{
    if (dir == History::FORWARD)
    {
        for (KeyEventMap::iterator i = _ending.begin(); i != _ending.end(); ++i) {
            i.value().second.invoke(FORWARD);
        }
    }
    else {

        for (KeyEventMap::iterator i = _beginning.begin(); i != _beginning.end(); ++i) {
            i.value().second.invoke(BACKWARD);
        }
    }
}
