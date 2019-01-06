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

    tagmenu = new QMenu(parent);

    // special icon for select all

    QPixmap pix(":/glmixer/icons/selectall.png");
    QIcon icon;
    icon.addPixmap(pix.scaled(42,42), QIcon::Normal, QIcon::Off);
    itemAll = new QListWidgetItem(icon, tr("All"));
    ui->tagsListWidget->addItem(itemAll);

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

        QAction *tagaction = new QAction(parent);
        tagaction->setData(t->getIndex());
        tagaction->setText(t->getLabel());
        tagaction->setIcon(icon);
        connect(tagaction, SIGNAL(triggered()), this, SLOT(applyTagtoCurrentSource()));
        tagmenu->addAction(tagaction);
    }

    // do not keep item selected
    deselectItem();
}

TagsManager::~TagsManager()
{
    delete ui;
    delete tagmenu;
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

void TagsManager::selectTag(Tag *t)
{
    if (t) {
        // toggle select all sources of this tag
        SourceList sl = t->getSources();
        for(SourceList::iterator  its = sl.begin(); its != sl.end(); its++) {

            if ( SelectionManager::getInstance()->isInSelection(*its) ) {
                SelectionManager::getInstance()->deselect(sl);
                break;
            }
            SelectionManager::getInstance()->select(*its);
        }
    }
}


void TagsManager::on_tagsListWidget_itemClicked(QListWidgetItem *i)
{
    if (i)
    {
        if (i == itemAll) {

            if (SelectionManager::getInstance()->hasSelection())
                SelectionManager::getInstance()->clearSelection();
            else
                SelectionManager::getInstance()->selectAll();

        }
        else {
            // get the key for the given item in the map
            QList<Tag *> tags = tagsMap.keys(i);
            if (!tags.empty()) {
                // select all sources of this tag
                selectTag( tags.first() );
            }
        }
    }

    // do not keep item selected
    deselectItem();
}

void TagsManager::applyTagtoCurrentSource()
{
    QAction *action = qobject_cast<QAction *>(sender());
    if (action) {
        applyTag( Tag::get(action->data().toInt()) );
    }
}

bool TagsManager::applyTag(Tag *t)
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
