#include "SnapshotManagerWidget.moc"
#include "ui_SnapshotManagerWidget.h"

SnapshotManagerWidget::SnapshotManagerWidget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::SnapshotManagerWidget)
{
    ui->setupUi(this);
}

SnapshotManagerWidget::~SnapshotManagerWidget()
{
    delete ui;
}
