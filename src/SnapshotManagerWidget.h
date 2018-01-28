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
    void on_addSnapshot_pressed();
    void on_deleteSnapshot_pressed();
    void on_snapshotsList_itemDoubleClicked(QListWidgetItem *);

private:
    Ui::SnapshotManagerWidget *ui;
    QSettings *appSettings;
};

#endif // FORM_H
