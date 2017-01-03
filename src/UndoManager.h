#ifndef UNDOMANAGER_H
#define UNDOMANAGER_H

#include "defines.h"

#include <QDomDocument>

class UndoManager : public QObject
{
    Q_OBJECT
public:
    static UndoManager *getInstance();

    void setMaximumSize(int m);
    inline int maximumSize() const { return _maximumSize; }

signals:
    void changed();

public slots:
    // clear the history
    void clear();
    // main actions
    void undo();
    void redo();
    // store status as is now
    void store();
    // store events from sources
    void store(QString signature);
    // restore status
    void restore(long i);
    // stop listening to store(signature) events
    // (will be reactivated on next call to clear or unsuspend)
    void suspend();
    // restart listenting to store(signature) events
    void unsuspend();

private:

    void addHistory(long int index);

    UndoManager();
    virtual ~UndoManager();
    static UndoManager *_instance;

    typedef enum {
        DISABLED = 0,
        IDLE,
        PENDING,
        READY,
        ACTIVE
    } status;

    status _status;
    QString _previousSignature;
    QString _previousSender;

    QDomDocument _history;
    long int _firstIndex, _lastIndex, _currentIndex;
    int _maximumSize;
};

#endif // UNDOMANAGER_H
