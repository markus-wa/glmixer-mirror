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
 *   Copyright 2009, 2012 Bruno Herbelin
 *
 */

#include <iostream>

#include <QApplication>
#include <QDomDocument>
#include <QtGui>

#include "common.h"
#include "CameraDialog.h"
#include "VideoFileDialog.h"
#include "AlgorithmSelectionDialog.h"
#include "SharedMemoryDialog.h"
#include "SharedMemoryManager.h"
#include "UserPreferencesDialog.h"
#include "ViewRenderWidget.h"
#include "RenderingManager.h"
#include "OutputRenderWindow.h"
#include "MixerView.h"
#include "RenderingSource.h"
#include "AlgorithmSource.h"
#include "VideoSource.h"
#include "SvgSource.h"
#include "SharedMemorySource.h"
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

GLMixer *GLMixer::_instance = 0;

GLMixer *GLMixer::getInstance() {

	if (_instance == 0) {
		_instance = new GLMixer;
		Q_CHECK_PTR(_instance);
	}

	return _instance;
}

GLMixer::GLMixer ( QWidget *parent): QMainWindow ( parent ), selectedSourceVideoFile(NULL), usesystemdialogs(false), maybeSave(true), refreshTimingTimer(0)
{
    setupUi ( this );
    setAcceptDrops ( true );

#ifndef OPEN_CV
    actionCameraSource->setVisible(false);
#endif

    // add the show/hide menu items for the dock widgets
    toolBarsMenu->addAction(previewDockWidget->toggleViewAction());
    toolBarsMenu->addAction(sourceDockWidget->toggleViewAction());
    toolBarsMenu->addAction(vcontrolDockWidget->toggleViewAction());
    toolBarsMenu->addAction(cursorDockWidget->toggleViewAction());
    toolBarsMenu->addAction(gammaDockWidget->toggleViewAction());
    toolBarsMenu->addAction(switcherDockWidget->toggleViewAction());
    toolBarsMenu->addAction(alignDockWidget->toggleViewAction());
    toolBarsMenu->addAction(logDockWidget->toggleViewAction());
    toolBarsMenu->addSeparator();
    toolBarsMenu->addAction(sourceToolBar->toggleViewAction());
    toolBarsMenu->addAction(viewToolBar->toggleViewAction());
    toolBarsMenu->addAction(fileToolBar->toggleViewAction());
    toolBarsMenu->addAction(toolsToolBar->toggleViewAction());

	QActionGroup *viewActions = new QActionGroup(this);
    Q_CHECK_PTR(viewActions);
    viewActions->addAction(actionMixingView);
    viewActions->addAction(actionGeometryView);
    viewActions->addAction(actionLayersView);
    QObject::connect(viewActions, SIGNAL(triggered(QAction *)), this, SLOT(setView(QAction *) ) );

	QActionGroup *toolActions = new QActionGroup(this);
    Q_CHECK_PTR(toolActions);
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
    Q_CHECK_PTR(cursorActions);
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
    Q_CHECK_PTR(aspectRatioActions);
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

    QAction *nextSession = new QAction("Next Session", this);
    nextSession->setShortcut(tr("Ctrl+Right"));
    addAction(nextSession);
    QAction *prevSession = new QAction("Previous Session", this);
    prevSession->setShortcut(tr("Ctrl+Left"));
    addAction(prevSession);


    // recent files history
    QMenu *recentFiles = new QMenu(this);
    Q_CHECK_PTR(recentFiles);
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
	RenderingManager::getRenderingWidget()->setViewContextMenu(zoomMenu);
	RenderingManager::getRenderingWidget()->setCatalogContextMenu(catalogMenu);
	RenderingManager::getRenderingWidget()->setSourceContextMenu(currentSourceMenu);

	// Setup the property browser
	SourcePropertyBrowser *propertyBrowser = RenderingManager::getPropertyBrowserWidget();
	propertyBrowser->setParent(sourceDockWidgetContents);
	sourceDockWidgetContentsLayout->addWidget(propertyBrowser);
    QObject::connect(this, SIGNAL(sourceMarksModified(bool)), propertyBrowser, SLOT(updateMarksProperties(bool) ) );
    QObject::connect(propertyBrowser, SIGNAL(changed(Source*)), this, SLOT(sourceChanged(Source*) ) );

	// Setup the gamma levels toolbox
	gammaAdjust = new GammaLevelsWidget(this);
    Q_CHECK_PTR(gammaAdjust);
	gammaDockWidgetContentsLayout->addWidget(gammaAdjust);
	QObject::connect(RenderingManager::getInstance(), SIGNAL(currentSourceChanged(SourceSet::iterator)), gammaAdjust, SLOT(connectSource(SourceSet::iterator) ) );

	// Setup the session switcher toolbox
	SessionSwitcherWidget *switcherSession = new SessionSwitcherWidget(this, &settings);
	switcherDockWidgetContentsLayout->addWidget(switcherSession);
	QObject::connect(switcherSession, SIGNAL(sessionTriggered(QString)), this, SLOT(switchToSessionFile(QString)) );
	QObject::connect(this, SIGNAL(sessionSaved()), switcherSession, SLOT(updateFolder()) );
	QObject::connect(this, SIGNAL(sessionLoaded()), switcherSession, SLOT(unsuspend()));
	QObject::connect(RenderingManager::getSessionSwitcher(), SIGNAL(transitionSourceChanged(Source *)), switcherSession, SLOT(setTransitionSourcePreview(Source *)));
	switcherSession->restoreSettings();

	QObject::connect(nextSession, SIGNAL(triggered()), switcherSession, SLOT(startTransitionToNextSession()));
	QObject::connect(prevSession, SIGNAL(triggered()), switcherSession, SLOT(startTransitionToPreviousSession()));


	setDockOptions(dockOptions () | QMainWindow::VerticalTabs);

	// setup render window
    QIcon icon;
    icon.addFile(QString::fromUtf8(":/glmixer/icons/glmixer.png"), QSize(), QIcon::Normal, QIcon::Off);
	OutputRenderWindow::getInstance()->setWindowIcon(icon);
	QObject::connect(OutputRenderWindow::getInstance(), SIGNAL(keyRightPressed()), switcherSession, SLOT(startTransitionToNextSession()));
	QObject::connect(OutputRenderWindow::getInstance(), SIGNAL(keyLeftPressed()), switcherSession, SLOT(startTransitionToPreviousSession()));

    // Setup dialogs
    mfd = new VideoFileDialog(this, "Open a video or a picture", QDir::currentPath());
    Q_CHECK_PTR(mfd);
    sfd = new QFileDialog(this);
    Q_CHECK_PTR(sfd);
    upd = new UserPreferencesDialog(this);
    Q_CHECK_PTR(upd);

    // Create preview widget
    outputpreview = new OutputRenderWidget(previewDockWidgetContents, mainRendering);
	previewDockWidgetContentsLayout->insertWidget(0, outputpreview);

    // Default state without source selected
    vcontrolDockWidgetContents->setEnabled(false);
    sourceDockWidgetContents->setEnabled(false);

    // signals for source management with RenderingManager
    QObject::connect(RenderingManager::getInstance(), SIGNAL(currentSourceChanged(SourceSet::iterator)), this, SLOT(connectSource(SourceSet::iterator) ) );
    QObject::connect(startButton, SIGNAL(toggled(bool)), RenderingManager::getInstance(), SLOT(startCurrentSource(bool)));

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
	QObject::connect(actionShareToRAM, SIGNAL(toggled(bool)), RenderingManager::getInstance(), SLOT(setFrameSharingEnabled(bool)));

	output_aspectratio->setMenu(aspectRatioMenu);
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
	QObject::connect(RenderingManager::getRecorder(), SIGNAL(activated(bool)), aspectRatioMenu, SLOT(setDisabled(bool)));
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

    // start with new file
    currentSessionFileName = QString();
    confirmSessionFileName();
}

GLMixer::~GLMixer()
{
	delete mfd;
    delete sfd;
    delete upd;
    delete refreshTimingTimer;
    delete gammaAdjust;
}


void GLMixer::exitHandler() {

	_instance->saveSettings();

	// no message handling when quit
	qInstallMsgHandler(0);

	RenderingManager::deleteInstance();
	OutputRenderWindow::deleteInstance();
	SharedMemoryManager::deleteInstance();

	if (_instance)
		delete _instance;

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

void GLMixer::on_copyLogsToClipboard_clicked() {

	if (logTexts->topLevelItemCount() > 0) {
		QString logs;
		QTreeWidgetItemIterator it(logTexts->topLevelItem(0));
		while (*it) {
			logs.append( QString("%1:%2\n").arg((*it)->text(0)).arg((*it)->text(1)) );
			++it;
		}
		QApplication::clipboard()->setText(logs);
	}
}

void GLMixer::msgHandler(QtMsgType type, const char *msg)
{
	QString txt = QString(msg).remove("\"");

	// message handler
	switch (type) {
	case QtFatalMsg:
		QMessageBox::critical(0, tr("%1 -- Fatal error").arg(QCoreApplication::applicationName()), txt);
		abort();
		break;
	default:
		break;
	}

	// forward message to logger
	if (_instance) {
		// invoke a delayed call (in Qt event loop) of the GLMixer real Message handler SLOT
		static int methodIndex = _instance->metaObject()->indexOfSlot("Log(int,QString)");
		static QMetaMethod method = _instance->metaObject()->method(methodIndex);
		method.invoke(_instance, Qt::QueuedConnection, Q_ARG(int, (int)type), Q_ARG(QString, txt));
	}
}

void GLMixer::Log(int type, QString msg)
{
	// create log entry
	QTreeWidgetItem *item  = new QTreeWidgetItem();
	logTexts->addTopLevelItem( item );

	// reads the text passed and split into object|message
	QStringList message = msg.split('|', QString::SkipEmptyParts);
	if (message.count() > 1 ) {
		item->setText(0, message[1]);
		item->setText(1, message[0]);
	} else {
		item->setText(0, message[0]);
		item->setIcon(0, QIcon(":/glmixer/icons/info.png"));
	}

	// adjust color and show dialog according to message type
	switch ( (QtMsgType) type) {
	case QtWarningMsg:
		 item->setBackgroundColor(0, QColor(220, 180, 50, 50));
		 item->setBackgroundColor(1, QColor(220, 180, 50, 50));
		 item->setIcon(0, QIcon(":/glmixer/icons/warning.png"));
		 break;
	case QtCriticalMsg:
		item->setBackgroundColor(0, QColor(220, 90, 50, 50));
		item->setBackgroundColor(1, QColor(220, 90, 50, 50));
		item->setIcon(0, QIcon(":/glmixer/icons/warning.png"));
		QMessageBox::warning(0, tr("%1 -- Problem").arg(QCoreApplication::applicationName()), QString(msg).replace("|","\n") +
														QObject::tr("\n\nPlease check the logs for details.") );
		break;
	default:
		break;
	}

	// auto scroll to new item
	logTexts->setCurrentItem( item );
    logTexts->setColumnWidth(0, logTexts->width() * 70 / 100);
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
													tr("Video (*.mov *.avi *.wmv *.mpeg *.mp4 *.mpg *.vob *.swf *.flv *.mod);;Image (*.png *.jpg *.jpeg *.tif *.tiff *.gif *.tga *.sgi *.bmp)");

	d.setPath(fileNames.first());
#endif

	QStringListIterator fileNamesIt(fileNames);
	while (fileNamesIt.hasNext()){

	    VideoFile *newSourceVideoFile = NULL;
	    QString filename = fileNamesIt.next();

#ifndef NO_VIDEO_FILE_DIALOG_PREVIEW
		if ( !mfd->configCustomSize() && (glSupportsExtension("GL_EXT_texture_non_power_of_two") || glSupportsExtension("GL_ARB_texture_non_power_of_two") ) )
#else
		if ( glSupportsExtension("GL_EXT_texture_non_power_of_two") || glSupportsExtension("GL_ARB_texture_non_power_of_two")  )
#endif
			newSourceVideoFile = new VideoFile(this);
		else
			newSourceVideoFile = new VideoFile(this, true, SWS_POINT);


	    QString caption = tr("%1 Cannot create source").arg(QCoreApplication::applicationName());
		// if the video file was created successfully
		if (newSourceVideoFile){
			// can we open the file ?
			if ( newSourceVideoFile->open( filename ) ) {
				Source *s = RenderingManager::getInstance()->newMediaSource(newSourceVideoFile);
				// create the source as it is a valid video file (this also set it to be the current source)
				if ( s ) {
					RenderingManager::getInstance()->addSourceToBasket(s);
					qDebug() << s->getName() << tr("|New media source created with file ") << filename;
				} else {
			        qCritical() << filename << tr("|Could not create media source with this file.");
			        delete newSourceVideoFile;
				}
			} else {
				qCritical() << filename << tr("|The file could not be loaded.");
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

		// enable properties and actions on the current valid source
		sourceDockWidgetContents->setEnabled(true);
		currentSourceMenu->setEnabled(true);
		toolButtonZoomCurrent->setEnabled(true);

		// Enable start button if the source is playable
		startButton->setEnabled( (*csi)->isPlayable() );
		vcontrolDockWidgetContents->setEnabled( (*csi)->isPlayable() );
		vcontrolDockWidgetControls->setEnabled( (*csi)->isPlayable() );

		// except for media source, these pannels are disabled
		vcontrolDockWidgetOptions->setEnabled(false);
		videoFrame->setEnabled(false);
		timingControlFrame->setEnabled(false);

		// Set the status of start button without circular call to startCurrentSource
	    QObject::disconnect(startButton, SIGNAL(toggled(bool)), RenderingManager::getInstance(), SLOT(startCurrentSource(bool)));
		startButton->setChecked( (*csi)->isPlaying() );
		QObject::connect(startButton, SIGNAL(toggled(bool)), RenderingManager::getInstance(), SLOT(startCurrentSource(bool)));

		// Among playable sources, there is the particular case of video sources :
		if ((*csi)->isPlayable() && (*csi)->rtti() == Source::VIDEO_SOURCE ) {
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

				// Consistency and update timer control from VideoFile
				QObject::connect(selectedSourceVideoFile, SIGNAL(markingChanged()), this, SLOT(updateMarks()));
				QObject::connect(selectedSourceVideoFile, SIGNAL(running(bool)), this, SLOT(updateRefreshTimerState()));

				// display info (time or frames)
				on_actionShow_frames_toggled(actionShow_frames->isChecked());

			} // end video file
		} // end video source
	} else {  // it is not a valid source

		// disable panel widgets
		currentSourceMenu->setEnabled(false);
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
	// update the status (enabled / disabled) of source control actions
	updateStatusControlActions();

}


void GLMixer::sourceChanged(Source *s) {

	if (s)
		maybeSave = true;

}

void GLMixer::on_actionCameraSource_triggered() {

#ifdef OPEN_CV
	static CameraDialog *cd = 0;
	if (!cd)
		cd = new CameraDialog(this);

	if (cd->exec() == QDialog::Accepted) {
		int selectedCamIndex = cd->indexOpencvCamera();
		if (selectedCamIndex > -1 ) {

			Source *s = RenderingManager::getInstance()->newOpencvSource(selectedCamIndex);
			if ( s ) {
				RenderingManager::getInstance()->addSourceToBasket(s);

				CloneSource *cs = dynamic_cast<CloneSource*> (s);
				if (cs) {
					qDebug() << s->getName() << '|' <<  tr("OpenCV device source %1 was cloned.").arg(cs->getOriginalName());
					statusbar->showMessage( tr("The device source %1 was cloned.").arg(cs->getOriginalName()), 3000 );
				} else {
					qDebug() << s->getName() << '|' <<  tr("New OpenCV source created (device index %2).").arg(selectedCamIndex);
					statusbar->showMessage( tr("Source created with OpenCV drivers for Camera %1").arg(selectedCamIndex), 3000 );
				}
			} else
				qCritical() << tr("Could not open OpenCV device index %2. ").arg(selectedCamIndex);

		}
	}
#endif
}


void GLMixer::on_actionSvgSource_triggered(){

	QString current = currentSessionFileName.isEmpty() ? QDir::currentPath() : QFileInfo(currentSessionFileName).absolutePath();
    QString fileName = QFileDialog::getOpenFileName(this, tr("OpenSVG file"), current, tr("Scalable Vector Graphics (*.svg)"), 0, usesystemdialogs ? QFlags<QFileDialog::Option>() : QFileDialog::DontUseNativeDialog);

	if ( !fileName.isEmpty() ) {

		QFileInfo file(fileName);
		if ( !file.exists() || !file.isReadable()) {
			qCritical() << fileName << '|' <<   tr("File does not exist or is unreadable.");
			return;
		}

		QSvgRenderer *svg = new QSvgRenderer(fileName);

		Source *s = RenderingManager::getInstance()->newSvgSource(svg);
		if ( s ){
			RenderingManager::getInstance()->addSourceToBasket(s);
			qDebug() << s->getName() << '|' <<  tr("New vector Graphics source created with file ")<< fileName;
			statusbar->showMessage( tr("Source created with the vector graphics file %1.").arg( fileName ), 3000 );
		} else
			qCritical() << fileName << '|' <<  tr("Could not create a vector graphics source with this file.");
	}
}


void GLMixer::on_actionShmSource_triggered(){

	// popup a question dialog to select the shared memory block
	static SharedMemoryDialog *shmd = 0;
	if(!shmd)
		shmd = new SharedMemoryDialog(this);

	if (shmd->exec() == QDialog::Accepted) {
		Source *s = RenderingManager::getInstance()->newSharedMemorySource(shmd->getSelectedId());
		if ( s ){
			RenderingManager::getInstance()->addSourceToBasket(s);
			qDebug() << s->getName() << '|' <<  tr("New shared memory source created (")<< shmd->getSelectedProcess() << ").";
			statusbar->showMessage( tr("Source created with the process %1.").arg( shmd->getSelectedProcess() ), 3000 );
		} else
			qCritical() << shmd->getSelectedProcess() << '|' << tr("Could not create shared memory source.");
	}
}

void GLMixer::on_actionAlgorithmSource_triggered(){

	// popup a question dialog to select the type of algorithm
	static AlgorithmSelectionDialog *asd = 0;
	if (!asd)
		asd = new AlgorithmSelectionDialog(this);

	if (asd->exec() == QDialog::Accepted) {
		Source *s = RenderingManager::getInstance()->newAlgorithmSource(asd->getSelectedAlgorithmIndex(),
					asd->getSelectedWidth(), asd->getSelectedHeight(), asd->getSelectedVariability(), asd->getUpdatePeriod(), asd->getIngoreAlpha());
		if ( s ){
			RenderingManager::getInstance()->addSourceToBasket(s);
			qDebug() << s->getName() << '|' << tr("New Algorithm source created (")<< AlgorithmSource::getAlgorithmDescription(asd->getSelectedAlgorithmIndex()) << ").";
			statusbar->showMessage( tr("Source created with the algorithm %1.").arg( AlgorithmSource::getAlgorithmDescription(asd->getSelectedAlgorithmIndex())), 3000 );
		} else
			qCritical() << AlgorithmSource::getAlgorithmDescription(asd->getSelectedAlgorithmIndex()) << '|' << tr("Could not create algorithm source.");
	}
}


void GLMixer::on_actionRenderingSource_triggered(){

	Source *s = RenderingManager::getInstance()->newRenderingSource();
	if ( s ){
		RenderingManager::getInstance()->addSourceToBasket(s);
		qDebug() << s->getName() <<  tr("|New rendering loopback source created.");
		statusbar->showMessage( tr("Source created with the rendering output loopback."), 3000 );
	}else
		qCritical() << tr("Could not create rendering loopback source.");
}


void GLMixer::on_actionCloneSource_triggered(){

	if ( RenderingManager::getInstance()->notAtEnd(RenderingManager::getInstance()->getCurrentSource()) ) {
		Source *s = RenderingManager::getInstance()->newCloneSource( RenderingManager::getInstance()->getCurrentSource());
		if ( s ) {
			QString name = (*RenderingManager::getInstance()->getCurrentSource())->getName();
			RenderingManager::getInstance()->addSourceToBasket(s);
			qDebug() << s->getName() << tr("|New clone of source %1 created.").arg(name);
			statusbar->showMessage( tr("The current source has been cloned."), 3000);
		} else
			qCritical() << tr("Could not clone source %1.").arg((*RenderingManager::getInstance()->getCurrentSource())->getName());
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
	QFileDialog fd( parentWidget(), tr("Save captured image"),fname.absoluteFilePath(), "PNG image(*.png);;JPEG Image(*.jpg);;TIFF image(*.tiff);;XPM image(*.xpm)");
	fd.setAcceptMode(QFileDialog::AcceptSave);
	fd.setFileMode(QFileDialog::AnyFile);
	fd.setDefaultSuffix("png");
	fd.setOption(QFileDialog::DontUseNativeDialog, !GLMixer::getInstance()->useSystemDialogs());

	if (fd.exec()) {
	    QString filename = fd.selectedFiles().front();

		// save the file
		if (!filename.isEmpty()) {
			if (!img.save(filename)) {
				qCritical() << filename << tr("|Could not save file.");
				return QString();
			}
			// remember location and base file name for next time
			fname = QFileInfo( filename );
			dname = fname.dir();
			basename =  fname.baseName().section("_", 0, fname.baseName().count("_")-1);

			return filename;
		}
	}

	return QString();
}


void GLMixer::on_actionCaptureSource_triggered(){

	// capture screen
	QImage capture = RenderingManager::getInstance()->captureFrameBuffer();
	capture = capture.convertToFormat(QImage::Format_RGB32);

	// display and request action with this capture
	CaptureDialog cd(this, capture, tr("Create a source with this image ?"));

	if (cd.exec() == QDialog::Accepted) {

		Source *s = RenderingManager::getInstance()->newCaptureSource(cd.img);
		if ( s ){
			RenderingManager::getInstance()->addSourceToBasket(s);
			qDebug() << s->getName() <<  tr("|New capture source created.");
		}
		else
			qCritical() << tr("Could not create capture source.");
	}
}


void GLMixer::on_actionDeleteSource_triggered()
{
	// lisst of sources to delete
	SourceList todelete;

	// if the current source is valid, add it todelete
	SourceSet::iterator cs = RenderingManager::getInstance()->getCurrentSource();
	if ( RenderingManager::getInstance()->isValid(cs) )
		//  delete only the current
		todelete.insert(*cs);
	// if there is a selection and no source is current, delete the whole selection
	else if ( View::hasSelection() )
		// make a copy of the selection (to make sure we do not mess with pointers when removing from selection)
		todelete = View::copySelection();

	// remove all the source in the list todelete
	for(SourceList::iterator  its = todelete.begin(); its != todelete.end(); its++) {

		SourceSet::iterator sit = RenderingManager::getInstance()->getById((*its)->getId());
		if ( RenderingManager::getInstance()->isValid(sit) ) {
			// test for clones of this source
			int numclones = (*sit)->getClones()->size();
			// popup a question dialog 'are u sure' if there are clones attached;
			if ( numclones ){
				QString msg = tr("This source was cloned %1 times; Do you want to delete all the clones too?").arg(numclones);
				if ( QMessageBox::question(this," Are you sure?", msg, QMessageBox::Ok | QMessageBox::Cancel, QMessageBox::Ok) == QMessageBox::Ok)
					numclones = 0;
			}

			if ( !numclones ){
				QString d = (*sit)->getName();
				RenderingManager::getInstance()->removeSource(sit);
				statusbar->showMessage( tr("Source %1 deleted.").arg( d ), 3000 );
			}
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
	// inform the user that data might be lost
	int ret = QMessageBox::Discard;
	if (maybeSave) {
		 QMessageBox msgBox;
		 msgBox.setText(tr("The session have been modified."));
		 msgBox.setInformativeText(tr("Do you want to save your changes ?"));
		 msgBox.setIconPixmap( QPixmap(QString::fromUtf8(":/glmixer/icons/question.png")) );
		 msgBox.setStandardButtons(QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel);
		 msgBox.setDefaultButton(QMessageBox::Save);
		 ret = msgBox.exec();
	}
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
	maybeSave = false;
}


void GLMixer::on_actionSave_Session_triggered(){

	if (currentSessionFileName.isNull())
		on_actionSave_Session_as_triggered();
	else {

		QFile file(currentSessionFileName);
		if (!file.open(QFile::WriteOnly | QFile::Text) ) {
			qWarning() << currentSessionFileName << tr("| Problem writing; ") << file.errorString();
			qCritical() << currentSessionFileName << tr("|Cannot save file %1.");
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

	maybeSave = false;
}

void GLMixer::on_actionSave_Session_as_triggered()
{
	sfd->setAcceptMode(QFileDialog::AcceptSave);
	sfd->setFileMode(QFileDialog::AnyFile);
	sfd->setFilter(tr("GLMixer session (*.glm)"));
	sfd->setOption(QFileDialog::DontUseNativeDialog, !usesystemdialogs);
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
	sfd->setOption(QFileDialog::DontUseNativeDialog, !usesystemdialogs);
	sfd->setFilter(tr("GLMixer session (*.glm)"));

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

	if (filename.isEmpty())
		newSession();
	else
	{
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
	if (!file.open(QFile::ReadOnly | QFile::Text)) {
		qWarning() << currentSessionFileName << tr("|Problem reading file; ") << file.errorString();
		qCritical() << currentSessionFileName << tr("|Cannot open file.");
		currentSessionFileName = QString();
		return;
	}
	// load content
    if (!doc.setContent(&file, true, &errorStr, &errorLine, &errorColumn)) {
		qWarning() << currentSessionFileName << tr("|XML parsing error line ") << errorLine << "(" << errorColumn << "); " << errorStr;
		qCritical() << currentSessionFileName << tr("|Cannot open file.");
    	currentSessionFileName = QString();
    	return;
    }
    // close file
    file.close();
    // verify it is a GLM file
    QDomElement root = doc.documentElement();
    if (root.tagName() != "GLMixer") {
		qWarning() << currentSessionFileName << tr("|This is not a GLMixer session file; ");
		qCritical() << currentSessionFileName << tr("|Cannot open file.");
    	currentSessionFileName = QString();
        return;
    } else if (root.hasAttribute("version") && root.attribute("version") != XML_GLM_VERSION) {
		qWarning() << currentSessionFileName << tr("|The version of the file is ") << root.attribute("version") << tr(" instead of ") <<XML_GLM_VERSION;
		qCritical() << currentSessionFileName << tr("|Incorrect file version. Trying to read what is compatible.");
    }
	// if we got up to here, it should be fine ; reset for a new session and apply loaded configurations
	RenderingManager::getInstance()->clearSourceSet();
    // read the source list and its configuration
    QDomElement renderConfig = root.firstChildElement("SourceList");
    if (renderConfig.isNull())
    	qWarning() << currentSessionFileName << tr("|There is no source to load.");
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
    		break;
    	}
    	float g = renderConfig.attribute("gammaShift", "1").toFloat();
    	gammaShiftSlider->setValue(GammaToScale(g));
    	gammaShiftText->setText( QString().setNum( g, 'f', 2) );
    	RenderingManager::getInstance()->setGammaShift(g);
    	// read the list of sources
    	qDebug() << currentSessionFileName << tr("|Loading session.");
	    // if we got up to here, it should be fine
	    int errors = RenderingManager::getInstance()->addConfiguration(renderConfig, QFileInfo(currentSessionFileName).canonicalPath());
	    if ( errors > 0)
	    	qCritical() << currentSessionFileName << "| " << errors << tr(" error(s) occurred when reading session.");

    }
    // read the views configuration
	RenderingManager::getRenderingWidget()->clearViews();
    QDomElement vconfig = root.firstChildElement("Views");
    if (vconfig.isNull())
    	qDebug() << currentSessionFileName << tr("|No configuration specified.");
    else  {
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
	maybeSave = false;

	// start the smooth transition
    RenderingManager::getSessionSwitcher()->startTransition(true);
}


void GLMixer::on_actionAppend_Session_triggered(){

	QDomDocument doc;
    QString errorStr;
    int errorLine;
    int errorColumn;

	sfd->setAcceptMode(QFileDialog::AcceptOpen);
	sfd->setFileMode(QFileDialog::ExistingFile);
	sfd->setOption(QFileDialog::DontUseNativeDialog, !usesystemdialogs);
	sfd->setFilter(tr("GLMixer session (*.glm)"));

	if (sfd->exec()) {
		// get the first file name selected
		QString fileName =  sfd->selectedFiles().front() ;

		QFile file(fileName);

		if ( !file.open(QFile::ReadOnly | QFile::Text) ) {
			qWarning() << fileName << tr("|Problem reading file; ") << file.errorString();
			qCritical() << fileName << tr("|Cannot open file.");
			return;
		}

		if ( !doc.setContent(&file, true, &errorStr, &errorLine, &errorColumn) ) {
			qWarning() << fileName << tr("|XML parsing error line ") << errorLine << "(" << errorColumn << "); " << errorStr;
			qCritical() << fileName << tr("|Cannot open file.");
			return;
		}

		file.close();

		QDomElement root = doc.documentElement();
		if ( root.tagName() != "GLMixer" ) {
			qWarning() << fileName << tr("|This is not a GLMixer session file.");
			qCritical() << fileName << tr("|Cannot open file.");
			return;
		} else if ( root.hasAttribute("version") && root.attribute("version") != XML_GLM_VERSION ) {
			qWarning() << fileName << tr("|The version of the file is ") << root.attribute("version") << tr(" instead of ") <<XML_GLM_VERSION;
			qCritical() << fileName << tr("|Incorrect file version. Trying to read what is compatible.");
			return;
		}

		// read the content of the source list to make sure the file is correct :
		QDomElement srcconfig = root.firstChildElement("SourceList");
		if ( srcconfig.isNull() ) {
			qWarning() << fileName << tr("|There is no source to load.");
			return;
		}

		// if we got up to here, it should be fine
		qDebug() << fileName << tr("|Adding list of sources.");
		int errors = RenderingManager::getInstance()->addConfiguration(srcconfig, QFileInfo(currentSessionFileName).canonicalPath());
		if ( errors > 0)
			qCritical() << currentSessionFileName << "|" << errors << tr(" error(s) occurred when reading session.");

		// confirm the loading of the file
		statusbar->showMessage( tr("Sources from %1 appended to %2.").arg( fileName ).arg( currentSessionFileName ), 3000 );
	}

	maybeSave = true;
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
	QStringList svgFiles;
	QString glmfile;
	int errors = 0;

    // browse the list of urls dropped
	if (mimeData->hasUrls()) {
		// deal with the urls dropped
		event->acceptProposedAction();
		QList<QUrl> urlList = mimeData->urls();

		// arbitrary limitation in the amount of drops allowed (avoid manipulation mistakes)
		if (urlList.size() > MAX_DROP_FILES)
			qWarning() << "[" << ++errors << "]" << tr("Cannot open more than %1 files at a time.").arg(MAX_DROP_FILES);

		for (int i = 0; i < urlList.size() && i < MAX_DROP_FILES; ++i) {
			QFileInfo urlname(urlList.at(i).toLocalFile());
			if ( urlname.exists() && urlname.isReadable() && urlname.isFile()) {

				if ( urlname.suffix() == "glm") {
					if (glmfile.isNull())
						glmfile = urlname.absoluteFilePath();
					else
						break;
				}
				else if ( urlname.suffix() == "svg") {
					svgFiles.append(urlname.absoluteFilePath());
				}
				else //  maybe a video ?
					mediaFiles.append(urlname.absoluteFilePath());
			}
			else
				qWarning() << urlname.absoluteFilePath() <<  "|[" << ++errors << "]" << tr("Not a valid file; Ignoring.");
		}
	}
	else
		return;

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

		if (!mediaFiles.isEmpty() || !svgFiles.isEmpty())
			qWarning() <<  "[" << ++errors << "]" << tr("Discarding %1 media files and %2 svg files; only loading the glm session.").arg(mediaFiles.count()).arg(svgFiles.count());

	} else {
		// loading Media files
		int i = 0;
		for (; i < mediaFiles.size(); ++i)
		{
			VideoFile *newSourceVideoFile  = new VideoFile(this);

			// if the video file was created successfully
			if (newSourceVideoFile){
				// can we open the file ?
				if ( newSourceVideoFile->open( mediaFiles.at(i) ) ) {
					Source *s = RenderingManager::getInstance()->newMediaSource(newSourceVideoFile);
					// create the source as it is a valid video file (this also set it to be the current source)
					if ( s ) {
						RenderingManager::getInstance()->addSourceToBasket(s);
						qDebug() << s->getName() << tr("|New media source created with file ") << mediaFiles.at(i);
					} else {
						qWarning() << mediaFiles.at(i) <<  "|[" << ++errors << "]" << tr("Could not be created.");
						delete newSourceVideoFile;
					}
				} else {
					qWarning() << mediaFiles.at(i) <<  "|[" << ++errors << "]" << tr("Could not be loaded.");
					delete newSourceVideoFile;
				}
			}
		}
		// loading SVG files
		for (i = 0; i < svgFiles.size(); ++i)
		{
			QSvgRenderer *svg = new QSvgRenderer(svgFiles.at(i));
			Source *s = RenderingManager::getInstance()->newSvgSource(svg);
			if ( s ) {
				RenderingManager::getInstance()->addSourceToBasket(s);
				qDebug() << s->getName() <<  tr("|New vector Graphics source created with file ")<< svgFiles.at(i);
			} else {
				qWarning() << svgFiles.at(i) <<  "|[" << ++errors << "]" << tr("Could not be created.");
				delete svg;
			}
		}
	}

	if (errors > 0)
		qCritical() << tr("Not all the dropped files could be loaded.");

	maybeSave = true;
}

void GLMixer::dragLeaveEvent(QDragLeaveEvent *event)
{
    event->accept();
}


void GLMixer::readSettings()
{
    // preferences
    if (settings.contains("UserPreferences"))
    	restorePreferences(settings.value("UserPreferences").toByteArray());
    else
    	restorePreferences(QByteArray());

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
    else
        settings.setValue("defaultOutputRenderWindow", (OutputRenderWindow::getInstance())->saveGeometry());

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


	qDebug() << tr("All settings restored.");
}

void GLMixer::saveSettings()
{
    // preferences
	settings.setValue("UserPreferences", getPreferences());

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
	// make sure system saves settings NOW
    settings.sync();
	qDebug() << tr("Settings saved.");
}


void GLMixer::on_actionResetToolbars_triggered()
{
	restoreGeometry(settings.value("defaultGeometry").toByteArray());
	restoreState(settings.value("defaultWindowState").toByteArray());
	OutputRenderWindow::getInstance()->restoreGeometry(settings.value("defaultOutputRenderWindow").toByteArray());
	restoreDockWidget(previewDockWidget);
	restoreDockWidget(sourceDockWidget);
	restoreDockWidget(vcontrolDockWidget);
	restoreDockWidget(cursorDockWidget);
	restoreDockWidget(gammaDockWidget);
	restoreDockWidget(switcherDockWidget);
	restoreDockWidget(logDockWidget);

	qDebug() << tr("Default layout restored.");
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

	const quint32 magicNumber = MAGIC_NUMBER;
    const quint16 currentMajorVersion = QSETTING_PREFERENCE_VERSION;
	quint32 storedMagicNumber = 0;
    quint16 majorVersion = 0;

    if (!state.isEmpty()) {
    	QByteArray sd = state;
		QDataStream stream(&sd, QIODevice::ReadOnly);
		stream >> storedMagicNumber >> majorVersion;
	}

    if (storedMagicNumber != magicNumber || majorVersion != currentMajorVersion) {

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
	stream >> storedMagicNumber >> majorVersion;

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
	RenderingManager::getPropertyBrowserWidget()->setFilterPropertyEnabled(!disablefilter);

	// j. antialiasing
	bool antialiasing = true;
	stream >> antialiasing;
	RenderingManager::getRenderingWidget()->setAntiAliasing(antialiasing);
	gammaAdjust->setAntialiasing(antialiasing);

	// k. mouse buttons and modifiers
	QMap<int, int> mousemap;
	stream >> mousemap;
	View::setMouseButtonsMap(mousemap);
	QMap<int, int> modifiermap;
	stream >> modifiermap;
	View::setMouseModifiersMap(modifiermap);

	// l. zoom config
	int zoomspeed = 120;
	stream >> zoomspeed;
	View::setZoomSpeed((float)zoomspeed);
	bool zoomcentered = true;
	stream >> zoomcentered;
	View::setZoomCentered(zoomcentered);

	// m. useSystemDialogs
	stream >> usesystemdialogs;

	//	 n. shared memory depth
	uint shmdepth = 0;
	stream >> shmdepth;
	RenderingManager::getInstance()->setSharedMemoryColorDepth(shmdepth);

	// Refresh widgets to make changes visible
	OutputRenderWindow::getInstance()->refresh();
	outputpreview->refresh();
	// de-select current source
	RenderingManager::getInstance()->unsetCurrentSource();

	qDebug() << tr("Preferences loaded.");
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

	// k. mouse buttons and modifiers
	stream << View::getMouseButtonsMap();
	stream << View::getMouseModifiersMap();

	// l. zoom config
	stream << (int) View::zoomSpeed();
	stream << View::zoomCentered();

	// m. useSystemDialogs
	stream << _instance->useSystemDialogs();

	// n. shared memory color depth
	stream << (uint)  RenderingManager::getInstance()->getSharedMemoryColorDepth();

	return data;
}


void GLMixer::on_gammaShiftSlider_valueChanged(int val)
{
	float g = ScaleToGamma(val);
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
		if (!filename.isEmpty())
			qDebug() << filename << tr("|Snapshot saved.");
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


void GLMixer::on_actionSelectAll_triggered()
{
	SourceList sl;
	SourceSet::iterator sit = RenderingManager::getInstance()->getBegin();
	// check for the existence of an opencv source which would already be on this same index
	for ( ; RenderingManager::getInstance()->notAtEnd(sit); sit++)
		sl.insert(*sit);
	View::select(sl);
}

void GLMixer::on_actionSelectInvert_triggered()
{
	SourceSet::iterator sit = RenderingManager::getInstance()->getBegin();
	// check for the existence of an opencv source which would already be on this same index
	for ( ; RenderingManager::getInstance()->notAtEnd(sit); sit++)
		View::select(*sit);
}

void GLMixer::on_actionSelectCurrent_triggered()
{
	SourceSet::iterator sit = RenderingManager::getInstance()->getCurrentSource();
	if ( RenderingManager::getInstance()->isValid(sit)) {
		if (View::isInSelection(*sit))
			View::deselect(*sit);
		else
			View::select(*sit);
	}
}

void GLMixer::on_actionSelectNone_triggered()
{
	View::clearSelection();
}

void GLMixer::updateStatusControlActions() {

	bool playEnabled = false, controlsEnabled = false;

	// get current source
	SourceSet::iterator cs = RenderingManager::getInstance()->getCurrentSource();
	if (RenderingManager::getInstance()->isValid(cs)) {
		// test if the current source is playable ; if yes, enable action start/stop
		if ( (*cs)->isPlayable() ) {
			playEnabled = true;
			// test if the current source is Media source ; the selectedSourceVideoFile has been set in currentChanged method
			if (selectedSourceVideoFile)
				// if yes, enable actions for media control (and return)
				controlsEnabled = true;
		}
	}

	// test the presence of playable source to enable action Start/Stop
	// test the presence of Media source to enable actions for media control
	for(SourceList::iterator  its = View::selectionBegin(); its != View::selectionEnd(); its++) {

		if ( (*its)->isPlayable() ) {
			playEnabled = true;
			if ( (*its)->rtti() == Source::VIDEO_SOURCE )
				// enable actions for media control (and return)
				controlsEnabled = true;
		}
	}

	sourceControlMenu->setEnabled( playEnabled );
	sourceControlToolBar->setEnabled( playEnabled );
	actionSourceRestart->setEnabled( controlsEnabled );
	actionSourceSeekBackward->setEnabled( controlsEnabled );
	actionSourcePause->setEnabled( controlsEnabled );
	actionSourceSeekForward->setEnabled( controlsEnabled );
 }

bool GLMixer::useSystemDialogs()
{
	return usesystemdialogs;
}


void GLMixer::on_actionFullscreenMode_toggled(bool on){

	// discard non-changing state (NOT XOR)
	if ( !(on ^ (windowState() & Qt::WindowFullScreen)) )
		return;

	// other cases ; need to switch fullscreen <-> not fullscreen
	menubar->setVisible(!on);
	actionFullscreen->setChecked(on);
	setWindowState(windowState() ^ Qt::WindowFullScreen);
	update();
}


void GLMixer::on_actionSourcePlay_triggered(){

	// toggle play/stop of current source
	SourceSet::iterator cs = RenderingManager::getInstance()->getCurrentSource();
	if (RenderingManager::getInstance()->isValid(cs))
		(*cs)->play(!(*cs)->isPlaying());

	// loop over the selection and toggle play/stop of each source (but the current source already toggled)
	for(SourceList::iterator  its = View::selectionBegin(); its != View::selectionEnd(); its++)
		if (*its != *cs)
			(*its)->play(!(*its)->isPlaying());
}

void GLMixer::on_actionSourceRestart_triggered(){

	// apply action to current source
	SourceSet::iterator cs = RenderingManager::getInstance()->getCurrentSource();
	if (RenderingManager::getInstance()->isValid(cs) && selectedSourceVideoFile )
		selectedSourceVideoFile->seekBegin();

	// loop over the selection and apply action of each source (but the current source already done)
	for(SourceList::iterator  its = View::selectionBegin(); its != View::selectionEnd(); its++)
		if (*its != *cs && (*its)->rtti() == Source::VIDEO_SOURCE ){
			VideoFile *vf = (dynamic_cast<VideoSource *>(*its))->getVideoFile();
			vf->seekBegin();
		}
}
void GLMixer::on_actionSourceSeekBackward_triggered(){

	// apply action to current source
	SourceSet::iterator cs = RenderingManager::getInstance()->getCurrentSource();
	if (RenderingManager::getInstance()->isValid(cs) && selectedSourceVideoFile )
		selectedSourceVideoFile->seekBackward();

	// loop over the selection and apply action of each source (but the current source already done)
	for(SourceList::iterator  its = View::selectionBegin(); its != View::selectionEnd(); its++)
		if (*its != *cs && (*its)->rtti() == Source::VIDEO_SOURCE ){
			VideoFile *vf = (dynamic_cast<VideoSource *>(*its))->getVideoFile();
			vf->seekBackward();
		}
}
void GLMixer::on_actionSourcePause_triggered(){

	// toggle pause/resume of current source
	SourceSet::iterator cs = RenderingManager::getInstance()->getCurrentSource();
	if (RenderingManager::getInstance()->isValid(cs) && selectedSourceVideoFile )
		selectedSourceVideoFile->pause(!selectedSourceVideoFile->isPaused());

	// loop over the selection and toggle pause/resume of each source (but the current source already toggled)
	for(SourceList::iterator  its = View::selectionBegin(); its != View::selectionEnd(); its++)
		if (*its != *cs && (*its)->rtti() == Source::VIDEO_SOURCE ){
			VideoFile *vf = (dynamic_cast<VideoSource *>(*its))->getVideoFile();
			vf->pause(!vf->isPaused());
		}
}
void GLMixer::on_actionSourceSeekForward_triggered(){

	// apply action to current source
	SourceSet::iterator cs = RenderingManager::getInstance()->getCurrentSource();
	if (RenderingManager::getInstance()->isValid(cs) && selectedSourceVideoFile )
		selectedSourceVideoFile->seekForward();

	// loop over the selection and apply action of each source (but the current source already done)
	for(SourceList::iterator  its = View::selectionBegin(); its != View::selectionEnd(); its++)
		if (*its != *cs && (*its)->rtti() == Source::VIDEO_SOURCE ){
			VideoFile *vf = (dynamic_cast<VideoSource *>(*its))->getVideoFile();
			vf->seekForward();
		}
}

//
// Align and distribute toolbox
//
void GLMixer::on_alignHorizontalLeftButton_clicked(){

	RenderingManager::getRenderingWidget()->alignSelection(View::AXIS_HORIZONTAL, View::ALIGN_BOTTOM_LEFT);
}

void GLMixer::on_alignHorizontalCenterButton_clicked(){

	RenderingManager::getRenderingWidget()->alignSelection(View::AXIS_HORIZONTAL, View::ALIGN_CENTER);
}

void GLMixer::on_alignHorizontalRightButton_clicked(){

	RenderingManager::getRenderingWidget()->alignSelection(View::AXIS_HORIZONTAL, View::ALIGN_TOP_RIGHT);
}

void GLMixer::on_alignVerticalBottomButton_clicked(){

	RenderingManager::getRenderingWidget()->alignSelection(View::AXIS_VERTICAL, View::ALIGN_BOTTOM_LEFT);
}

void GLMixer::on_alignVerticalCenterButton_clicked(){

	RenderingManager::getRenderingWidget()->alignSelection(View::AXIS_VERTICAL, View::ALIGN_CENTER);
}

void GLMixer::on_alignVerticalTopButton_clicked(){

	RenderingManager::getRenderingWidget()->alignSelection(View::AXIS_VERTICAL, View::ALIGN_TOP_RIGHT);
}

void GLMixer::on_distributeHorizontalLeftButton_clicked(){

	RenderingManager::getRenderingWidget()->distributeSelection(View::AXIS_HORIZONTAL, View::ALIGN_BOTTOM_LEFT);
}

void GLMixer::on_distributeHorizontalCenterButton_clicked(){

	RenderingManager::getRenderingWidget()->distributeSelection(View::AXIS_HORIZONTAL, View::ALIGN_CENTER);
}

void GLMixer::on_distributeHorizontalGapsButton_clicked(){

	RenderingManager::getRenderingWidget()->distributeSelection(View::AXIS_HORIZONTAL, View::ALIGN_EQUAL_GAPS);
}

void GLMixer::on_distributeHorizontalRightButton_clicked(){

	RenderingManager::getRenderingWidget()->distributeSelection(View::AXIS_HORIZONTAL, View::ALIGN_TOP_RIGHT);
}

void GLMixer::on_distributeVerticalBottomButton_clicked(){

	RenderingManager::getRenderingWidget()->distributeSelection(View::AXIS_VERTICAL, View::ALIGN_BOTTOM_LEFT);
}

void GLMixer::on_distributeVerticalCenterButton_clicked(){

	RenderingManager::getRenderingWidget()->distributeSelection(View::AXIS_VERTICAL, View::ALIGN_CENTER);
}

void GLMixer::on_distributeVerticalGapsButton_clicked(){

	RenderingManager::getRenderingWidget()->distributeSelection(View::AXIS_VERTICAL, View::ALIGN_EQUAL_GAPS);
}

void GLMixer::on_distributeVerticalTopButton_clicked(){

	RenderingManager::getRenderingWidget()->distributeSelection(View::AXIS_VERTICAL, View::ALIGN_TOP_RIGHT);
}
