#include "SnapshotManagerWidget.moc"
#include "ui_SnapshotManagerWidget.h"

#include "SnapshotManager.h"

SnapshotManagerWidget::SnapshotManagerWidget(QWidget *parent, QSettings *settings) :
    QWidget(parent),
    ui(new Ui::SnapshotManagerWidget),
    appSettings(settings)
{
    ui->setupUi(this);
}

SnapshotManagerWidget::~SnapshotManagerWidget()
{
    delete ui;
}


void SnapshotManagerWidget::on_addSnapshot_pressed()
{
    // create snapshot
    QString id = SnapshotManager::getInstance()->addSnapshot();

    // new item
    QIcon icon;
    icon.addPixmap( SnapshotManager::getInstance()->getSnapshotPixmap(id) );
    QListWidgetItem *item = new QListWidgetItem(icon, id);
    item->setData(Qt::UserRole, id);

    // add item to list
    ui->snapshotsList->addItem(item);

}

void SnapshotManagerWidget::on_deleteSnapshot_pressed()
{

}

void SnapshotManagerWidget::on_snapshotsList_itemDoubleClicked(QListWidgetItem *item)
{
    QString id = item->data(Qt::UserRole).toString();

    SnapshotManager::getInstance()->restoreSnapshot(id);

}
