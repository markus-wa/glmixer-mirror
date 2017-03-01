#ifndef HISTORYMANAGERWIDGET_H
#define HISTORYMANAGERWIDGET_H

#include <QAbstractTableModel>
#include <QTableView>

#include "HistoryManager.h"


class HistoryManagerModel : public QAbstractTableModel
{
    Q_OBJECT

public:
    explicit HistoryManagerModel(HistoryManager *hm, QWidget *parent = 0);

    // implementation of QAbstractTable
    int rowCount(const QModelIndex &parent) const;
    int columnCount(const QModelIndex &parent) const;
    QVariant headerData(int section, Qt::Orientation orientation, int role) const;

    QVariant data(const QModelIndex &index, int role) const;

private:
    HistoryManager *_historyManager;

};

class HistoryManagerWidget : public QTableView
{
    Q_OBJECT

public:
    explicit HistoryManagerWidget(HistoryManager *hm = NULL, QWidget *parent = 0);

private:
    HistoryManagerModel *_historyModel;
};

#endif // HISTORYMANAGERWIDGET_H
