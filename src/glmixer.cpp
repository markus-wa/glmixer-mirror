/*
 * glmixer.cpp
 *
 *  Created on: Jul 14, 2009
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
 *   Copyright 2009, 2010 Bruno Herbelin
 *
 */

#define XML_GLM_VERSION "0.5"

#include <QApplication>
#include <QDomDocument>


#include "CameraDialog.h"
#include "VideoFileDialog.h"
#include "AlgorithmSelectionDialog.h"
#include "UserPreferencesDialog.h"
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
#include "CatalogView.h"

#include "glmixer.moc"

GLMixer::GLMixer ( QWidget *parent): QMainWindow ( parent ), selectedSourceVideoFile(NULL), refreshTimingTimer(0)
{
    setupUi ( this );
    setAcceptDrops ( true );

#ifndef OPEN_CV
    actionCameraSource->setEnabled(false);
#endif

    // add the show/hide menu items for the dock widgets
    menuToolBars->addAction(previewDockWidget->toggleViewAction());
    menuToolBars->addAction(sourceDockWidget->toggleViewAction());
    menuToolBars->addAction(vcontrolDockWidget->toggleViewAction());
    menuToolBars->addAction(cursorDockWidget->toggleViewAction());
    menuToolBars->addSeparator();
    menuToolBars->addAction(sourceToolBar->toggleViewAction());
    menuToolBars->addAction(viewToolBar->toggleViewAction());
    menuToolBars->addAction(fileToolBar->toggleViewAction());
    menuToolBars->addAction(toolsToolBar->toggleViewAction());

	QActionGroup *viewActions = new QActionGroup(this);
    viewActions->addAction(actionMixingView);
    viewActions->addAction(actionGeometryView);
    viewActions->addAction(actionLayersView);
    QObject::connect(viewActions, SIGNAL(triggered(QAction *)), this, SLOT(setView(QAction *) ) );

	QActionGroup *toolActions = new QActionGroup(this);
	toolActions->addAction(actionToolGrab);
	toolActions->addAction(actionToolScale);
	toolActions->addAction(actionToolRotate);
	toolActions->addAction(actionToolCut);
    QObject::connect(toolActions, SIGNAL(triggered(QAction *)), this, SLOT(setTool(QAction *) ) );

	QActionGroup *cursorActions = new QActionGroup(this);
	cursorActions->addAction(actionCursorNormal);
	cursorActions->addAction(actionCursorSpring);
	cursorActions->addAction(actionCursorDelay);
	cursorActions->addAction(actionCursorCurve);
    QObject::connect(cursorActions, SIGNAL(triggered(QAction *)), this, SLOT(setCursor(QAction *) ) );

    // Setup the central widget
    centralViewLayout->removeWidget(mainRendering);
	delete mainRendering;
	mainRendering = (QGLWidget *) RenderingManager::getRenderingWidget();
	mainRendering->setParent(centralwidget);
	centralViewLayout->addWidget(mainRendering);
	//activate the default view
	setView(actionMixingView);
	// share menus as context menus of the main view
	RenderingManager::getRenderingWidget()->setViewContextMenu(menuZoom);
	RenderingManager::getRenderingWidget()->setCatalogContextMenu(menuCatalog);

	// Setup the property browser
	SourcePropertyBrowser *propertyBrowser = RenderingManager::getPropertyBrowserWidget();
	propertyBrowser->setParent(sourceDockWidgetContents);
	sourceDockWidgetContentsLayout->addWidget(propertyBrowser);
    QObject::connect(this, SIGNAL(sourceMarksModified(bool)), propertyBrowser, SLOT(updateMarksProperties(bool) ) );

    // Setup Video file dialog
    mfd = new VideoFileDialog(this, "Open a video or a picture", QDir::currentPath());

    // Create preview widget
    OutputRenderWidget *outputpreview = new OutputRenderWidget(previewDockWidgetContents, mainRendering);
	previewDockWidgetContentsLayout->addWidget(outputpreview);
	outputpreview->setCursor(Qt::ArrowCursor);

    // signals for source management with RenderingManager
    QObject::connect(RenderingManager::getInstance(), SIGNAL(currentSourceChanged(SourceSet::iterator)), this, SLOT(connectSource(SourceSet::iterator) ) );

	// QUIT event
    QObject::connect(actionQuit, SIGNAL(triggered()), this, SLOT(close()));
    QObject::connect(actionAbout_Qt, SIGNAL(triggered()), QApplication::instance(), SLOT(aboutQt()));

    // Signals between GUI and output window
    QObject::connect(actionFullscreen, SIGNAL(toggled(bool)), OutputRenderWindow::getInstance(), SLOT(setFullScreen(bool)));
	QObject::connect(actionFree_aspect_ratio, SIGNAL(toggled(bool)), OutputRenderWindow::getInstance(), SLOT(useFreeAspectRatio(bool)));
	QObject::connect(actionWhite_background, SIGNAL(toggled(bool)), RenderingManager::getInstance(), SLOT(setClearToWhite(bool)));
	QObject::connect(OutputRenderWindow::getInstance(), SIGNAL(resized(bool)), outputpreview, SLOT(useFreeAspectRatio(bool)));
	QObject::connect(OutputRenderWindow::getInstance(), SIGNAL(resized(bool)), RenderingManager::getRenderingWidget(), SLOT(refresh()));
	QObject::connect(actionShow_Catalog, SIGNAL(toggled(bool)), RenderingManager::getRenderingWidget(), SLOT(setCatalogVisible(bool)));
	// group the menu items of the catalog sizes ;
	QActionGroup *catalogActionGroup = new QActionGroup(this);
	catalogActionGroup->addAction(actionCatalogSmall);
	catalogActionGroup->addAction(actionCatalogMedium);
	catalogActionGroup->addAction(actionCatalogLarge);
	QObject::connect(actionCatalogSmall, SIGNAL(triggered()), RenderingManager::getRenderingWidget(), SLOT(setCatalogSizeSmall()));
	QObject::connect(actionCatalogMedium, SIGNAL(triggered()), RenderingManager::getRenderingWidget(), SLOT(setCatalogSizeMedium()));
	QObject::connect(actionCatalogLarge, SIGNAL(triggered()), RenderingManager::getRenderingWidget(), SLOT(setCatalogSizeLarge()));

	// Signals between GUI and rendering widget
	QObject::connect(actionFree_aspect_ratio, SIGNAL(toggled(bool)), RenderingManager::getRenderingWidget(), SLOT(refresh()));
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

    // recall settings from last time
    readSettings();
}

GLMixer::~GLMixer()
{
	saveSettings();

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


void GLMixer::displayInfoMessage(QString msg){

	statusbar->showMessage( msg, 3000 );
}


void GLMixer::displayWarningMessage(QString msg){

	QMessageBox::warning(0, tr("%1 Warning").arg(QCoreApplication::applicationName()), tr("The following error occurred:\n\n%1").arg(msg));
}


void GLMixer::setView(QAction *a){

	if (a == actionMixingView)
		RenderingManager::getRenderingWidget()->setViewMode(ViewRenderWidget::MIXING);
	else if (a == actionGeometryView)
		RenderingManager::getRenderingWidget()->setViewMode(ViewRenderWidget::GEOMETRY);
	else if (a == actionLayersView)
		RenderingManager::getRenderingWidget()->setViewMode(ViewRenderWidget::LAYER);

	viewIcon->setPixmap(RenderingManager::getRenderingWidget()->getViewIcon());

	switch ( RenderingManager::getRenderingWidget()->getToolMode() ){
	case ViewRenderWidget::TOOL_SCALE:
		actionToolScale->trigger();
		break;
	case ViewRenderWidget::TOOL_ROTATE:
		actionToolRotate->trigger();
		break;
	case ViewRenderWidget::TOOL_CUT:
		actionToolCut->trigger();
		break;
	default:
	case ViewRenderWidget::TOOL_GRAB:
		actionToolGrab->trigger();
		break;
	}
}

void GLMixer::setTool(QAction *a){

	if (a == actionToolGrab)
		RenderingManager::getRenderingWidget()->setToolMode(ViewRenderWidget::TOOL_GRAB);
	else if (a == actionToolScale)
		RenderingManager::getRenderingWidget()->setToolMode(ViewRenderWidget::TOOL_SCALE);
	else if (a == actionToolRotate)
		RenderingManager::getRenderingWidget()->setToolMode(ViewRenderWidget::TOOL_ROTATE);
	else if (a == actionToolCut)
		RenderingManager::getRenderingWidget()->setToolMode(ViewRenderWidget::TOOL_CUT);

}


void GLMixer::setCursor(QAction *a){

	if (a == actionCursorNormal)
		RenderingManager::getRenderingWidget()->setCursorMode(ViewRenderWidget::CURSOR_NORMAL);
	else if (a == actionCursorSpring)
		RenderingManager::getRenderingWidget()->setCursorMode(ViewRenderWidget::CURSOR_SPRING);
	else if (a == actionCursorDelay)
		RenderingManager::getRenderingWidget()->setCursorMode(ViewRenderWidget::CURSOR_DELAY);

}

void GLMixer::on_actionMediaSource_triggered(){

	QStringList fileNames;

#ifndef NO_VIDEO_FILE_DIALOG_PREVIEW

	if (mfd->exec())
		fileNames = mfd->selectedFiles();

#else
	fileNames = QFileDialog::getOpenFileNames(this, tr("Open File"),
													QDir::currentPath(),
													tr("Video (*.mov *.avi *.wmv *.mpeg *.mp4 *.mpg *.vob *.swf *.flv);;Image (*.png *.jpg *.jpeg *.tif *.tiff *.gif *.tga *.sgi *.bmp)"));

	d.setPath(fileNames.first());
#endif

	QStringListIterator fileNamesIt(fileNames);
	while (fileNamesIt.hasNext()){

	    VideoFile *newSourceVideoFile = NULL;
	    QString filename = fileNamesIt.next();

#ifndef NO_VIDEO_FILE_DIALOG_PREVIEW
		if ( !mfd->configCustomSize() && (glSupportsExtension("GL_EXT_texture_non_power_of_two") || glSupportsExtension("GL_ARB_texture_non_power_of_two") ) )
			newSourceVideoFile = new VideoFile(this);
		else
			newSourceVideoFile = new VideoFile(this, true, SWS_POINT);
#else
		newSourceVideoFile = new VideoFile(this);
#endif
	    Q_CHECK_PTR(newSourceVideoFile);

	    QString caption = tr("%1 create source").arg(QCoreApplication::applicationName());
		// if the video file was created successfully
		if (newSourceVideoFile){
			// forward error messages to display
			QObject::connect(newSourceVideoFile, SIGNAL(error(QString)), this, SLOT(displayWarningMessage(QString)));
			QObject::connect(newSourceVideoFile, SIGNAL(info(QString)), this, SLOT(displayInfoMessage(QString)));
			// can we open the file ?
			if ( newSourceVideoFile->open( filename ) ) {
				Source *s = RenderingManager::getInstance()->newMediaSource(newSourceVideoFile);
				// create the source as it is a valid video file (this also set it to be the current source)
				if ( s ) {
					RenderingManager::getInstance()->addSourceToBasket(s);
				} else {
			        QMessageBox::warning(this, caption, tr("Could not create media source."));
			        delete newSourceVideoFile;
				}
			} else {
				displayInfoMessage (tr("The file %1 was not loaded.").arg(filename));
				delete newSourceVideoFile;
			}
		}

	}
	if (RenderingManager::getInstance()->getSourceBasketSize() > 0)
		statusbar->showMessage( tr("%1 media source(s) created; you can now drop them.").arg( RenderingManager::getInstance()->getSourceBasketSize() ), 3000 );
}


// method called when a source is made current (either after loading, either after clicking on it).
// The goal is to have the GUI display the current state of the video file to be able to control the video playback
// and to read the correct information and configuration options
void GLMixer::connectSource(SourceSet::iterator csi){

	static Source *playableSelection = NULL;

	if (playableSelection != NULL) {

		switch (playableSelection->rtti()){
		case Source::ALGORITHM_SOURCE:
			QObject::disconnect(startButton, SIGNAL(toggled(bool)), dynamic_cast<AlgorithmSource *>(playableSelection), SLOT(play(bool)));
			break;
#ifdef OPEN_CV
		case Source::CAMERA_SOURCE:
			QObject::disconnect(startButton, SIGNAL(toggled(bool)), dynamic_cast<OpencvSource *>(playableSelection), SLOT(play(bool)));
			break;
#endif
		case Source::VIDEO_SOURCE:
			QObject::disconnect(startButton, SIGNAL(toggled(bool)), dynamic_cast<VideoSource *>(playableSelection), SLOT(play(bool)));
			break;
		default:
			break;
		}

		// clear it for next time
		playableSelection = NULL;
	}

	// whatever happens, we will drop the control on the current source
	//   (this slot is called by MainRenderWidget through signal currentSourceChanged
	//    which is sent ONLY when the current source is changed)
	if (selectedSourceVideoFile) {

        QObject::disconnect(selectedSourceVideoFile, SIGNAL(paused(bool)), pauseButton, SLOT(setChecked(bool)));
        QObject::disconnect(selectedSourceVideoFile, SIGNAL(running(bool)), videoFrame, SLOT(setEnabled(bool)));
        QObject::disconnect(selectedSourceVideoFile, SIGNAL(running(bool)), timingControlFrame, SLOT(setEnabled(bool)));
        QObject::disconnect(selectedSourceVideoFile, SIGNAL(markingChanged()), this, SLOT(updateMarks()));
        QObject::disconnect(selectedSourceVideoFile, SIGNAL(running(bool)), this, SLOT(updateRefreshTimerState()));
        QObject::disconnect(selectedSourceVideoFile, SIGNAL(paused(bool)), this, SLOT(updateRefreshTimerState()));

		// disconnect control buttons
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

		// enable properties and actions on the current valid source
		sourceDockWidgetContents->setEnabled(true);
		actionDeleteSource->setEnabled(true);
		actionCloneSource->setEnabled(true);

		// Enable start button if the source is playable
		startButton->setEnabled( (*csi)->isPlayable() );
		vcontrolDockWidgetContents->setEnabled( (*csi)->isPlayable() );
		vcontrolDockWidgetControls->setEnabled( (*csi)->isPlayable() );

		startButton->setChecked( (*csi)->isPlaying() );

		if ((*csi)->isPlayable()) {
			playableSelection = (*csi);

			switch (playableSelection->rtti()){
			case Source::ALGORITHM_SOURCE:
				QObject::connect(startButton, SIGNAL(toggled(bool)), dynamic_cast<AlgorithmSource *>(playableSelection), SLOT(play(bool)));
				break;
#ifdef OPEN_CV
			case Source::CAMERA_SOURCE:
				QObject::connect(startButton, SIGNAL(toggled(bool)), dynamic_cast<OpencvSource *>(playableSelection), SLOT(play(bool)));
				break;
#endif
			case Source::VIDEO_SOURCE:
				QObject::connect(startButton, SIGNAL(toggled(bool)), dynamic_cast<VideoSource *>(playableSelection), SLOT(play(bool)));
				break;
			default:
				break;
			}

			// except for video source, these pannels are disabled
			vcontrolDockWidgetOptions->setEnabled(false);
			videoFrame->setEnabled(false);
			timingControlFrame->setEnabled(false);

			// Among playable sources, there is the particular case of video sources :
			if ( (*csi)->rtti() == Source::VIDEO_SOURCE ) {
				// get the pointer to the video to control
		        selectedSourceVideoFile = (dynamic_cast<VideoSource *>(*csi))->getVideoFile();

		        // control this video if it is valid
		        if (selectedSourceVideoFile){

		        	// setup GUI button states to match the current state of the videofile; do it before connecting slots to avoid re-emiting signals
					vcontrolDockWidgetOptions->setEnabled(true);
		        	videoFrame->setEnabled(selectedSourceVideoFile->isRunning());
					timingControlFrame->setEnabled(selectedSourceVideoFile->isRunning());

					pauseButton->setChecked( selectedSourceVideoFile->isPaused());
					dirtySeekCheckBox->setChecked(selectedSourceVideoFile->getOptionAllowDirtySeek());
					resetToBlackCheckBox->setChecked(selectedSourceVideoFile->getOptionRevertToBlackWhenStop());
					restartWhereStoppedCheckBox->setChecked(selectedSourceVideoFile->getOptionRestartToMarkIn());
					videoLoopButton->setChecked(selectedSourceVideoFile->isLoop());
					playSpeedBox->setCurrentIndex(selectedSourceVideoFile->getPlaySpeed());

		            // CONTROL signals from GUI to VideoFile
		            QObject::connect(pauseButton, SIGNAL(toggled(bool)), selectedSourceVideoFile, SLOT(pause(bool)));
		            QObject::connect(playSpeedBox, SIGNAL(currentIndexChanged(int)), selectedSourceVideoFile, SLOT(setPlaySpeed(int)));
		            QObject::connect(dirtySeekCheckBox, SIGNAL(toggled(bool)), selectedSourceVideoFile, SLOT(setOptionAllowDirtySeek(bool)));
		            QObject::connect(resetToBlackCheckBox, SIGNAL(toggled(bool)), selectedSourceVideoFile, SLOT(setOptionRevertToBlackWhenStop(bool)));
		            QObject::connect(restartWhereStoppedCheckBox, SIGNAL(toggled(bool)), selectedSourceVideoFile, SLOT(setOptionRestartToMarkIn(bool)));

		            QObject::connect(seekBackwardButton, SIGNAL(clicked()), selectedSourceVideoFile, SLOT(seekBackward()));
		            QObject::connect(seekForwardButton, SIGNAL(clicked()), selectedSourceVideoFile, SLOT(seekForward()));
		            QObject::connect(seekBeginButton, SIGNAL(clicked()), selectedSourceVideoFile, SLOT(seekBegin()));
		            QObject::connect(seekBackwardButton, SIGNAL(clicked()), this, SLOT(unpauseBeforeSeek()));
		            QObject::connect(seekForwardButton, SIGNAL(clicked()), this, SLOT(unpauseBeforeSeek()));
		            QObject::connect(seekBeginButton, SIGNAL(clicked()), this, SLOT(unpauseBeforeSeek()));


		            QObject::connect(videoLoopButton, SIGNAL(toggled(bool)), selectedSourceVideoFile, SLOT(setLoop(bool)));
		            QObject::connect(markInButton, SIGNAL(clicked()), selectedSourceVideoFile, SLOT(setMarkIn()));
		            QObject::connect(markOutButton, SIGNAL(clicked()), selectedSourceVideoFile, SLOT(setMarkOut()));
		            QObject::connect(resetMarkInButton, SIGNAL(clicked()), selectedSourceVideoFile, SLOT(resetMarkIn()));
		            QObject::connect(resetMarkOutButton, SIGNAL(clicked()), selectedSourceVideoFile, SLOT(resetMarkOut()));

		            // DISPLAY consistency from VideoFile to GUI
		            QObject::connect(selectedSourceVideoFile, SIGNAL(paused(bool)), pauseButton, SLOT(setChecked(bool)));
		            QObject::connect(selectedSourceVideoFile, SIGNAL(running(bool)), videoFrame, SLOT(setEnabled(bool)));
		            QObject::connect(selectedSourceVideoFile, SIGNAL(running(bool)), timingControlFrame, SLOT(setEnabled(bool)));
		            QObject::connect(selectedSourceVideoFile, SIGNAL(error(QString)), this, SLOT(displayWarningMessage(QString)));
		            QObject::connect(selectedSourceVideoFile, SIGNAL(info(QString)), this, SLOT(displayInfoMessage(QString)));

		            // Consistency and update timer control from VideoFile
		            QObject::connect(selectedSourceVideoFile, SIGNAL(markingChanged()), this, SLOT(updateMarks()));
		            QObject::connect(selectedSourceVideoFile, SIGNAL(running(bool)), this, SLOT(updateRefreshTimerState()));
		            QObject::connect(selectedSourceVideoFile, SIGNAL(paused(bool)), this, SLOT(updateRefreshTimerState()));

					// display info (time or frames)
					on_actionShow_frames_toggled(actionShow_frames->isChecked());

					// restart slider timer
					updateRefreshTimerState();
					updateMarks();
		        } // end video file
			} // end video source
		} // end playable
	} else {  // it is not a valid source

		// disable panel widgets
		actionDeleteSource->setEnabled(false);
		actionCloneSource->setEnabled(false);
		vcontrolDockWidgetContents->setEnabled(false);
		startButton->setEnabled( false );
		startButton->setChecked( false );

		sourceDockWidgetContents->setEnabled(false);
	}

}

void GLMixer::on_actionCameraSource_triggered() {

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
				Source *s = RenderingManager::getInstance()->newCloneSource(sit);
				if ( s ) {
					RenderingManager::getInstance()->addSourceToBasket(s);
					statusbar->showMessage( tr("Clone created of the source with OpenCV drivers for Camera %1").arg(cd.indexOpencvCamera()), 3000 );
				} else
			        QMessageBox::warning(this, tr("%1 create source").arg(QCoreApplication::applicationName()), tr("Could not create clone source."));
			}
			//else create a new opencv source :
			else {
				Source *s = RenderingManager::getInstance()->newOpencvSource(cd.indexOpencvCamera());
				if ( s ) {
					RenderingManager::getInstance()->addSourceToBasket(s);
					statusbar->showMessage( tr("Source created with OpenCV drivers for Camera %1").arg(cd.indexOpencvCamera()), 3000 );
				} else
			        QMessageBox::warning(this, tr("%1 create source").arg(QCoreApplication::applicationName()), tr("Could not create OpenCV source."));
			}
		}
#endif

	}

}


void GLMixer::on_actionAlgorithmSource_triggered(){

	// popup a question dialog to select the type of algorithm
	static AlgorithmSelectionDialog *asd = 0;
	if (!asd) {
		asd = new AlgorithmSelectionDialog(this);
		asd->setModal(false);
	}

	if (asd->exec() == QDialog::Accepted) {
		Source *s = RenderingManager::getInstance()->newAlgorithmSource(asd->getSelectedAlgorithmIndex(),
					asd->getSelectedWidth(), asd->getSelectedHeight(), asd->getSelectedVariability(), asd->getUpdatePeriod());
		if ( s ){
			RenderingManager::getInstance()->addSourceToBasket(s);
			statusbar->showMessage( tr("Source created with the algorithm %1.").arg( AlgorithmSource::getAlgorithmDescription(asd->getSelectedAlgorithmIndex())), 3000 );
		} else
	        QMessageBox::warning(this, tr("%1 create source").arg(QCoreApplication::applicationName()), tr("Could not create algorithm source."));
	}
}


void GLMixer::on_actionRenderingSource_triggered(){

	Source *s = RenderingManager::getInstance()->newRenderingSource();
	if ( s ){
		RenderingManager::getInstance()->addSourceToBasket(s);
		statusbar->showMessage( tr("Source created with the rendering output loopback."), 3000 );
	}else
        QMessageBox::warning(this, tr("%1 create source").arg(QCoreApplication::applicationName()), tr("Could not create rendering source."));
}


void GLMixer::on_actionCloneSource_triggered(){

	if ( RenderingManager::getInstance()->notAtEnd(RenderingManager::getInstance()->getCurrentSource()) ) {
		Source *s = RenderingManager::getInstance()->newCloneSource( RenderingManager::getInstance()->getCurrentSource());
		if ( s ) {
			RenderingManager::getInstance()->addSourceToBasket(s);
			statusbar->showMessage( tr("The current source has been cloned."), 3000);
		} else
	        QMessageBox::warning(this, tr("%1 create source").arg(QCoreApplication::applicationName()), tr("Could not create clone source."));
	}
}


CaptureDialog::CaptureDialog(QWidget *parent, QImage capture) : QDialog(parent), img(capture) {

	QVBoxLayout *verticalLayout;
	QLabel *Question, *Display;
	QDialogButtonBox *DecisionButtonBox;
	setObjectName(QString::fromUtf8("CaptureDialog"));
	resize(300, 179);
	setWindowTitle(tr( "Output capture source"));
	verticalLayout = new QVBoxLayout(this);
	verticalLayout->setObjectName(QString::fromUtf8("verticalLayout"));
	Question = new QLabel(this);
	Question->setObjectName(QString::fromUtf8("Question"));
	QSizePolicy sizePolicy(QSizePolicy::Preferred, QSizePolicy::Maximum);
	sizePolicy.setHorizontalStretch(0);
	sizePolicy.setVerticalStretch(0);
	sizePolicy.setHeightForWidth(Question->sizePolicy().hasHeightForWidth());
	Question->setSizePolicy(sizePolicy);
	Question->setText(tr("Create a source with this image ?"));
	verticalLayout->addWidget(Question);

	Display = new QLabel(this);
	Display->setObjectName(QString::fromUtf8("Display"));
	Display->setPixmap(QPixmap::fromImage(img).scaledToWidth(300));
	verticalLayout->addWidget(Display);

	DecisionButtonBox = new QDialogButtonBox(this);
	DecisionButtonBox->setObjectName(QString::fromUtf8("DecisionButtonBox"));
	DecisionButtonBox->setStandardButtons(QDialogButtonBox::Cancel|QDialogButtonBox::Ok);
	QPushButton *save =  DecisionButtonBox->addButton ( "Save as..", QDialogButtonBox::ActionRole );
	QObject::connect(save, SIGNAL(clicked()), this, SLOT(saveImage()));

	verticalLayout->addWidget(DecisionButtonBox);
	QObject::connect(DecisionButtonBox, SIGNAL(accepted()), this, SLOT(accept()));
	QObject::connect(DecisionButtonBox, SIGNAL(rejected()), this, SLOT(reject()));
}


void CaptureDialog::saveImage()
{
	filename = QFileDialog::getSaveFileName ( this, tr("Save captured image"), filename,  tr("Images (*.png *.xpm *.jpg *.jpeg *.tiff)"));

	if (!filename.isEmpty()) {
		if (!img.save(filename))
			qWarning("** Warning **\n\nCould not save file %s.", qPrintable(filename));
		filename = QDir::fromNativeSeparators(filename);
		filename.resize( filename.lastIndexOf('/') );
	}
}


void GLMixer::on_actionCaptureSource_triggered(){

	// capture screen
	QImage capture = RenderingManager::getInstance()->captureFrameBuffer();

	// display and request action with this capture
	CaptureDialog cd(this, capture);

	if (cd.exec() == QDialog::Accepted) {
		Source *s = RenderingManager::getInstance()->newCaptureSource(capture);
		if ( s ){
			RenderingManager::getInstance()->addSourceToBasket(s);
			statusbar->showMessage( tr("Source created with a capture of the output."), 3000 );
		} else
	        QMessageBox::warning(this, tr("%1 create source").arg(QCoreApplication::applicationName()), tr("Could not create capture source."));
	}
}


void GLMixer::on_actionDeleteSource_triggered(){

	if ( RenderingManager::getInstance()->isValid(RenderingManager::getInstance()->getCurrentSource()) ) {

		int numclones = (*RenderingManager::getInstance()->getCurrentSource())->getClones()->size();
		// popup a question dialog 'are u sure' if there are clones attached;
		if ( numclones ){
			QString msg = tr("This source was cloned %1 times; all these clones will be removed with this source if you confirm the removal.").arg(numclones);
			if ( QMessageBox::question(this," Are you sure?", msg, QMessageBox::Ok | QMessageBox::Cancel, QMessageBox::Ok) == QMessageBox::Ok)
				numclones = 0;
		}

		if ( !numclones ){
			QString d = (*RenderingManager::getInstance()->getCurrentSource())->getName();
			RenderingManager::getInstance()->removeSource(RenderingManager::getInstance()->getCurrentSource());
			statusbar->showMessage( tr("Source %1 deleted.").arg( d ), 3000 );
		}
	}
}


void GLMixer::on_actionSelect_Next_triggered(){

	if (RenderingManager::getInstance()->setCurrentNext())
		statusbar->showMessage( tr("Source %1 selected.").arg( (*RenderingManager::getInstance()->getCurrentSource())->getName() ), 3000 );
}

void GLMixer::on_actionSelect_Previous_triggered(){

	if (RenderingManager::getInstance()->setCurrentPrevious())
		statusbar->showMessage( tr("Source %1 selected.").arg( (*RenderingManager::getInstance()->getCurrentSource())->getName() ), 3000 );

}


void GLMixer::on_actionShow_frames_toggled(bool on){

	if (selectedSourceVideoFile) {
	    // update property for marks in / out
	    emit sourceMarksModified(on);
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
            timeLineEdit->setText( selectedSourceVideoFile->getExactFrameFromFrame(selectedSourceVideoFile->getCurrentFrameTime()) );
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
    // compute where we should jump to
    double percent = (double)(v)/ (double)frameSlider->maximum();
    int64_t pos = (int64_t) ( selectedSourceVideoFile->getEnd()  * percent );
    pos += selectedSourceVideoFile->getBegin();

    // request seek ; we need to have the VideoFile process running to go there
    selectedSourceVideoFile->seekToPosition(pos);

    // disconnect the button from the VideoFile signal ; this way when we'll unpause bellow, the button will keep its state
    QObject::disconnect(selectedSourceVideoFile, SIGNAL(paused(bool)), pauseButton, SLOT(setChecked(bool)));

    // the trick; call a method when the frame will be ready!
    QObject::connect(selectedSourceVideoFile, SIGNAL(frameReady(int)), this, SLOT(pauseAfterFrame()));

    // let the VideoFile run till it displays the frame seeked
    selectedSourceVideoFile->pause(false);

    // cosmetics to show the time of the frame (refreshTiming disabled)
    if (actionShow_frames->isChecked())
        timeLineEdit->setText( selectedSourceVideoFile->getExactFrameFromFrame(pos) );
    else
        timeLineEdit->setText( selectedSourceVideoFile->getTimeFromFrame(pos) );

}

void GLMixer::on_frameForwardButton_clicked(){

    // disconnect the button from the VideoFile signal ; this way when we'll unpause bellow, the button will keep its state
    QObject::disconnect(selectedSourceVideoFile, SIGNAL(paused(bool)), pauseButton, SLOT(setChecked(bool)));

    // the trick; call a method when the frame will be ready!
    QObject::connect(selectedSourceVideoFile, SIGNAL(frameReady(int)), this, SLOT(pauseAfterFrame()));

    // let the VideoFile run till it displays 1 frame
    selectedSourceVideoFile->pause(false);

    // cosmetics to show the time of the frame (refreshTiming disabled)
    if (actionShow_frames->isChecked())
        timeLineEdit->setText( selectedSourceVideoFile->getExactFrameFromFrame(selectedSourceVideoFile->getCurrentFrameTime()) );
    else
        timeLineEdit->setText( selectedSourceVideoFile->getTimeFromFrame(selectedSourceVideoFile->getCurrentFrameTime()) );
}

void GLMixer::pauseAfterFrame (){

	selectedSourceVideoFile->pause(true);

	// do not keep calling pause method for each frame !
	QObject::disconnect(selectedSourceVideoFile, SIGNAL(frameReady(int)), this, SLOT(pauseAfterFrame()));
	// reconnect the pause button
	QObject::connect(selectedSourceVideoFile, SIGNAL(paused(bool)), pauseButton, SLOT(setChecked(bool)));
}

void GLMixer::unpauseBeforeSeek() {

    // disconnect the button from the VideoFile signal ; this way when we'll unpause bellow, the button will keep its state
    QObject::disconnect(selectedSourceVideoFile, SIGNAL(paused(bool)), pauseButton, SLOT(setChecked(bool)));

    // the trick; call a method when the frame will be ready!
    QObject::connect(selectedSourceVideoFile, SIGNAL(frameReady(int)), this, SLOT(pauseAfterSeek()));

    // let the VideoFile run till it displays the frame seeked
    selectedSourceVideoFile->pause(false);

}

void GLMixer::pauseAfterSeek (){

//	static int frameCounter = 0;

	// if the button 'Pause' is checked, we shall go back to pause once
	// we'll have displayed the seeked frame
	if (pauseButton->isChecked()) {

//		// make sure we display all the pictures in the queue
//		if (frameCounter++ <= VIDEO_PICTURE_QUEUE_SIZE)
//			return;

		selectedSourceVideoFile->pause(true);
//		frameCounter = 0;

		refreshTiming();
	}

	// do not keep calling pause method for each frame !
	QObject::disconnect(selectedSourceVideoFile, SIGNAL(frameReady(int)), this, SLOT(pauseAfterSeek()));
	// reconnect the pause button
	QObject::connect(selectedSourceVideoFile, SIGNAL(paused(bool)), pauseButton, SLOT(setChecked(bool)));
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

            if (pauseButton->isChecked()) {
                on_frameSlider_sliderMoved (frameSlider->sliderPosition ());
            } else {
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



void GLMixer::updateMarks (){

	// adjust the marking sliders according to the source marks in and out
    int i_percent = (int) ( (double)( selectedSourceVideoFile->getMarkIn() - selectedSourceVideoFile->getBegin() ) / (double)( selectedSourceVideoFile->getEnd() - selectedSourceVideoFile->getBegin() ) * 1000.0) ;
    int o_percent = (int) ( (double)( selectedSourceVideoFile->getMarkOut() - selectedSourceVideoFile->getBegin() ) / (double)( selectedSourceVideoFile->getEnd() - selectedSourceVideoFile->getBegin() ) * 1000.0) ;

    markInSlider->setValue(i_percent);
    markOutSlider->setValue(o_percent);

    // update property for marks in / out
    emit sourceMarksModified(actionShow_frames->isChecked());
}


void GLMixer::on_actionShowFPS_toggled(bool on){

	RenderingManager::getRenderingWidget()->showFramerate(on);
}

void GLMixer::on_actionAbout_triggered(){

	QString msg = QString("%1 :     \tGraphic Live Mixer\n\n").arg(QCoreApplication::applicationName());
	msg.append(QString("Author:   \tBruno Herbelin\n"));
	msg.append(QString("Contact:  \tbruno.herbelin@gmail.com\n"));
	msg.append(QString("License:  \tGNU GPL version 3\n"));
    msg.append(QString("Version:  \t%1\n").arg(QCoreApplication::applicationVersion()));
	
#ifdef GLMIXER_REVISION
    msg.append(QString("SVN:      \t%1\n").arg(GLMIXER_REVISION));
#endif
	
    msg.append(tr("\nGLMixer is a video mixing software for live performance.\n"));
    msg.append(tr("\nUpdates and source code at:\nhttp://code.google.com/p/glmixer/\n"));
    msg.append(tr("\nCopyright 2009, 2010 Bruno Herbelin"));
	QMessageBox::information(this, tr("About %1").arg(QCoreApplication::applicationName()), msg, QMessageBox::Ok, QMessageBox::Ok);

}


void GLMixer::changeWindowTitle(){

	//	session file name
//	QString session = currentStageFileName.isNull() ? "unsaved session" : QFileInfo(currentStageFileName).fileName();

	setWindowFilePath(currentStageFileName);
	setWindowModified(false);

//    setWindowTitle(QString("%1 %2 - %3[*]").arg(QCoreApplication::applicationName()).arg(QCoreApplication::applicationVersion()).arg(session));

//	actionAppend_Session->setEnabled(!currentStageFileName.isNull());
}

//QString title = QString("Super Mixer %1 - ").arg(QApplication::applicationVersion());
//
////	Trick to get only the filename from the full path for every platform ; use the QDir class
//title += currentStageFileName.isNull() ? "unsaved session" : QDir(currentStageFileName).dirName();
//
//
//setWindowTitle(title);
//
//actionAppend_Session->setEnabled(!currentStageFileName.isNull());

void GLMixer::on_actionNew_Session_triggered()
{
	// inform the user that data might be lost
	int ret = QMessageBox::Discard;

// TODO : implement good mechanism to know if something was changed
//	if (!currentStageFileName.isNull()) {
//		 QMessageBox msgBox;
//		 msgBox.setText("The session may have been modified.");
//		 msgBox.setInformativeText("Do you want to save your changes?");
//		 msgBox.setIconPixmap( QPixmap(QString::fromUtf8(":/glmixer/icons/question.png")) );
//		 msgBox.setStandardButtons(QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel);
//		 msgBox.setDefaultButton(QMessageBox::Save);
//		 ret = msgBox.exec();
//	}
	// react according to user's answer
	switch (ret) {
	   case QMessageBox::Save:
		   // Save was clicked
		   on_actionSave_Session_triggered();
		   break;
	   case QMessageBox::Cancel:
		   // Cancel was clicked
		   return;
	   case QMessageBox::Discard:
	   default:
		   // keep on to create new session
		   break;
	}
	// make a new session
	currentStageFileName = QString();
	changeWindowTitle();
	RenderingManager::getInstance()->clearSourceSet();
	RenderingManager::getRenderingWidget()->clearViews();

	// TODO: maybe a default for these options into the user preferences?
	actionWhite_background->setChecked(false);
	actionFree_aspect_ratio->setChecked(false);
}


void GLMixer::on_actionSave_Session_triggered(){

	if (currentStageFileName.isNull())
		on_actionSave_Session_as_triggered();
	else {

		QFile file(currentStageFileName);
		if (!file.open(QFile::WriteOnly | QFile::Text) ) {
			QMessageBox::warning(this, tr("%1 session save").arg(QCoreApplication::applicationName()), tr("Cannot write file %1:\n%2.").arg(currentStageFileName).arg(file.errorString()));
			return;
		}
		QTextStream out(&file);

		QDomDocument doc;
		QDomProcessingInstruction instr = doc.createProcessingInstruction("xml", "version='1.0' encoding='UTF-8'");
		doc.appendChild(instr);

		QDomElement root = doc.createElement("GLMixer");
		root.setAttribute("version", XML_GLM_VERSION);

		QDomElement renderConfig = RenderingManager::getInstance()->getConfiguration(doc);
		root.appendChild(renderConfig);

		QDomElement viewConfig =  RenderingManager::getRenderingWidget()->getConfiguration(doc);
		root.appendChild(viewConfig);

		QDomElement rendering = doc.createElement("Rendering");
		rendering.setAttribute("clearToWhite", (int) RenderingManager::getInstance()->clearToWhite());
		rendering.setAttribute("freeAspectRatio", (int) OutputRenderWindow::getInstance()->freeAspectRatio());
		root.appendChild(rendering);

		doc.appendChild(root);
		doc.save(out, 4);

		changeWindowTitle();
		statusbar->showMessage( tr("File %1 saved.").arg( currentStageFileName ), 3000 );
	}
}

void GLMixer::on_actionSave_Session_as_triggered(){

    static QFileDialog *SaveSessionFileDialog = 0;
    if (!SaveSessionFileDialog) {
    	SaveSessionFileDialog = new QFileDialog(this);
        SaveSessionFileDialog->restoreState(settings.value("SaveSessionFileDialog").toByteArray());
    	SaveSessionFileDialog->setAcceptMode(QFileDialog::AcceptSave);
    	SaveSessionFileDialog->setFileMode(QFileDialog::AnyFile);
    	SaveSessionFileDialog->setFilter(tr("GLMixer workspace (*.glm)"));
    	SaveSessionFileDialog->setDefaultSuffix("glm");
    }

	if (SaveSessionFileDialog->exec()) {
	    QString fileName = SaveSessionFileDialog->selectedFiles().front();
		// now we got a filename, save the file:
		currentStageFileName = fileName;
		on_actionSave_Session_triggered();
	}

    settings.setValue("SaveSessionFileDialog", SaveSessionFileDialog->saveState());
    settings.sync();
}

void GLMixer::on_actionLoad_Session_triggered()
{
//	QString fileName = QDir::fromNativeSeparators(currentStageFileName);
//	fileName.resize( fileName.lastIndexOf('/') );
//
//    fileName = QFileDialog::getOpenFileName(this, tr("Open session file"), fileName, tr("GLMixer workspace (*.glm)"));
//    if (fileName.isEmpty())
//         return;
//	openSessionFile(fileName);

    static QFileDialog *LoadSessionFileDialog = 0;
    if (!LoadSessionFileDialog) {
    	LoadSessionFileDialog = new QFileDialog(this);
    	LoadSessionFileDialog->restoreState(settings.value("LoadSessionFileDialog").toByteArray());
    	LoadSessionFileDialog->setAcceptMode(QFileDialog::AcceptOpen);
    	LoadSessionFileDialog->setFileMode(QFileDialog::ExistingFile);
    	LoadSessionFileDialog->setFilter(tr("GLMixer workspace (*.glm)"));
    }

	if (LoadSessionFileDialog->exec()) {
	    QString fileName = LoadSessionFileDialog->selectedFiles().front();
		openSessionFile(fileName);
	}

    settings.setValue("LoadSessionFileDialog", LoadSessionFileDialog->saveState());
    settings.sync();
}

void GLMixer::openSessionFile(QString fileName)
{
	QDomDocument doc;
    QString errorStr;
    int errorLine;
    int errorColumn;

     QFile file(fileName);
     QString caption = tr("%1 session open").arg(QCoreApplication::applicationName());
     if (!file.open(QFile::ReadOnly | QFile::Text)) {
         QMessageBox::warning(this, caption, tr("Cannot open file %1:\n\n%2.").arg(fileName).arg(file.errorString()));
         return;
     }

    if (!doc.setContent(&file, true, &errorStr, &errorLine, &errorColumn)) {
        QMessageBox::warning(this, caption, tr("Problem reading %1.\n\nParse error at line %2, column %3:\n%4").arg(fileName).arg(errorLine).arg(errorColumn).arg(errorStr));
        return;
    }

    QDomElement root = doc.documentElement();
    if (root.tagName() != "GLMixer") {
        QMessageBox::critical(this, caption, tr("The file %1 is not a valid GLMixer session file.").arg(fileName));
        return;
    } else if (root.hasAttribute("version") && root.attribute("version") != XML_GLM_VERSION) {
        QMessageBox::warning(this, caption, tr("Problem loading %1\n\nThe version of the file is not compatible (%2 instead of %3).\nI will try to do what I can...\n").arg(fileName).arg(root.attribute("version")).arg(XML_GLM_VERSION));
    }

    // read all the content to make sure the file is correct :
    QDomElement srcconfig = root.firstChildElement("SourceList");
    if (!srcconfig.isNull()){
    	// if we got up to here, it should be fine ; reset for a new session and apply loaded configurations
        on_actionNew_Session_triggered();
    	RenderingManager::getInstance()->addConfiguration(srcconfig);
        // confirm the loading of the file
    	currentStageFileName = fileName;
    	changeWindowTitle();
    	statusbar->showMessage( tr("Session file %1 loaded.").arg( currentStageFileName ), 5000 );
    }

    // less important ; the views config
    QDomElement vconfig = root.firstChildElement("Views");
    if (!vconfig.isNull()){

    	RenderingManager::getRenderingWidget()->setConfiguration(vconfig);

    	// activate the view specified as 'current' in the xml config
    	switch ( (ViewRenderWidget::viewMode) vconfig.attribute("current").toInt()){
    	case (ViewRenderWidget::NONE):
    	case (ViewRenderWidget::MIXING):
    		actionMixingView->trigger();
    		break;
    	case (ViewRenderWidget::GEOMETRY):
    		actionGeometryView->trigger();
    		break;
    	case (ViewRenderWidget::LAYER):
    		actionLayersView->trigger();
    		break;
    	}
    	// show the catalog as specified in xlm config
    	QDomElement cat = vconfig.firstChildElement("Catalog");
    	actionShow_Catalog->setChecked(cat.attribute("visible").toInt());
    	switch( (CatalogView::catalogSize) (cat.firstChildElement("Parameters").attribute("catalogSize").toInt()) ){
    	case (CatalogView::SMALL):
    		actionCatalogSmall->trigger();
    		break;
    	case (CatalogView::MEDIUM):
    		actionCatalogMedium->trigger();
    		break;
    	case (CatalogView::LARGE):
    		actionCatalogLarge->trigger();
    		break;
    	}
    }

    // finally, the render config
    QDomElement rconfig = root.firstChildElement("Rendering");
    if (!vconfig.isNull()) {
    	actionWhite_background->setChecked(rconfig.attribute("clearToWhite").toInt());
		actionFree_aspect_ratio->setChecked(rconfig.attribute("freeAspectRatio").toInt());
	}

}


void GLMixer::on_actionAppend_Session_triggered(){

	QDomDocument doc;
    QString errorStr;
    int errorLine;
    int errorColumn;

    QString fileName = QDir::fromNativeSeparators(currentStageFileName);
    fileName.resize( fileName.lastIndexOf('/') );

    fileName = QFileDialog::getOpenFileName(this, tr("Append session file"), fileName, tr("GLMixer workspace (*.glm)"));
	if ( fileName.isEmpty() )
		return;

	QFile file(fileName);
    QString caption = tr("%1 session append").arg(QCoreApplication::applicationName());
	if ( !file.open(QFile::ReadOnly | QFile::Text) ) {
		QMessageBox::warning(this, caption, tr("Cannot read file %1:\n%2.").arg(fileName).arg(file.errorString()));
		return;
	}

    if ( !doc.setContent(&file, true, &errorStr, &errorLine, &errorColumn) ) {
		QMessageBox::warning(this, caption, tr("Parse error at line %1, column %2:\n%3").arg(errorLine).arg(errorColumn).arg(errorStr));
		return;
    }

    QDomElement root = doc.documentElement();
    if ( root.tagName() != "GLMixer" ) {
        QMessageBox::warning(this, caption,  tr("The file is not a GLMixer session file."));
        return;
    } else if ( root.hasAttribute("version") && root.attribute("version") != XML_GLM_VERSION ) {
        QMessageBox::warning(this, caption, tr("The version of this GLMixer session file is not %2.").arg(XML_GLM_VERSION));
        return;
    }

    // read the content of the source list to make sure the file is correct :
    QDomElement srcconfig = root.firstChildElement("SourceList");
    if ( srcconfig.isNull() )
    	return;

    // if we got up to here, it should be fine
    RenderingManager::getInstance()->addConfiguration(srcconfig);

    // confirm the loading of the file
	statusbar->showMessage( tr("Sources from %1 appended to %2.").arg( fileName ).arg( currentStageFileName ), 3000 );
}

void GLMixer::dragEnterEvent(QDragEnterEvent *event)
{
    event->acceptProposedAction();
}

void GLMixer::dragMoveEvent(QDragMoveEvent *event)
{
    event->acceptProposedAction();
}

void GLMixer::dropEvent(QDropEvent *event)
{
    const QMimeData *mimeData = event->mimeData();

	if (mimeData->hasUrls()) {
		QList<QUrl> urlList = mimeData->urls();
		QString text;
	    QString caption = tr("%1 create source").arg(QCoreApplication::applicationName());
		// arbitrary limitation in the amount of drops allowed
		if (urlList.size() > 30)
			QMessageBox::warning(this, caption, tr("Cannot drop more than 30 files at a time."));
		for (int i = 0; i < urlList.size() && i < 30; ++i) {
			QString filename = urlList.at(i).toLocalFile();

		    VideoFile *newSourceVideoFile  = new VideoFile(this);
		    Q_CHECK_PTR(newSourceVideoFile);

			// if the video file was created successfully
			if (newSourceVideoFile){
				// forward error messages to display
				QObject::connect(newSourceVideoFile, SIGNAL(error(QString)), this, SLOT(displayWarningMessage(QString)));
				QObject::connect(newSourceVideoFile, SIGNAL(info(QString)), this, SLOT(displayInfoMessage(QString)));
				// can we open the file ?
				if ( newSourceVideoFile->open( filename ) ) {
					Source *s = RenderingManager::getInstance()->newMediaSource(newSourceVideoFile);
					// create the source as it is a valid video file (this also set it to be the current source)
					if ( s ) {
						RenderingManager::getInstance()->addSourceToBasket(s);
					} else {
				        QMessageBox::warning(this, caption, tr("Could not create media source."));
				        delete newSourceVideoFile;
					}
				} else {
					displayInfoMessage ( tr("Could not open %1.").arg(filename));
				}
			}
		}
	}

    event->acceptProposedAction();
}

void GLMixer::dragLeaveEvent(QDragLeaveEvent *event)
{
    event->accept();
}


void GLMixer::readSettings(){

	// Main window config
    restoreGeometry(settings.value("geometry").toByteArray());
    restoreState(settings.value("windowState").toByteArray());

    // dialogs configs
    vcontrolOptionSplitter->restoreState(settings.value("vcontrolOptionSplitter").toByteArray());
    mfd->restoreState(settings.value("VideoFileDialog").toByteArray());
    OutputRenderWindow::getInstance()->restoreGeometry(settings.value("OutputRenderWindow").toByteArray());

    // preferences
    restorePreferences(settings.value("UserPreferences").toByteArray());

	// display time as frame
	actionShow_frames->setChecked(settings.value("DisplayTimeAsFrames").toBool());
}

void GLMixer::saveSettings(){

	// Main window config
    settings.setValue("geometry", saveGeometry());
    settings.setValue("windowState", saveState());
    settings.setValue("vcontrolOptionSplitter", vcontrolOptionSplitter->saveState());
    settings.setValue("VideoFileDialog", mfd->saveState());
    settings.setValue("OutputRenderWindow", (OutputRenderWindow::getInstance())->saveGeometry());

    // preferences
	settings.setValue("UserPreferences", getPreferences());

	// display time as frame
	settings.setValue("DisplayTimeAsFrames", actionShow_frames->isChecked());

	// make sure system saves settings NOW
    settings.sync();
}


void GLMixer::on_actionPreferences_triggered(){

	// popup a question dialog to select the type of algorithm
	static UserPreferencesDialog *upd = 0;
	if (!upd) {
		upd = new UserPreferencesDialog(this);
	}

	upd->showPreferences( getPreferences() );

	if (upd->exec() == QDialog::Accepted) {
		restorePreferences( upd->getUserPreferences() );
	}
}


bool GLMixer::restorePreferences(const QByteArray & state){

	if (state.isEmpty())
	        return false;

	QByteArray sd = state;
	QDataStream stream(&sd, QIODevice::ReadOnly);

	const quint32 magicNumber = 0x1D9D0CB;
    const quint16 currentMajorVersion = 2;
	quint32 storedMagicNumber;
    quint16 majorVersion = 0;
	stream >> storedMagicNumber >> majorVersion;
	if (storedMagicNumber != magicNumber || majorVersion != currentMajorVersion)
		return false;

	// a. Apply rendering preferences
	QSize RenderingSize;
	stream  >> RenderingSize;
	if (RenderingSize != QSize(0,0)) {
		RenderingManager::getInstance()->setFrameBufferResolution(RenderingSize);
		OutputRenderWindow::getInstance()->resizeGL();
	}
	bool useBlitFboExtension = false;
	stream >> useBlitFboExtension;
	RenderingManager::setUseFboBlitExtension(useBlitFboExtension);

	// b. Apply source preferences
	stream >> RenderingManager::getInstance()->defaultSource();

	// c.  DefaultScalingMode
	unsigned int sm = 0;
	stream >> sm;
	RenderingManager::getInstance()->setDefaultScalingMode( (Source::scalingMode) sm );

	// d. defaultStartPlaying
	bool defaultStartPlaying = false;
	stream >> defaultStartPlaying;
	RenderingManager::getInstance()->setDefaultPlayOnDrop(defaultStartPlaying);

	// e. PreviousFrameDelay
	unsigned int  PreviousFrameDelay = 1;
	stream >> PreviousFrameDelay;
	RenderingManager::getInstance()->setPreviousFrameDelay(PreviousFrameDelay);

	// f. Stippling mode
	int stipplingMode = 0;
	stream >> stipplingMode;
	RenderingManager::getRenderingWidget()->setStipplingMode(stipplingMode);

	return true;
}

QByteArray GLMixer::getPreferences() const {

    QByteArray data;
    QDataStream stream(&data, QIODevice::WriteOnly);
    const quint32 magicNumber = 0x1D9D0CB;
    quint16 majorVersion = 2;
	stream << magicNumber << majorVersion;

	// a. Store rendering preferences
	stream << RenderingManager::getInstance()->getFrameBufferResolution();
	stream << RenderingManager::getUseFboBlitExtension();

	// b. Store source preferences
	stream << RenderingManager::getInstance()->defaultSource();

	// c. DefaultScalingMode
	stream << (unsigned int) RenderingManager::getInstance()->getDefaultScalingMode();

	// d. defaultStartPlaying
	stream << RenderingManager::getInstance()->getDefaultPlayOnDrop();

	// e.  PreviousFrameDelay
	stream << RenderingManager::getInstance()->getPreviousFrameDelay();

	// f. Stippling mode
	stream << RenderingManager::getRenderingWidget()->getStipplingMode();

	return data;
}
