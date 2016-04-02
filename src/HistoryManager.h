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

#include "defines.h"

/*
 * Arguments stored in history have to keep any type of value
 * A pointer to the member variable is given inside the QGenericArgument
 * which is used when the method is invoked.
 *
*/
class HistoryArgument
{
    int intValue;
    uint uintValue;
    double doubleValue;
    bool boolValue;
    QRectF rectValue;
    QString stringValue;
    QColor colorValue;

public:
    HistoryArgument(QVariant v = QVariant());
    QVariant variant() const;
    QString string() const;
    QGenericArgument argument;
};

QDebug operator << ( QDebug out, const HistoryArgument & a );

/*
 * History manager keeps a list of method calls given to the
 * rememberMethodCall slot. Each call is stored with all information
 * necessary for calling the method again : object to send to,
 * method to call, and arguments. The time of the call is also
 * stored.
 *
 * The history manager then offers functionnality for replaying
 * the list of method calls with the same timing.
 *
 * Key events are remembered too (e.g. for undo)
 *
 * */

class HistoryManager : public QObject
{
    Q_OBJECT

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

        void setKey(bool k);
        bool isKey() const;

        bool operator == ( const Event & other ) const;
        bool operator != ( const Event & other ) const;

    private:
        QObject *_object;
        QMetaMethod _method;
        QVector< QVector<HistoryArgument> > _arguments;
        bool _iskey;
    };

    typedef QMultiMap<qint64, Event *> EventMap;

    // get the history of events
    EventMap getEvents(Direction dir = BACKWARD) const;

    // get the cursor time
    qint64 getCursorPosition() const;

    // get maximum size
    int getMaximumSize() const;

    // clear the history
    void clear();

signals:
    void changed();

public slots:

    // go to the position given time
    void setCursorPosition(qint64 t);
    // jump to the next event in history (bakward or forward)
    void setCursorNextPosition(Direction dir);
    void setCursorNextPositionForward()  { setCursorNextPosition(FORWARD); }
    void setCursorNextPositionBackward() { setCursorNextPosition(BACKWARD); }
    // jump to the next KEY event in history (bakward or forward)
    void setCursorNextKey(Direction dir);
    void setCursorNextKeyForward()  { setCursorNextKey(FORWARD); }
    void setCursorNextKeyBackward() { setCursorNextKey(BACKWARD); }

    // store an event in history
    void rememberEvent(QString signature, QVariantPair arg0, QVariantPair arg1, QVariantPair arg2, QVariantPair arg3, QVariantPair arg4);

    // set the maximum number of items in the history
    void setMaximumSize(int max);

private:

//    QStack<qint64> _keyEvents;
    EventMap _eventHistory;
    EventMap::iterator _currentEvent;
    qint64 _currentKey;
    int _maximumSize;

    QElapsedTimer _timer;
};

#endif // HISTORYMANAGER_H
