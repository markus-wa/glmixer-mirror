#include <QBitmap>
#include <QListWidget>

#include "TagsManager.moc"
#include "ui_TagsManager.h"
#include "qtcolorpicker.h"

TagsManager::TagsManager(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::TagsManager)
{
    ui->setupUi(this);

    // add color picker
    colorpick = new QtColorPicker(this);
    colorpick->setStandardColors();
    colorpick->setCurrentColor(QColor("cyan"));
    ui->buttonsLayout->insertWidget(1, colorpick);

    // setup default tag
    currentItem = addTag(Tag::getDefault());
    ui->tagsListWidget->setCurrentItem(currentItem);
}

TagsManager::~TagsManager()
{
    delete ui;
}

QListWidgetItem *TagsManager::addTag(Tag *t)
{
    // create icon
    static QBitmap mask(":/glmixer/icons/tagMask.png");
    QPixmap pix(60,60);
    pix.fill(t->color());
    pix.setMask(mask);
    QIcon icon;
    icon.addPixmap(pix, QIcon::Normal, QIcon::Off);
    icon.addPixmap(pix, QIcon::Selected, QIcon::Off);

    // create tag item
    QListWidgetItem *item = new QListWidgetItem(icon,t->label());
    item->setData(Qt::UserRole, QVariant(t->color()));

    // add to the list
    ui->tagsListWidget->addItem(item);

    return item;
}

void TagsManager::createTag()
{
    Tag t("Custom", colorpick->currentColor());
    currentItem = addTag(&t);
    ui->tagsListWidget->setCurrentItem(currentItem);

    currentItem->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsEditable );
    ui->tagsListWidget->editItem(currentItem);
}


void TagsManager::deleteTag()
{

}


void TagsManager::useTag(QListWidgetItem *i)
{
    currentItem = i;

    // allow delete non default
    ui->tagRemove->setEnabled(true);
}

