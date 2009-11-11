/*
 * videoFileDialogPreview.cpp
 *
 *  Created on: Aug 3, 2009
 *      Author: bh
 */

#include <QWidget>
#include <QLayout>
#include <QLabel>

#include "VideoFileDialogPreview.moc"

VideoFileDialogPreview::VideoFileDialogPreview(QWidget *parent) : QWidget(parent) {

    setupUi(this);
    is = NULL;

    if ( VideoFileDisplayWidget::glSupportsExtension("GL_EXT_texture_non_power_of_two") || VideoFileDisplayWidget::glSupportsExtension("GL_ARB_texture_non_power_of_two") )
        customSizeCheckBox->setEnabled(true);
    else {
        customSizeCheckBox->setChecked(true);
        customSizeCheckBox->setEnabled(false);
    }

}

VideoFileDialogPreview::~VideoFileDialogPreview() {

    if (is) {
        // unset video display
        previewWidget->setVideo(NULL);
        // stop video
        is->stop();
        // delete video File
        delete is;
    }
}


void VideoFileDialogPreview::showFilePreview(const QString & file){

    // reset all to disabled
    startButton->setChecked( false );
    startButton->setEnabled( false );
    setEnabled(false);

    //reset view to blank screen
    previewWidget->setVideo(NULL);

    if (is) {
        delete is;
        is = NULL;
    }

    QFileInfo fi(file);
    if ( fi.isFile() && isVisible() ) {

        if ( customSizeCheckBox->isChecked() )
            //  non-power of two supporting hardware;
            is = new VideoFile(this, true);
        else
            is = new VideoFile(this);

        // transfer error signal
        QObject::connect(is, SIGNAL(error(QString)), this, SIGNAL(error(QString)));

        // CONTROL signals from GUI to VideoFile
        QObject::connect(startButton, SIGNAL(toggled(bool)), is, SLOT(play(bool)));
        QObject::connect(seekBackwardButton, SIGNAL(clicked()), is, SLOT(seekBackward()));
        QObject::connect(seekForwardButton, SIGNAL(clicked()), is, SLOT(seekForward()));
        QObject::connect(seekBeginButton, SIGNAL(clicked()), is, SLOT(seekBegin()));
        // CONTROL signals from VideoFile to GUI
        QObject::connect(is, SIGNAL(running(bool)), startButton, SLOT(setChecked(bool)));
        QObject::connect(is, SIGNAL(running(bool)), videoControlFrame, SLOT(setEnabled(bool)));

        if ( is->open(file) ) {

            // enable all
            setEnabled(true);
            // activate preview
            previewWidget->setVideo(is);
            previewWidget->updateFrame(-1);
            // fill in details
            CodecNameLineEdit->setText(is->getCodecName());
            endLineEdit->setText( is->getTimeFromFrame(is->getEnd()) );
            // is there more than one frame ?
            if ( is->getEnd() > 1 ) {
                startButton->setEnabled( true );
                is->start();
            }
            // display size
            if (customSizeCheckBox->isChecked()) {
                widthLineEdit->setText( QString("%1 (%2)").arg(is->getStreamFrameWidth()).arg(is->getFrameWidth()));
                heightLineEdit->setText( QString("%1 (%2)").arg(is->getStreamFrameHeight()).arg(is->getFrameHeight()));
            } else {
                widthLineEdit->setText( QString("%1").arg(is->getFrameWidth()));
                heightLineEdit->setText( QString("%1").arg(is->getFrameHeight()));
            }
            previewWidget->setFixedWidth( previewWidget->height() * is->getStreamAspectRatio() );
        }


    }
}

void VideoFileDialogPreview::on_customSizeCheckBox_toggled(bool on){

    if (is && is->isOpen())
        showFilePreview(is->getFileName());

}

