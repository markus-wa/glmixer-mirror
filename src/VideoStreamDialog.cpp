#include "VideoStreamDialog.moc"
#include "ui_VideoStreamDialog.h"
#include "glmixer.h"
#include "VideoStreamSource.h"

#include <QDesktopServices>

VideoStreamDialog::VideoStreamDialog(QWidget *parent, QSettings *settings) :
    QDialog(parent),
    ui(new Ui::VideoStreamDialog),
    s(NULL), appSettings(settings)
{
    testingtimeout = new QTimer(this);
    testingtimeout->setSingleShot(true);
    connect(testingtimeout, SIGNAL(timeout()), this, SLOT(failedInfo()));

    respawn = new QTimer(this);
    respawn->setSingleShot(true);
    connect(respawn, SIGNAL(timeout()), this, SLOT(cancelSourcePreview()));

    ui->setupUi(this);

    // restore settings
    if (appSettings) {
        if (appSettings->contains("VideoStreamURL"))
            ui->URL->setText(appSettings->value("VideoStreamURL").toString());
        if (appSettings->contains("dialogStreamGeometry"))
            restoreGeometry(appSettings->value("dialogStreamGeometry").toByteArray());
    }
}

VideoStreamDialog::~VideoStreamDialog()
{
    if (s)
        delete s;
    delete ui;
}

void VideoStreamDialog::showHelp()
{
    QDesktopServices::openUrl(QUrl("https://sourceforge.net/p/glmixer/wiki/Network%20Stream%20Capture%20-%20MPEG%20TS/", QUrl::TolerantMode));
}

QString VideoStreamDialog::getUrl() {

    //    QString urlstream = "rtp://@239.0.0.1:5004";
    QString urlstream = "udp://@:1234";
    //    QString urlstream = "udp://@239.0.0.1:1234";
    //    urlstream = "https://youtu.be/fmGM5vhy2IM";
    //    urlstream = "v4l2://";

    urlstream = ui->URL->text();

    return urlstream;
}

QString VideoStreamDialog::getFormat() {

    QString format = "";

    // Selected UDP
    if (ui->UDPStream->isChecked())
        format = "mpegts";
    // Selected RTP
    else if (ui->RTPStream->isChecked())
        format = "rtp_mpegts";

    return format;
}

void VideoStreamDialog::showEvent(QShowEvent *e){

    updateURL();

    QWidget::showEvent(e);
}


void VideoStreamDialog::done(int r){

    cancelSourcePreview();

    // save settings
    if (appSettings) {
        appSettings->setValue("VideoStreamURL", getUrl());
        appSettings->setValue("dialogStreamGeometry", saveGeometry());
    }

    QDialog::done(r);
}

void VideoStreamDialog::updateURL(){

    // Selected UDP
    if (ui->UDPStream->isChecked()) {
        ui->URL->setText(QString("udp://@:%1").arg(ui->UDPPort->value()));
    }
    // Selected RTP
    else if (ui->RTPStream->isChecked()) {
        ui->URL->setText(QString("rtp://@127.0.0.1:%1").arg(ui->RTPPort->value()));
    }

    cancelSourcePreview();
}

void VideoStreamDialog::connectedInfo()
{
    testingtimeout->stop();
    ui->info->setCurrentIndex(2);
}

void VideoStreamDialog::failedInfo()
{
    ui->info->setCurrentIndex(3);
    respawn->start(1000);
}

void VideoStreamDialog::cancelSourcePreview(){

    testingtimeout->stop();
    ui->info->setCurrentIndex(0);
    ui->connect->setEnabled(true);

    // remove source from preview: this deletes the texture in the preview
    ui->preview->setSource(0);

    // delete previous
    if(s) {
        // delete the source:
        delete s;
        s = NULL;
    }
}

void VideoStreamDialog::updateSourcePreview(){

    VideoStream *vs = new VideoStream();
    vs->open(getUrl(), getFormat());

    GLuint tex = ui->preview->getNewTextureIndex();

    try {
        // create a new source with a new texture index and the new parameters
        s = new VideoStreamSource(vs, tex, 0);

        QObject::connect(s, SIGNAL(failed()), this, SLOT(failedInfo()));
        QObject::connect(vs, SIGNAL(openned()), this, SLOT(connectedInfo()));
        QObject::connect(vs, SIGNAL(openned()), s, SLOT(updateAspectRatioStream()));

        // update GUI
        ui->info->setCurrentIndex(1);
        ui->connect->setEnabled(false);
        testingtimeout->start(10000);

    }
    catch (...)  {
        qCritical() << tr("Video Network Stream Creation error; ");
        // free the OpenGL texture
        glDeleteTextures(1, &tex);
        // return an invalid pointer
        s = NULL;
    }

    // apply the source to the preview (null pointer is ok to reset preview)
    ui->preview->setSource(s);
    ui->preview->playSource(true);

}
