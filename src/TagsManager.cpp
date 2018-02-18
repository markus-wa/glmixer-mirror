#include <QBitmap>
#include <QListWidget>
#include <QMetaType>

#include "TagsManager.moc"
#include "ui_TagsManager.h"
#include "qtcolorpicker.h"

#include "RenderingManager.h"
#include "SelectionManager.h"

TagsManager::TagsManager(QWidget *parent) :
    QWidget(parent), ui(new Ui::TagsManager), currentSource(NULL)
{
    ui->setupUi(this);

    // fill the list of items with all possible tags
    for (int i=0; i < Tag::getNumTags(); ++i) {
        // get Tag
        Tag *t = Tag::get(i);
        // create icon
        QBitmap mask(":/glmixer/icons/tagMask.png");
        QPixmap pix(mask.width(), mask.height());
        pix.fill(t->getColor());
        pix.setMask(mask);
        QIcon icon;
        icon.addPixmap(pix, QIcon::Normal, QIcon::Off);
        icon.addPixmap(pix, QIcon::Selected, QIcon::Off);
        // create tag item
        QListWidgetItem *item = new QListWidgetItem(icon, t->getLabel());
        // add to the lists
        ui->tagsListWidget->addItem(item);
        tagsMap[t] = item;
    }

    // do not keep item selected
    deselectItem();

    // context menu
    ui->tagsListWidget->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(ui->tagsListWidget, SIGNAL(customContextMenuRequested(const QPoint &)),  SLOT(ctxMenu(const QPoint &)));
}

TagsManager::~TagsManager()
{
    delete ui;
}

void TagsManager::ctxMenu(const QPoint &pos)
{
    static QMenu *contextmenu = NULL;
    if (contextmenu == NULL) {
        contextmenu = new QMenu(this);
        connect(contextmenu, SIGNAL(aboutToHide()), this, SLOT(deselectItem()));
        // clear selection
        QAction *clearselection = new QAction(QIcon(":/glmixer/icons/selectclear.png"), tr("Clear selection"), this);
        connect(clearselection, SIGNAL(triggered()), SelectionManager::getInstance(), SLOT(clearSelection()));
        connect(clearselection, SIGNAL(triggered()), this, SLOT(deselectItem()));
        contextmenu->addAction(clearselection);
        // clear all tags
        QAction *clearall = new QAction(QIcon(":/glmixer/icons/cleanbig.png"), tr("Cancel tags of all Sources"), this);
        connect(clearall, SIGNAL(triggered()), this, SLOT(clearAllTags()));
        connect(clearall, SIGNAL(triggered()), this, SLOT(deselectItem()));
        contextmenu->addAction(clearall);
    }

    // offer context menu only in background
    QListWidgetItem *item = ui->tagsListWidget->itemAt(pos);
    if (!item)
        contextmenu->popup(ui->tagsListWidget->viewport()->mapToGlobal(pos));

    // do not keep item selected
    deselectItem();
}


void TagsManager::deselectItem()
{
    ui->tagsListWidget->setCurrentRow( -1 );
}

void TagsManager::clearAllTags()
{
    Tag::remove();

    // do not keep item selected
    deselectItem();
}

QListWidgetItem *TagsManager::getTagItem(Tag *t)
{
    QListWidgetItem *item = NULL;

    // try to find the item for this tag
    QHash<Tag *, QListWidgetItem *>::Iterator it = tagsMap.find(t);

    if (it != tagsMap.end())
        // found item; return it
        item = it.value();

    return item;
}


void TagsManager::selectTag(Tag *t)
{
    if (t) {
        // select all sources of this tag
        SelectionManager::getInstance()->setSelection( t->getSources() );
    }

}


void TagsManager::on_tagsListWidget_itemClicked(QListWidgetItem *i)
{
    if (i)
    {
        // get the key for the given item in the map
        QList<Tag *> tags = tagsMap.keys(i);
        if (!tags.empty()) {
            // try to apply this tag
            if ( !useTag( tags.first() ) )
                // select all sources of this tag
                selectTag( tags.first() );
        }
    }

    // do not keep item selected
    deselectItem();
}

bool TagsManager::useTag(Tag *t)
{
    bool changed = false;

    if (t)
    {
        // apply this tag to the  selection
        if ( SelectionManager::getInstance()->hasSelection() ) {

            for(SourceList::iterator  its = SelectionManager::getInstance()->selectionBegin(); its != SelectionManager::getInstance()->selectionEnd(); its++) {
                // apply the tag to selected sources
                if (*its)
                    changed |= t->set(*its);
            }
        }
        // apply this tag to the current source
        else if (currentSource) {
            // apply the tag to current source
            changed |= t->set(currentSource);
        }

    }

    return changed;
}

void TagsManager::connectSource(SourceSet::iterator csi)
{
    if (RenderingManager::getInstance()->isValid(csi)) {
        // remember current source pointer
        currentSource = *csi;
    }
    else {
        // disable current source
        currentSource = NULL;
    }

}
