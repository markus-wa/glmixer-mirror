/*
 * CameraDialog.cpp
 *
 *  Created on: Dec 19, 2009
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
 *   Copyright 2009, 2018 Bruno Herbelin
 *
 */

#include "CameraDialog.moc"
#include "ui_CameraDialog.h"

#include "VideoStreamSource.h"
#include "common.h"
#include "CodecManager.h"

#ifdef GLM_OPENCV
#include "OpencvSource.h"
#endif

#ifdef Q_OS_MAC
#include "avfoundation.h"
#endif

#include <QDesktopServices>
#include <QDesktopWidget>
#include <QProcessEnvironment>



CameraDialog::CameraDialog(QWidget *parent, QSettings *settings) :
    QDialog(parent),
    ui(new Ui::CameraDialog),
    s(NULL), appSettings(settings)
{
    testingtimeout = new QTimer(this);
    testingtimeout->setSingleShot(true);
    connect(testingtimeout, SIGNAL(timeout()), this, SLOT(failedInfo()));

    respawn = new QTimer(this);
    respawn->setSingleShot(true);
    connect(respawn, SIGNAL(timeout()), this, SLOT(cancelSourcePreview()));

    ui->setupUi(this);

    // discard opencv if not available
#ifndef GLM_OPENCV
    ui->deviceSelection->removeTab( ui->deviceSelection->indexOf(ui->deviceOpenCV));
#endif

    // discard decklink if not available
    if ( !CodecManager::hasFormat("decklink") )
        ui->deviceSelection->removeTab( ui->deviceSelection->indexOf(ui->deviceDecklink) );

#ifdef Q_OS_LINUX
    ui->screenCaptureSelection->addItem( "Capture entire screen", i.key());
    ui->screenCaptureSelection->addItem( "Capture custom area", i.key());
#else
    ui->geometryBox->setVisible(false);
#endif

}

CameraDialog::~CameraDialog()
{
    if (s)
        delete s;
    delete ui;
}

void CameraDialog::showHelp()
{
    QDesktopServices::openUrl(QUrl("https://sourceforge.net/p/glmixer/wiki/Input%20devices%20support/", QUrl::TolerantMode));
}

QString CameraDialog::getUrl() const
{

    QString url = "";

#ifdef Q_OS_LINUX
    // webcam
    if ( ui->deviceSelection->currentWidget() == ui->deviceWebcam ) {
        // read data which gives the device id
        url = ui->webcamDevice->itemData( ui->webcamDevice->currentIndex()).toString();
    }
    // screen capture
    else if (ui->deviceSelection->currentWidget() == ui->deviceScreen ) {
        // read system DISPLAY
        url = QProcessEnvironment::systemEnvironment().value("DISPLAY");
    }
#endif
#ifdef Q_OS_MAC
    // webcam
    if (ui->deviceSelection->currentWidget() == ui->deviceWebcam ) {
        // url = "0:";
        // read data which gives the device id
        url = ui->webcamDevice->itemData( ui->webcamDevice->currentIndex()).toString();
    }
    // screen capture
    else if (ui->deviceSelection->currentWidget() == ui->deviceScreen ) {
        // url = "1:";
        // read index of the first screen
        url = ui->screenCaptureSelection->itemData( ui->screenCaptureSelection->currentIndex()).toString();
    }
#endif
#ifdef Q_OS_WIN 
    if (ui->deviceSelection->currentWidget() == ui->deviceScreen ) {
        // read desktop
        url = "desktop";
    }
#endif

    return url;
}

QString CameraDialog::getFormat() const
{
    QString format = "";

#ifdef Q_OS_LINUX
    // webcam
    if ( ui->deviceSelection->currentWidget() == ui->deviceWebcam ) {
        format = "video4linux2";
    }
    // screen capture
    else if (ui->deviceSelection->currentWidget() == ui->deviceScreen ) {
        format = "x11grab";
    }
#elif defined Q_OS_MAC

    format = "avfoundation";

#else 
    // webcam
    if ( ui->deviceSelection->currentWidget() == ui->deviceWebcam ) {
        format = "dshow";
    }
    // screen capture
    else if (ui->deviceSelection->currentWidget() == ui->deviceScreen ) {
        format = "gdigrab";
    }
#endif

    return format;
}

QHash<QString, QString> CameraDialog::getFormatOptions() const
{
    QHash<QString, QString> options;

#ifdef Q_OS_LINUX
    // webcam
    if ( ui->deviceSelection->currentWidget() == ui->deviceWebcam ) {

        switch(ui->webcamResolution->currentIndex()) {
        case 0:
        case 1:
            options["video_size"] = "1920x1080";
            break;
        case 2:
            options["video_size"] = "1280x720";
            break;
        case 3:
            options["video_size"] = "640x480";
            break;
        case 4:
            options["video_size"] = "320x240";
            break;
        }

        switch(ui->webcamFramerate->currentIndex()) {
        case 0:
        case 1:
            options["framerate"] = "30";
            break;
        case 2:
            options["framerate"] = "25";
            break;
        case 3:
            options["framerate"] = "15";
            break;
        }
    }
    // screen capture
    else if (ui->deviceSelection->currentWidget() == ui->deviceScreen ) {
        options["framerate"] = "30";
        int w = ui->screen_w_selection->itemData( ui->screen_w_selection->currentIndex()).toInt();
        options["video_size"] = QString("%1x%2").arg(w).arg(ui->screen_h->value());
        options["grab_x"] = QString::number(ui->screen_x->value());
        options["grab_y"] = QString::number(ui->screen_y->value());
        options["draw_mouse"] = ui->screen_cursor->isChecked() ? "1" : "0";
    }
#elif defined Q_OS_MAC
    // webcam
    if ( ui->deviceSelection->currentWidget() == ui->deviceWebcam ) {

       switch(ui->webcamResolution->currentIndex()) {
        case 1:
            options["video_size"] = "1920x1080";
            break;
        case 2:
            options["video_size"] = "1280x720";
            break;
        case 3:
            options["video_size"] = "640x480";
            break;
        case 4:
            options["video_size"] = "320x240";
            break;
        }
 
        switch(ui->webcamFramerate->currentIndex()) {
        case 0:
        case 1:
            options["framerate"] = "30";
            break;
        case 2:
            options["framerate"] = "25";
            break;
        case 3:
            options["framerate"] = "15";
            break;
        }

    }
    // screen capture
    else if (ui->deviceSelection->currentWidget() == ui->deviceScreen ) {
        options["framerate"] = "30";
        options["capture_cursor"] = ui->screen_cursor->isChecked() ? "1" : "0";
    }

#else 
    if (ui->deviceSelection->currentWidget() == ui->deviceScreen ) {
        options["framerate"] = "25";
        //int w = ui->screen_w_selection->itemData( ui->screen_w_selection->currentIndex()).toInt();
        //options["video_size"] = QString("%1x%2").arg(w).arg(ui->screen_h->value());
        //options["offset_x"] = QString::number(ui->screen_x->value());
        //options["offset_y"] = QString::number(ui->screen_y->value());
        //options["draw_mouse"] = ui->screen_cursor->isChecked() ? "1" : "0";
    }
#endif

    return options;
}

void CameraDialog::showEvent(QShowEvent *e){

    // read the device list
    ui->webcamDevice->clear();
    QHash<QString, QString> devices;

#ifdef Q_OS_LINUX
    devices = CodecManager::getDeviceList( "video4linux2" );
#elif defined Q_OS_MAC
    devices = avfoundation::getDeviceList();
#else 
    if ( !CodecManager::hasFormat("dshow") ) 
        ui->deviceWebcam->setEnabled(false);
    devices = CodecManager::getDeviceList( "dshow" );
#endif
    // fill-in list of devices
    QHashIterator<QString, QString> i(devices);
    while (i.hasNext()) {
        i.next();
        ui->webcamDevice->addItem(i.value(), i.key());
    }



#ifdef Q_OS_LINUX
    // read dimensions of the desktop to set screen capture maximum
    screendimensions = QApplication::desktop()->screen()->geometry();
    ui->screen_h->setMaximum(screendimensions.height());

    // fill-in
    ui->screen_w_selection->clear();
    int w =  screendimensions.width() ;
    while ( w > 255 ) {
        ui->screen_w_selection->addItem(QString::number(w),w);
        w = roundPowerOfTwo(w/2);
    }
    ui->screen_w_selection->setCurrentIndex(0);
    // update display
    setScreenCaptureArea( ui->screenCaptureSelection->currentIndex());

#elif defined Q_OS_MAC

    ui->screenCaptureSelection->clear();
    QHash<QString, QString> screens;
    screens = avfoundation::getScreenList();
    QHashIterator<QString, QString> j(screens);
    while (j.hasNext()) {
        j.next();
        ui->screenCaptureSelection->addItem(j.value(), j.key());
    }
#else 

#endif


    QWidget::showEvent(e);
}


void CameraDialog::done(int r)
{
    cancelSourcePreview();

    // save settings
    if (appSettings) {

    }

    QDialog::done(r);
}

void CameraDialog::updateScreenCaptureArea()
{
    // limit x and y
    int w = ui->screen_w_selection->itemData( ui->screen_w_selection->currentIndex()).toInt();
    ui->screen_x->setMaximum(screendimensions.width() - w);
    ui->screen_y->setMaximum(screendimensions.height() - ui->screen_h->value());

    cancelSourcePreview();
}

void CameraDialog::setScreenCaptureArea(int index)
{
#ifdef Q_OS_LINUX
    ui->geometryBox->setEnabled(index  != 0);

    // custom
    if (index == 0)  {
        ui->screen_x->setValue( screendimensions.x());
        ui->screen_y->setValue( screendimensions.y());
        ui->screen_w_selection->setCurrentIndex(0);
        ui->screen_h->setValue( screendimensions.height());
    }
#elif defined Q_OS_MAC
    
#else 
    
#endif

    cancelSourcePreview();
}

void CameraDialog::connectedInfo()
{
    testingtimeout->stop();
    ui->info->setCurrentIndex(2);
}

void CameraDialog::failedInfo()
{
    cancelSourcePreview();
    ui->info->setCurrentIndex(3);
    respawn->start(1000);
}

void CameraDialog::cancelSourcePreview(){

    testingtimeout->stop();
    ui->info->setCurrentIndex(0);

    // remove source from preview: this deletes the texture in the preview
    ui->preview->setSource(0);

    // delete previous
    if(s) {
        // delete the source:
        delete s;
        s = NULL;
    }
}

void CameraDialog::updateSourcePreview(){

    // texture for source
    GLuint tex = ui->preview->getNewTextureIndex();

#ifdef GLM_OPENCV
    if ( ui->deviceSelection->currentWidget() == ui->deviceOpenCV )
    {
        try {
            // create a new source with a new texture index and the new parameters
            s = new OpencvSource( getOpencvIndex(), OpencvSource::DEFAULT_MODE, tex, 0);

            QObject::connect(s, SIGNAL(failed()), this, SLOT(failedInfo()));
            QObject::connect(s, SIGNAL(playing(bool)), this, SLOT(connectedInfo()));

            // update GUI
            ui->info->setCurrentIndex(1);
            testingtimeout->start(5000);

        }
        catch (...)  {
            qCritical() << tr("Opencv Source Creation error; ");
            // free the OpenGL texture
            glDeleteTextures(1, &tex);
            // return an invalid pointer
            s = NULL;
        }
    }
    else
#endif
    {
        // create video stream
        VideoStream *vs = new VideoStream();
        // open with parameters
        vs->open(getUrl(), getFormat(), getFormatOptions());

        try {
            // create a new source with a new texture index and the new parameters
            s = new VideoStreamSource(vs, tex, 0);

            QObject::connect(s, SIGNAL(failed()), this, SLOT(failedInfo()));
            QObject::connect(vs, SIGNAL(openned()), this, SLOT(connectedInfo()));

            // update GUI
            ui->info->setCurrentIndex(1);
            testingtimeout->start(5000);

        }
        catch (...)  {
            qCritical() << tr("Video Stream Creation error; ");
            // free the OpenGL texture
            glDeleteTextures(1, &tex);
            // return an invalid pointer
            s = NULL;
        }
    }

    // apply the source to the preview (null pointer is ok to reset preview)
    ui->preview->setSource(s);
    ui->preview->playSource(true);

}


#ifdef GLM_OPENCV

bool CameraDialog::isOpencvSelected() const
{
    return ( ui->deviceSelection->currentWidget() == ui->deviceOpenCV ) ;
}

int CameraDialog::getOpencvIndex() const
{
    return ui->opencvId->currentIndex();
}

#endif

/*
#define CAMERA_PREVIEW 1

CameraDialog::CameraDialog(QWidget *parent, int startTabIndex) : QDialog(parent), s(0), preview(0)
{
    setupUi(this);

#ifndef CAMERA_PREVIEW
    preview = 0;
    showPreview->setEnabled(false);
    showPreview->hide();
    nopreview->setText(tr("Preview disabled in this version."));
#else
    preview = new SourceDisplayWidget(this);
    preview->setSizePolicy( QSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding) );
    preview->hide();
    QObject::connect(showPreview, SIGNAL(toggled(bool)), this, SLOT(setPreviewEnabled(bool)));
#endif

#ifdef GLM_OPENCV
    currentCameraIndex = -1;
    QObject::connect( indexSelection, SIGNAL(activated(int)), this, SLOT(setOpencvCamera(int)));
#endif

}


CameraDialog::~CameraDialog() {
    if (preview)
        delete preview;
}

void CameraDialog::createSource(){

    if (!preview)
        return;

    if(s) {
        preview->setSource(0);
        // this deletes the texture in the preview
        delete s;
        s = 0;
    }

#ifdef GLM_OPENCV
    if (currentCameraIndex >= 0) {

        if ( !OpencvSource::getExistingSourceForCameraIndex(currentCameraIndex) ) {

            GLuint tex = preview->getNewTextureIndex();
            try {
                s = (Source *) new OpencvSource(currentCameraIndex, OpencvSource::LOWRES_MODE, tex, 0);

            } catch (AllocationException &e){
                qCritical() << "Error creating OpenCV camera source; " << e.message();
                // free the OpenGL texture
                glDeleteTextures(1, &tex);
                // return an invalid pointer
                s = 0;
            }
            // apply the source to the preview
            preview->setSource(s);
            preview->playSource(true);
        } else
            preview->setSource( OpencvSource::getExistingSourceForCameraIndex(currentCameraIndex) );
    }
#endif

}


void CameraDialog::showEvent(QShowEvent *e){

#ifdef GLM_OPENCV
    setOpencvCamera(indexSelection->currentIndex());
#endif
    QWidget::showEvent(e);
}

void CameraDialog::done(int r){

    if (preview)
        preview->setSource(0);

    if (s) {
        delete s;
        s = 0;
    }

    QDialog::done(r);
}

void CameraDialog::setPreviewEnabled(bool on){

    // remove the top item
    verticalLayout->itemAt(0)->widget()->hide();
    verticalLayout->removeItem(verticalLayout->itemAt(0));

    // add a top idem according to preview mode
    if(on) {
        verticalLayout->insertWidget(0, preview);
        createSource();
    } else {
        verticalLayout->insertWidget(0, nopreview);
    }

    verticalLayout->itemAt(0)->widget()->show();
}

#ifdef GLM_OPENCV

void CameraDialog::setOpencvCamera(int i){

    currentCameraIndex = i;

    // create the source
    if (showPreview->isChecked())
        createSource();
}


int CameraDialog::modeOpencvCamera() const {

    return ModeSelection->currentIndex();
}

#endif

*/

