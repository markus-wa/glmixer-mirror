#ifndef HISTORYRECORDER_H
#define HISTORYRECORDER_H

#include <QObject>

#include "HistoryManager.h"

/*
 * History HistoryRecorder keeps a list of method calls given to the
 * storeEvent slot. Each call is stored with all information
 * necessary for calling the method again : object to send to,
 * method to call, and arguments. The time of the call is also
 * stored.
 *
 * */

class HistoryRecorder : public QObject
{
    Q_OBJECT

public:
    explicit HistoryRecorder(QObject *parent = 0);
    virtual ~HistoryRecorder();

    HistoryManager::EventMap getEvents();

signals:

public slots:

    // store an event in history
    void storeEvent(QString signature, QVariantPair arg1, QVariantPair arg2, QVariantPair arg3, QVariantPair arg4, QVariantPair arg5, QVariantPair arg6, QVariantPair arg7);


private:
    HistoryManager::EventMap _history;
    QElapsedTimer _timer;
};



#endif // HISTORYRECORDER_H
