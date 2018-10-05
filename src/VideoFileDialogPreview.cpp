/*
 * videoFileDialogPreview.cpp
 *
 *  Created on: Aug 3, 2009
 *      Author: bh
 *
 *  This file is part of GLMixer.
 *
 *   GLMixer is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation, either version 3 of the License, or
 *   (at your option) any later version.
 *
 *   GLMixer is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with GLMixer.  If not, see <http://www.gnu.org/licenses/>.
 *
 *   Copyright 2009, 2012 Bruno Herbelin
 *
 */

#include <QWidget>
#include <QLayout>
#include <QLabel>

#include "common.h"
#include "glmixerdialogs.h"
#include "VideoFileDialogPreview.moc"
#include "VideoFileDialog.h"
#include "RenderingManager.h"

VideoFileDialogPreview::VideoFileDialogPreview(QWidget *parent) : QWidget(parent) {

    setupUi(this);
    is = NULL;

    if ( glewIsSupported("GL_EXT_texture_non_power_of_two") || glewIsSupported("GL_ARB_texture_non_power_of_two") ){
        customSizeCheckBox->setChecked(false);
        customSizeCheckBox->setEnabled(true);
    }
    else {
        customSizeCheckBox->setChecked(true);
        customSizeCheckBox->setEnabled(false);
    }


    QObject::connect(customSizeCheckBox, SIGNAL(clicked()), this, SLOT(updateFilePreview()));
    QObject::connect(hardwareDecodingcheckBox, SIGNAL(clicked()), this, SLOT(updateFilePreview()));

}

VideoFileDialogPreview::~VideoFileDialogPreview() {

    closeFilePreview();
}

void VideoFileDialogPreview::closeFilePreview() {

    if (is) {
        // unset video display
        previewWidget->setVideo(NULL);
        CodecNameLineEdit->clear();
        endLineEdit->clear();
        widthLineEdit->clear();
        // stop video
        is->stop();
        // delete video File
        delete is;
        is = NULL;
    }
}

void VideoFileDialogPreview::showFilePreview(const QString & file, bool tryHardwareCodec){

    // reset all to disabled
    startButton->setChecked( false );
    startButton->setEnabled( false );
    setEnabled(false);

    //reset view to blank screen
    previewWidget->setVideo(NULL);
    CodecNameLineEdit->clear();
    endLineEdit->clear();
    widthLineEdit->clear();

    if (is) {
        delete is;
        is = NULL;
    }

    QFileInfo fi(file);
    if ( fi.isFile() && isVisible() ) {

        if ( customSizeCheckBox->isChecked() )
            // custom size choosen
            is = new VideoFile(this, true, RenderingManager::getInstance()->getFrameBufferWidth(), RenderingManager::getInstance()->getFrameBufferHeight());
        else
            is = new VideoFile(this);

        Q_CHECK_PTR(is);

        // CONTROL signals from GUI to VideoFile
        QObject::connect(startButton, SIGNAL(toggled(bool)), is, SLOT(play(bool)));
        QObject::connect(seekBackwardButton, SIGNAL(clicked()), is, SLOT(seekBackward()));
        QObject::connect(seekForwardButton, SIGNAL(clicked()), is, SLOT(seekForward()));
        QObject::connect(seekBeginButton, SIGNAL(clicked()), is, SLOT(seekBegin()));
        // CONTROL signals from VideoFile to GUI
        QObject::connect(is, SIGNAL(running(bool)), startButton, SLOT(setChecked(bool)));
        QObject::connect(is, SIGNAL(running(bool)), videoControlFrame, SLOT(setEnabled(bool)));

        if ( is->open(file, tryHardwareCodec) ) {

            // enable all
            setEnabled(true);

            // activate preview
            previewWidget->setVideo(is);
            previewWidget->updateFrame(is->getFirstFrame());

            // fill in details
            CodecNameLineEdit->setText(is->getCodecName());
            CodecNameLineEdit->home(false);
            endLineEdit->setText( QString("%1 frames").arg(qMax(1, is->getNumFrames())) );
            widthLineEdit->setText( QString("%1 x %2 px").arg(is->getStreamFrameWidth()).arg(is->getStreamFrameHeight()));

            // is there more than one frame to play?
            if ( is->getNumFrames() > 1 ) {
                startButton->setEnabled( true );
                is->start();
            }

        }
    }
}

void VideoFileDialogPreview::updateFilePreview(){

    if (is && is->isOpen())
        showFilePreview(is->getFileName(), hardwareDecodingcheckBox->isChecked());
}


