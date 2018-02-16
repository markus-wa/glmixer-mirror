#include "SnapshotManagerWidget.moc"
#include "ui_SnapshotManagerWidget.h"

#include "SnapshotManager.h"

SnapshotManagerWidget::SnapshotManagerWidget(QWidget *parent, QSettings *settings) :
    QWidget(parent),
    ui(new Ui::SnapshotManagerWidget),
    appSettings(settings)
{
    ui->setupUi(this);

    connect(ui->addSnapshot, SIGNAL(pressed()), SnapshotManager::getInstance(), SLOT(addSnapshot()));
    connect(SnapshotManager::getInstance(), SIGNAL(newSnapshot(QString)), SLOT(newSnapshot(QString)));
    connect(SnapshotManager::getInstance(), SIGNAL(deleteSnapshot(QString)), SLOT(deleteSnapshot(QString)));
    connect(SnapshotManager::getInstance(), SIGNAL(clear()), SLOT(clear()));
}

SnapshotManagerWidget::~SnapshotManagerWidget()
{
    delete ui;
}

// connected to signal newSnapshot
void SnapshotManagerWidget::newSnapshot(QString id)
{
    // new item
    QIcon icon;
    icon.addPixmap( QPixmap::fromImage( SnapshotManager::getInstance()->getSnapshotImage(id) ) );
    QString label = SnapshotManager::getInstance()->getSnapshotLabel(id);

    QListWidgetItem *item = new QListWidgetItem(icon, label);
    item->setData(Qt::UserRole, id);

    // add item to list
    ui->snapshotsList->addItem(item);
    item->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsEditable  | Qt::ItemIsDragEnabled );

    ui->snapshotsList->setCurrentItem(item);
}

// connected to signal removeSnapshot
void SnapshotManagerWidget::deleteSnapshot(QString id)
{
    for (int r = 0; r < ui->snapshotsList->count(); ++r) {
        QListWidgetItem *it = ui->snapshotsList->item(r);
        if ( it && id == it->data(Qt::UserRole).toString() ) {
             delete ui->snapshotsList->takeItem( r );
             break;
        }
    }
}

void SnapshotManagerWidget::clear()
{
    ui->snapshotsList->clear();
}

void SnapshotManagerWidget::on_deleteSnapshot_pressed()
{
    if ( ui->snapshotsList->currentItem() )
    {
        QString id = ui->snapshotsList->currentItem()->data(Qt::UserRole).toString();
        SnapshotManager::getInstance()->removeSnapshot(id);
    }
}

void SnapshotManagerWidget::on_snapshotsList_itemDoubleClicked(QListWidgetItem *item)
{
    QString id = item->data(Qt::UserRole).toString();

    SnapshotManager::getInstance()->restoreSnapshot(id);
}


void SnapshotManagerWidget::on_snapshotsList_itemChanged(QListWidgetItem *item)
{
    if ( ui->snapshotsList->findItems( item->text(), Qt::MatchFixedString ).length() > 1 )
        item->setText( item->text() + "_bis" );

    SnapshotManager::getInstance()->setSnapshotLabel( item->data(Qt::UserRole).toString(), item->text());
}


void SnapshotManagerWidget::on_snapshotsList_itemSelectionChanged()
{
    ui->deleteSnapshot->setEnabled(false);

    // enable delete action only if selected icon
    if ( ui->snapshotsList->currentItem()
         && ui->snapshotsList->currentItem()->isSelected() )
        ui->deleteSnapshot->setEnabled( true );
    else
        ui->snapshotsList->setCurrentRow(-1);
}

