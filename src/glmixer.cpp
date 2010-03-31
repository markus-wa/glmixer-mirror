/*
 * glv.cpp
 *
 *  Created on: Jul 14, 2009
 *      Author: bh
 */

#include <QApplication>


#include "CameraDialog.h"
#include "VideoFileDialog.h"
#include "AlgorithmSelectionDialog.h"
#include "ViewRenderWidget.h"
#include "RenderingManager.h"
#include "OutputRenderWindow.h"
#include "MixerView.h"
#include "SourceDisplayWidget.h"
#include "RenderingSource.h"
#include "AlgorithmSource.h"
#include "CloneSource.h"
#include "VideoSource.h"
#ifdef OPEN_CV
#include "OpencvSource.h"
#endif
#include "VideoFileDisplayWidget.h"
#include "SourcePropertyBrowser.h"

#include "glmixer.moc"

GLMixer::GLMixer ( QWidget *parent): QMainWindow ( parent ), selectedSourceVideoFile(NULL), refreshTimingTimer(0)
{
    setupUi ( this );

#ifndef OPEN_CV
    actionCameraSource->setEnabled(false);
#endif

    // add the show/hide menu items for the dock widgets
    menuToolBars->addAction(previewDockWidget->toggleViewAction());
    menuToolBars->addAction(sourceDockWidget->toggleViewAction());
    menuToolBars->addAction(vcontrolDockWidget->toggleViewAction());
    menuToolBars->addSeparator();
    menuToolBars->addAction(sourceToolBar->toggleViewAction());
    menuToolBars->addAction(viewToolBar->toggleViewAction());
    menuToolBars->addAction(FileToolBar->toggleViewAction());

    // Setup the central widget
    centralViewLayout->removeWidget(mainRendering);
	delete mainRendering;
	mainRendering = (QGLWidget *)  RenderingManager::getRenderingWidget();
	mainRendering->setParent(centralwidget);
	centralViewLayout->addWidget(mainRendering);
	// TODO : activate the default view read from preferences
	on_actionMixingView_triggered();
//	on_actionGeometryView_triggered();

	// Setup the property browser
	SourcePropertyBrowser *propertyBrowser = RenderingManager::getPropertyBrowserWidget();
	propertyBrowser->setParent(sourceDockWidgetContents);
	sourceDockWidgetContentsLayout->addWidget(propertyBrowser);
    QObject::connect(RenderingManager::getInstance(), SIGNAL(currentSourceChanged(SourceSet::iterator)), propertyBrowser, SLOT(showProperties(SourceSet::iterator) ) );

    QObject::connect(RenderingManager::getRenderingWidget(), SIGNAL(sourceMixingModified()), propertyBrowser, SLOT(updateMixingProperties() ) );
    QObject::connect(RenderingManager::getRenderingWidget(), SIGNAL(sourceGeometryModified()), propertyBrowser, SLOT(updateGeometryProperties() ) );
    QObject::connect(RenderingManager::getRenderingWidget(), SIGNAL(sourceLayerModified()), propertyBrowser, SLOT(updateLayerProperties() ) );


    // Create preview widget
	OutputRenderWidget *outputpreview = new OutputRenderWidget(previewDockWidgetContents, mainRendering);
	previewDockWidgetContentsLayout->addWidget(outputpreview);
	outputpreview->setCursor(Qt::ArrowCursor);

    // signals for source management with RenderingManager
    QObject::connect(RenderingManager::getInstance(), SIGNAL(currentSourceChanged(SourceSet::iterator)), this, SLOT(connectSource(SourceSet::iterator) ) );
	QObject::connect(actionCapture, SIGNAL(triggered()), RenderingManager::getInstance(), SLOT(captureFrameBuffer()));

	// QUIT event
    QObject::connect(actionQuit, SIGNAL(triggered()), this, SLOT(close()));

    // Signals between GUI and output window
    QObject::connect(actionKeep_aspect_ratio, SIGNAL(toggled(bool)), OutputRenderWindow::getInstance(), SLOT(useRenderingAspectRatio(bool)));
    QObject::connect(actionFullscreen, SIGNAL(toggled(bool)), OutputRenderWindow::getInstance(), SLOT(setFullScreen(bool)));
	QObject::connect(actionZoomIn, SIGNAL(triggered()), RenderingManager::getRenderingWidget(), SLOT(zoomIn()));
	QObject::connect(actionZoomOut, SIGNAL(triggered()), RenderingManager::getRenderingWidget(), SLOT(zoomOut()));
	QObject::connect(actionZoomReset, SIGNAL(triggered()), RenderingManager::getRenderingWidget(), SLOT(zoomReset()));
	QObject::connect(actionZoomBestFit, SIGNAL(triggered()), RenderingManager::getRenderingWidget(), SLOT(zoomBestFit()));

    // Initial state
    vcontrolDockWidgetContents->setEnabled(false);
    sourceDockWidgetContents->setEnabled(false);

    // a Timer to update sliders and counters
    refreshTimingTimer = new QTimer(this);
    Q_CHECK_PTR(refreshTimingTimer);
    refreshTimingTimer->setInterval(150);
    QObject::connect(refreshTimingTimer, SIGNAL(timeout()), this, SLOT(refreshTiming()));
    QObject::connect(vcontrolDockWidget, SIGNAL(visibilityChanged(bool)), this, SLOT(updateRefreshTimerState()));

}

GLMixer::~GLMixer() {

	RenderingManager::deleteInstance();
}


void GLMixer::closeEvent ( QCloseEvent * event ){
	OutputRenderWindow::getInstance()->close();
	event->accept();
}

void GLMixer::updateRefreshTimerState(){

    if (selectedSourceVideoFile && !selectedSourceVideoFile->isPaused() && selectedSourceVideoFile->isRunning() && vcontrolDockWidget->isVisible())
        refreshTimingTimer->start();
    else
        refreshTimingTimer->stop();

}

void GLMixer::on_actionFormats_and_Codecs_triggered(){

    VideoFile::displayFormatsCodecsInformation(QString::fromUtf8(":/glmixer/icons/video.png"));

}

void GLMixer::on_actionOpenGL_extensions_triggered(){

    glRenderWidget::showGlExtensionsInformationDialog(QString::fromUtf8(":/glmixer/icons/display.png"));

}


void GLMixer::displayLogMessage(QString msg){

    qDebug("Log %s", qPrintable(msg));
}


void GLMixer::displayErrorMessage(QString msg){

    qWarning("Warning %s", qPrintable(msg));
}


void GLMixer::on_actionMixingView_triggered(){

	RenderingManager::getRenderingWidget()->setViewMode(ViewRenderWidget::MIXING);
	viewIcon->setPixmap(RenderingManager::getRenderingWidget()->getViewIcon());
}

void GLMixer::on_actionGeometryView_triggered(){

	RenderingManager::getRenderingWidget()->setViewMode(ViewRenderWidget::GEOMETRY);
	viewIcon->setPixmap(RenderingManager::getRenderingWidget()->getViewIcon());
}


void GLMixer::on_actionLayersView_triggered(){

	RenderingManager::getRenderingWidget()->setViewMode(ViewRenderWidget::LAYER);
	viewIcon->setPixmap(RenderingManager::getRenderingWidget()->getViewIcon());
}

void GLMixer::on_actionMediaSource_triggered(){
	// remember the folder
	static QDir d = QDir::home();
	QStringList fileNames;

#ifndef NO_VIDEO_FILE_DIALOG_PREVIEW

	VideoFileDialog mfd(this, "Open a video or a picture", d.absolutePath());

	if (mfd.exec())
		fileNames = mfd.selectedFiles();

	if (!mfd.selectedFiles().empty())
		d.setPath(mfd.selectedFiles().first());

#else
	fileNames = QFileDialog::getOpenFileNames(this, tr("Open File"),
													d.absolutePath(),
													tr("Video (*.mov *.avi *.wmv *.mpeg *.mp4 *.mpg *.vob *.swf *.flv);;Image (*.png *.jpg *.jpeg *.tif *.tiff *.gif *.tga *.sgi *.bmp)"));

	d.setPath(fileNames.first());
#endif

	QStringListIterator fileNamesIt(fileNames);
	while (fileNamesIt.hasNext()){

	    VideoFile *newSourceVideoFile = NULL;

		if ( !VideoFileDialog::configCustomSize() && (glSupportsExtension("GL_EXT_texture_non_power_of_two") || glSupportsExtension("GL_ARB_texture_non_power_of_two") ) )
			newSourceVideoFile = new VideoFile(this);
		else
			//newSourceVideoFile = new VideoFile(this, true, SWS_FAST_BILINEAR, PIX_FMT_RGB24);
			newSourceVideoFile = new VideoFile(this, true, SWS_FAST_BILINEAR, PIX_FMT_RGB32);

	    Q_CHECK_PTR(newSourceVideoFile);

		// if the video file was created successfully
		if (newSourceVideoFile){
			// forward error messages to display
			QObject::connect(newSourceVideoFile, SIGNAL(error(QString)), this, SLOT(displayErrorMessage(QString)));
			QObject::connect(newSourceVideoFile, SIGNAL(info(QString)), statusbar, SLOT(showMessage(QString)));
			// can we open the file ?
			if ( newSourceVideoFile->open( fileNamesIt.next() ) ) {
				// create the source as it is a valid video file (this also set it to be the current source)
				RenderingManager::getInstance()->addMediaSource(newSourceVideoFile);
			}
		}
	}

}


// method called when a source is made current (either after loading, either after clicking on it).
// The goal is to have the GUI display the current state of the video file to be able to control the video playback
// and to read the correct information and configuration options
void GLMixer::connectSource(SourceSet::iterator csi){

#ifdef OPEN_CV
	static OpencvSource *cvs = NULL;
	// the static pointer to opencv source keeps last reference to it when selected (controlled here)
	// -> we should disconnect it to the play button if we change source
	if (cvs) {
        QObject::disconnect(startButton, SIGNAL(toggled(bool)), cvs, SLOT(play(bool)));
        // clear it for next time
        cvs = NULL;
	}
#endif

	// whatever happens, we will drop the control on the current source
	//   (this slot is called by MainRenderWidget through signal currentSourceChanged
	//    which is sent ONLY when the current source is changed)



	if (selectedSourceVideoFile) {

        QObject::disconnect(selectedSourceVideoFile, SIGNAL(paused(bool)), pauseButton, SLOT(setChecked(bool)));
        QObject::disconnect(selectedSourceVideoFile, SIGNAL(running(bool)), startButton, SLOT(setChecked(bool)));
        QObject::disconnect(selectedSourceVideoFile, SIGNAL(running(bool)), videoFrame, SLOT(setEnabled(bool)));
        QObject::disconnect(selectedSourceVideoFile, SIGNAL(running(bool)), timingControlFrame, SLOT(setEnabled(bool)));
        QObject::disconnect(selectedSourceVideoFile, SIGNAL(markingChanged()), this, SLOT(updateMarks()));
        QObject::disconnect(selectedSourceVideoFile, SIGNAL(running(bool)), this, SLOT(updateRefreshTimerState()));
        QObject::disconnect(selectedSourceVideoFile, SIGNAL(paused(bool)), this, SLOT(updateRefreshTimerState()));

		// disconnect control buttons
        QObject::disconnect(startButton, SIGNAL(toggled(bool)), selectedSourceVideoFile, SLOT(play(bool)));
        QObject::disconnect(pauseButton, SIGNAL(toggled(bool)), selectedSourceVideoFile, SLOT(pause(bool)));
        QObject::disconnect(playSpeedBox, SIGNAL(currentIndexChanged(int)), selectedSourceVideoFile, SLOT(setPlaySpeed(int)));
        QObject::disconnect(dirtySeekCheckBox, SIGNAL(toggled(bool)), selectedSourceVideoFile, SLOT(setOptionAllowDirtySeek(bool)));
        QObject::disconnect(resetToBlackCheckBox, SIGNAL(toggled(bool)), selectedSourceVideoFile, SLOT(setOptionRevertToBlackWhenStop(bool)));
        QObject::disconnect(restartWhereStoppedCheckBox, SIGNAL(toggled(bool)), selectedSourceVideoFile, SLOT(setOptionRestartToMarkIn(bool)));

        QObject::disconnect(seekBackwardButton, SIGNAL(clicked()), selectedSourceVideoFile, SLOT(seekBackward()));
        QObject::disconnect(seekForwardButton, SIGNAL(clicked()), selectedSourceVideoFile, SLOT(seekForward()));
        QObject::disconnect(seekBeginButton, SIGNAL(clicked()), selectedSourceVideoFile, SLOT(seekBegin()));
        QObject::disconnect(videoLoopButton, SIGNAL(toggled(bool)), selectedSourceVideoFile, SLOT(setLoop(bool)));
        QObject::disconnect(markInButton, SIGNAL(clicked()), selectedSourceVideoFile, SLOT(setMarkIn()));
        QObject::disconnect(markOutButton, SIGNAL(clicked()), selectedSourceVideoFile, SLOT(setMarkOut()));
        QObject::disconnect(resetMarkInButton, SIGNAL(clicked()), selectedSourceVideoFile, SLOT(resetMarkIn()));
        QObject::disconnect(resetMarkOutButton, SIGNAL(clicked()), selectedSourceVideoFile, SLOT(resetMarkOut()));

		// stop update of the GUI
		refreshTimingTimer->stop();

		// clear video file control
		selectedSourceVideoFile = NULL;
	}

	// if we are given a valid iterator, we have a source to control
	if ( RenderingManager::getInstance()->isValid(csi) ) {

		sourceDockWidgetContents->setEnabled(true);
		actionDeleteSource->setEnabled(true);
		actionCloneSource->setEnabled(true);
//		pageVideoFileConfig->setEnabled(false);

		// setup preview

		// test the class of the current source and deal accordingly :

		// if it is a VideoSource (video file)
		if ( (*csi)->rtti() == Source::VIDEO_SOURCE ) {
			VideoSource *vs = dynamic_cast<VideoSource *>(*csi);

			// get the pointer to the video to control
	        selectedSourceVideoFile = vs->getVideoFile();

	        // control this video if it is valid
	        if (selectedSourceVideoFile){

	        	// setup GUI button states to match the current state of the videofile; do it before connecting slots to avoid re-emiting signals
				startButton->setChecked( selectedSourceVideoFile->isRunning() );
				pauseButton->setChecked( selectedSourceVideoFile->isPaused());
				videoFrame->setEnabled(selectedSourceVideoFile->isRunning());
				timingControlFrame->setEnabled(selectedSourceVideoFile->isRunning());
				dirtySeekCheckBox->setChecked(selectedSourceVideoFile->getOptionAllowDirtySeek());
				resetToBlackCheckBox->setChecked(selectedSourceVideoFile->getOptionRevertToBlackWhenStop());
				restartWhereStoppedCheckBox->setChecked(selectedSourceVideoFile->getOptionRestartToMarkIn());

	            // CONTROL signals from GUI to VideoFile
	            QObject::connect(startButton, SIGNAL(toggled(bool)), selectedSourceVideoFile, SLOT(play(bool)));
	            QObject::connect(pauseButton, SIGNAL(toggled(bool)), selectedSourceVideoFile, SLOT(pause(bool)));
	            QObject::connect(playSpeedBox, SIGNAL(currentIndexChanged(int)), selectedSourceVideoFile, SLOT(setPlaySpeed(int)));
	            QObject::connect(dirtySeekCheckBox, SIGNAL(toggled(bool)), selectedSourceVideoFile, SLOT(setOptionAllowDirtySeek(bool)));
	            QObject::connect(resetToBlackCheckBox, SIGNAL(toggled(bool)), selectedSourceVideoFile, SLOT(setOptionRevertToBlackWhenStop(bool)));
	            QObject::connect(restartWhereStoppedCheckBox, SIGNAL(toggled(bool)), selectedSourceVideoFile, SLOT(setOptionRestartToMarkIn(bool)));

	            QObject::connect(seekBackwardButton, SIGNAL(clicked()), selectedSourceVideoFile, SLOT(seekBackward()));
	            QObject::connect(seekForwardButton, SIGNAL(clicked()), selectedSourceVideoFile, SLOT(seekForward()));
	            QObject::connect(seekBeginButton, SIGNAL(clicked()), selectedSourceVideoFile, SLOT(seekBegin()));
	            QObject::connect(videoLoopButton, SIGNAL(toggled(bool)), selectedSourceVideoFile, SLOT(setLoop(bool)));
	            QObject::connect(markInButton, SIGNAL(clicked()), selectedSourceVideoFile, SLOT(setMarkIn()));
	            QObject::connect(markOutButton, SIGNAL(clicked()), selectedSourceVideoFile, SLOT(setMarkOut()));
	            QObject::connect(resetMarkInButton, SIGNAL(clicked()), selectedSourceVideoFile, SLOT(resetMarkIn()));
	            QObject::connect(resetMarkOutButton, SIGNAL(clicked()), selectedSourceVideoFile, SLOT(resetMarkOut()));

	            // DISPLAY consistency from VideoFile to GUI
	            QObject::connect(selectedSourceVideoFile, SIGNAL(paused(bool)), pauseButton, SLOT(setChecked(bool)));
	            QObject::connect(selectedSourceVideoFile, SIGNAL(running(bool)), startButton, SLOT(setChecked(bool)));
	            QObject::connect(selectedSourceVideoFile, SIGNAL(running(bool)), videoFrame, SLOT(setEnabled(bool)));
	            QObject::connect(selectedSourceVideoFile, SIGNAL(running(bool)), timingControlFrame, SLOT(setEnabled(bool)));
	            QObject::connect(selectedSourceVideoFile, SIGNAL(error(QString)), this, SLOT(displayErrorMessage(QString)));
	            QObject::connect(selectedSourceVideoFile, SIGNAL(info(QString)), statusbar, SLOT(showMessage(QString)));

	            // Consistency and update timer control from VideoFile
	            QObject::connect(selectedSourceVideoFile, SIGNAL(markingChanged()), this, SLOT(updateMarks()));
	            QObject::connect(selectedSourceVideoFile, SIGNAL(running(bool)), this, SLOT(updateRefreshTimerState()));
	            QObject::connect(selectedSourceVideoFile, SIGNAL(paused(bool)), this, SLOT(updateRefreshTimerState()));

				videoLoopButton->setChecked(selectedSourceVideoFile->isLoop());
				playSpeedBox->setCurrentIndex(selectedSourceVideoFile->getPlaySpeed());

				// is there more than one frame ?
				if ( selectedSourceVideoFile->getEnd() > 1 ) {
					// yes, its a video, we can control it
					vcontrolDockWidgetContents->setEnabled(true);
					vcontrolDockWidgetOptions->setEnabled(true);
					startButton->setEnabled( true );

					// restore config
					selectedSourceVideoFile->setOptionRestartToMarkIn( restartWhereStoppedCheckBox->isChecked());
					selectedSourceVideoFile->setOptionRevertToBlackWhenStop( resetToBlackCheckBox->isChecked());
					selectedSourceVideoFile->setOptionAllowDirtySeek(dirtySeekCheckBox->isChecked());

					// display info (time or frames)
					on_actionShow_frames_toggled(actionShow_frames->isChecked());

					// restart slider timer
					updateRefreshTimerState();

				} else {
					// no, its a picture, we can't control it
					vcontrolDockWidgetContents->setEnabled(false);
					startButton->setEnabled( false );
				}
	        }
			return;
		}

#ifdef OPEN_CV
		// if it is an OpencvSource (camera)
		if ( (*csi)->rtti() == Source::CAMERA_SOURCE ) {
			cvs = dynamic_cast<OpencvSource *>(*csi);
			// we can play/stop  it
			vcontrolDockWidgetContents->setEnabled(true);
			startButton->setChecked( cvs->isRunning() );
			startButton->setEnabled( true );
			QObject::connect(startButton, SIGNAL(toggled(bool)), cvs, SLOT(play(bool)));

			// we can't configure it
			videoFrame->setEnabled(false);
			timingControlFrame->setEnabled(false);
			vcontrolDockWidgetOptions->setEnabled(false);
			return;
		}
#endif

		startButton->setEnabled( false );

	} else {
		// it is a basic  Source
		// fill in the information panel
//		FileNameLineEdit->setText(QString("Image"));
//		CodecNameLineEdit->setText("RGBA frame buffer");
//		widthLineEdit->setText( QString("%1").arg(RenderingManager::getInstance()->getFrameBufferWidth()));
//		heightLineEdit->setText( QString("%1").arg(RenderingManager::getInstance()->getFrameBufferHeight()));


		actionDeleteSource->setEnabled(false);
		actionCloneSource->setEnabled(false);

		// nothing to preview
		// disable panel widgets
		vcontrolDockWidgetContents->setEnabled(false);
		sourceDockWidgetContents->setEnabled(false);
		startButton->setEnabled( false );

	}

}

void GLMixer::on_actionCameraSource_triggered()
{

	CameraDialog cd(this);
	cd.setModal(false);

	if (cd.exec() == QDialog::Accepted) {
		// create a source according to the selected driver :

#ifdef OPEN_CV
		if (cd.getDriver() == CameraDialog::OPENCV_CAMERA && cd.indexOpencvCamera() >= 0)
		{
			SourceSet::iterator sit = RenderingManager::getInstance()->getBegin();
			// check for the existence of an opencv source which would already be on this same index
			for ( ; RenderingManager::getInstance()->notAtEnd(sit); sit++){
				OpencvSource *cvs = dynamic_cast<OpencvSource *>(*sit);
				if (cvs && cvs->getOpencvCameraIndex() == cd.indexOpencvCamera())
					break;
			}
			// if we find one, just clone the source
			if ( RenderingManager::getInstance()->notAtEnd(sit)) {
				RenderingManager::getInstance()->addCloneSource(sit);
				statusbar->showMessage( tr("Clone created of the source with OpenCV drivers for Camera %1").arg(cd.indexOpencvCamera()) );
			}
			//else create a new opencv source :
			else {
				RenderingManager::getInstance()->addOpencvSource(cd.indexOpencvCamera());
				statusbar->showMessage( tr("Source created with OpenCV drivers for Camera %1").arg(cd.indexOpencvCamera()) );
			}
		}
#endif

	}

}


void GLMixer::on_actionAlgorithmSource_triggered(){

	// popup a question dialog to select the type of algorithm
	AlgorithmSelectionDialog asd(this);
	asd.setModal(false);

	if (asd.exec() == QDialog::Accepted) {
		RenderingManager::getInstance()->addAlgorithmSource(asd.getSelectedAlgorithmIndex(), asd.getSelectedWidth(), asd.getSelectedHeight(), asd.getSelectedVariability(), asd.getUpdatePeriod());
		statusbar->showMessage( tr("Source created with the algorithm %1.").arg( AlgorithmSource::getAlgorithmDescription(asd.getSelectedAlgorithmIndex())) );
	}
}


void GLMixer::on_actionRenderingSource_triggered(){

	// TODO popup a question dialog 'are u sure'

	RenderingManager::getInstance()->addRenderingSource();
	statusbar->showMessage( tr("Source created with the rendering output loopback.") );
}


void GLMixer::on_actionCloneSource_triggered(){

	// TODO popup a question dialog 'are u sure'

	if ( RenderingManager::getInstance()->notAtEnd(RenderingManager::getInstance()->getCurrentSource()) ) {
		RenderingManager::getInstance()->addCloneSource( RenderingManager::getInstance()->getCurrentSource());
		statusbar->showMessage( tr("The current source has been cloned.") );
	}
}


void GLMixer::on_actionCaptureSource_triggered(){

	RenderingManager::getInstance()->addCaptureSource();
	statusbar->showMessage( tr("Source created with the last output capture.") );
}


void GLMixer::on_actionDeleteSource_triggered(){

	if ( RenderingManager::getInstance()->isValid(RenderingManager::getInstance()->getCurrentSource()) ) {

		int numclones = (*RenderingManager::getInstance()->getCurrentSource())->getClones()->size();
		// popup a question dialog 'are u sure' if there are clones attached;
		if ( numclones ){
			QString msg = tr("This source was cloned %1 times; all these clones will be removed with this source if you confirm the removal.").arg(numclones);
			if ( QMessageBox::question(this,"Are you sure?", msg, QMessageBox::Ok | QMessageBox::Cancel, QMessageBox::Ok) == QMessageBox::Ok)
				numclones = 0;
		}

		if ( !numclones )
			RenderingManager::getInstance()->removeSource(RenderingManager::getInstance()->getCurrentSource());
	}
}


void GLMixer::on_actionSaveCapture_triggered(){

	QString filename;
	static QDir cd = QDir::home();

	filename = QFileDialog::getSaveFileName ( this, tr("Save capture image"), cd.absolutePath(),  tr("Images (*.png *.xpm *.jpg)"));
	cd.setPath(filename);

	if (!filename.isEmpty())
		RenderingManager::getInstance()->saveCapturedFrameBuffer(filename);
}

void GLMixer::on_actionShow_frames_toggled(bool on){

	if (selectedSourceVideoFile) {
//		if (on){
//			endLineEdit->setText( tr("%1 frames").arg(selectedSourceVideoFile->getEnd()) );
//			timeLineEdit->setText( tr("frame %1").arg(selectedSourceVideoFile->getBegin()) );
//		} else {
//			endLineEdit->setText( selectedSourceVideoFile->getTimeFromFrame(selectedSourceVideoFile->getEnd()) );
//			timeLineEdit->setText( selectedSourceVideoFile->getTimeFromFrame(selectedSourceVideoFile->getBegin()) );
//		}
		// TODO : update property marks
		updateMarks();
	}
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
            timeLineEdit->setText( tr("frame %1").arg(selectedSourceVideoFile->getCurrentFrameTime()) );
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
        timeLineEdit->setText( tr("frame %1").arg(pos) );
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

//    if (actionShow_frames->isChecked()) {
//        markInLineEdit->setText( tr("frame %1").arg( selectedSourceVideoFile->getMarkIn() ));
//        markOutLineEdit->setText( tr("frame %1").arg( selectedSourceVideoFile->getMarkOut() ));
//    } else {
//        markInLineEdit->setText( selectedSourceVideoFile->getTimeFromFrame( selectedSourceVideoFile->getMarkIn() ));
//        markOutLineEdit->setText( selectedSourceVideoFile->getTimeFromFrame( selectedSourceVideoFile->getMarkOut() ));
//    }
    // todo update property for marks in / out
}


void GLMixer::on_actionShowFPS_toggled(bool on){

	glRenderWidget::showFramerate(on);
}

void GLMixer::on_actionAbout_triggered(){

	QString msg = QString("GLMixer : Graphic Live Mixer\n\n");
	msg.append(QString("Author:   \tBruno Herbelin\n"));
	msg.append(QString("Contact:  \tbruno.herbelin@gmail.com\n"));
	msg.append(QString("License:  \tGPL\n"));
	
#ifdef GLMIXER_VERSION
    msg.append(QString("Version:  \t%1\n").arg(GLMIXER_VERSION));
#endif
#ifdef GLMIXER_REVISION
    msg.append(QString("Revision: \t%1\n").arg(GLMIXER_REVISION));
#endif
	
	msg.append(tr("\nGLMixer is a video mixing software for live performance.\nCheck http://code.google.com/p/glmixer/ for more info."));
	QMessageBox::information(this, "About GlMixer", msg, QMessageBox::Ok, QMessageBox::Ok);

}


void GLMixer::on_actionNew_Session_triggered(){

	RenderingManager::getInstance()->clearSourceSet();
}


