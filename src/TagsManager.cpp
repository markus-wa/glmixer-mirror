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

    // select default tag
    ui->tagsListWidget->setCurrentItem( tagsMap[Tag::getDefault()] );

}

TagsManager::~TagsManager()
{
    delete ui;
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
    QListWidgetItem *currentItem = getTagItem(t);
    if (currentItem)
        ui->tagsListWidget->setCurrentItem(currentItem);
}

void TagsManager::selectTag(QListWidgetItem *i)
{
    if (i)
    {
        // get the key for the given item in the map
        QList<Tag *> tags = tagsMap.keys(i);
        if (!tags.empty()) {
            Tag *t = tags.first();

            qDebug() << "select tag "<< t->getLabel() << t->getSources().size();
            // select all sources of this tag
            SelectionManager::getInstance()->setSelection( t->getSources() );
        }
    }
}

void TagsManager::useTag(QListWidgetItem *i)
{
    // apply this tag to the current source
    if (i && currentSource)
    {
        // get the key for the given item in the map
        QList<Tag *> tags = tagsMap.keys(i);
        if (!tags.empty()) {
            Tag *t = tags.first();

            // apply the tag found
            t->set(currentSource);
        }
    }
}


void TagsManager::connectSource(SourceSet::iterator csi)
{
    if (RenderingManager::getInstance()->isValid(csi)) {
        // remember current source pointer
        currentSource = *csi;
        // activate item 
        selectTag( Tag::get(currentSource) );
    } 
    else {
        // disable current source
        currentSource = NULL;
        // select default tag
        ui->tagsListWidget->setCurrentItem( tagsMap[Tag::getDefault()] );
    }

}
