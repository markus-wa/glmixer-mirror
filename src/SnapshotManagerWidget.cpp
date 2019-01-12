#include "SnapshotManagerWidget.moc"
#include "ui_SnapshotManagerWidget.h"

#include "SnapshotManager.h"

SnapshotManagerWidget::SnapshotManagerWidget(QWidget *parent, QSettings *settings) :
    QWidget(parent),
    ui(new Ui::SnapshotManagerWidget),
    appSettings(settings)
{
    ui->setupUi(this);
    ui->snapshotsList->setAcceptDrops(false);

    // Create actions
    newAction = new QAction(QIcon(":/glmixer/icons/snapshot_new.png"), tr("Take Snapshot"), this);
    connect(newAction, SIGNAL(triggered()), SnapshotManager::getInstance(), SLOT(addSnapshot()));
    ui->addSnapshot->setDefaultAction(newAction);

    restoreAction = new QAction(QIcon(":/glmixer/icons/snapshot_new.png"), tr("Apply"), this);
    connect(restoreAction, SIGNAL(triggered()), this, SLOT(restoreSelectedSnapshot()));

    updateAction = new QAction(QIcon(":/glmixer/icons/view-refresh.png"), tr("Update"), this);
    updateAction->setEnabled(true);
    connect(updateAction, SIGNAL(triggered()), this, SLOT(updateSelectedSnapshot()));

    deleteAction = new QAction(QIcon(":/glmixer/icons/fileclose.png"), tr("Delete"), this);
    deleteAction->setEnabled(false);
    connect(deleteAction, SIGNAL(triggered()), this, SLOT(deleteSelectedSnapshot()));
    ui->deleteSnapshot->setDefaultAction(deleteAction);

    renameAction = new QAction(QIcon(":/glmixer/icons/rename.png"), tr("Rename"), this);
    connect(renameAction, SIGNAL(triggered()), this, SLOT(renameSelectedSnapshot()));

    // connect with snapshot manager
    connect(SnapshotManager::getInstance(), SIGNAL(newSnapshot(QString)), SLOT(newSnapshot(QString)));
    connect(SnapshotManager::getInstance(), SIGNAL(updateSnapshot(QString)), SLOT(updateSnapshot(QString)));
    connect(SnapshotManager::getInstance(), SIGNAL(deleteSnapshot(QString)), SLOT(deleteSnapshot(QString)));
    connect(SnapshotManager::getInstance(), SIGNAL(clear()), SLOT(clear()));

    // context menu
    ui->snapshotsList->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(ui->snapshotsList, SIGNAL(customContextMenuRequested(const QPoint &)),  SLOT(ctxMenu(const QPoint &)));
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

// connected to signal newSnapshot
void SnapshotManagerWidget::updateSnapshot(QString id)
{
    QIcon icon;
    icon.addPixmap( QPixmap::fromImage( SnapshotManager::getInstance()->getSnapshotImage(id) ) );
    QString label = SnapshotManager::getInstance()->getSnapshotLabel(id);

    for (int r = 0; r < ui->snapshotsList->count(); ++r) {
        QListWidgetItem *it = ui->snapshotsList->item(r);
        if ( it && id == it->data(Qt::UserRole).toString() ) {

            it->setIcon(icon);
            it->setText(label);
            break;
        }
    }

}


void SnapshotManagerWidget::setViewSimplified(bool on)
{
    ui->controlBox->setHidden(on);
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

void SnapshotManagerWidget::deleteSelectedSnapshot()
{
    if ( ui->snapshotsList->currentItem() )
    {
        QString id = ui->snapshotsList->currentItem()->data(Qt::UserRole).toString();
        SnapshotManager::getInstance()->removeSnapshot(id);
    }
}

// connected to signal renewSnapshot
void SnapshotManagerWidget::updateSelectedSnapshot()
{
    if ( ui->snapshotsList->currentItem() )
    {
        QString id = ui->snapshotsList->currentItem()->data(Qt::UserRole).toString();
        SnapshotManager::getInstance()->renewSnapshot(id);
    }
}

void SnapshotManagerWidget::restoreSelectedSnapshot()
{
    if ( ui->snapshotsList->currentItem() )
    {
        QString id = ui->snapshotsList->currentItem()->data(Qt::UserRole).toString();
        SnapshotManager::getInstance()->restoreSnapshot(id);
    }
}

void SnapshotManagerWidget::on_snapshotsList_itemDoubleClicked(QListWidgetItem *item)
{
    restoreSelectedSnapshot();
}

void SnapshotManagerWidget::on_snapshotsList_itemChanged(QListWidgetItem *item)
{
    // ensure unique name
    if ( ui->snapshotsList->findItems( item->text(), Qt::MatchFixedString ).length() > 1 )
        item->setText( item->text() + "_bis" );

    // rename snapshot in manager
    QString id = item->data(Qt::UserRole).toString();
    SnapshotManager::getInstance()->setSnapshotLabel(id, item->text());

    // set tooltip
    QString tip = tr("Snapshot '%1'").arg(item->text());
    tip += tr("\nContains %1 sources").arg(SnapshotManager::getInstance()->getSnapshot(id).size());
    tip += tr("\nCreated on %1/%2/%3").arg(id.left(9).right(2)).arg(id.left(7).right(2)).arg(id.left(5).right(4));
    item->setToolTip(tip);
}


void SnapshotManagerWidget::on_snapshotsList_itemSelectionChanged()
{
    deleteAction->setEnabled(false);
    QListWidgetItem *current = ui->snapshotsList->currentItem();

    // enable delete action only if selected icon
    if ( current && current->isSelected() )
        deleteAction->setEnabled( true );
    else
        ui->snapshotsList->setCurrentRow(-1);
}


void SnapshotManagerWidget::renameSelectedSnapshot()
{
    if ( ui->snapshotsList->currentItem() )
    {
        ui->snapshotsList->editItem(ui->snapshotsList->currentItem());
    }
}

void SnapshotManagerWidget::ctxMenu(const QPoint &pos)
{
    static QMenu *contextmenu_item = NULL;
    if (contextmenu_item == NULL) {
        contextmenu_item = new QMenu(this);
        contextmenu_item->addAction(restoreAction);
        contextmenu_item->addAction(updateAction);
        contextmenu_item->addAction(renameAction);
        contextmenu_item->addAction(deleteAction);
    }
    static QMenu *contextmenu_background = NULL;
    if (contextmenu_background == NULL) {
        contextmenu_background = new QMenu(this);
        contextmenu_background->addAction(newAction);
    }

    QListWidgetItem *item = ui->snapshotsList->itemAt(pos);
    if (item) {
        ui->snapshotsList->setCurrentItem(item);
        contextmenu_item->popup(ui->snapshotsList->viewport()->mapToGlobal(pos));
    }
    else {
        ui->snapshotsList->setCurrentRow(-1);
        contextmenu_background->popup(ui->snapshotsList->viewport()->mapToGlobal(pos));
    }

}

