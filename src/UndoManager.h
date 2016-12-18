#ifndef UNDOMANAGER_H
#define UNDOMANAGER_H

#include "defines.h"

#include <QDomDocument>

class UndoManager : public QObject
{
    Q_OBJECT
public:
    static UndoManager *getInstance();

    inline void setMaximumSize(int m) { _maximumSize = m; }
    inline int maximumSize() const { return _maximumSize; }

signals:
    void changed();

public slots:
    // clear the history
    void clear();
    // main actions
    void undo();
    void redo();
    // store status
    void store();
    // store events from sources
    void store(QString signature);
    // restore status
    void restore(int i);

    // stop listening (will be reactivated on next call to store()
    void suspend();


private:

    UndoManager();
    virtual ~UndoManager();
    static UndoManager *_instance;

    typedef enum {
        DISABLED = 0,
        IDLE,
        PENDING,
        ACTIVE
    } status;

    status _status;
    int _maximumSize;
    QString _previousSignature;
    QString _previousSender;

    QDomDocument _history;
    int _firstIndex, _lastIndex, _currentIndex;
};

#endif // UNDOMANAGER_H
