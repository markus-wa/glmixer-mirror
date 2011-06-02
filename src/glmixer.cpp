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


#include <QApplication>
#include <QDomDocument>
#include <QtGui>

#include "common.h"
#include "CameraDialog.h"
#include "VideoFileDialog.h"
#include "AlgorithmSelectionDialog.h"
#include "UserPreferencesDialog.h"
#include "ViewRenderWidget.h"
#include "RenderingManager.h"
#include "OutputRenderWindow.h"
#include "MixerView.h"
#include "RenderingSource.h"
#include "AlgorithmSource.h"
#include "VideoSource.h"
#ifdef OPEN_CV
#include "OpencvSource.h"
#endif
#include "VideoFileDisplayWidget.h"
#include "SourcePropertyBrowser.h"
#include "CloneSource.h"
#include "GammaLevelsWidget.h"
#include "SessionSwitcherWidget.h"
#include "CatalogView.h"
#include "DelayCursor.h"
#include "SpringCursor.h"
#include "AxisCursor.h"
#include "LineCursor.h"
#include "FuzzyCursor.h"
#include "RenderingEncoder.h"
#include "SessionSwitcher.h"

#include "glmixer.moc"


GLMixer::GLMixer ( QWidget *parent): QMainWindow ( parent ), selectedSourceVideoFile(NULL), refreshTimingTimer(0)
{
    setupUi ( this );
    setAcceptDrops ( true );
    errorMessageDialog = new QErrorMessage(this);

#ifndef OPEN_CV
    actionCameraSource->setEnabled(false);
#endif

    // add the show/hide menu items for the dock widgets
    menuToolBars->addAction(previewDockWidget->toggleViewAction());
    menuToolBars->addAction(sourceDockWidget->toggleViewAction());
    menuToolBars->addAction(vcontrolDockWidget->toggleViewAction());
    menuToolBars->addAction(cursorDockWidget->toggleViewAction());
    menuToolBars->addAction(gammaDockWidget->toggleViewAction());
    menuToolBars->addAction(switcherDockWidget->toggleViewAction());
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
	actionToolGrab->setData(ViewRenderWidget::TOOL_GRAB);
	toolActions->addAction(actionToolScale);
	actionToolScale->setData(ViewRenderWidget::TOOL_SCALE);
	toolActions->addAction(actionToolRotate);
	actionToolRotate->setData(ViewRenderWidget::TOOL_ROTATE);
	toolActions->addAction(actionToolCut);
	actionToolCut->setData(ViewRenderWidget::TOOL_CUT);
    QObject::connect(toolActions, SIGNAL(triggered(QAction *)), this, SLOT(setTool(QAction *) ) );

	QActionGroup *cursorActions = new QActionGroup(this);
	cursorActions->addAction(actionCursorNormal);
	actionCursorNormal->setData(ViewRenderWidget::CURSOR_NORMAL);
	cursorActions->addAction(actionCursorSpring);
	actionCursorSpring->setData(ViewRenderWidget::CURSOR_SPRING);
	cursorActions->addAction(actionCursorDelay);
	actionCursorDelay->setData(ViewRenderWidget::CURSOR_DELAY);
	cursorActions->addAction(actionCursorAxis);
	actionCursorAxis->setData(ViewRenderWidget::CURSOR_AXIS);
	cursorActions->addAction(actionCursorLine);
	actionCursorLine->setData(ViewRenderWidget::CURSOR_LINE);
	cursorActions->addAction(actionCursorFuzzy);
	actionCursorFuzzy->setData(ViewRenderWidget::CURSOR_FUZZY);
    QObject::connect(cursorActions, SIGNAL(triggered(QAction *)), this, SLOT(setCursor(QAction *) ) );

	QActionGroup *aspectRatioActions = new QActionGroup(this);
	aspectRatioActions->addAction(action4_3_aspect_ratio);
	action4_3_aspect_ratio->setData(ASPECT_RATIO_4_3);
	aspectRatioActions->addAction(action3_2_aspect_ratio);
	action3_2_aspect_ratio->setData(ASPECT_RATIO_3_2);
	aspectRatioActions->addAction(action16_10_aspect_ratio);
	action16_10_aspect_ratio->setData(ASPECT_RATIO_16_10);
	aspectRatioActions->addAction(action16_9_aspect_ratio);
	action16_9_aspect_ratio->setData(ASPECT_RATIO_16_9);
	aspectRatioActions->addAction(actionFree_aspect_ratio);
	actionFree_aspect_ratio->setData(ASPECT_RATIO_FREE);
    QObject::connect(aspectRatioActions, SIGNAL(triggered(QAction *)), this, SLOT(setAspectRatio(QAction *) ) );

    // recent files history
    QMenu *recentFiles = new QMenu(this);
    actionRecent_session->setMenu(recentFiles);
    for (int i = 0; i < MAX_RECENT_FILES; ++i) {
        recentFileActs[i] = new QAction(this);
        recentFileActs[i]->setVisible(false);
        connect(recentFileActs[i], SIGNAL(triggered()), this, SLOT(actionLoad_RecentSession_triggered()));
        recentFiles->addAction(recentFileActs[i]);
    }

    // Setup the central widget
    centralViewLayout->removeWidget(mainRendering);
	delete mainRendering;
	mainRendering = (QGLWidget *) RenderingManager::getRenderingWidget();
	mainRendering->setParent(centralwidget);
	centralViewLayout->addWidget(mainRendering);
	//activate the default view & share labels to display in
	setView(actionMixingView);
	RenderingManager::getRenderingWidget()->setLabels(zoomLabel, fpsLabel);
	// share menus as context menus of the main view
	RenderingManager::getRenderingWidget()->setViewContextMenu(menuZoom);
	RenderingManager::getRenderingWidget()->setCatalogContextMenu(menuCatalog);
	RenderingManager::getRenderingWidget()->setSourceContextMenu(menuCurrent_source);

	// Setup the property browser
	SourcePropertyBrowser *propertyBrowser = RenderingManager::getPropertyBrowserWidget();
	propertyBrowser->setParent(sourceDockWidgetContents);
	sourceDockWidgetContentsLayout->addWidget(propertyBrowser);
    QObject::connect(this, SIGNAL(sourceMarksModified(bool)), propertyBrowser, SLOT(updateMarksProperties(bool) ) );

	// Setup the gamma levels toolbox
	GammaLevelsWidget *gammaAdjust = new GammaLevelsWidget(this);
	gammaDockWidgetContentsLayout->addWidget(gammaAdjust);
	QObject::connect(RenderingManager::getInstance(), SIGNAL(currentSourceChanged(SourceSet::iterator)), gammaAdjust, SLOT(connectSource(SourceSet::iterator) ) );

	// Setup the session switcher toolbox
	SessionSwitcherWidget *switcherSession = new SessionSwitcherWidget(this, &settings);
	switcherDockWidgetContentsLayout->addWidget(switcherSession);
	QObject::connect(switcherSession, SIGNAL(sessionTriggered(QString)), this, SLOT(switchToSessionFile(QString)) );
	QObject::connect(this, SIGNAL(sessionSaved()), switcherSession, SLOT(updateFolder()) );
	QObject::connect(this, SIGNAL(sessionLoaded()), switcherSession, SLOT(unsuspend()));
	QObject::connect(OutputRenderWindow::getInstance(), SIGNAL(keyRightPressed()), switcherSession, SLOT(startTransitionToNextSession()));
	QObject::connect(OutputRenderWindow::getInstance(), SIGNAL(keyLeftPressed()), switcherSession, SLOT(startTransitionToPreviousSession()));
	QObject::connect(RenderingManager::getSessionSwitcher(), SIGNAL(transitionSourceChanged(Source *)), switcherSession, SLOT(setTransitionSourcePreview(Source *)));
	switcherSession->restoreSettings();

    // Setup dialogs
    mfd = new VideoFileDialog(this, "Open a video or a picture", QDir::currentPath());
    sfd = new QFileDialog(this);
    upd = new UserPreferencesDialog(this);

    // Create preview widget
    outputpreview = new OutputRenderWidget(previewDockWidgetContents, mainRendering);
	previewDockWidgetContentsLayout->insertWidget(0, outputpreview);

    // Default state without source selected
    vcontrolDockWidgetContents->setEnabled(false);
    sourceDockWidgetContents->setEnabled(false);

    // signals for source management with RenderingManager
    QObject::connect(RenderingManager::getInstance(), SIGNAL(currentSourceChanged(SourceSet::iterator)), this, SLOT(connectSource(SourceSet::iterator) ) );

	// QUIT event
    QObject::connect(actionQuit, SIGNAL(triggered()), this, SLOT(close()));
    QObject::connect(actionAbout_Qt, SIGNAL(triggered()), QApplication::instance(), SLOT(aboutQt()));

    // Rendering control
    QObject::connect(OutputRenderWindow::getInstance(), SIGNAL(toggleFullscreen(bool)), actionFullscreen, SLOT(setChecked(bool)) );

    QObject::connect(actionFullscreen, SIGNAL(toggled(bool)), OutputRenderWindow::getInstance(), SLOT(setFullScreen(bool)));
    QObject::connect(actionFullscreen, SIGNAL(toggled(bool)), RenderingManager::getInstance(), SLOT(disableProgressBars(bool)));
    QObject::connect(actionPause, SIGNAL(toggled(bool)), RenderingManager::getInstance(), SLOT(pause(bool)));
	QObject::connect(actionPause, SIGNAL(toggled(bool)), RenderingManager::getRenderingWidget(), SLOT(setFaded(bool)));
	QObject::connect(actionPause, SIGNAL(toggled(bool)), vcontrolDockWidget, SLOT(setDisabled(bool)));

	output_aspectratio->setMenu(menuAspect_Ratio);
	output_onair->setDefaultAction(actionToggleRenderingVisible);
	output_fullscreen->setDefaultAction(actionFullscreen);
	QObject::connect(actionToggleRenderingVisible, SIGNAL(toggled(bool)), RenderingManager::getInstance()->getSessionSwitcher(), SLOT(smoothAlphaTransition(bool)));
	QObject::connect(RenderingManager::getInstance()->getSessionSwitcher(), SIGNAL(alphaChanged(int)), output_alpha, SLOT(setValue(int)));

	// session switching
	QObject::connect(this, SIGNAL(sessionLoaded()), this, SLOT(confirmSessionFileName()));

	// Recording triggers
	QObject::connect(actionRecord, SIGNAL(toggled(bool)), RenderingManager::getRecorder(), SLOT(setActive(bool)));
	QObject::connect(RenderingManager::getRecorder(), SIGNAL(activated(bool)), actionRecord, SLOT(setChecked(bool)));
	QObject::connect(RenderingManager::getRecorder(), SIGNAL(status(const QString &, int)), statusbar , SLOT(showMessage(const QString &, int)));
	QObject::connect(actionRecord, SIGNAL(toggled(bool)), actionPause_recording, SLOT(setEnabled(bool)));
	QObject::connect(actionPause_recording, SIGNAL(toggled(bool)), actionRecord, SLOT(setDisabled(bool)));
	QObject::connect(actionPause_recording, SIGNAL(toggled(bool)), RenderingManager::getRecorder(), SLOT(setPaused(bool)));

	// connect to disable many actions, like quitting, opening session, preferences, etc.
	QObject::connect(RenderingManager::getRecorder(), SIGNAL(activated(bool)), actionNew_Session, SLOT(setDisabled(bool)));
	QObject::connect(RenderingManager::getRecorder(), SIGNAL(activated(bool)), actionLoad_Session, SLOT(setDisabled(bool)));
	QObject::connect(RenderingManager::getRecorder(), SIGNAL(activated(bool)), actionRecent_session, SLOT(setDisabled(bool)));
	QObject::connect(RenderingManager::getRecorder(), SIGNAL(activated(bool)), actionQuit, SLOT(setDisabled(bool)));
	QObject::connect(RenderingManager::getRecorder(), SIGNAL(activated(bool)), actionPreferences, SLOT(setDisabled(bool)));
	QObject::connect(RenderingManager::getRecorder(), SIGNAL(activated(bool)), menuAspect_Ratio, SLOT(setDisabled(bool)));
	QObject::connect(RenderingManager::getRecorder(), SIGNAL(activated(bool)), output_aspectratio, SLOT(setDisabled(bool)));

	// do not allow to record without a fixed aspect ratio
	QObject::connect(actionFree_aspect_ratio, SIGNAL(toggled(bool)), actionRecord, SLOT(setDisabled(bool)));
	QObject::connect(RenderingManager::getRecorder(), SIGNAL(selectAspectRatio(const standardAspectRatio )), switcherSession, SLOT(setAllowedAspectRatio(const standardAspectRatio)));

	// group the menu items of the catalog sizes ;
	QActionGroup *catalogActionGroup = new QActionGroup(this);
	catalogActionGroup->addAction(actionCatalogSmall);
	catalogActionGroup->addAction(actionCatalogMedium);
	catalogActionGroup->addAction(actionCatalogLarge);
	QObject::connect(actionCatalogSmall, SIGNAL(triggered()), RenderingManager::getRenderingWidget(), SLOT(setCatalogSizeSmall()));
	QObject::connect(actionCatalogMedium, SIGNAL(triggered()), RenderingManager::getRenderingWidget(), SLOT(setCatalogSizeMedium()));
	QObject::connect(actionCatalogLarge, SIGNAL(triggered()), RenderingManager::getRenderingWidget(), SLOT(setCatalogSizeLarge()));

	// Signals between GUI and rendering widget
	QObject::connect(actionShow_Catalog, SIGNAL(toggled(bool)), RenderingManager::getRenderingWidget(), SLOT(setCatalogVisible(bool)));
	QObject::connect(actionWhite_background, SIGNAL(toggled(bool)), RenderingManager::getInstance(), SLOT(setClearToWhite(bool)));
	QObject::connect(actionZoomIn, SIGNAL(triggered()), RenderingManager::getRenderingWidget(), SLOT(zoomIn()));
	QObject::connect(actionZoomOut, SIGNAL(triggered()), RenderingManager::getRenderingWidget(), SLOT(zoomOut()));
	QObject::connect(actionZoomReset, SIGNAL(triggered()), RenderingManager::getRenderingWidget(), SLOT(zoomReset()));
	QObject::connect(actionZoomBestFit, SIGNAL(triggered()), RenderingManager::getRenderingWidget(), SLOT(zoomBestFit()));
	QObject::connect(actionZoomCurrentSource, SIGNAL(triggered()), RenderingManager::getRenderingWidget(), SLOT(zoomCurrentSource()));

	QObject::connect(actionToggle_fixed, SIGNAL(triggered()), RenderingManager::getInstance(), SLOT(toggleMofifiableCurrentSource()));
	QObject::connect(actionResetSource, SIGNAL(triggered()), RenderingManager::getInstance(), SLOT(resetCurrentSource()));

	// Signals between cursors and their configuration gui
	QObject::connect(dynamic_cast<LineCursor*>(RenderingManager::getRenderingWidget()->getCursor(ViewRenderWidget::CURSOR_LINE)), SIGNAL(speedChanged(int)), cursorLineSpeed, SLOT(setValue(int)) );
	QObject::connect(cursorLineSpeed, SIGNAL(valueChanged(int)), dynamic_cast<LineCursor*>(RenderingManager::getRenderingWidget()->getCursor(ViewRenderWidget::CURSOR_LINE)), SLOT(setSpeed(int)) );
	QObject::connect(cursorLineWaitDuration, SIGNAL(valueChanged(double)), dynamic_cast<LineCursor*>(RenderingManager::getRenderingWidget()->getCursor(ViewRenderWidget::CURSOR_LINE)), SLOT(setWaitTime(double)) );
	QObject::connect(dynamic_cast<DelayCursor*>(RenderingManager::getRenderingWidget()->getCursor(ViewRenderWidget::CURSOR_DELAY)), SIGNAL(latencyChanged(double)), cursorDelayLatency, SLOT(setValue(double)) );
	QObject::connect(cursorDelayLatency, SIGNAL(valueChanged(double)), dynamic_cast<DelayCursor*>(RenderingManager::getRenderingWidget()->getCursor(ViewRenderWidget::CURSOR_DELAY)), SLOT(setLatency(double)) );
	QObject::connect(cursorDelayFiltering, SIGNAL(valueChanged(int)), dynamic_cast<DelayCursor*>(RenderingManager::getRenderingWidget()->getCursor(ViewRenderWidget::CURSOR_DELAY)), SLOT(setFiltering(int)) );
	QObject::connect(dynamic_cast<SpringCursor*>(RenderingManager::getRenderingWidget()->getCursor(ViewRenderWidget::CURSOR_SPRING)), SIGNAL(massChanged(int)), cursorSpringMass, SLOT(setValue(int)) );
	QObject::connect(cursorSpringMass, SIGNAL(valueChanged(int)), dynamic_cast<SpringCursor*>(RenderingManager::getRenderingWidget()->getCursor(ViewRenderWidget::CURSOR_SPRING)), SLOT(setMass(int)) );
	QObject::connect(dynamic_cast<FuzzyCursor*>(RenderingManager::getRenderingWidget()->getCursor(ViewRenderWidget::CURSOR_FUZZY)), SIGNAL(radiusChanged(int)), cursorFuzzyRadius, SLOT(setValue(int)) );
	QObject::connect(cursorFuzzyRadius, SIGNAL(valueChanged(int)), dynamic_cast<FuzzyCursor*>(RenderingManager::getRenderingWidget()->getCursor(ViewRenderWidget::CURSOR_FUZZY)), SLOT(setRadius(int)) );
	QObject::connect(cursorFuzzyFiltering, SIGNAL(valueChanged(int)), dynamic_cast<FuzzyCursor*>(RenderingManager::getRenderingWidget()->getCursor(ViewRenderWidget::CURSOR_FUZZY)), SLOT(setFiltering(int)) );

    // a Timer to update sliders and counters
    refreshTimingTimer = new QTimer(this);
    Q_CHECK_PTR(refreshTimingTimer);
    refreshTimingTimer->setInterval(150);
    QObject::connect(refreshTimingTimer, SIGNAL(timeout()), this, SLOT(refreshTiming()));
    QObject::connect(vcontrolDockWidget, SIGNAL(visibilityChanged(bool)), this, SLOT(updateRefreshTimerState()));

    // recall settings from last time
    readSettings();

    // start with new file
    currentSessionFileName = QString();
    confirmSessionFileName();
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

    if (selectedSourceVideoFile && /*!selectedSourceVideoFile->isPaused() &&*/ selectedSourceVideoFile->isRunning() && vcontrolDockWidget->isVisible())
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

	viewIcon->setPixmap(RenderingManager::getRenderingWidget()->getView()->getIcon());
	viewLabel->setText(RenderingManager::getRenderingWidget()->getView()->getTitle());

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

	RenderingManager::getRenderingWidget()->setToolMode( (ViewRenderWidget::toolMode) a->data().toInt() );
}

void GLMixer::setCursor(QAction *a){

	RenderingManager::getRenderingWidget()->setCursorMode( (ViewRenderWidget::cursorMode) a->data().toInt() );
	cursorOptionWidget->setCurrentIndex(a->data().toInt());
}

void GLMixer::on_actionMediaSource_triggered(){

	QStringList fileNames;

#ifndef NO_VIDEO_FILE_DIALOG_PREVIEW

	if (mfd->exec())
		fileNames = mfd->selectedFiles();

#else
	fileNames = QFileDialog::getOpenFileNames(this, tr("Open File"),
													QDir::currentPath(),
													tr("Video (*.mov *.avi *.wmv *.mpeg *.mp4 *.mpg *.vob *.swf *.flv *.mod);;Image (*.png *.jpg *.jpeg *.tif *.tiff *.gif *.tga *.sgi *.bmp)"));

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
		if ( glSupportsExtension("GL_EXT_texture_non_power_of_two") || glSupportsExtension("GL_ARB_texture_non_power_of_two")  )
			newSourceVideoFile = new VideoFile(this);
		else
			newSourceVideoFile = new VideoFile(this, true, SWS_POINT);
#endif
	    Q_CHECK_PTR(newSourceVideoFile);

	    QString caption = tr("%1 Cannot create source").arg(QCoreApplication::applicationName());
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
				displayInfoMessage (tr("The file %1 could not be loaded.").arg(filename));
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

        QObject::disconnect(selectedSourceVideoFile, SIGNAL(running(bool)), startButton, SLOT(setChecked(bool)));
        QObject::disconnect(selectedSourceVideoFile, SIGNAL(paused(bool)), pauseButton, SLOT(setChecked(bool)));
        QObject::disconnect(selectedSourceVideoFile, SIGNAL(running(bool)), videoFrame, SLOT(setEnabled(bool)));
        QObject::disconnect(selectedSourceVideoFile, SIGNAL(running(bool)), timingControlFrame, SLOT(setEnabled(bool)));
        QObject::disconnect(selectedSourceVideoFile, SIGNAL(markingChanged()), this, SLOT(updateMarks()));
        QObject::disconnect(selectedSourceVideoFile, SIGNAL(running(bool)), this, SLOT(updateRefreshTimerState()));

		// disconnect control buttons
        QObject::disconnect(pauseButton, SIGNAL(toggled(bool)), selectedSourceVideoFile, SLOT(pause(bool)));
        QObject::disconnect(playSpeedSlider, SIGNAL(valueChanged(int)), selectedSourceVideoFile, SLOT(setPlaySpeed(int)));
        QObject::disconnect(dirtySeekCheckBox, SIGNAL(toggled(bool)), selectedSourceVideoFile, SLOT(setOptionAllowDirtySeek(bool)));
        QObject::disconnect(resetToBlackCheckBox, SIGNAL(toggled(bool)), selectedSourceVideoFile, SLOT(setOptionRevertToBlackWhenStop(bool)));
        QObject::disconnect(restartWhereStoppedCheckBox, SIGNAL(toggled(bool)), selectedSourceVideoFile, SLOT(setOptionRestartToMarkIn(bool)));
        QObject::disconnect(seekBackwardButton, SIGNAL(clicked()), selectedSourceVideoFile, SLOT(seekBackward()));
        QObject::disconnect(seekForwardButton, SIGNAL(clicked()), selectedSourceVideoFile, SLOT(seekForward()));
        QObject::disconnect(seekBeginButton, SIGNAL(clicked()), selectedSourceVideoFile, SLOT(seekBegin()));
        QObject::disconnect(seekBackwardButton, SIGNAL(clicked()), this, SLOT(unpauseBeforeSeek()));
        QObject::disconnect(seekForwardButton, SIGNAL(clicked()), this,  SLOT(unpauseBeforeSeek()));
        QObject::disconnect(seekBeginButton, SIGNAL(clicked()), this, SLOT(unpauseBeforeSeek()));
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

		// enable cursor on a source clic
		RenderingManager::getRenderingWidget()->setCursorEnabled(true);

		// enable properties and actions on the current valid source
		sourceDockWidgetContents->setEnabled(true);
		menuCurrent_source->setEnabled(true);
		toolButtonZoomCurrent->setEnabled(true);

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
					playSpeedSlider->setValue(selectedSourceVideoFile->getPlaySpeed());

		            // CONTROL signals from GUI to VideoFile
		            QObject::connect(pauseButton, SIGNAL(toggled(bool)), selectedSourceVideoFile, SLOT(pause(bool)));
		            QObject::connect(playSpeedSlider, SIGNAL(valueChanged(int)), selectedSourceVideoFile, SLOT(setPlaySpeed(int)));
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
		            QObject::connect(selectedSourceVideoFile, SIGNAL(running(bool)), startButton, SLOT(setChecked(bool)));
		            QObject::connect(selectedSourceVideoFile, SIGNAL(paused(bool)), pauseButton, SLOT(setChecked(bool)));
		            QObject::connect(selectedSourceVideoFile, SIGNAL(running(bool)), videoFrame, SLOT(setEnabled(bool)));
		            QObject::connect(selectedSourceVideoFile, SIGNAL(running(bool)), timingControlFrame, SLOT(setEnabled(bool)));
		            QObject::connect(selectedSourceVideoFile, SIGNAL(error(QString)), this, SLOT(displayWarningMessage(QString)));
//		            QObject::connect(selectedSourceVideoFile, SIGNAL(info(QString)), this, SLOT(displayInfoMessage(QString)));

		            // Consistency and update timer control from VideoFile
		            QObject::connect(selectedSourceVideoFile, SIGNAL(markingChanged()), this, SLOT(updateMarks()));
		            QObject::connect(selectedSourceVideoFile, SIGNAL(running(bool)), this, SLOT(updateRefreshTimerState()));

					// display info (time or frames)
					on_actionShow_frames_toggled(actionShow_frames->isChecked());

		        } // end video file
			} // end video source
		} // end playable
	} else {  // it is not a valid source

		// disable cursor on a source clic
		RenderingManager::getRenderingWidget()->setCursorEnabled(false);

		// disable panel widgets
		menuCurrent_source->setEnabled(false);
		toolButtonZoomCurrent->setEnabled(false);
		vcontrolDockWidgetContents->setEnabled(false);
		startButton->setEnabled( false );
		startButton->setChecked( false );

		sourceDockWidgetContents->setEnabled(false);
	}

	// update gui content from timings
	refreshTiming();
	updateMarks();
	// restart slider timer if necessary
	updateRefreshTimerState();

}

void GLMixer::on_actionCameraSource_triggered() {

	CameraDialog cd(this);

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
			        QMessageBox::warning(this, tr("%1 Cannot create source").arg(QCoreApplication::applicationName()), tr("Could not create clone source."));
			}
			//else create a new opencv source :
			else {
				Source *s = RenderingManager::getInstance()->newOpencvSource(cd.indexOpencvCamera());
				if ( s ) {
					RenderingManager::getInstance()->addSourceToBasket(s);
					statusbar->showMessage( tr("Source created with OpenCV drivers for Camera %1").arg(cd.indexOpencvCamera()), 3000 );
				} else
			        QMessageBox::warning(this, tr("%1 Cannot create source").arg(QCoreApplication::applicationName()), tr("Could not create OpenCV source."));
			}
		}
#endif

	}

}


void GLMixer::on_actionAlgorithmSource_triggered(){

	// popup a question dialog to select the type of algorithm
	static AlgorithmSelectionDialog *asd = 0;
	if (!asd)
		asd = new AlgorithmSelectionDialog(this);

	if (asd->exec() == QDialog::Accepted) {
		Source *s = RenderingManager::getInstance()->newAlgorithmSource(asd->getSelectedAlgorithmIndex(),
					asd->getSelectedWidth(), asd->getSelectedHeight(), asd->getSelectedVariability(), asd->getUpdatePeriod());
		if ( s ){
			RenderingManager::getInstance()->addSourceToBasket(s);
			statusbar->showMessage( tr("Source created with the algorithm %1.").arg( AlgorithmSource::getAlgorithmDescription(asd->getSelectedAlgorithmIndex())), 3000 );
		} else
	        QMessageBox::warning(this, tr("%1 Cannot create source").arg(QCoreApplication::applicationName()), tr("Could not create algorithm source."));
	}
}


void GLMixer::on_actionRenderingSource_triggered(){

	Source *s = RenderingManager::getInstance()->newRenderingSource();
	if ( s ){
		RenderingManager::getInstance()->addSourceToBasket(s);
		statusbar->showMessage( tr("Source created with the rendering output loopback."), 3000 );
	}else
        QMessageBox::warning(this, tr("%1 Cannot create source").arg(QCoreApplication::applicationName()), tr("Could not create rendering source."));
}


void GLMixer::on_actionCloneSource_triggered(){

	if ( RenderingManager::getInstance()->notAtEnd(RenderingManager::getInstance()->getCurrentSource()) ) {
		Source *s = RenderingManager::getInstance()->newCloneSource( RenderingManager::getInstance()->getCurrentSource());
		if ( s ) {
			RenderingManager::getInstance()->addSourceToBasket(s);
			statusbar->showMessage( tr("The current source has been cloned."), 3000);
		} else
	        QMessageBox::warning(this, tr("%1 Cannot create source").arg(QCoreApplication::applicationName()), tr("Could not create clone source."));
	}
}


CaptureDialog::CaptureDialog(QWidget *parent, QImage capture, QString caption) : QDialog(parent), img(capture) {

	QVBoxLayout *verticalLayout;
	QLabel *Question, *Display;
	QDialogButtonBox *DecisionButtonBox;
	setObjectName(QString::fromUtf8("CaptureDialog"));
	resize(300, 179);
	setWindowTitle(tr( "Frame captured"));
	verticalLayout = new QVBoxLayout(this);
	verticalLayout->setObjectName(QString::fromUtf8("verticalLayout"));
	Question = new QLabel(this);
	Question->setObjectName(QString::fromUtf8("Question"));
	QSizePolicy sizePolicy(QSizePolicy::Preferred, QSizePolicy::Maximum);
	sizePolicy.setHorizontalStretch(0);
	sizePolicy.setVerticalStretch(0);
	sizePolicy.setHeightForWidth(Question->sizePolicy().hasHeightForWidth());
	Question->setSizePolicy(sizePolicy);
	Question->setText(caption);
	verticalLayout->addWidget(Question);

	Display = new QLabel(this);
	Display->setObjectName(QString::fromUtf8("Display"));
	Display->setPixmap(QPixmap::fromImage(img).scaledToWidth(300));
	verticalLayout->addWidget(Display);

	DecisionButtonBox = new QDialogButtonBox(this);
	DecisionButtonBox->setObjectName(QString::fromUtf8("DecisionButtonBox"));
	DecisionButtonBox->setStandardButtons(QDialogButtonBox::Cancel|QDialogButtonBox::Ok);

	verticalLayout->addWidget(DecisionButtonBox);
	QObject::connect(DecisionButtonBox, SIGNAL(accepted()), this, SLOT(accept()));
	QObject::connect(DecisionButtonBox, SIGNAL(rejected()), this, SLOT(reject()));
}


QString CaptureDialog::saveImage()
{
	// keep a static instance of location and base file name
	static QDir dname(QDir::currentPath());
	static QString basename("capture");

	// try to suggest an incremented file name
	QFileInfo fname = QFileInfo( dname, basename + ".png");
	for (int i = 1; fname.exists() && i<1000; i++)
		fname = QFileInfo( dname, QString("%1_%2.png").arg(basename).arg(i));

	// ask for file name
	QString filename = QFileDialog::getSaveFileName ( parentWidget(), tr("Save captured image"), fname.absoluteFilePath(), tr("Images (*.png *.xpm *.jpg *.jpeg *.tiff)"), 0,  QFileDialog::DontUseNativeDialog);

	// save the file
	if (!filename.isEmpty()) {
		if (!img.save(filename)) {
			qCritical("** Error **\n\nCould not save file %s.", qPrintable(filename));
			return tr("not");
		}
		// remember location and base file name for next time
		fname = QFileInfo( filename );
		dname = fname.dir();
		basename =  fname.baseName().section("_", 0, fname.baseName().count("_")-1);

		return filename;
	}
	// return "not" so that the caller can use it in a sentence "File *not* saved"...
	return tr("not");
}


void GLMixer::on_actionCaptureSource_triggered(){

	// capture screen
	QImage capture = RenderingManager::getInstance()->captureFrameBuffer();
	capture = capture.convertToFormat(QImage::Format_RGB32);

	// display and request action with this capture
	CaptureDialog cd(this, capture, tr("Create a source with this image ?"));

	if (cd.exec() == QDialog::Accepted) {
		QString filename = cd.saveImage();

		if ( QFileInfo(filename).exists() ){
			VideoFile *newSourceVideoFile = new VideoFile(this);
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
						qCritical("** Error **\n\nCould not create media source with %s.", qPrintable(filename));
						delete newSourceVideoFile;
					}
				} else {
					qCritical("** Error **\n\nCould not load %s.", qPrintable(filename));
					delete newSourceVideoFile;
				}
			}
		}
	}
}


void GLMixer::on_actionDeleteSource_triggered()
{
	// lisst of sources to delete
	SourceList todelete;

	// if the current source is valid, add it todelete
	SourceSet::iterator cs = RenderingManager::getInstance()->getCurrentSource();
	if ( RenderingManager::getInstance()->isValid(cs) ) {
		// if the current source is in the selection, delete the whole selection
		if ( View::selection().size() > 0  && View::selection().count(*cs) > 0 ) {
			// make a copy of the selection (to make sure we do not mess with pointers)
			todelete = SourceList(View::selection());
		}
		else
			// else delete only the current
			todelete.insert(*cs);
	}

	// remove all the source in the list todelete
	for(SourceList::iterator  its = todelete.begin(); its != todelete.end(); its++) {
		// test for clones of this source
		int numclones = (*its)->getClones()->size();
		// popup a question dialog 'are u sure' if there are clones attached;
		if ( numclones ){
			QString msg = tr("This source was cloned %1 times; Do you want to delete all the clones too?").arg(numclones);
			if ( QMessageBox::question(this," Are you sure?", msg, QMessageBox::Ok | QMessageBox::Cancel, QMessageBox::Ok) == QMessageBox::Ok)
				numclones = 0;
		}

		if ( !numclones ){
			QString d = (*its)->getName();
			RenderingManager::getInstance()->removeSource((*its)->getId());
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

	// reset to zero if no video file
	if (!selectedSourceVideoFile) {
		frameSlider->setValue(0);
		timeLineEdit->setText( "" );
		return;
	}

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
//    QObject::connect(selectedSourceVideoFile, SIGNAL(frameReady(int)), this, SLOT(pauseAfterFrame()));
    QObject::connect(selectedSourceVideoFile, SIGNAL(frameReady(int)), this, SLOT(pauseAfterSeek()));

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
    QObject::connect(selectedSourceVideoFile, SIGNAL(frameReady(int)), this, SLOT(pauseAfterSeek()));

    // let the VideoFile run till it displays 1 frame
    selectedSourceVideoFile->pause(false);

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

	// if the button 'Pause' is checked, we shall go back to pause once
	// we'll have displayed the seeked frame
	if (pauseButton->isChecked()) {
		selectedSourceVideoFile->pause(true);
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

	if (!vcontrolDockWidget->isVisible())
		return;

	int i_percent = 0;
	int o_percent = 1000;

	if (selectedSourceVideoFile){
		// adjust the marking sliders according to the source marks in and out
		i_percent = (int) ( (double)( selectedSourceVideoFile->getMarkIn() - selectedSourceVideoFile->getBegin() ) / (double)( selectedSourceVideoFile->getEnd() - selectedSourceVideoFile->getBegin() ) * 1000.0) ;
		o_percent = (int) ( (double)( selectedSourceVideoFile->getMarkOut() - selectedSourceVideoFile->getBegin() ) / (double)( selectedSourceVideoFile->getEnd() - selectedSourceVideoFile->getBegin() ) * 1000.0) ;
	}

    markInSlider->setValue(i_percent);
    markOutSlider->setValue(o_percent);

    // update property for marks in / out
    emit sourceMarksModified(actionShow_frames->isChecked());
}


void GLMixer::on_actionShowFPS_toggled(bool on){

	RenderingManager::getRenderingWidget()->showFramerate(on);
}


void GLMixer::setAspectRatio(QAction *a)
{
	standardAspectRatio ar = (standardAspectRatio) a->data().toInt();

	RenderingManager::getInstance()->setRenderingAspectRatio(ar);

	// apply config; this also refreshes the rendering areas
	// if none of the above, the FREE aspect ratio was requested
	if (ar == ASPECT_RATIO_FREE) {
		OutputRenderWindow::getInstance()->useFreeAspectRatio(true);
		outputpreview->useFreeAspectRatio(true);
		QObject::connect(OutputRenderWindow::getInstance(), SIGNAL(resized()), outputpreview, SLOT(refresh()));
		QObject::connect(OutputRenderWindow::getInstance(), SIGNAL(resized()), RenderingManager::getRenderingWidget(), SLOT(refresh()));
	}
	// otherwise, disable the free aspect ratio
	else {
		OutputRenderWindow::getInstance()->useFreeAspectRatio(false);
		outputpreview->useFreeAspectRatio(false);
		QObject::disconnect(OutputRenderWindow::getInstance(), SIGNAL(resized()), outputpreview, SLOT(refresh()));
		QObject::disconnect(OutputRenderWindow::getInstance(), SIGNAL(resized()), RenderingManager::getRenderingWidget(), SLOT(refresh()));
	}

	RenderingManager::getRenderingWidget()->refresh();
}

void setupAboutDialog(QDialog *AboutGLMixer)
{
	AboutGLMixer->resize(420, 270);
	QGridLayout *gridLayout = new QGridLayout(AboutGLMixer);
	gridLayout->setObjectName(QString::fromUtf8("gridLayout"));
	QLabel *Icon = new QLabel(AboutGLMixer);
	Icon->setPixmap(QPixmap(QString::fromUtf8(":/glmixer/icons/glmixer.png")));
	QLabel *Title = new QLabel(AboutGLMixer);
	Title->setStyleSheet(QString::fromUtf8("font: 14pt \"Sans Serif\";"));
	QLabel *VERSION = new QLabel(AboutGLMixer);
	VERSION->setStyleSheet(QString::fromUtf8("font: 14pt \"Sans Serif\";"));
	QLabel *textsvn = new QLabel(AboutGLMixer);
	QLabel *SVN = new QLabel(AboutGLMixer);
	QTextBrowser *textBrowser = new QTextBrowser(AboutGLMixer);
	textBrowser->setOpenExternalLinks (true);
	QDialogButtonBox *validate = new QDialogButtonBox(AboutGLMixer);
	validate->setOrientation(Qt::Horizontal);
	validate->setStandardButtons(QDialogButtonBox::Close);

	gridLayout->addWidget(Icon, 0, 0, 1, 1);
	gridLayout->addWidget(Title, 0, 1, 1, 1);
	gridLayout->addWidget(VERSION, 0, 2, 1, 1);
	gridLayout->addWidget(textsvn, 1, 1, 1, 1);
	gridLayout->addWidget(SVN, 1, 2, 1, 1);
	gridLayout->addWidget(textBrowser, 2, 0, 1, 3);
	gridLayout->addWidget(validate, 3, 0, 1, 3);

	Icon->setText(QString());
	Title->setText(QApplication::translate("AboutGLMixer", "Graphic Live Mixer", 0, QApplication::UnicodeUTF8));
	textBrowser->setHtml(QApplication::translate("AboutGLMixer", "<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 4.0//EN\" \"http://www.w3.org/TR/REC-html40/strict.dtd\">\n"
	"<html><head><meta name=\"qrichtext\" content=\"1\" /><style type=\"text/css\">\n"
	"p, li { white-space: pre-wrap; }\n"
	"</style></head><body style=\" font-family:'Sans Serif'; font-size:10pt; font-weight:400; font-style:normal;\">\n"
	"<p>GLMixer is a video mixing software for live performance.</p>\n"
	"<p>Author:	Bruno Herbelin<br>\n"
	"Contact:	bruno.herbelin@gmail.com<br>\n"
	"License: 	GNU GPL version 3</p>\n"
	"<p>Copyright 2009-2011 Bruno Herbelin</p>\n"
	"<p>Updates and source code at: <br>\n"
	"   	<a href=\"http://code.google.com/p/glmixer/\"><span style=\" text-decoration: underline; color:#7d400a;\">http://code.google.com/p/glmixer/</span>"
	"</a></p>"
	"<p>GLMixer is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation.</p>"
	"<p>GLMixer is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more details (see http://www.gnu.org/licenses).</p>"
	"</body></html>", 0, QApplication::UnicodeUTF8));

	VERSION->setText( QString("%1").arg(QCoreApplication::applicationVersion()) );
	
#ifdef GLMIXER_REVISION
	SVN->setText(QString("%1").arg(GLMIXER_REVISION));
	textsvn->setText(QApplication::translate("AboutGLMixer", "SVN repository revision:", 0, QApplication::UnicodeUTF8));
#endif
	
	QObject::connect(validate, SIGNAL(accepted()), AboutGLMixer, SLOT(accept()));
	QObject::connect(validate, SIGNAL(rejected()), AboutGLMixer, SLOT(reject()));

}

void GLMixer::on_actionAbout_triggered(){

	QDialog *aboutglm = new QDialog(this);
	setupAboutDialog(aboutglm);

	aboutglm->exec();
}


void GLMixer::confirmSessionFileName(){

	// recent files history
	QStringList files = settings.value("recentFileList").toStringList();

	if (currentSessionFileName.isNull()) {
		setWindowTitle(QString("%1 %2 - unsaved").arg(QCoreApplication::applicationName()).arg(QCoreApplication::applicationVersion()));
		actionAppend_Session->setEnabled(false);
	} else {
		// title and menu
		setWindowTitle(QString("%1 %2 - %3").arg(QCoreApplication::applicationName()).arg(QCoreApplication::applicationVersion()).arg(QFileInfo(currentSessionFileName).fileName()));
		actionAppend_Session->setEnabled(true);

		files.removeAll(currentSessionFileName);
		files.prepend(currentSessionFileName);
		while (files.size() > MAX_RECENT_FILES)
			files.removeLast();
	}

	for (int i = 0; i < files.size(); ++i) {
		 QString text = tr("&%1 - %2").arg(i + 1).arg(QFileInfo(files[i]).fileName());
		 recentFileActs[i]->setText(text);
		 recentFileActs[i]->setData(files[i]);
		 recentFileActs[i]->setVisible(true);
	}
	for (int j = files.size(); j < MAX_RECENT_FILES; ++j)
		recentFileActs[j]->setVisible(false);

	settings.setValue("recentFileList", files);

	// message
	statusbar->showMessage( tr("Session file %1 loaded.").arg( currentSessionFileName ), 5000 );

}


void GLMixer::on_actionNew_Session_triggered()
{
// TODO : implement good mechanism to know if something was changed
//	// inform the user that data might be lost
//	int ret = QMessageBox::Discard;
//	if (!currentStageFileName.isNull()) {
//		 QMessageBox msgBox;
//		 msgBox.setText("The session may have been modified.");
//		 msgBox.setInformativeText("Do you want to save your changes?");
//		 msgBox.setIconPixmap( QPixmap(QString::fromUtf8(":/glmixer/icons/question.png")) );
//		 msgBox.setStandardButtons(QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel);
//		 msgBox.setDefaultButton(QMessageBox::Save);
//		 ret = msgBox.exec();
//	}
//	// react according to user's answer
//	switch (ret) {
//	   case QMessageBox::Save:
//		   // Save was clicked
//		   on_actionSave_Session_triggered();
//		   break;
//	   case QMessageBox::Cancel:
//		   // Cancel was clicked
//		   return;
//	   case QMessageBox::Discard:
//	   default:
//		   // keep on to create new session
//		   break;
//	}

	// make a new session
	currentSessionFileName = QString();
	confirmSessionFileName();

	// trigger newSession after the smooth transition to black is finished (action is disabled meanwhile)
	actionToggleRenderingVisible->setEnabled(false);
	QObject::connect(RenderingManager::getSessionSwitcher(), SIGNAL(animationFinished()), this, SLOT(newSession()) );
	RenderingManager::getSessionSwitcher()->startTransition(false);
}


void GLMixer::newSession()
{
	// if coming from animation, disconnect it.
	QObject::disconnect(RenderingManager::getSessionSwitcher(), SIGNAL(animationFinished()), this, SLOT(newSession()) );
	actionToggleRenderingVisible->setEnabled(true);
	RenderingManager::getSessionSwitcher()->startTransition(true);

	// reset
	RenderingManager::getInstance()->clearSourceSet();
	actionWhite_background->setChecked(false);
	RenderingManager::getRenderingWidget()->clearViews();

	// refreshes the rendering areas
	outputpreview->refresh();
	// reset
	on_gammaShiftReset_clicked();

}


void GLMixer::on_actionSave_Session_triggered(){

	if (currentSessionFileName.isNull())
		on_actionSave_Session_as_triggered();
	else {

		QFile file(currentSessionFileName);
		if (!file.open(QFile::WriteOnly | QFile::Text) ) {
			QMessageBox::warning(this, tr("%1 session save").arg(QCoreApplication::applicationName()), tr("Cannot write file %1:\n%2.").arg(currentSessionFileName).arg(file.errorString()));
			return;
		}
		QTextStream out(&file);

		QDomDocument doc;
		QDomProcessingInstruction instr = doc.createProcessingInstruction("xml", "version='1.0' encoding='UTF-8'");
		doc.appendChild(instr);

		QDomElement root = doc.createElement("GLMixer");
		root.setAttribute("version", XML_GLM_VERSION);

		QDomElement renderConfig = RenderingManager::getInstance()->getConfiguration(doc, QFileInfo(currentSessionFileName).canonicalPath());
		renderConfig.setAttribute("aspectRatio", (int) RenderingManager::getInstance()->getRenderingAspectRatio());
		renderConfig.setAttribute("gammaShift", RenderingManager::getInstance()->getGammaShift());
		root.appendChild(renderConfig);

		QDomElement viewConfig =  RenderingManager::getRenderingWidget()->getConfiguration(doc);
		root.appendChild(viewConfig);

		QDomElement rendering = doc.createElement("Rendering");
		rendering.setAttribute("clearToWhite", (int) RenderingManager::getInstance()->clearToWhite());
		root.appendChild(rendering);

		doc.appendChild(root);
		doc.save(out, 4);

	    file.close();

		confirmSessionFileName();
		statusbar->showMessage( tr("File %1 saved.").arg( currentSessionFileName ), 3000 );
		emit sessionSaved();
	}
}

void GLMixer::on_actionSave_Session_as_triggered()
{
	sfd->setAcceptMode(QFileDialog::AcceptSave);
	sfd->setFileMode(QFileDialog::AnyFile);
	sfd->setFilter(tr("GLMixer workspace (*.glm)"));
	sfd->setDefaultSuffix("glm");

	if (sfd->exec()) {
	    QString fileName = sfd->selectedFiles().front();
		// now we got a filename, save the file:
		currentSessionFileName = fileName;
		on_actionSave_Session_triggered();
	}
}

void GLMixer::on_actionLoad_Session_triggered()
{
	sfd->setAcceptMode(QFileDialog::AcceptOpen);
	sfd->setFileMode(QFileDialog::ExistingFile);
	sfd->setFilter(tr("GLMixer workspace (*.glm)"));

	if (sfd->exec()) {
		// get the first file name selected
		switchToSessionFile( sfd->selectedFiles().front() );
	}
}


void GLMixer::actionLoad_RecentSession_triggered()
{
	QAction *action = qobject_cast<QAction *>(sender());
	if (action)
		switchToSessionFile(action->data().toString());

}


void GLMixer::switchToSessionFile(QString filename){

	if (filename.isEmpty()) {
		newSession();
	} else {
		// setup filename
		currentSessionFileName = filename;

		// no need for fade off transition if nothing loaded
		if (RenderingManager::getInstance()->empty())
			openSessionFile();
		else {
			// trigger openSessionFile after the smooth transition to black is finished (action is disabled meanwhile)
			actionToggleRenderingVisible->setEnabled(false);
			QObject::connect(RenderingManager::getSessionSwitcher(), SIGNAL(animationFinished()), this, SLOT(openSessionFile()) );
			RenderingManager::getSessionSwitcher()->startTransition(false);
		}
	}
}

void GLMixer::openSessionFile(QString filename)
{
	// unpause if it was
	actionPause->setChecked ( false );

	// if we come from the smooth transition, disconnect the signal and enforce session switcher to show transition
	QObject::disconnect(RenderingManager::getSessionSwitcher(), SIGNAL(animationFinished()), this, SLOT(openSessionFile()) );
	RenderingManager::getSessionSwitcher()->setOverlay(1.0);
	actionToggleRenderingVisible->setEnabled(true);

	// in case the argument is valid, use it
	if (!filename.isNull())
		currentSessionFileName = filename;

	// Ok, ready to load XML ?
	QDomDocument doc;
    QString errorStr;
    int errorLine;
    int errorColumn;

    // open file
	QFile file(currentSessionFileName);
	QString caption = tr("%1 Cannot open session").arg(QCoreApplication::applicationName());
	if (!file.open(QFile::ReadOnly | QFile::Text)) {
		QMessageBox::warning(this, caption, tr("Cannot open file %1:\n\n%2.").arg(currentSessionFileName).arg(file.errorString()));
		currentSessionFileName = QString();
		return;
	}
	// load content
    if (!doc.setContent(&file, true, &errorStr, &errorLine, &errorColumn)) {
        QMessageBox::warning(this, caption, tr("Problem reading %1.\n\nParse error at line %2, column %3:\n%4").arg(currentSessionFileName).arg(errorLine).arg(errorColumn).arg(errorStr));
    	currentSessionFileName = QString();
    	return;
    }
    // close file
    file.close();
    // verify it is a GLM file
    QDomElement root = doc.documentElement();
    if (root.tagName() != "GLMixer") {
        QMessageBox::warning(this, caption, tr("The file %1 is not a valid GLMixer session file.").arg(currentSessionFileName));
    	currentSessionFileName = QString();
        return;
    } else if (root.hasAttribute("version") && root.attribute("version") != XML_GLM_VERSION) {
        QMessageBox::warning(this, caption, tr("Problem loading %1\n\nThe version of the file is not compatible (%2 instead of %3).\nI will try to do what I can...\n").arg(currentSessionFileName).arg(root.attribute("version")).arg(XML_GLM_VERSION));
    }
	// if we got up to here, it should be fine ; reset for a new session and apply loaded configurations
	RenderingManager::getInstance()->clearSourceSet();
    // read the source list and its configuration
    QDomElement renderConfig = root.firstChildElement("SourceList");
    if (renderConfig.isNull())
        QMessageBox::warning(this, caption, tr("The file %1 is empty.").arg(currentSessionFileName));
    else {
    	standardAspectRatio ar = (standardAspectRatio) renderConfig.attribute("aspectRatio", "0").toInt();
    	switch(ar) {
    	case ASPECT_RATIO_FREE:
    		actionFree_aspect_ratio->trigger();
    		break;
    	case ASPECT_RATIO_16_10:
    		action16_10_aspect_ratio->trigger();
    		break;
    	case ASPECT_RATIO_16_9:
    		action16_9_aspect_ratio->trigger();
    		break;
    	case ASPECT_RATIO_3_2:
    		action3_2_aspect_ratio->trigger();
    		break;
    	default:
    	case ASPECT_RATIO_4_3:
    		action4_3_aspect_ratio->trigger();
    	}
    	float g = renderConfig.attribute("gammaShift", "1").toFloat();
    	gammaShiftSlider->setValue(GammaToSlider(g));
    	gammaShiftText->setText( QString().setNum( g, 'f', 2) );
    	RenderingManager::getInstance()->setGammaShift(g);
    	// read the list of sources
		RenderingManager::getInstance()->addConfiguration(renderConfig, QFileInfo(currentSessionFileName).canonicalPath());
    }
    // read the views configuration
	RenderingManager::getRenderingWidget()->clearViews();
    QDomElement vconfig = root.firstChildElement("Views");
    if (!vconfig.isNull()){
    	// apply the views configuration
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
    // finally, read the rendering configuration
    QDomElement rconfig = root.firstChildElement("Rendering");
    if (!rconfig.isNull()) {
    	actionWhite_background->setChecked(rconfig.attribute("clearToWhite").toInt());
	}

    // broadcast that the session is loaded
	emit sessionLoaded();

	// start the smooth transition
    RenderingManager::getSessionSwitcher()->startTransition(true);
}


void GLMixer::on_actionAppend_Session_triggered(){

	QDomDocument doc;
    QString errorStr;
    int errorLine;
    int errorColumn;

    QString fileName = QFileDialog::getOpenFileName(this, tr("Append session file"), QFileInfo(currentSessionFileName).absolutePath(), tr("GLMixer workspace (*.glm)"), 0,  QFileDialog::DontUseNativeDialog);
	if ( fileName.isEmpty() )
		return;

	QFile file(fileName);
    QString caption = tr("Cannot append %1 to current session").arg(QCoreApplication::applicationName());
	if ( !file.open(QFile::ReadOnly | QFile::Text) ) {
		QMessageBox::warning(this, caption, tr("Cannot read file %1:\n%2.").arg(fileName).arg(file.errorString()));
		return;
	}

    if ( !doc.setContent(&file, true, &errorStr, &errorLine, &errorColumn) ) {
		QMessageBox::warning(this, caption, tr("Parse error at line %1, column %2:\n%3").arg(errorLine).arg(errorColumn).arg(errorStr));
		return;
    }

    file.close();

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
    RenderingManager::getInstance()->addConfiguration(srcconfig, QFileInfo(currentSessionFileName).canonicalPath());

    // confirm the loading of the file
	statusbar->showMessage( tr("Sources from %1 appended to %2.").arg( fileName ).arg( currentSessionFileName ), 3000 );
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
	QStringList mediaFiles;
	QString glmfile;

    // browse the list of urls dropped
	if (mimeData->hasUrls()) {
		QList<QUrl> urlList = mimeData->urls();
		QString text;
	    QString caption = tr("%1 Cannot create source").arg(QCoreApplication::applicationName());

		// arbitrary limitation in the amount of drops allowed (avoid manipulation mistakes)
		if (urlList.size() > 20)
			errorMessageDialog->showMessage( tr("Cannot open more than 20 files at a time."));

		for (int i = 0; i < urlList.size() && i < 20; ++i) {
			QFileInfo urlname(urlList.at(i).toLocalFile());

			if ( urlname.suffix() == "glm") {
				if (glmfile.isNull())
					glmfile = urlname.absoluteFilePath();
				else {
					errorMessageDialog->showMessage( tr("Cannot open more than one .glm session file."));
					break;
				}
			}
			else //  maybe a video ?
				mediaFiles.append(urlname.absoluteFilePath());
		}
	}

	if (!glmfile.isNull()) {
		currentSessionFileName = glmfile;

		if (RenderingManager::getInstance()->empty())
			openSessionFile();
		else {
			// trigger openSessionFile after the smooth transition to black is finished (action is disabled meanwhile)
			actionToggleRenderingVisible->setEnabled(false);
			QObject::connect(RenderingManager::getSessionSwitcher(), SIGNAL(animationFinished()), this, SLOT(openSessionFile()) );
			RenderingManager::getSessionSwitcher()->startTransition(false);
		}

		if (!mediaFiles.isEmpty())
			errorMessageDialog->showMessage( tr("Loading only the .glm session file; discarding the other files provided."));

	} else {

		QProgressDialog progress("Loading sources...", "Abort", 1, mediaFiles.size());
		progress.setWindowModality(Qt::WindowModal);
		progress.setMinimumDuration( 600 );
		for (int i = 0; i < mediaFiles.size(); ++i)
		{
			progress.setValue(i);
			if (progress.wasCanceled())
				break;

			VideoFile *newSourceVideoFile  = new VideoFile(this);
			Q_CHECK_PTR(newSourceVideoFile);

			// if the video file was created successfully
			if (newSourceVideoFile){
				// forward error messages to display
				QObject::connect(newSourceVideoFile, SIGNAL(error(QString)), this, SLOT(displayWarningMessage(QString)));
				QObject::connect(newSourceVideoFile, SIGNAL(info(QString)), this, SLOT(displayInfoMessage(QString)));
				// can we open the file ?
				if ( newSourceVideoFile->open( mediaFiles.at(i) ) ) {
					Source *s = RenderingManager::getInstance()->newMediaSource(newSourceVideoFile);
					// create the source as it is a valid video file (this also set it to be the current source)
					if ( s ) {
						RenderingManager::getInstance()->addSourceToBasket(s);
					} else {
						displayInfoMessage ( tr("Could not create media source."));
						delete newSourceVideoFile;
					}
				} else {
					displayInfoMessage ( tr("Could not open %1.").arg(mediaFiles.at(i)));
					delete newSourceVideoFile;
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


void GLMixer::readSettings()
{
	// windows config
    if (settings.contains("geometry"))
    	restoreGeometry(settings.value("geometry").toByteArray());
    else
    	settings.setValue("defaultGeometry", saveGeometry());

    if (settings.contains("windowState"))
    	restoreState(settings.value("windowState").toByteArray());
    else
        settings.setValue("defaultWindowState", saveState());

    if (settings.contains("OutputRenderWindow"))
    	OutputRenderWindow::getInstance()->restoreGeometry(settings.value("OutputRenderWindow").toByteArray());
    // dialogs configs
    if (settings.contains("vcontrolOptionSplitter"))
    	vcontrolOptionSplitter->restoreState(settings.value("vcontrolOptionSplitter").toByteArray());
    if (settings.contains("VideoFileDialog"))
    	mfd->restoreState(settings.value("VideoFileDialog").toByteArray());
    if (settings.contains("SessionFileDialog"))
    	sfd->restoreState(settings.value("SessionFileDialog").toByteArray());
    if (settings.contains("RenderingEncoder"))
    	RenderingManager::getRecorder()->restoreState(settings.value("RenderingEncoder").toByteArray());
	// boolean options
    if (settings.contains("DisplayTimeAsFrames"))
    	actionShow_frames->setChecked(settings.value("DisplayTimeAsFrames").toBool());
    if (settings.contains("DisplayFramerate"))
    	actionShowFPS->setChecked(settings.value("DisplayFramerate").toBool());
    // preferences
	restorePreferences(settings.value("UserPreferences").toByteArray());

}

void GLMixer::saveSettings()
{
	// windows config
    settings.setValue("geometry", saveGeometry());
    settings.setValue("windowState", saveState());
    settings.setValue("OutputRenderWindow", (OutputRenderWindow::getInstance())->saveGeometry());
    // dialogs configs
    settings.setValue("vcontrolOptionSplitter", vcontrolOptionSplitter->saveState());
    settings.setValue("VideoFileDialog", mfd->saveState());
    settings.setValue("SessionFileDialog", sfd->saveState());
    settings.setValue("RenderingEncoder", RenderingManager::getRecorder()->saveState());
	// boolean options
	settings.setValue("DisplayTimeAsFrames", actionShow_frames->isChecked());
	settings.setValue("DisplayFramerate", actionShowFPS->isChecked());
    // preferences
	settings.setValue("UserPreferences", getPreferences());
	// make sure system saves settings NOW
    settings.sync();
}


void GLMixer::on_actionResetToolbars_triggered()
{
	restoreGeometry(settings.value("defaultGeometry").toByteArray());
	restoreState(settings.value("defaultWindowState").toByteArray());
	restoreDockWidget(previewDockWidget);
	restoreDockWidget(sourceDockWidget);
	restoreDockWidget(vcontrolDockWidget);
	restoreDockWidget(cursorDockWidget);
	restoreDockWidget(gammaDockWidget);
	restoreDockWidget(switcherDockWidget);

}

void GLMixer::on_actionPreferences_triggered()
{
	// fill in the saved preferences
	upd->showPreferences( getPreferences() );

	// show the dialog and apply preferences if it was accepted
	if (upd->exec() == QDialog::Accepted)
		restorePreferences( upd->getUserPreferences() );

}


void GLMixer::restorePreferences(const QByteArray & state){

	// no preference?
    if (state.isEmpty()) {

    	// set dialog in minimal mode
    	upd->setModeMinimal(true);

    	// show the dialog and apply preferences
    	upd->exec();
		restorePreferences( upd->getUserPreferences() );

		upd->setModeMinimal(false);
		return;
    }

	QByteArray sd = state;
	QDataStream stream(&sd, QIODevice::ReadOnly);

	const quint32 magicNumber = MAGIC_NUMBER;
    const quint16 currentMajorVersion = QSETTING_PREFERENCE_VERSION;
	quint32 storedMagicNumber;
    quint16 majorVersion = 0;
	stream >> storedMagicNumber >> majorVersion;
	if (storedMagicNumber != magicNumber || majorVersion != currentMajorVersion) {
		// display dialog warning error
		qCritical("** Error **\n\nCould not load preferences.");
		return;
	}

	// a. Apply rendering preferences
	uint RenderingQuality;
	bool useBlitFboExtension = true;
	stream >> RenderingQuality >> useBlitFboExtension;
	RenderingManager::setUseFboBlitExtension(useBlitFboExtension);
	RenderingManager::getInstance()->setRenderingQuality((frameBufferQuality) RenderingQuality);
	int targetPeriod = 20;
	stream >> targetPeriod;
	if (targetPeriod > 0)
		glRenderWidget::setUpdatePeriod( targetPeriod );

	// b. Apply source preferences
	stream >> RenderingManager::getInstance()->defaultSource();

	// c.  DefaultScalingMode
	uint sm = 0;
	stream >> sm;
	RenderingManager::getInstance()->setDefaultScalingMode( (Source::scalingMode) sm );

	// d. defaultStartPlaying
	bool defaultStartPlaying = false;
	stream >> defaultStartPlaying;
	RenderingManager::getInstance()->setDefaultPlayOnDrop(defaultStartPlaying);

	// e. PreviousFrameDelay
	uint  PreviousFrameDelay = 1;
	stream >> PreviousFrameDelay;
	RenderingManager::getInstance()->setPreviousFrameDelay(PreviousFrameDelay);

	// f. Stippling mode
	uint stipplingMode = 0;
	stream >> stipplingMode;
	ViewRenderWidget::setStipplingMode(stipplingMode);

	// g. recording format
	uint recformat = 0;
	stream >> recformat;
	RenderingManager::getRecorder()->setEncodingFormat( (encodingformat) recformat);
	uint rtfr = 40;
	stream >> rtfr;
	RenderingManager::getRecorder()->setUpdatePeriod(rtfr > 0 ? rtfr : 40);

	// h. recording folder
	bool automaticSave = false;
	stream >> automaticSave;
	RenderingManager::getRecorder()->setAutomaticSavingMode(automaticSave);
	QString automaticSaveFolder;
	stream >> automaticSaveFolder;
	QDir d(automaticSaveFolder);
	if ( !d.exists())
		d = QDir::current();
	RenderingManager::getRecorder()->setAutomaticSavingFolder(d);

	// i. disable filtering
	bool disablefilter = false;
	stream >> disablefilter;
	// better make the view render widget current before setting filtering enabled
	RenderingManager::getRenderingWidget()->refresh();
	RenderingManager::getRenderingWidget()->setFilteringEnabled(!disablefilter);

	// j. antialiasing
	bool antialiasing = true;
	stream >> antialiasing;
	RenderingManager::getRenderingWidget()->setAntiAliasing(antialiasing);

	// Refresh widgets to make changes visible
	OutputRenderWindow::getInstance()->refresh();
	outputpreview->refresh();
	// de-select current source
	RenderingManager::getInstance()->setCurrentSource(-1);
}

QByteArray GLMixer::getPreferences() const {

    QByteArray data;
    QDataStream stream(&data, QIODevice::WriteOnly);
    const quint32 magicNumber = MAGIC_NUMBER;
    const quint16 majorVersion = QSETTING_PREFERENCE_VERSION;
	stream << magicNumber << majorVersion;

	// a. Store rendering preferences
	stream << (uint) RenderingManager::getInstance()->getRenderingQuality();
	stream << RenderingManager::getUseFboBlitExtension();
	stream << glRenderWidget::updatePeriod();

	// b. Store source preferences
	stream << RenderingManager::getInstance()->defaultSource();

	// c. DefaultScalingMode
	stream << (uint) RenderingManager::getInstance()->getDefaultScalingMode();

	// d. defaultStartPlaying
	stream << RenderingManager::getInstance()->getDefaultPlayOnDrop();

	// e.  PreviousFrameDelay
	stream << RenderingManager::getInstance()->getPreviousFrameDelay();

	// f. Stippling mode
	stream << ViewRenderWidget::getStipplingMode();

	// g. recording format
	stream << (uint) RenderingManager::getRecorder()->encodingFormat();
	stream << RenderingManager::getRecorder()->updatePeriod();

	// h. recording folder
	stream << RenderingManager::getRecorder()->automaticSavingMode();
	stream << RenderingManager::getRecorder()->automaticSavingFolder().absolutePath();

	// i. filtering
	stream << !ViewRenderWidget::filteringEnabled();

	// j. antialiasing
	stream << RenderingManager::getRenderingWidget()->antiAliasing();

	return data;
}


void GLMixer::on_gammaShiftSlider_valueChanged(int val)
{
	float g = SliderToGamma(val);
	gammaShiftText->setText( QString().setNum( g, 'f', 2) );
	RenderingManager::getInstance()->setGammaShift(g);
}

void GLMixer::on_gammaShiftReset_clicked()
{
	gammaShiftSlider->setValue( 470 );
	RenderingManager::getInstance()->setGammaShift(1.0);
}


void GLMixer::on_controlOptionsButton_clicked()
{
	QList<int> splitSizes = vcontrolOptionSplitter->sizes();

	if (splitSizes.last() == 0) {
		splitSizes[0] -= 140;
		splitSizes[1] = 140;
	} else {
		splitSizes[0] += splitSizes[0];
		splitSizes[1] = 0;
	}
	vcontrolOptionSplitter->setSizes(splitSizes);
}


void GLMixer::on_actionSave_snapshot_triggered(){

	// capture screen
	QImage capture = RenderingManager::getInstance()->captureFrameBuffer();
	capture = capture.convertToFormat(QImage::Format_RGB32);

	// display and request action with this capture
	CaptureDialog cd(this, capture, tr("Save this image ?"));

	if (cd.exec() == QDialog::Accepted) {
		QString filename = cd.saveImage();
		displayInfoMessage(tr("File %1 saved.").arg(filename));
	}
}


void GLMixer::on_output_alpha_valueChanged(int v){

	static int previous_v = 0;

	if (v == 100 || previous_v == 100) {
		QObject::disconnect(actionToggleRenderingVisible, SIGNAL(toggled(bool)), RenderingManager::getInstance()->getSessionSwitcher(), SLOT(smoothAlphaTransition(bool)));

		if(previous_v < 100)
			actionToggleRenderingVisible->setChecked(false);
		else
			actionToggleRenderingVisible->setChecked(true);

		QObject::connect(actionToggleRenderingVisible, SIGNAL(toggled(bool)), RenderingManager::getInstance()->getSessionSwitcher(), SLOT(smoothAlphaTransition(bool)));
	}

	RenderingManager::getInstance()->getSessionSwitcher()->setAlpha(v);

	previous_v = v;
}



