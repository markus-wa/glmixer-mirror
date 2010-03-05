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

bool VideoFileDialog::_configPreviewVisible = true;
bool VideoFileDialog::_configCustomSize = false;
QString VideoFileDialog::_filterSelected = QString();

VideoFileDialog::VideoFileDialog( QWidget * parent, const QString & caption, const QString & directory, const QString & filter) : QFileDialog(parent, caption, directory, filter) {

    setFilter(tr("Video (*.mov *.avi *.wmv *.mpeg *.mp4 *.mpg *.vob *.swf *.flv);;Image (*.png *.jpg *.jpeg *.tif *.tiff *.gif *.tga *.sgi *.bmp *.ppm);;Any file (*.*)"));

    if ( _filterSelected.isEmpty())
        _filterSelected = this->selectedFilter();
    else
        selectFilter(_filterSelected);

    QObject::connect(this, SIGNAL(filterSelected( const QString & )), this, SLOT(rememberFilter(const QString &)));

    QLayout *grid = layout();

    pv = new QCheckBox(tr("Preview"), this);
    grid->addWidget(pv);

    preview = new VideoFileDialogPreview(this);
    preview->setEnabled(false);
    grid->addWidget(preview);

    setPreviewVisible(_configPreviewVisible);
    QObject::connect(pv, SIGNAL(toggled(bool)), this, SLOT(setPreviewVisible(bool)));

	QObject::connect(this, SIGNAL(currentChanged ( const QString & )), preview, SLOT(showFilePreview(const QString &)));
    QObject::connect(preview, SIGNAL(error(QString)), this, SIGNAL(error(QString)));

    setFileMode(QFileDialog::ExistingFiles);
}

VideoFileDialog::~VideoFileDialog() {

    delete preview;
    delete pv;
}


void VideoFileDialog::rememberFilter(const QString &){

    _filterSelected = this->selectedFilter();

    this->selectFile("");
    emit currentChanged("");
}


void VideoFileDialog::setPreviewVisible(bool visible){

    _configPreviewVisible = visible;

    pv->setChecked(_configPreviewVisible);
    preview->setVisible(_configPreviewVisible);
    if (visible && !selectedFiles().empty())
    	preview->showFilePreview( this->selectedFiles().first() );
}

