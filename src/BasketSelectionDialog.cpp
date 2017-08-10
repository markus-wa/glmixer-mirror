#include "BasketSelectionDialog.moc"
#include "ui_BasketSelectionDialog.h"

#include <QApplication>

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
    setHorizontalScrollMode(QAbstractItemView::ScrollPerPixel);

    dropHintItem = new QListWidgetItem(this);
    dropHintItem->setText("Drop Files here...");
    dropHintItem->setFlags(dropHintItem->flags() & ~(Qt::ItemIsDropEnabled));
//    insertItem(0, dropHintItem);

    new QShortcut(QKeySequence(Qt::Key_Delete), this, SLOT(deleteSelectedItems()));
}

void ImageFilesList::dragEnterEvent(QDragEnterEvent *event)
{
    if (event->mimeData()->hasUrls())
    {
        event->acceptProposedAction();
    } else
    {
        QListWidget::dragEnterEvent(event);
    }
}

void ImageFilesList::dragMoveEvent(QDragMoveEvent *event)
{
    if (event->mimeData()->hasUrls())
    {
        event->acceptProposedAction();

    } else
    {
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

    // an external file has been dropped
    if (mimeData->hasUrls()) {

        // deal with all the urls dropped
        QList<QUrl> urlList = mimeData->urls();
        for (int i = 0; i < urlList.size(); ++i) {

            // Make sure the file exists and is readable
            QFileInfo urlname = getFileInfoFromURL(urlList.at(i));
            if ( urlname.exists() && urlname.isReadable() && urlname.isFile()) {

                // accept the drop action
                event->acceptProposedAction();
                setEnabled(false);
                QCoreApplication::processEvents();

                QListWidgetItem *newitem = 0;

                // try to find the file in the list
                int index = _fileNames.indexOf(urlname.absoluteFilePath());
                if (index < 0) {

                    // try to make an image: accept if not null
                    QPixmap image(urlname.absoluteFilePath());
                    if (image.isNull())
                        continue;

                    // no more need for drop hint
                    if ( item(0) == dropHintItem)
                        takeItem(0);

                    // if it is an image not in the list
                    // add it to the list
                    index = _fileNames.size();
                    _fileNames.insert( index, urlname.absoluteFilePath() );

                    // create a new item with the image file information
                    newitem = new QListWidgetItem(this);
                    newitem->setText(urlname.baseName());
                    newitem->setIcon(image.scaledToHeight(64));
                    newitem->setData(Qt::UserRole, index);
//                    newitem->setToolTip(QString("%1 %2 x %3").arg(urlname.absoluteFilePath()).arg(image.width()).arg(image.height()));
                    newitem->setToolTip(QString("%1").arg(index));  // DEBUG

                    _referenceItems.insert( index, newitem );

                }
                // already in the list : clone the item
                else {
                    // clone item
                    newitem = new QListWidgetItem( *_referenceItems[index]);

                }

                // add the item
                addItem(newitem);
                setCurrentItem(newitem, QItemSelectionModel::ClearAndSelect);

            }
        }

        // done
        setEnabled(true);
        QCoreApplication::processEvents();

    }
    // an internal item has been moved
    else {

        qDebug() << "drag move " << getPlayList();
        // update source preview with new playlist

    }

    // inform & update source preview
    emit countChanged( count() );

    // default management of Drop Event
    QListWidget::dropEvent(event);
}

void ImageFilesList::deleteSelectedItems()
{
    // loop over all items selected
    foreach (QListWidgetItem *it, selectedItems()) {
        if (it == dropHintItem)
            // do not remove, just take the drop hint item
            takeItem(0);
        else {
            // try to find the item in the reference list
            int index = _referenceItems.indexOf(it);
            if (index > -1) {
                // remove found element from the list of files
                _referenceItems.removeAt(index);
                _fileNames.removeAt(index);
            }
            // delete item
            delete it;
        }
    }

    // inform
    emit countChanged( count() );

    // show hint
    if (count() < 1)
        insertItem(0, dropHintItem);
}

void ImageFilesList::deleteAllItems()
{
    selectAll();
    deleteSelectedItems();
}

void ImageFilesList::sortAlphabetical()
{
    sortItems();
    // inform // TODO emit orderChanged( getPlayList() );
    emit countChanged( count() );
}


QStringList ImageFilesList::getFilesList()
{
    return _fileNames;
}

QList<int> ImageFilesList::getPlayList()
{
    QList<int>  list;

    // if the widget is not empty
    if (item(0) != dropHintItem) {
        // loop over rows of QListWidget
        for (int i = 0; i < count(); ++i) {
            list.append( item(i)->data(Qt::UserRole).toInt() );
            qDebug() << "row " << i << " index " <<  item(i)->data(Qt::UserRole).toInt() << _fileNames[item(i)->data(Qt::UserRole).toInt()];
        }
    }

    return list;
}

BasketSelectionDialog::BasketSelectionDialog(QWidget *parent, QSettings *settings) :
    QDialog(parent),
    ui(new Ui::BasketSelectionDialog),
    s(NULL), appSettings(settings)
{
    ui->setupUi(this);

    // create basket list
    basket = new ImageFilesList(ui->leftFrame);
    connect(basket, SIGNAL(countChanged(int)), SLOT(displayCount(int)));

    // insert basket into ui
    delete (ui->basket);
    ui->basket = (QListWidget *) basket;
    ui->basket->setObjectName(QString::fromUtf8("basket"));
    QSizePolicy sizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    sizePolicy.setHorizontalStretch(0);
    sizePolicy.setVerticalStretch(0);
    sizePolicy.setHeightForWidth(ui->basket->sizePolicy().hasHeightForWidth());
    ui->basket->setSizePolicy(sizePolicy);
    ui->leftLayout->insertWidget(1, ui->basket);

    // refresh of preview source
    connect(ui->basket, SIGNAL(countChanged(int)), SLOT(updateSourcePreview()));
    connect(ui->sizeselection, SIGNAL(sizeChanged()), SLOT(updateSourcePreview()));

    // Actions from GUI buttons
    connect(ui->clearBasket, SIGNAL(pressed()), basket, SLOT(deleteAllItems()));
    connect(ui->removeCurrentImage, SIGNAL(pressed()), basket, SLOT(deleteSelectedItems()));
    connect(ui->orderAlphanumeric, SIGNAL(pressed()), basket, SLOT(sortAlphabetical()));

}

BasketSelectionDialog::~BasketSelectionDialog()
{
    delete ui;
}


void BasketSelectionDialog::updateSourcePreview(){

    if(s) {
        // remove source from preview: this deletes the texture in the preview
        ui->preview->setSource(0);
        // delete the source:
        delete s;
        s = 0;
    }

    QStringList files = basket->getFilesList();
    if (files.count() < 1)
        return;

    try {
         s = new BasketSource(files, 0,
                              ui->sizeselection->getWidth(),
                              ui->sizeselection->getHeight(),
                              (qint64) getSelectedPeriod());

         s->setPlaylist(basket->getPlayList());
         s->setBidirectional(ui->bidirectional->isChecked());
         s->setShuffle(ui->shuffle->isChecked());

    } catch (AllocationException &e){
        qCritical() << tr("Error creating Basket source; ") << e.message();
        // return an invalid pointer
        s = 0;
    }

    // apply the source to the preview
    ui->preview->setSource(s);
    ui->preview->playSource(true);
}


void BasketSelectionDialog::showEvent(QShowEvent *e){

    // clear
    basket->deleteAllItems();

    // show no source
    updateSourcePreview();

    QWidget::showEvent(e);
}


void BasketSelectionDialog::done(int r){

    // remove source from preview
    ui->preview->setSource(0);

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

    return qRound( 1000.0 / (double) ui->frequencySlider->value()) ;
}

QStringList BasketSelectionDialog::getSelectedFiles() {

    return basket->getFilesList();
}


bool BasketSelectionDialog::getSelectedBidirectional(){

    return ui->bidirectional->isChecked();
}

bool BasketSelectionDialog::getSelectedShuffle(){

    return ui->shuffle->isChecked();
}


void BasketSelectionDialog::on_frequencySlider_valueChanged(int v){

    if (s)
        s->setPeriod(getSelectedPeriod());
}

void BasketSelectionDialog::on_bidirectional_toggled(bool on){

    if (s)
        s->setBidirectional(on);
}

void BasketSelectionDialog::on_shuffle_toggled(bool on){

    if (s)
        s->setShuffle(on);
}

void BasketSelectionDialog::displayCount(int v){

    ui->informationLabel->setText(tr("%1 images in basket.").arg( v ));

    ui->removeCurrentImage->setEnabled( v > 0 );
    ui->orderAlphanumeric->setEnabled( v > 0 );
    ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled( v > 0 );
}
