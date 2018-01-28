#ifndef FORM_H
#define FORM_H

#include <QWidget>

namespace Ui {
class SnapshotManagerWidget;
}

class SnapshotManagerWidget : public QWidget
{
    Q_OBJECT

public:
    explicit SnapshotManagerWidget(QWidget *parent = 0);
    ~SnapshotManagerWidget();

private:
    Ui::SnapshotManagerWidget *ui;
};

#endif // FORM_H
