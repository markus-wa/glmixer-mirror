#ifndef HISTORYMANAGERWIDGET_H
#define HISTORYMANAGERWIDGET_H

#include <QAbstractTableModel>
#include <QTableView>

#include "HistoryManager.h"


class HistoryModel : public QAbstractTableModel
{
    Q_OBJECT

public:
    explicit HistoryModel(History *h, QWidget *parent = 0);

    // implementation of QAbstractTable
    int rowCount(const QModelIndex &parent) const;
    int columnCount(const QModelIndex &parent) const;
    QVariant headerData(int section, Qt::Orientation orientation, int role) const;

    QVariant data(const QModelIndex &index, int role) const;

private:
    History *_history;

};

class HistoryWidget : public QTableView
{
    Q_OBJECT

public:
    explicit HistoryWidget(History *h = NULL, QWidget *parent = 0);

private:
    HistoryModel *_historyModel;
};



class HistoryModel : public QAbstractTableModel
{
    Q_OBJECT

public:
    explicit HistoryModel(History *h, QWidget *parent = 0);

    // implementation of QAbstractTree
    int rowCount(const QModelIndex &parent) const;
    int columnCount(const QModelIndex &parent) const;
    QVariant headerData(int section, Qt::Orientation orientation, int role) const;

    QVariant data(const QModelIndex &index, int role) const;

private:
    History *_history;

};

class HistoryManagerWidget : public QTableView
{
    Q_OBJECT

public:
    explicit HistoryManagerWidget(History *h = NULL, QWidget *parent = 0);

private:
    HistoryModel *_historyModel;
};


#endif // HISTORYMANAGERWIDGET_H
