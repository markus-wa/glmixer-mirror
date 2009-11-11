/*
 * glv.cpp
 *
 *  Created on: Jul 14, 2009
 *      Author: bh
 */

#include <QApplication>

#include "glmixer.moc"

#include "VideoFileDialog.h"
#include "MainRenderWidget.h"
#include "MixerViewWidget.h"


GLMixer::GLMixer ( QWidget *parent): QMainWindow ( parent ), tmpvideofile(NULL), refreshTimingTimer(0)
{
    setupUi ( this );

    // SWITCH central widget
    centralLayout->removeWidget(interactionView);
    delete interactionView;
    MixerViewWidget *tmp = new MixerViewWidget( (QWidget *)this, MainRenderWidget::getQGLWidget());
    interactionView = (glRenderWidget*) tmp;
    centralLayout->addWidget(interactionView, 0, 0, 1, 1);

    // QUIT event
    QObject::connect(actionQuit, SIGNAL(activated()), this, SLOT(close()));

    // Signals between GUI and VideoFileDisplay widget
    QObject::connect(actionKeep_aspect_ratio, SIGNAL(toggled(bool)), MainRenderWidget::getInstance(), SLOT(useAspectRatio(bool)));
    QObject::connect(actionFullscreen, SIGNAL(toggled(bool)), MainRenderWidget::getInstance(), SLOT(setFullScreen(bool)));

    // Init state
    vcontrolDockWidget->setEnabled(false);
    vinfoDockWidget->setEnabled(false);
    vinfoDockWidget->setVisible(false);
    vconfigDockWidget->setEnabled(false);
    vconfigDockWidget->setVisible(false);

    // Timer to update sliders and counters
    refreshTimingTimer = new QTimer(this);
    Q_CHECK_PTR(refreshTimingTimer);
    refreshTimingTimer->setInterval(200);
    QObject::connect(refreshTimingTimer, SIGNAL(timeout()), this, SLOT(refreshTiming()));
    QObject::connect(vcontrolDockWidget, SIGNAL(visibilityChanged(bool)), this, SLOT(updateRefreshTimerState()));

}

GLMixer::~GLMixer() {

    delete refreshTimingTimer;

}


void GLMixer::closeEvent ( QCloseEvent * event ){

	MainRenderWidget::getInstance()->deleteInstance();
	event->accept();
}

void GLMixer::updateRefreshTimerState(){

    if (tmpvideofile && !tmpvideofile->isPaused() && tmpvideofile->isRunning() && vcontrolDockWidget->isVisible())
        refreshTimingTimer->start();
    else
        refreshTimingTimer->stop();

}

void GLMixer::on_actionFormats_and_Codecs_activated(){

    VideoFile::displayFormatsCodecsInformation(QString::fromUtf8(":/glmixer/icons/video.png"));

}

void GLMixer::on_actionOpenGL_extensions_activated(){

    glRenderWidget::showGlExtensionsInformationDialog(QString::fromUtf8(":/glmixer/icons/display.png"));

}


void GLMixer::displayLogMessage(QString msg){

//    QMessageBox::information(this, "glv Log", msg, QMessageBox::Ok, QMessageBox::Ok);
    qDebug("Log %s", msg.toLatin1().data());

}

void GLMixer::displayErrorMessage(QString msg){

    QMessageBox::critical(this, "glv error", msg, QMessageBox::Ok, QMessageBox::Ok);

}



QString GLMixer::OpenVideo(VideoFile **is) {
    QString fileName = QString();
    static QDir d = QDir::home();

#ifndef NO_VIDEO_FILE_DIALOG_PREVIEW
    static VideoFileDialog *mfd = new VideoFileDialog(this, "Open a video or a picture", d.absolutePath());

    QObject::connect(mfd, SIGNAL(error(QString)), this, SLOT(displayErrorMessage(QString)));

    QStringList fileNames;
    if (mfd->exec())
        fileNames = mfd->selectedFiles();

    if (!fileNames.empty())
        fileName = fileNames.first();

#else
    fileName = QFileDialog::getOpenFileName(this, tr("Open File"),
                                                    d.absolutePath(),
                                                    tr("Video (*.mov *.avi *.wmv *.mpeg *.mp4 *.mpg *.vob *.swf *.flv);;Image (*.png *.jpg *.jpeg *.tif *.tiff *.gif *.tga *.sgi *.bmp)"));

    d.setPath(fileName);
#endif


    if ( !fileName.isEmpty() && QFileInfo(fileName).isFile() ) {

        if ( !mfd->customSizeChecked() && (glRenderWidget::glSupportsExtension("GL_EXT_texture_non_power_of_two") || glRenderWidget::glSupportsExtension("GL_ARB_texture_non_power_of_two") ) )
            *is = new VideoFile(this);
        else
            *is = new VideoFile(this, true, SWS_FAST_BILINEAR, PIX_FMT_RGB24);

    }

    return fileName;
}

void GLMixer::on_actionOpen_activated(){

    if (tmpvideofile) {
        delete tmpvideofile;
        tmpvideofile = NULL;
    }

    QObject::disconnect(startButton, 0, 0, 0);
    QObject::disconnect(pauseButton, 0, 0, 0);
    QObject::disconnect(seekBackwardButton, 0, 0, 0);
    QObject::disconnect(seekForwardButton, 0, 0, 0);
    QObject::disconnect(seekBeginButton, 0, 0, 0);
    QObject::disconnect(playSpeedBox, 0, 0, 0);
    QObject::disconnect(videoLoopButton, 0, 0, 0);
    QObject::disconnect(markInButton, 0, 0, 0);
    QObject::disconnect(markOutButton, 0, 0, 0);
    QObject::disconnect(resetMarkInButton, 0, 0, 0);
    QObject::disconnect(resetMarkOutButton, 0, 0, 0);
    QObject::disconnect(dirtySeekCheckBox, 0, 0, 0);
    QObject::disconnect(resetToBlackCheckBox, 0, 0, 0);
    QObject::disconnect(restartWhereStoppedCheckBox, 0, 0, 0);

    refreshTimingTimer->stop();
    QString fileName = OpenVideo(&tmpvideofile);

    if (tmpvideofile){
        // forward error messages to display fn
        QObject::connect(tmpvideofile, SIGNAL(error(QString)), this, SLOT(displayErrorMessage(QString)));

        // sets the video source for the video display widget
//        videowidget->setVideo(source);
        MainRenderWidget::getInstance()->createSource(tmpvideofile);

		actionOpen_right->setEnabled(true);

        // CONTROL signals from GUI to VideoFile
        QObject::connect(startButton, SIGNAL(toggled(bool)), tmpvideofile, SLOT(play(bool)));
        QObject::connect(pauseButton, SIGNAL(toggled(bool)), tmpvideofile, SLOT(pause(bool)));
        QObject::connect(seekBackwardButton, SIGNAL(clicked()), tmpvideofile, SLOT(seekBackward()));
        QObject::connect(seekForwardButton, SIGNAL(clicked()), tmpvideofile, SLOT(seekForward()));
        QObject::connect(seekBeginButton, SIGNAL(clicked()), tmpvideofile, SLOT(seekBegin()));
        QObject::connect(playSpeedBox, SIGNAL(currentIndexChanged(int)), tmpvideofile, SLOT(setPlaySpeed(int)));
        QObject::connect(videoLoopButton, SIGNAL(toggled(bool)), tmpvideofile, SLOT(setLoop(bool)));
        QObject::connect(markInButton, SIGNAL(clicked()), tmpvideofile, SLOT(setMarkIn()));
        QObject::connect(markOutButton, SIGNAL(clicked()), tmpvideofile, SLOT(setMarkOut()));
        QObject::connect(resetMarkInButton, SIGNAL(clicked()), tmpvideofile, SLOT(resetMarkIn()));
        QObject::connect(resetMarkOutButton, SIGNAL(clicked()), tmpvideofile, SLOT(resetMarkOut()));
        QObject::connect(dirtySeekCheckBox, SIGNAL(toggled(bool)), tmpvideofile, SLOT(setOptionAllowDirtySeek(bool)));
        QObject::connect(resetToBlackCheckBox, SIGNAL(toggled(bool)), tmpvideofile, SLOT(setOptionRevertToBlackWhenStop(bool)));
        QObject::connect(restartWhereStoppedCheckBox, SIGNAL(toggled(bool)), tmpvideofile, SLOT(setOptionRestartToMarkIn(bool)));

        // DISPLAY consistency from VideoFile to GUI
        QObject::connect(tmpvideofile, SIGNAL(paused(bool)), pauseButton, SLOT(setChecked(bool)));
        QObject::connect(tmpvideofile, SIGNAL(running(bool)), startButton, SLOT(setChecked(bool)));
        QObject::connect(tmpvideofile, SIGNAL(running(bool)), videoControlFrame, SLOT(setEnabled(bool)));
        QObject::connect(tmpvideofile, SIGNAL(running(bool)), speedControlFrame, SLOT(setEnabled(bool)));
        QObject::connect(tmpvideofile, SIGNAL(running(bool)), timingControlFrame, SLOT(setEnabled(bool)));
        QObject::connect(tmpvideofile, SIGNAL(info(QString)), statusbar, SLOT(showMessage(QString)));

        // Consistency and update timer control from VideoFile
        QObject::connect(tmpvideofile, SIGNAL(markingChanged()), this, SLOT(updateMarks()));
        QObject::connect(tmpvideofile, SIGNAL(running(bool)), this, SLOT(updateRefreshTimerState()));
        QObject::connect(tmpvideofile, SIGNAL(paused(bool)), this, SLOT(updateRefreshTimerState()));

        // can we open the file ?
        if ( tmpvideofile->open(fileName) ) {
            vinfoDockWidget->setEnabled(true);
            FileNameLineEdit->setText(fileName);
            CodecNameLineEdit->setText(tmpvideofile->getCodecName());
            // display size (special case when power of two dimensions are generated
            if (tmpvideofile->getStreamFrameWidth() != tmpvideofile->getFrameWidth() || tmpvideofile->getStreamFrameHeight() != tmpvideofile->getFrameHeight()) {
                widthLineEdit->setText( QString("%1 (%2)").arg(tmpvideofile->getStreamFrameWidth()).arg(tmpvideofile->getFrameWidth()));
                heightLineEdit->setText( QString("%1 (%2)").arg(tmpvideofile->getStreamFrameHeight()).arg(tmpvideofile->getFrameHeight()));
            } else {
                widthLineEdit->setText( QString("%1").arg(tmpvideofile->getFrameWidth()));
                heightLineEdit->setText( QString("%1").arg(tmpvideofile->getFrameHeight()));
            }
            framerateLineEdit->setText(QString().setNum(tmpvideofile->getFrameRate(),'f',2));
            videoLoopButton->setChecked(tmpvideofile->isLoop());
            actionShow_frames->setEnabled(true);

            // is there more than one frame ?
            if ( tmpvideofile->getEnd() > 1 ) {
                // yes, its a video, we can control it
                vcontrolDockWidget->setEnabled(true);
                vconfigDockWidget->setEnabled(true);
                // display times
                on_actionShow_frames_toggled(actionShow_frames->isChecked());

                // restore config
                tmpvideofile->setOptionRestartToMarkIn( restartWhereStoppedCheckBox->isChecked());
                tmpvideofile->setOptionRevertToBlackWhenStop( resetToBlackCheckBox->isChecked());
                tmpvideofile->setOptionAllowDirtySeek(dirtySeekCheckBox->isChecked());

                // only the speed is not connected to signals:
                playSpeedBox->setCurrentIndex(3);

                startButton->setEnabled( true );
            } else {
                // no, its a picture, we can't control it
                vcontrolDockWidget->setEnabled(false);
                vconfigDockWidget->setEnabled(false);
                // show some info even if one frame only
                endLineEdit->setText( QString().setNum( tmpvideofile->getEnd()) );
                timeLineEdit->setText( QString().setNum( tmpvideofile->getBegin()) );
                markInLineEdit->setText( QString().setNum( tmpvideofile->getMarkIn() ));
                markOutLineEdit->setText( QString().setNum( tmpvideofile->getMarkOut() ));
            }
        }
    }
}


void GLMixer::on_actionShow_frames_toggled(bool on){

    if (on){
        endLineEdit->setText( QString().setNum(tmpvideofile->getEnd()) );
        timeLineEdit->setText( QString().setNum(tmpvideofile->getBegin()) );
    } else {
        endLineEdit->setText( tmpvideofile->getTimeFromFrame(tmpvideofile->getEnd()) );
        timeLineEdit->setText( tmpvideofile->getTimeFromFrame(tmpvideofile->getBegin()) );
    }
    updateMarks();
}

void GLMixer::on_markInSlider_sliderReleased (){

    double percent = (double)(markInSlider->value())/1000.0;
    int64_t pos = (int64_t) ( tmpvideofile->getEnd()  * percent );
    pos += tmpvideofile->getBegin();
    tmpvideofile->setMarkIn(pos);
}

void GLMixer::on_markOutSlider_sliderReleased (){

    double percent = (double)(markOutSlider->value())/1000.0;
    int64_t pos = (int64_t) ( tmpvideofile->getEnd()  * percent );
    pos += tmpvideofile->getBegin();
    tmpvideofile->setMarkOut(pos);
}

void GLMixer::refreshTiming(){

    if (!skipNextRefresh) {

        int f_percent = (int) ( (double)( tmpvideofile->getCurrentFrameTime() - tmpvideofile->getBegin() ) / (double)( tmpvideofile->getEnd() - tmpvideofile->getBegin() ) * 1000.0) ;
        frameSlider->setValue(f_percent);

        if (actionShow_frames->isChecked())
            timeLineEdit->setText( QString().setNum(tmpvideofile->getCurrentFrameTime()) );
        else
            timeLineEdit->setText( tmpvideofile->getTimeFromFrame(tmpvideofile->getCurrentFrameTime()) );
    }
    else
        skipNextRefresh = false;

}



void  GLMixer::on_frameSlider_sliderPressed (){

    // do not update slider position automatically anymore ; this interferes with user input
    refreshTimingTimer->stop();
}

void  GLMixer::on_frameSlider_sliderReleased (){

    // slider moved, frame was displayed, still paused; we un-pause if it was playing.
	tmpvideofile->pause(pauseButton->isChecked());

    // restart the refresh timer if it should be
    updateRefreshTimerState();
}

void GLMixer::on_frameSlider_sliderMoved (int v){

    // disconnect the button from the VideoFile signal ; this way when we'll unpause bellow, the button will keep its state
    QObject::disconnect(tmpvideofile, SIGNAL(paused(bool)), pauseButton, SLOT(setChecked(bool)));

    // the trick; call a method when the frame will be ready!
    QObject::connect(tmpvideofile, SIGNAL(frameReady(int)), this, SLOT(pauseAfterFrame()));

    // compute where we should jump to
    double percent = (double)(v)/ (double)frameSlider->maximum();
    int64_t pos = (int64_t) ( tmpvideofile->getEnd()  * percent );
    pos += tmpvideofile->getBegin();

    // request seek ; we need to have the VideoFile process running to go there
    tmpvideofile->seekToPosition(pos);

    // let the VideoFile run till it displays the frame seeked
    tmpvideofile->pause(false);

    // cosmetics to show the time of the frame (refreshTiming disabled)
    if (actionShow_frames->isChecked())
        timeLineEdit->setText( QString().setNum(pos) );
    else
        timeLineEdit->setText( tmpvideofile->getTimeFromFrame(pos) );
}


void GLMixer::on_frameSlider_actionTriggered (int a) {


    switch (a) {
        case QAbstractSlider::SliderMove: // wheel
        case QAbstractSlider::SliderSingleStepAdd :
        case QAbstractSlider::SliderSingleStepSub :
        case QAbstractSlider::SliderPageStepAdd : // clic forward
        case QAbstractSlider::SliderPageStepSub : // clic backward
            // avoid jump in slider (due to timer refresh of the slider position from movie values)
            skipNextRefresh = true;

            if (pauseButton->isChecked())
                on_frameSlider_sliderMoved (frameSlider->sliderPosition ());
            else{
                // compute where we should jump to
                double percent = (double)(frameSlider->sliderPosition ())/ (double)frameSlider->maximum();
                int64_t pos = (int64_t) ( tmpvideofile->getEnd()  * percent );
                pos += tmpvideofile->getBegin();

                // request seek ; we need to have the VideoFile process running to go there
                tmpvideofile->seekToPosition(pos);

            }
            // avoid jump in slider (due to timer refresh of the slider position from movie values)
            skipNextRefresh = true;
            break;
    }

}

// OLD CODE for embeding / disembeding a widget : maybe to use for multiple views
//	if (on) {
//		centralLayout->removeWidget(videowidget);
//		videowidget->setParent(NULL, Qt::Window);  // You may want other widget flags/positions...
//		videowidget->show();
//		videowidget->move(100,100);
//	}	else {
//		videowidget->setWindowState (Qt::WindowNoState);
//		videowidget->setParent(centralwidget, Qt::Widget);
//		centralLayout->addWidget(videowidget, 0, 0, 1, 1);
//	}



void GLMixer::pauseAfterFrame (){

	tmpvideofile->pause(true);

    // do not keep calling pause method for each frame !
    QObject::disconnect(tmpvideofile, SIGNAL(frameReady(int)), this, SLOT(pauseAfterFrame()));
    // reconnect the pause button
    QObject::connect(tmpvideofile, SIGNAL(paused(bool)), pauseButton, SLOT(setChecked(bool)));

}


void GLMixer::updateMarks (){

    int i_percent = (int) ( (double)( tmpvideofile->getMarkIn() - tmpvideofile->getBegin() ) / (double)( tmpvideofile->getEnd() - tmpvideofile->getBegin() ) * 1000.0) ;
    int o_percent = (int) ( (double)( tmpvideofile->getMarkOut() - tmpvideofile->getBegin() ) / (double)( tmpvideofile->getEnd() - tmpvideofile->getBegin() ) * 1000.0) ;

    markInSlider->setValue(i_percent);
    markOutSlider->setValue(o_percent);

    if (actionShow_frames->isChecked()) {
        markInLineEdit->setText( QString().setNum( tmpvideofile->getMarkIn() ));
        markOutLineEdit->setText( QString().setNum( tmpvideofile->getMarkOut() ));
    } else {
        markInLineEdit->setText( tmpvideofile->getTimeFromFrame( tmpvideofile->getMarkIn() ));
        markOutLineEdit->setText( tmpvideofile->getTimeFromFrame( tmpvideofile->getMarkOut() ));
    }

}


void GLMixer::on_actionAbout_activated(){

	QString msg = QString("GLMixer : OpenGL Video Mixer for live mix\n\n");
	msg.append(QString("Author:     \tBruno Herbelin\n"));
	msg.append(QString("Contact:    \tbruno.herbelin@gmail.com\n"));
	msg.append(QString("License:    \tGPL\n"));
	msg.append(QString("Version:    \t%1\n").arg(GLMIXER_VERSION));
	msg.append("\nGlMixer is a video mixing software WITHOUT sound.\nIt is in early stage of development ;)...");
	QMessageBox::information(this, "About GlMixer", msg, QMessageBox::Ok, QMessageBox::Ok);

}

