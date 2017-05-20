#ifndef HISTORYMANAGER_H
#define HISTORYMANAGER_H

#include <QDebug>
#include <QRectF>
#include <QColor>
#include <QStack>
#include <QElapsedTimer>

#include "History.h"

class HistoryManager : public QObject
{
    Q_OBJECT

    friend class HistoryManagerModel;

public:
    HistoryManager(QObject *parent = 0);

    // add history
    void addHistory(History history);


public slots:

    // clear all
    void clear();



private:

    // history of events
    History _history;


};

#endif // HISTORYMANAGER_H
