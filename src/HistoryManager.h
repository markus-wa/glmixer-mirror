#ifndef HISTORYMANAGER_H
#define HISTORYMANAGER_H

#include <QObject>
#include <QElapsedTimer>
#include <QMap>

#include "Source.h"


class HistoryManager : public QObject
{
    Q_OBJECT
public:
    explicit HistoryManager(QObject *parent = 0);

signals:

public slots:


private:

    QMap<qint64, Source*> _events;
};

#endif // HISTORYMANAGER_H
