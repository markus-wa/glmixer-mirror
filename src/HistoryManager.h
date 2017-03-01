#ifndef HISTORYMANAGER_H
#define HISTORYMANAGER_H

#include <QVariant>
#include <QDebug>
#include <QMultiMap>
#include <QRectF>
#include <QColor>
#include <QMetaMethod>
#include <QStack>
#include <QElapsedTimer>

#include "ProtoSource.h"


/*
 * History manager stores a list of events (method call)
 * and allows to append, remove or modify events in the list.
 *
 * It can take input from History Recorder to add elements
 * to the list (append events).
 *
 * It manages a cursor into the list of events to allow
 * executing the commands (invoke event).
 *
 * */

class HistoryManager : public QObject
{
    Q_OBJECT

    friend class HistoryManagerModel;

public:
    HistoryManager(QObject *parent = 0);

    // Direction for navigating in the history
    typedef enum {
        BACKWARD = 0,
        FORWARD
    } Direction;

    // historical event : an object o called the method m with arguments 1 & 2
    class Event {
    public:
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
        bool _iskey;
    };

    typedef QMultiMap<qint64, Event> EventMap;


    // append history
    void appendEvents(EventMap history);

    // duration
    double duration() const;

    // playback
    bool isPlaying() const { return _play; }
    bool isLoop() const { return _loop; }
    bool isReverse() const { return _reverse; }

    // get maximum size
    int maximumSize() const;

signals:
    void changed();
    void playing(bool);

public slots:

    // clear the history
    void clear();

    // navigate into history
    qint64 cursorPosition() const;
    void setCursorPosition(qint64 t = 0);
    void setCursorNextPosition(Direction dir);
    void setCursorNextPositionForward();
    void setCursorNextPositionBackward();

    // control playback
    void rewind();
    void play(bool on);
    void loop(bool on) { _loop = on; }
    void reverse(bool on) { _reverse = on; }

    // synchronize with display
    void updateCursor();

    // set the maximum number of items in the history
    void setMaximumSize(int max);

private:

    EventMap _history;

    // replay toolbox
    EventMap::iterator _current;
    qint64 _currentTime;
    QElapsedTimer _timer;
    Direction _direction;
    bool _play, _loop, _reverse;

    // TODO : limit number of elements in history
    int _maximumSize;

};

typedef HistoryManager * HistoryManagerStar;
Q_DECLARE_METATYPE(HistoryManagerStar);

#endif // HISTORYMANAGER_H
