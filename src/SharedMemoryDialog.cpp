/*
 * SharedMemoryDialog.cpp
 *
 *  Created on: Aug 6, 2011
 *      Author: bh
 */

#include <QtDebug>
#include <QAbstractListModel>

#include "SharedMemorySource.h"
#include "SharedMemoryManager.h"
#include "Source.h"
#include "SourceDisplayWidget.h"

#include "SharedMemoryDialog.moc"

#define KEY_ROLE Qt::UserRole + 1
#define SIZE_ROLE Qt::UserRole + 2
#define FORMAT_ROLE Qt::UserRole + 3
#define INFO_ROLE Qt::UserRole + 4


void fillModel(QStandardItemModel *model, QMap<qint64, QVariantMap> map)
{

    // do not add elements of the map which are already in the model
	// and remove elements from the model if they are not in the map
    for (int r = 0; r < model->rowCount(); ++r) {
    	qint64 key = model->item(r)->data(Qt::UserRole).toLongLong();
    	if ( map.count(key) )
    		map.remove(key);
    	else
    		model->removeRow(r);
    }

    // add the remaining elements of the map to the model
	QMapIterator<qint64, QVariantMap> i(map);
	while (i.hasNext()) {
		i.next();

		// try to get an icon
		QPixmap icon;
		QVariant variant = i.value()["icon"];
		if (variant.isValid())
			icon = variant.value<QPixmap>();
		else
			icon = QPixmap(":/glmixer/icons/gear.png");

		// create list item
		QStandardItem  *item = new QStandardItem(QIcon(icon), i.value()["program"].toString());
		item->setData(i.key(), Qt::UserRole );

		// store user data in the item
		item->setData(i.value()["key"].toString(), KEY_ROLE);
		item->setData(i.value()["size"].toSize(), SIZE_ROLE);
		item->setData(i.value()["format"].toInt(), FORMAT_ROLE);
		item->setData(i.value()["info"].toString(), INFO_ROLE);

		// setup a tooltip to inform user
	    QString tooltip = QString("%1 (%2x%3)").arg(i.value()["info"].toString()).arg(i.value()["size"].toSize().width()).arg(i.value()["size"].toSize().height());
	    item->setData(tooltip, Qt::ToolTipRole );

	    // append item
		model->appendRow(item);
	}

}


SharedMemoryDialog::SharedMemoryDialog(QWidget *parent) : QDialog(parent), s(0), preview(0), selectedItem(0)  {

    setupUi(this);

    preview = new SourceDisplayWidget(this);
    preview->setSource(0);
    preview->hide();

    listExistingShmModel = new QStandardItemModel(0, 0, this);
    listExistingShm->setModel(listExistingShmModel);

    connect(listExistingShm->selectionModel(), SIGNAL( selectionChanged(const QItemSelection &, const QItemSelection &) ), this, SLOT( setCurrent(const QItemSelection &, const QItemSelection &) )  );
}

SharedMemoryDialog::~SharedMemoryDialog() {

	if (preview)
		delete preview;

	delete listExistingShmModel;

}

void SharedMemoryDialog::done(int r){

	if (preview)
		preview->setSource(0);
	if (s) {
		delete s;
		s = 0;
	}

	killTimer(updateListTimer);

	QDialog::done(r);
}


void SharedMemoryDialog::timerEvent(QTimerEvent *event) {

	fillModel(listExistingShmModel, SharedMemoryManager::getInstance()->getSharedMap());

}


QString SharedMemoryDialog::getSelectedProcess(){

	if (selectedItem)
		return selectedItem->data(Qt::DisplayRole).toString();

	return QString();
}

QString SharedMemoryDialog::getSelectedKey(){

	if (selectedItem)
		return selectedItem->data(KEY_ROLE).toString();

	return QString();
}

QString SharedMemoryDialog::getSelectedInfo(){

	if (selectedItem)
		return selectedItem->data(INFO_ROLE).toString();

	return QString();
}

QSize SharedMemoryDialog::getSelectedSize(){

	if (selectedItem)
		return selectedItem->data(SIZE_ROLE).toSize();

	return QSize();
}

QImage::Format SharedMemoryDialog::getSelectedFormat(){

	if (selectedItem)
		return (QImage::Format) selectedItem->data(FORMAT_ROLE).toInt();

	return QImage::Format_Invalid;
}

void SharedMemoryDialog::showEvent(QShowEvent *e){

	// clear selection
    listExistingShm->clearSelection();

	// start the update of the list of existing shared memory clients
	fillModel(listExistingShmModel, SharedMemoryManager::getInstance()->getSharedMap());
	updateListTimer = startTimer(1000);

	QWidget::showEvent(e);
}


void SharedMemoryDialog::createSource(){

	if (!preview)
		return;

	if(s) {
		preview->setSource(0);
		// this deletes the texture in the preview
		delete s;
		s = 0;

	}

	// create a source reading the shm selected
	if (selectedItem) {

		GLuint tex = preview->getNewTextureIndex();
		try {
			// create a new source with a new texture index and the new parameters
			s = new SharedMemorySource(tex, 0, selectedItem->data(KEY_ROLE).toString(),
					selectedItem->data(SIZE_ROLE).toSize(), (QImage::Format) selectedItem->data(FORMAT_ROLE).toInt(),
					selectedItem->data(Qt::DisplayRole).toString(), selectedItem->data(INFO_ROLE).toString() );

		} catch (AllocationException &e){
			qCritical() << "SharedMemoryDialog|" << e.message();
			// free the OpenGL texture
			glDeleteTextures(1, &tex);
			// return an invalid pointer
			s = 0;
		}

		// apply the source to the preview
		preview->setSource(s);

	}

}


void SharedMemoryDialog::setCurrent(const QItemSelection & selected, const QItemSelection & deselected) {

	static QPushButton *okbutton = buttonBox->addButton(QDialogButtonBox::Ok);;

	if (selected.isEmpty()) {

		// cannot Ok this dialog without a selection
		okbutton->hide();

		// no item selected
		selectedItem = 0;

		// no preview if no item selected
		preview->hide();
		verticalLayout->removeItem(verticalLayout->itemAt(0));
	    verticalLayout->insertWidget(0, nopreview);
	    nopreview->show();

	} else
	{
		// can confirm
		okbutton->show ();

		// read parameters of currently selected item
		selectedItem = listExistingShmModel->itemFromIndex(selected.indexes().first());

		// show preview of item selected
		nopreview->hide();
		verticalLayout->removeItem(verticalLayout->itemAt(0));
	    verticalLayout->insertWidget(0, preview);
	    preview->show();
	}

	// create the new source to preview (and delete the former one)
	createSource();

}

