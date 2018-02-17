#ifndef SNAPSHOTMANAGERWIDGET_H
#define SNAPSHOTMANAGERWIDGET_H

#include <QtGui>

namespace Ui {
class SnapshotManagerWidget;
}

class SnapshotManagerWidget : public QWidget
{
    Q_OBJECT

public:
    explicit SnapshotManagerWidget(QWidget *parent, QSettings *settings);
    ~SnapshotManagerWidget();

public:

public slots:
    void clear();
    void newSnapshot(QString id);
    void deleteSnapshot(QString id);

    void deleteSelectedSnapshot();
    void restoreSelectedSnapshot();
    void renameSelectedSnapshot();

    void on_snapshotsList_itemDoubleClicked(QListWidgetItem *);
    void on_snapshotsList_itemChanged(QListWidgetItem *);
    void on_snapshotsList_itemSelectionChanged();

    // contex menu
    void ctxMenu(const QPoint &pos);

private:
    Ui::SnapshotManagerWidget *ui;
    QSettings *appSettings;
    QAction *newAction, *restoreAction, *deleteAction, *renameAction;
};

#endif // FORM_H
