#include "BasketSelectionDialog.moc"
#include "ui_BasketSelectionDialog.h"

#include <QApplication>

#include "glmixer.h"
#include "BasketSource.h"
#include "VideoFile.h"
#include "common.h"


bool listWidgetItemLessThan(const QListWidgetItem *i1, const QListWidgetItem *i2)
{
    QListWidget *lw = i1->listWidget();
    return lw->row(i1) < lw->row(i2);
}

bool listWidgetItemMoreThan(const QListWidgetItem *i1, const QListWidgetItem *i2)
{
    QListWidget *lw = i1->listWidget();
    return lw->row(i1) > lw->row(i2);
}

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

    new QShortcut(QKeySequence(Qt::Key_Delete), this, SLOT(deleteSelectedImages()));
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


QList<QListWidgetItem*> ImageFilesList::selectedImages()
{
    QList<QListWidgetItem*> selection = selectedItems();

    if (!selection.empty()) {
        qSort(selection.begin(), selection.end(), listWidgetItemLessThan);

        if (selection.first() == dropHintItem)
            selection.clear();
    }

    return selection;
}

void ImageFilesList::appendImageFiles(QList<QUrl> urlList)
{
    int c = count();
    QStringList invalidfiles;

    for (int i = 0; i < urlList.size(); ++i) {

        // Make sure the file exists and is readable
        QFileInfo urlname = getFileInfoFromURL(urlList.at(i));
        if ( urlname.exists() && urlname.isReadable() && urlname.isFile()) {

            // get filename
            QString filename = urlname.absoluteFilePath();

            // prepare to create a new item
            QListWidgetItem *newitem = 0;

            // try to find the filename in the list
            QList<QListWidgetItem *> previousitems = findItemsData( filename );

            // the image not in the list
            if (previousitems.size() == 0) {

//TODO : SUPPORT FOR GIF ANIM

                // try to make an image: accept if not null
                VideoFile mediafile;
                if ( !mediafile.open( filename, false, true ) ) {
                    invalidfiles << filename;
                    continue;
                }
                VideoPicture *p = mediafile.getFirstFrame();
                if ( !p ) {
                    invalidfiles << filename;
                    continue;
                }

                // no more need for drop hint
                if ( item(0) == dropHintItem)
                    takeItem(0);

                // add it to the list
                _fileNames.append( filename );

                // create a new item with the image file information
                newitem = new QListWidgetItem(this);
                newitem->setText(urlname.baseName());
                newitem->setData(Qt::UserRole, filename);

                // create a QImage from buffer
                QImage icon((uchar *)p->getBuffer(), p->getWidth(), p->getHeight(), p->getRowLength() * 3,  QImage::Format_RGB888);
                newitem->setIcon( QPixmap::fromImage(icon).scaledToHeight(64));

                newitem->setToolTip(QString("%1 %2 x %3").arg(urlname.absoluteFilePath()).arg(p->getWidth()).arg(p->getHeight()));

            }
            // already in the list : clone the item
            else {
                // clone item
                newitem = new QListWidgetItem( *(previousitems.first()) );

            }

            // add the item
            addItem(newitem);
            setCurrentItem(newitem, QItemSelectionModel::ClearAndSelect);

        }
    }

    // inform & update source preview
    emit changed( count() );

    if (!invalidfiles.empty())
        emit unsupportedFilesDropped(invalidfiles);

}

void ImageFilesList::dropEvent(QDropEvent *event)
{
    // default management of Drop Event
    QListWidget::dropEvent(event);

    // react according to event data
    const QMimeData *mimeData = event->mimeData();

    // an external file has been dropped
    if (mimeData->hasUrls()) {

        // accept the drop action
        event->acceptProposedAction();
        setEnabled(false);
        QCoreApplication::processEvents();

        // deal with all the urls dropped
        appendImageFiles(mimeData->urls());

        // done
        setEnabled(true);
        QCoreApplication::processEvents();

    }
    // an internal item has been moved
    else {

        // nothind to do, just
        // inform & update source preview
        emit changed( count() );

    }

}


QList<QListWidgetItem *> ImageFilesList::findItemsData(QString filename)
{
    QList<QListWidgetItem *> list;

    // loop over rows of QListWidget
    for (int i = 0; i < count(); ++i) {
        if ( item(i)->data(Qt::UserRole).toString().compare(filename) == 0)
            list.append(item(i));
    }

    return list;
}

void ImageFilesList::deleteSelectedImages()
{
    // loop over all items selected
    foreach (QListWidgetItem *it, selectedItems()) {
        if (it == dropHintItem)
            // do not remove, just take the drop hint item
            takeItem(0);
        else {
            // what file is referenced by this item ?
            QString filename = it->data(Qt::UserRole).toString();
            // if only one item reference this filename, remove it
            QList<QListWidgetItem *> items = findItemsData( filename );
            if (items.size() < 2)
                _fileNames.removeAll( filename );

            // delete item
            delete it;
        }
    }

    // inform
    emit changed( count() );

    // show hint
    if (count() < 1)
        insertItem(0, dropHintItem);
}


void ImageFilesList::duplicateSelectedImages()
{
    QList<QListWidgetItem*> items = selectedItems();
    qSort(items.begin(), items.end(), listWidgetItemMoreThan);

    // clear selection
    setCurrentRow(-1);

    // where to insert
    int r = row(items.last());

    // prepare to create a new item
    QListWidgetItem *newitem = 0;
    foreach (QListWidgetItem *it, items) {

        newitem = new QListWidgetItem( *it );
        insertItem(r, newitem);
        setCurrentItem(newitem, QItemSelectionModel::Select);
    }

    // inform
    emit changed( count() );
}

void ImageFilesList::deleteAllImages()
{
    selectAll();
    deleteSelectedImages();
}

void ImageFilesList::sortAlphabetically()
{
    sortItems();

    // inform
    emit changed( count() );
}


void ImageFilesList::moveSelectionUp(){

    // NB : not checking for validity of the action :
    // the button is enable only if the action is possible in updateActions()
    QList<QListWidgetItem*> items = selectedItems();
    qSort(items.begin(), items.end(), listWidgetItemLessThan);

    // moving up is decrementing row of selected items, starting from the first
    int r = -1;
    foreach (QListWidgetItem *it, items) {
        r = row(it);
        takeItem(r);
        insertItem(--r,it);
        setCurrentRow(r, QItemSelectionModel::Select);
    }

    // inform
    emit changed( count() );
}

void ImageFilesList::moveSelectionDown(){

    // NB : not checking for validity of the action :
    // the button is enable only if the action is possible in updateActions()
    QList<QListWidgetItem*> items = selectedItems();
    qSort(items.begin(), items.end(), listWidgetItemMoreThan);

    // moving down is incrementing row of selected items, starting from the last
    int r = -1;
    foreach (QListWidgetItem *it, items) {
        r = row(it);
        takeItem(r);
        insertItem(++r,it);
        setCurrentRow(r, QItemSelectionModel::Select);
    }

    // inform
    emit changed( count() );
}

QStringList ImageFilesList::getFiles()
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

            // try to find the item in the reference list
            int index = _fileNames.indexOf(item(i)->data(Qt::UserRole).toString());
            // the item is in the list
            if (index > -1) {
                list.append( index );
            }
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
    connect(basket, SIGNAL(changed(int)), SLOT(displayCount(int)));

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

    if (appSettings) {
        if (appSettings->contains("dialogBasketGeometry"))
            restoreGeometry(appSettings->value("dialogBasketGeometry").toByteArray());
        // size selection : default to 1024x768
        ui->sizeselection->setPreset(appSettings->value("dialogBasketSizePreset", "14").toInt());
    }

    // refresh of preview source
    connect(basket, SIGNAL(changed(int)), SLOT(updateSourcePreview()));
    connect(ui->sizeselection, SIGNAL(sizeChanged()), SLOT(updateSourcePreview()));

    // Actions from GUI buttons
    connect(ui->clearBasket, SIGNAL(pressed()), basket, SLOT(deleteAllImages()));
    connect(ui->removeCurrentImage, SIGNAL(pressed()), basket, SLOT(deleteSelectedImages()));
    connect(ui->duplicateCurrentImage, SIGNAL(pressed()), basket, SLOT(duplicateSelectedImages()));
    connect(ui->orderAlphanumeric, SIGNAL(pressed()), basket, SLOT(sortAlphabetically()));

    // enable / disable actions for selection
    connect(basket, SIGNAL(itemSelectionChanged()), SLOT(updateActions()));

    // error handling
    connect(basket, SIGNAL(unsupportedFilesDropped(QStringList)), SLOT(errorLoadingFiles(QStringList)));

}

BasketSelectionDialog::~BasketSelectionDialog()
{
    delete ui;
}


void BasketSelectionDialog::errorLoadingFiles(QStringList l)
{

    QMessageBox::warning(this, tr("Basket Source error"), tr("Only image files can be used in a Basket Source.\nThe following files have been ignored:\n\n%1").arg(l.join("\n")) );

}

void BasketSelectionDialog::updateSourcePreview(){

    if (s) {
        // remove source from preview: this deletes the texture in the preview
        ui->preview->setSource(0);
        // delete the source:
        delete s;
        s = 0;
    }

    QStringList files = basket->getFiles();
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
    basket->deleteAllImages();

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

    // save settings
    if (appSettings) {
        appSettings->setValue("dialogBasketGeometry", saveGeometry());
        appSettings->setValue("dialogBasketSizePreset", ui->sizeselection->getPreset());
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

    return basket->getFiles();
}

QString BasketSelectionDialog::getSelectedPlayList() {

    QStringList pl;
    foreach (int i, basket->getPlayList()) {
        pl.append(QString::number(i));
    }

    return pl.join(" ");
}

bool BasketSelectionDialog::getSelectedBidirectional(){

    return ui->bidirectional->isChecked();
}

bool BasketSelectionDialog::getSelectedShuffle(){

    return ui->shuffle->isChecked();
}


void BasketSelectionDialog::on_addImages_pressed() {

    QFileInfo fi( appSettings->value("recentImageFile", "").toString() );
    QDir di(QDesktopServices::storageLocation(QDesktopServices::PicturesLocation));
    if (fi.isReadable())
        di = fi.dir();

    QStringList fileNames = QFileDialog::getOpenFileNames(this,
                                                          tr("GLMixer - Open Pictures"),
                                                          di.absolutePath(),
                                                          "Images (*.png *.jpg *.jpeg *.bmp)" );
    // add selected file names
    if (!fileNames.empty()) {

        QList<QUrl> urlList;
        foreach (QString filename, fileNames) {
            urlList.append(QUrl(filename));
        }

        setEnabled(false);
        QCoreApplication::processEvents();

        // deal with all the urls selected
        basket->appendImageFiles(urlList);

        // remember last image location
        appSettings->setValue("recentImageFile", fileNames.front());
    }
    // done
    setEnabled(true);
    QCoreApplication::processEvents();
}


void BasketSelectionDialog::on_moveUp_pressed() {

    basket->moveSelectionUp();
}

void BasketSelectionDialog::on_moveDown_pressed(){

    basket->moveSelectionDown();
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
    ui->orderAlphanumeric->setEnabled( v > 0 );
    ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled( v > 0 );
}


void BasketSelectionDialog::updateActions(){

    // get a SORTED list of images (by increasing row)
    QList<QListWidgetItem*> selection = basket->selectedImages();

    // reset
    ui->moveUp->setEnabled( true ) ;
    ui->moveDown->setEnabled( true ) ;
    ui->itemActions->setEnabled( false );

    // if selection is not empty
    if (!selection.empty()) {
        // enable the buttons of actions on selection
        ui->itemActions->setEnabled( true );
        // update status for move up and down (min and max tests on ordered list)
        ui->moveUp->setEnabled( basket->row(selection.first()) > 0 ) ;
        ui->moveDown->setEnabled( basket->row(selection.last()) < basket->count() - 1 ) ;
    }

}
