/*
 * glv.cpp
 *
 *  Created on: Jul 14, 2009
 *      Author: bh
 */

#include <QApplication>

#include "glmixer.moc"

#include "CameraDialog.h"
#include "VideoFileDialog.h"
#include "MainRenderWidget.h"
#include "MixerViewWidget.h"


GLMixer::GLMixer ( QWidget *parent): QMainWindow ( parent ), selectedSourceVideoFile(NULL), refreshTimingTimer(0)
{
    setupUi ( this );

#ifndef OPEN_CV
    actionCamera->setEnabled(false);
#endif

    // SET central widget to MIXER VIEW
    centralLayout->removeWidget(interactionView);
    delete interactionView;
    MixerViewWidget *tmp = new MixerViewWidget( (QWidget *)this, MainRenderWidget::getQGLWidget());
    interactionView = (glRenderWidget*) tmp;
    centralLayout->addWidget(interactionView, 0, 0, 1, 1);

    // signal from source management in MainRenderWidget
    QObject::connect(MainRenderWidget::getInstance(), SIGNAL(currentSourceChanged(SourceSet::iterator)), this, SLOT(controlSource(SourceSet::iterator) ) );

    // QUIT event
    QObject::connect(actionQuit, SIGNAL(activated()), this, SLOT(close()));

    // Signals between GUI and VideoFileDisplay widget
    QObject::connect(actionKeep_aspect_ratio, SIGNAL(toggled(bool)), MainRenderWidget::getInstance(), SLOT(useRenderingAspectRatio(bool)));
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

	MainRenderWidget::getInstance()->deleteInstance();

}


void GLMixer::closeEvent ( QCloseEvent * event ){
	MainRenderWidget::getInstance()->close();
	event->accept();
}

void GLMixer::updateRefreshTimerState(){

    if (selectedSourceVideoFile && !selectedSourceVideoFile->isPaused() && selectedSourceVideoFile->isRunning() && vcontrolDockWidget->isVisible())
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
    bool customsize = false;


#ifndef NO_VIDEO_FILE_DIALOG_PREVIEW
    static VideoFileDialog *mfd = new VideoFileDialog(this, "Open a video or a picture", d.absolutePath());

    mfd->setModal(false);

    QObject::connect(mfd, SIGNAL(error(QString)), this, SLOT(displayErrorMessage(QString)));

    QStringList fileNames;
    if (mfd->exec())
        fileNames = mfd->selectedFiles();

    if (!fileNames.empty())
        fileName = fileNames.first();

    customsize = mfd->customSizeChecked();
#else
    fileName = QFileDialog::getOpenFileName(this, tr("Open File"),
                                                    d.absolutePath(),
                                                    tr("Video (*.mov *.avi *.wmv *.mpeg *.mp4 *.mpg *.vob *.swf *.flv);;Image (*.png *.jpg *.jpeg *.tif *.tiff *.gif *.tga *.sgi *.bmp)"));

    d.setPath(fileName);
#endif


    if ( !fileName.isEmpty() && QFileInfo(fileName).isFile() ) {

        if ( !customsize && (glRenderWidget::glSupportsExtension("GL_EXT_texture_non_power_of_two") || glRenderWidget::glSupportsExtension("GL_ARB_texture_non_power_of_two") ) )
            *is = new VideoFile(this);
        else
            *is = new VideoFile(this, true, SWS_FAST_BILINEAR, PIX_FMT_RGB24);

    }

    return fileName;
}

void GLMixer::on_actionOpen_activated(){

//    if (selectedSourceVideoFile) {
//        //delete selectedSourceVideoFile;
//        selectedSourceVideoFile = NULL;
//    }

//    QObject::disconnect(startButton, 0, 0, 0);
//    QObject::disconnect(pauseButton, 0, 0, 0);
//    QObject::disconnect(seekBackwardButton, 0, 0, 0);
//    QObject::disconnect(seekForwardButton, 0, 0, 0);
//    QObject::disconnect(seekBeginButton, 0, 0, 0);
//    QObject::disconnect(playSpeedBox, 0, 0, 0);
//    QObject::disconnect(videoLoopButton, 0, 0, 0);
//    QObject::disconnect(markInButton, 0, 0, 0);
//    QObject::disconnect(markOutButton, 0, 0, 0);
//    QObject::disconnect(resetMarkInButton, 0, 0, 0);
//    QObject::disconnect(resetMarkOutButton, 0, 0, 0);
//    QObject::disconnect(dirtySeekCheckBox, 0, 0, 0);
//    QObject::disconnect(resetToBlackCheckBox, 0, 0, 0);
//    QObject::disconnect(restartWhereStoppedCheckBox, 0, 0, 0);
//
//    refreshTimingTimer->stop();
    VideoFile *newSourceVideoFile = NULL;
    QString fileName = OpenVideo(&newSourceVideoFile);

    if (newSourceVideoFile){
        // forward error messages to display
        QObject::connect(newSourceVideoFile, SIGNAL(error(QString)), this, SLOT(displayErrorMessage(QString)));
        QObject::connect(newSourceVideoFile, SIGNAL(info(QString)), statusbar, SLOT(showMessage(QString)));
        // can we open the file ?
        if ( newSourceVideoFile->open(fileName) ) {
        	// create the source as it is a valid video file (this also set it to be the current source)
        	MainRenderWidget::getInstance()->addSource(newSourceVideoFile);
        }
    }

//    if (selectedSourceVideoFile){
//        // forward error messages to display fn
////        QObject::connect(selectedSourceVideoFile, SIGNAL(error(QString)), this, SLOT(displayErrorMessage(QString)));
//
//
//
//        // CONTROL signals from GUI to VideoFile
//        QObject::connect(startButton, SIGNAL(toggled(bool)), selectedSourceVideoFile, SLOT(play(bool)));
//        QObject::connect(pauseButton, SIGNAL(toggled(bool)), selectedSourceVideoFile, SLOT(pause(bool)));
//        QObject::connect(seekBackwardButton, SIGNAL(clicked()), selectedSourceVideoFile, SLOT(seekBackward()));
//        QObject::connect(seekForwardButton, SIGNAL(clicked()), selectedSourceVideoFile, SLOT(seekForward()));
//        QObject::connect(seekBeginButton, SIGNAL(clicked()), selectedSourceVideoFile, SLOT(seekBegin()));
//        QObject::connect(playSpeedBox, SIGNAL(currentIndexChanged(int)), selectedSourceVideoFile, SLOT(setPlaySpeed(int)));
//        QObject::connect(videoLoopButton, SIGNAL(toggled(bool)), selectedSourceVideoFile, SLOT(setLoop(bool)));
//        QObject::connect(markInButton, SIGNAL(clicked()), selectedSourceVideoFile, SLOT(setMarkIn()));
//        QObject::connect(markOutButton, SIGNAL(clicked()), selectedSourceVideoFile, SLOT(setMarkOut()));
//        QObject::connect(resetMarkInButton, SIGNAL(clicked()), selectedSourceVideoFile, SLOT(resetMarkIn()));
//        QObject::connect(resetMarkOutButton, SIGNAL(clicked()), selectedSourceVideoFile, SLOT(resetMarkOut()));
//        QObject::connect(dirtySeekCheckBox, SIGNAL(toggled(bool)), selectedSourceVideoFile, SLOT(setOptionAllowDirtySeek(bool)));
//        QObject::connect(resetToBlackCheckBox, SIGNAL(toggled(bool)), selectedSourceVideoFile, SLOT(setOptionRevertToBlackWhenStop(bool)));
//        QObject::connect(restartWhereStoppedCheckBox, SIGNAL(toggled(bool)), selectedSourceVideoFile, SLOT(setOptionRestartToMarkIn(bool)));
//
//        // DISPLAY consistency from VideoFile to GUI
//        QObject::connect(selectedSourceVideoFile, SIGNAL(paused(bool)), pauseButton, SLOT(setChecked(bool)));
//        QObject::connect(selectedSourceVideoFile, SIGNAL(running(bool)), startButton, SLOT(setChecked(bool)));
//        QObject::connect(selectedSourceVideoFile, SIGNAL(running(bool)), videoControlFrame, SLOT(setEnabled(bool)));
//        QObject::connect(selectedSourceVideoFile, SIGNAL(running(bool)), speedControlFrame, SLOT(setEnabled(bool)));
//        QObject::connect(selectedSourceVideoFile, SIGNAL(running(bool)), timingControlFrame, SLOT(setEnabled(bool)));
////        QObject::connect(selectedSourceVideoFile, SIGNAL(info(QString)), statusbar, SLOT(showMessage(QString)));
//
//        // Consistency and update timer control from VideoFile
//        QObject::connect(selectedSourceVideoFile, SIGNAL(markingChanged()), this, SLOT(updateMarks()));
//        QObject::connect(selectedSourceVideoFile, SIGNAL(running(bool)), this, SLOT(updateRefreshTimerState()));
//        QObject::connect(selectedSourceVideoFile, SIGNAL(paused(bool)), this, SLOT(updateRefreshTimerState()));
//
//        // can we open the file ?
////        if ( selectedSourceVideoFile->open(fileName) ) {
//
//
//            vinfoDockWidget->setEnabled(true);
//            FileNameLineEdit->setText(fileName);
//            CodecNameLineEdit->setText(selectedSourceVideoFile->getCodecName());
//            // display size (special case when power of two dimensions are generated
//            if (selectedSourceVideoFile->getStreamFrameWidth() != selectedSourceVideoFile->getFrameWidth() || selectedSourceVideoFile->getStreamFrameHeight() != selectedSourceVideoFile->getFrameHeight()) {
//                widthLineEdit->setText( QString("%1 (%2)").arg(selectedSourceVideoFile->getStreamFrameWidth()).arg(selectedSourceVideoFile->getFrameWidth()));
//                heightLineEdit->setText( QString("%1 (%2)").arg(selectedSourceVideoFile->getStreamFrameHeight()).arg(selectedSourceVideoFile->getFrameHeight()));
//            } else {
//                widthLineEdit->setText( QString("%1").arg(selectedSourceVideoFile->getFrameWidth()));
//                heightLineEdit->setText( QString("%1").arg(selectedSourceVideoFile->getFrameHeight()));
//            }
//            framerateLineEdit->setText(QString().setNum(selectedSourceVideoFile->getFrameRate(),'f',2));
//            videoLoopButton->setChecked(selectedSourceVideoFile->isLoop());
//            actionShow_frames->setEnabled(true);
//
//            // is there more than one frame ?
//            if ( selectedSourceVideoFile->getEnd() > 1 ) {
//                // yes, its a video, we can control it
//                vcontrolDockWidget->setEnabled(true);
//                vconfigDockWidget->setEnabled(true);
//                // display times
//                on_actionShow_frames_toggled(actionShow_frames->isChecked());
//
//                // restore config
//                selectedSourceVideoFile->setOptionRestartToMarkIn( restartWhereStoppedCheckBox->isChecked());
//                selectedSourceVideoFile->setOptionRevertToBlackWhenStop( resetToBlackCheckBox->isChecked());
//                selectedSourceVideoFile->setOptionAllowDirtySeek(dirtySeekCheckBox->isChecked());
//
//                // only the speed is not connected to signals:
//                playSpeedBox->setCurrentIndex(3);
//
//                startButton->setEnabled( true );
//            } else {
//                // no, its a picture, we can't control it
//                vcontrolDockWidget->setEnabled(false);
//                vconfigDockWidget->setEnabled(false);
//                // show some info even if one frame only
//                endLineEdit->setText( QString().setNum( selectedSourceVideoFile->getEnd()) );
//                timeLineEdit->setText( QString().setNum( selectedSourceVideoFile->getBegin()) );
//                markInLineEdit->setText( QString().setNum( selectedSourceVideoFile->getMarkIn() ));
//                markOutLineEdit->setText( QString().setNum( selectedSourceVideoFile->getMarkOut() ));
//            }
////        }
//    }
}



void GLMixer::controlSource(SourceSet::iterator csi){

	// whatever happens, we will drop the control on the current video source
	//   (this slot is called by MainRenderWidget through signal currentSourceChanged
	//    which is sent ONLY when the current source is changed)
	if (selectedSourceVideoFile) {
		// clear video file control
		selectedSourceVideoFile = NULL;
		// disconnect any control buttons
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

		// stop update of the GUI
		refreshTimingTimer->stop();

	}

	// if we are given a valid iterator, we have a source to control
	if ( MainRenderWidget::getInstance()->isValid(csi) ) {

		// test the class of the current source ;

		// if it is a VideoSource (video file)
		VideoSource *vs = dynamic_cast<VideoSource *>(*csi);
		if (vs != NULL) {

			// get the pointer to the video to control
	        selectedSourceVideoFile = vs->getVideoFile();

	        // control this video if it is valid
	        if (selectedSourceVideoFile){

	            // CONTROL signals from GUI to VideoFile
	            QObject::connect(startButton, SIGNAL(toggled(bool)), selectedSourceVideoFile, SLOT(play(bool)));
	            QObject::connect(pauseButton, SIGNAL(toggled(bool)), selectedSourceVideoFile, SLOT(pause(bool)));
	            QObject::connect(seekBackwardButton, SIGNAL(clicked()), selectedSourceVideoFile, SLOT(seekBackward()));
	            QObject::connect(seekForwardButton, SIGNAL(clicked()), selectedSourceVideoFile, SLOT(seekForward()));
	            QObject::connect(seekBeginButton, SIGNAL(clicked()), selectedSourceVideoFile, SLOT(seekBegin()));
	            QObject::connect(playSpeedBox, SIGNAL(currentIndexChanged(int)), selectedSourceVideoFile, SLOT(setPlaySpeed(int)));
	            QObject::connect(videoLoopButton, SIGNAL(toggled(bool)), selectedSourceVideoFile, SLOT(setLoop(bool)));
	            QObject::connect(markInButton, SIGNAL(clicked()), selectedSourceVideoFile, SLOT(setMarkIn()));
	            QObject::connect(markOutButton, SIGNAL(clicked()), selectedSourceVideoFile, SLOT(setMarkOut()));
	            QObject::connect(resetMarkInButton, SIGNAL(clicked()), selectedSourceVideoFile, SLOT(resetMarkIn()));
	            QObject::connect(resetMarkOutButton, SIGNAL(clicked()), selectedSourceVideoFile, SLOT(resetMarkOut()));
	            QObject::connect(dirtySeekCheckBox, SIGNAL(toggled(bool)), selectedSourceVideoFile, SLOT(setOptionAllowDirtySeek(bool)));
	            QObject::connect(resetToBlackCheckBox, SIGNAL(toggled(bool)), selectedSourceVideoFile, SLOT(setOptionRevertToBlackWhenStop(bool)));
	            QObject::connect(restartWhereStoppedCheckBox, SIGNAL(toggled(bool)), selectedSourceVideoFile, SLOT(setOptionRestartToMarkIn(bool)));

	            // DISPLAY consistency from VideoFile to GUI
	            QObject::connect(selectedSourceVideoFile, SIGNAL(paused(bool)), pauseButton, SLOT(setChecked(bool)));
	            QObject::connect(selectedSourceVideoFile, SIGNAL(running(bool)), startButton, SLOT(setChecked(bool)));
	            QObject::connect(selectedSourceVideoFile, SIGNAL(running(bool)), videoControlFrame, SLOT(setEnabled(bool)));
	            QObject::connect(selectedSourceVideoFile, SIGNAL(running(bool)), speedControlFrame, SLOT(setEnabled(bool)));
	            QObject::connect(selectedSourceVideoFile, SIGNAL(running(bool)), timingControlFrame, SLOT(setEnabled(bool)));
	    //        QObject::connect(selectedSourceVideoFile, SIGNAL(info(QString)), statusbar, SLOT(showMessage(QString)));

	            // Consistency and update timer control from VideoFile
	            QObject::connect(selectedSourceVideoFile, SIGNAL(markingChanged()), this, SLOT(updateMarks()));
	            QObject::connect(selectedSourceVideoFile, SIGNAL(running(bool)), this, SLOT(updateRefreshTimerState()));
	            QObject::connect(selectedSourceVideoFile, SIGNAL(paused(bool)), this, SLOT(updateRefreshTimerState()));

	            // Fill in information
				vinfoDockWidget->setEnabled(true);
				FileNameLineEdit->setText(selectedSourceVideoFile->getFileName());
				CodecNameLineEdit->setText(selectedSourceVideoFile->getCodecName());
				// display size (special case when power of two dimensions are generated
				if (selectedSourceVideoFile->getStreamFrameWidth() != selectedSourceVideoFile->getFrameWidth() || selectedSourceVideoFile->getStreamFrameHeight() != selectedSourceVideoFile->getFrameHeight()) {
					widthLineEdit->setText( QString("%1 (%2)").arg(selectedSourceVideoFile->getStreamFrameWidth()).arg(selectedSourceVideoFile->getFrameWidth()));
					heightLineEdit->setText( QString("%1 (%2)").arg(selectedSourceVideoFile->getStreamFrameHeight()).arg(selectedSourceVideoFile->getFrameHeight()));
				} else {
					widthLineEdit->setText( QString("%1").arg(selectedSourceVideoFile->getFrameWidth()));
					heightLineEdit->setText( QString("%1").arg(selectedSourceVideoFile->getFrameHeight()));
				}
				framerateLineEdit->setText(QString().setNum(selectedSourceVideoFile->getFrameRate(),'f',2));
				videoLoopButton->setChecked(selectedSourceVideoFile->isLoop());
				actionShow_frames->setEnabled(true);

				// is there more than one frame ?
				if ( selectedSourceVideoFile->getEnd() > 1 ) {
					// yes, its a video, we can control it
					vcontrolDockWidget->setEnabled(true);
					vconfigDockWidget->setEnabled(true);
					// display times
					on_actionShow_frames_toggled(actionShow_frames->isChecked());

					// restore config
					selectedSourceVideoFile->setOptionRestartToMarkIn( restartWhereStoppedCheckBox->isChecked());
					selectedSourceVideoFile->setOptionRevertToBlackWhenStop( resetToBlackCheckBox->isChecked());
					selectedSourceVideoFile->setOptionAllowDirtySeek(dirtySeekCheckBox->isChecked());

					// only the speed is not connected to signals:
					playSpeedBox->setCurrentIndex(3);

					startButton->setEnabled( true );
				} else {
					// no, its a picture, we can't control it
					vcontrolDockWidget->setEnabled(false);
					vconfigDockWidget->setEnabled(false);
					// show some info even if one frame only
					endLineEdit->setText( QString().setNum( selectedSourceVideoFile->getEnd()) );
					timeLineEdit->setText( QString().setNum( selectedSourceVideoFile->getBegin()) );
					markInLineEdit->setText( QString().setNum( selectedSourceVideoFile->getMarkIn() ));
					markOutLineEdit->setText( QString().setNum( selectedSourceVideoFile->getMarkOut() ));
				}
	        }

		}
		else
		{
			// if it is an OpencvSource (camera)

			// fill in the information panel
		}


	} else {
		// disable panel widgets
		vcontrolDockWidget->setEnabled(false);
		vinfoDockWidget->setEnabled(false);
		vconfigDockWidget->setEnabled(false);
	}

}

void GLMixer::on_actionCamera_activated()
{

//	MainRenderWidget::getInstance()->addSource(1);

	CameraDialog cd(this);
	cd.setModal(false);

	if (cd.exec() == QDialog::Accepted) {
		// create a source according to the selected driver :

#ifdef OPEN_CV
		if (cd.getDriver() == CameraDialog::OPENCV_CAMERA && cd.indexOpencvCamera() >= 0)
		{
			MainRenderWidget::getInstance()->addSource(cd.indexOpencvCamera());
			statusbar->showMessage( tr("Source created with OpenCV drivers for webcam (%1)").arg(cd.indexOpencvCamera()) );
		}
#endif

	}

}

void GLMixer::on_actionShow_frames_toggled(bool on){

    if (on){
        endLineEdit->setText( QString().setNum(selectedSourceVideoFile->getEnd()) );
        timeLineEdit->setText( QString().setNum(selectedSourceVideoFile->getBegin()) );
    } else {
        endLineEdit->setText( selectedSourceVideoFile->getTimeFromFrame(selectedSourceVideoFile->getEnd()) );
        timeLineEdit->setText( selectedSourceVideoFile->getTimeFromFrame(selectedSourceVideoFile->getBegin()) );
    }
    updateMarks();
}

void GLMixer::on_markInSlider_sliderReleased (){

    double percent = (double)(markInSlider->value())/1000.0;
    int64_t pos = (int64_t) ( selectedSourceVideoFile->getEnd()  * percent );
    pos += selectedSourceVideoFile->getBegin();
    selectedSourceVideoFile->setMarkIn(pos);
}

void GLMixer::on_markOutSlider_sliderReleased (){

    double percent = (double)(markOutSlider->value())/1000.0;
    int64_t pos = (int64_t) ( selectedSourceVideoFile->getEnd()  * percent );
    pos += selectedSourceVideoFile->getBegin();
    selectedSourceVideoFile->setMarkOut(pos);
}

void GLMixer::refreshTiming(){

    if (!skipNextRefresh) {

        int f_percent = (int) ( (double)( selectedSourceVideoFile->getCurrentFrameTime() - selectedSourceVideoFile->getBegin() ) / (double)( selectedSourceVideoFile->getEnd() - selectedSourceVideoFile->getBegin() ) * 1000.0) ;
        frameSlider->setValue(f_percent);

        if (actionShow_frames->isChecked())
            timeLineEdit->setText( QString().setNum(selectedSourceVideoFile->getCurrentFrameTime()) );
        else
            timeLineEdit->setText( selectedSourceVideoFile->getTimeFromFrame(selectedSourceVideoFile->getCurrentFrameTime()) );
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
	selectedSourceVideoFile->pause(pauseButton->isChecked());

    // restart the refresh timer if it should be
    updateRefreshTimerState();
}

void GLMixer::on_frameSlider_sliderMoved (int v){

    // disconnect the button from the VideoFile signal ; this way when we'll unpause bellow, the button will keep its state
    QObject::disconnect(selectedSourceVideoFile, SIGNAL(paused(bool)), pauseButton, SLOT(setChecked(bool)));

    // the trick; call a method when the frame will be ready!
    QObject::connect(selectedSourceVideoFile, SIGNAL(frameReady(int)), this, SLOT(pauseAfterFrame()));

    // compute where we should jump to
    double percent = (double)(v)/ (double)frameSlider->maximum();
    int64_t pos = (int64_t) ( selectedSourceVideoFile->getEnd()  * percent );
    pos += selectedSourceVideoFile->getBegin();

    // request seek ; we need to have the VideoFile process running to go there
    selectedSourceVideoFile->seekToPosition(pos);

    // let the VideoFile run till it displays the frame seeked
    selectedSourceVideoFile->pause(false);

    // cosmetics to show the time of the frame (refreshTiming disabled)
    if (actionShow_frames->isChecked())
        timeLineEdit->setText( QString().setNum(pos) );
    else
        timeLineEdit->setText( selectedSourceVideoFile->getTimeFromFrame(pos) );
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
                int64_t pos = (int64_t) ( selectedSourceVideoFile->getEnd()  * percent );
                pos += selectedSourceVideoFile->getBegin();

                // request seek ; we need to have the VideoFile process running to go there
                selectedSourceVideoFile->seekToPosition(pos);

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

	selectedSourceVideoFile->pause(true);

    // do not keep calling pause method for each frame !
    QObject::disconnect(selectedSourceVideoFile, SIGNAL(frameReady(int)), this, SLOT(pauseAfterFrame()));
    // reconnect the pause button
    QObject::connect(selectedSourceVideoFile, SIGNAL(paused(bool)), pauseButton, SLOT(setChecked(bool)));

}


void GLMixer::updateMarks (){

    int i_percent = (int) ( (double)( selectedSourceVideoFile->getMarkIn() - selectedSourceVideoFile->getBegin() ) / (double)( selectedSourceVideoFile->getEnd() - selectedSourceVideoFile->getBegin() ) * 1000.0) ;
    int o_percent = (int) ( (double)( selectedSourceVideoFile->getMarkOut() - selectedSourceVideoFile->getBegin() ) / (double)( selectedSourceVideoFile->getEnd() - selectedSourceVideoFile->getBegin() ) * 1000.0) ;

    markInSlider->setValue(i_percent);
    markOutSlider->setValue(o_percent);

    if (actionShow_frames->isChecked()) {
        markInLineEdit->setText( QString().setNum( selectedSourceVideoFile->getMarkIn() ));
        markOutLineEdit->setText( QString().setNum( selectedSourceVideoFile->getMarkOut() ));
    } else {
        markInLineEdit->setText( selectedSourceVideoFile->getTimeFromFrame( selectedSourceVideoFile->getMarkIn() ));
        markOutLineEdit->setText( selectedSourceVideoFile->getTimeFromFrame( selectedSourceVideoFile->getMarkOut() ));
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

