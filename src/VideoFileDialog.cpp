/*
 * videoFileDialog.cpp
 *
 *  Created on: Aug 3, 2009
 *      Author: bh
 */

#include <QFileDialog>
#include <QLayout>
#include <QLabel>

#include "VideoFileDialog.moc"

VideoFileDialog::VideoFileDialog( QWidget * parent, const QString & caption, const QString & directory, const QString & filter) : QFileDialog(parent, caption, directory, filter) {

    setFilter(tr("Video (*.mov *.avi *.wmv *.mpeg *.mp4 *.mpg *.vob *.swf *.flv);;Image (*.png *.jpg *.jpeg *.tif *.tiff *.gif *.tga *.sgi *.bmp *.ppm);;Any file (*.*)"));

    QLayout *grid = layout();

    pv = new QCheckBox(tr("Preview"), this);
    grid->addWidget(pv);

    preview = new VideoFileDialogPreview(this);
    preview->setEnabled(false);
    grid->addWidget(preview);

    QObject::connect(pv, SIGNAL(toggled(bool)), this, SLOT(setPreviewVisible(bool)));
    pv->setChecked(true);

	QObject::connect(this, SIGNAL(currentChanged ( const QString & )), preview, SLOT(showFilePreview(const QString &)));
    QObject::connect(preview, SIGNAL(error(QString)), this, SIGNAL(error(QString)));

    setFileMode(QFileDialog::ExistingFiles);
}

VideoFileDialog::~VideoFileDialog() {

    delete preview;
    delete pv;
}

void VideoFileDialog::setPreviewVisible(bool visible){

    preview->setVisible(visible);
    if (visible && !selectedFiles().empty())
    	preview->showFilePreview( this->selectedFiles().first() );
}

bool VideoFileDialog::configCustomSize(){

	return preview->customSizeCheckBox->isChecked();
}
