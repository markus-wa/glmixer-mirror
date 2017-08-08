#include "BasketSelectionDialog.moc"
#include "ui_BasketSelectionDialog.h"

#include "glmixer.h"
#include "BasketSource.h"
#include "common.h"


ImageFilesList::ImageFilesList(QWidget *parent) : QListWidget(parent)
{
    setEditTriggers(QAbstractItemView::NoEditTriggers);
    setAcceptDrops ( true );
    setDragEnabled(true);
    setDragDropMode(QAbstractItemView::InternalMove);
    setSelectionMode(QAbstractItemView::ContiguousSelection);
    setIconSize(QSize(128, 64));
    setSpacing(4);
    setUniformItemSizes(true);
    setWordWrap(true);
    setSelectionRectVisible(true);
    setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
    setHorizontalScrollMode(QAbstractItemView::ScrollPerPixel);

    dropHintItem = new QListWidgetItem(this);
    dropHintItem->setText("Drop Files here...");
    dropHintItem->setFlags(dropHintItem->flags() & ~(Qt::ItemIsDropEnabled));
    insertItem(0, dropHintItem);

    new QShortcut(QKeySequence(Qt::Key_Delete), this, SLOT(deleteSelectedItem()));
}

void ImageFilesList::dragEnterEvent(QDragEnterEvent *event)
{
    if (event->mimeData()->hasUrls())
    {
        event->acceptProposedAction();
    } else {
        QListWidget::dragEnterEvent(event);
    }
}

void ImageFilesList::dragMoveEvent(QDragMoveEvent *event)
{
    if (event->mimeData()->hasUrls())
    {
        event->acceptProposedAction();
    } else {
        QListWidget::dragMoveEvent(event);
    }
}

void ImageFilesList::dragLeaveEvent(QDragLeaveEvent *event)
{
    event->ignore();
}

void ImageFilesList::dropEvent(QDropEvent *event)
{
    const QMimeData *mimeData = event->mimeData();

    // browse the list of urls dropped
    if (mimeData->hasUrls()) {

        // deal with all the urls dropped
        QList<QUrl> urlList = mimeData->urls();
        for (int i = 0; i < urlList.size(); ++i) {

            // Make sure the file exists and is readable
            QFileInfo urlname = getFileInfoFromURL(urlList.at(i));
            if ( urlname.exists() && urlname.isReadable() && urlname.isFile()) {

                // try to make an image: accept if not null
                QPixmap newimage(urlname.absoluteFilePath());
                if (!newimage.isNull()) {

                    // accept the drop action
                    event->acceptProposedAction();

                    // no more need for drop hint
                    if ( item(0) == dropHintItem)
                        takeItem(0);

                    // create a new item with the file information
                    QListWidgetItem *newitem = new QListWidgetItem(this);
                    newitem->setText(urlname.baseName());
                    newitem->setIcon(newimage.scaledToHeight(64));
                    newitem->setData(Qt::UserRole, urlname.absoluteFilePath());
                    newitem->setToolTip(urlname.absoluteFilePath());

                    // add the item
                    addItem(newitem);

                    // inform
                    emit countChanged( count() );
                }
            }
        }
    }
    // default management of Drop Event
    QListWidget::dropEvent(event);
}

void ImageFilesList::deleteSelectedItem()
{
    foreach (QListWidgetItem *item, selectedItems())
        delete item;

    // inform
    emit countChanged( count() );
}

QStringList ImageFilesList::getFilesList()
{
    QStringList list;

    return list;
}

BasketSelectionDialog::BasketSelectionDialog(QWidget *parent, QSettings *settings) :
    QDialog(parent),
    ui(new Ui::BasketSelectionDialog),
    s(NULL), appSettings(settings)
{
    ui->setupUi(this);

    // create basket list
    ImageFilesList *basket = new ImageFilesList(ui->leftFrame);
    connect(basket, SIGNAL(countChanged(int)), SLOT(displayCount(int)));

    // insert into ui
    delete (ui->basket);
    ui->basket = (QListWidget *) basket;
    ui->basket->setObjectName(QString::fromUtf8("basket"));
    QSizePolicy sizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    sizePolicy.setHorizontalStretch(0);
    sizePolicy.setVerticalStretch(0);
    sizePolicy.setHeightForWidth(ui->basket->sizePolicy().hasHeightForWidth());
    ui->basket->setSizePolicy(sizePolicy);
    ui->leftLayout->insertWidget(1, ui->basket);

}

BasketSelectionDialog::~BasketSelectionDialog()
{
    delete ui;
}


void BasketSelectionDialog::done(int r){

    // remove source from preview: this deletes the texture in the preview
//    ui->preview->setSource(0);

    // delete previous
    if(s) {
        // delete the source:
        delete s;
        s = NULL;
    }

    QDialog::done(r);
}

int  BasketSelectionDialog::getSelectedWidth(){

    return ui->sizeselection->getWidth();
}


int  BasketSelectionDialog::getSelectedHeight(){

    return ui->sizeselection->getHeight();
}


int BasketSelectionDialog::getSelectedPeriod() {

    return 25;
}

QStringList BasketSelectionDialog::getSelectedFiles() {

    return QStringList();
}

void BasketSelectionDialog::on_frequencySlider_valueChanged(int v){
//    if (s)
//        s->setPeriodicity(getUpdatePeriod());
}


void BasketSelectionDialog::displayCount(int v){

    ui->informationLabel->setText(tr("%1 images in basket.").arg(v));
}
