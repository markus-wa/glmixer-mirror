#ifndef HISTORY_H
#define HISTORY_H

#include <QMetaMethod>
#include <QMultiMap>
#include <QVector>

#include "ProtoSource.h"


class History
{
    friend class HistoryManager;

public:

    // Direction for navigating in the history
    typedef enum {
        BACKWARD = 0,
        FORWARD
    } Direction;

    // historical event : an object o called the method m with arguments 1 & 2
    class Event {
    public:
        Event();
        Event(const Event &e);
        Event(QObject *o, QMetaMethod m, QVector< QVariantPair > args);

        void invoke(Direction dir = FORWARD);
        QString objectName() const;
        QString signature() const;
        QString arguments(Direction dir = FORWARD) const;

        bool operator == ( const Event & other ) const;
        bool operator != ( const Event & other ) const;
        Event & operator = (const Event & other );

    private:
        QObject *_object;
        QMetaMethod _method;
        QVector< QVector<SourceArgument> > _arguments;
    };

    typedef QMultiMap<qint64, Event> EventMap;
    typedef QMap<QString, QPair<qint64, Event> > KeyEventMap;

    History();
    History & operator = (const History & other );

    void clear();
    void append(qint64 keytime, Event e);
    void invoke(Direction dir = FORWARD);

    EventMap::iterator begin();
    EventMap::iterator end();
    bool empty() const;
    int size() const;
    double duration() const;

private:
    // complete history of events, indexed by time
    EventMap _events;
    // sets of key events, indexed by source
    KeyEventMap _beginning, _ending;

};


typedef History * HistoryStar;
Q_DECLARE_METATYPE(HistoryStar);

#endif // HISTORY_H
