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
#include "VideoFileDialog.h"
#include "AlgorithmSelectionDialog.h"
#include "UserPreferencesDialog.h"
#include "ViewRenderWidget.h"
#include "RenderingManager.h"
#include "SelectionManager.h"
#include "SourceDisplayWidget.h"
#include "OutputRenderWindow.h"
#include "MixerView.h"
#include "RenderingSource.h"
#include "AlgorithmSource.h"
#include "VideoSource.h"
#include "WebSource.h"
#include "SvgSource.h"
#include "VideoFileDisplayWidget.h"
#include "SourcePropertyBrowser.h"
#include "CloneSource.h"
#include "SessionSwitcherWidget.h"
#include "CatalogView.h"
#include "DelayCursor.h"
#include "SpringCursor.h"
#include "AxisCursor.h"
#include "LineCursor.h"
#include "FuzzyCursor.h"
#include "RenderingEncoder.h"
#include "SessionSwitcher.h"
#include "MixingToolboxWidget.h"
#include "LayoutToolboxWidget.h"
#include "GammaLevelsWidget.h"
#include "OpenSoundControlManager.h"
#include "NewSourceDialog.h"
#include "WebSourceCreationDialog.h"

#ifdef SHM
#include "SharedMemorySource.h"
#include "SharedMemoryDialog.h"
#include "SharedMemoryManager.h"
#endif

#ifdef OPEN_CV
#include "CameraDialog.h"
#include "OpencvSource.h"
#endif

#ifdef FFGL
#include "FFGLSource.h"
#include "FFGLPluginSource.h"
#include "FFGLPluginSourceShadertoy.h"
#include "FFGLSourceCreationDialog.h"
#include "GLSLCodeEditorWidget.h"
#endif

#include "glmixer.moc"

GLMixer *GLMixer::_instance = 0;

QByteArray static_windowstate =
QByteArray::fromHex("000000ff00000000fd0000000300000000000001150000022bfc0200000003fc0000003b000000d5000000d500fffffffa000000000100000002fb0000002200700072006500760069006500770044006f0063006b0057006900640067006500740100000000ffffffff000000d400fffffffb000000200063007500720073006f00720044006f0063006b005700690064006700650074010000000000000136000000e600fffffffc00000112000001540000009100fffffffa000000000100000002fb00000024007300770069007400630068006500720044006f0063006b0057006900640067006500740100000000ffffffff000000e500fffffffb000000240062006c006f0063006e006f007400650044006f0063006b0057006900640067006500740100000000ffffffff000000ac00fffffffb0000001a006c006f00670044006f0063006b005700690064006700650074020000044e0000029d000002ef0000018400000001000001040000022bfc0200000003fc0000003b0000022b000001c300fffffffa000000000100000003fb00000020006d006900780069006e00670044006f0063006b0057006900640067006500740100000000ffffffff000000ec00fffffffb000000200073006f00750072006300650044006f0063006b0057006900640067006500740100000000ffffffff0000004100fffffffb00000020006c00610079006f007500740044006f0063006b0057006900640067006500740100000000ffffffff000000a600fffffffc000002f2000000300000000000fffffffa000000000100000001fb0000001e0061006c00690067006e0044006f0063006b0057006900640067006500740100000000ffffffff0000000000000000fb0000001e00670061006d006d00610044006f0063006b0057006900640067006500740100000000ffffffff0000000000000000000000030000042d00000054fc0100000001fb0000002400760063006f006e00740072006f006c0044006f0063006b00570069006400670065007401000000000000042d000003070007ffff000002100000022b00000004000000040000000800000008fc0000000100000002000000060000001a0073006f00750072006300650054006f006f006c0042006100720100000000ffffffff00000000000000000000001600760069006500770054006f006f006c00420061007201000001e4ffffffff00000000000000000000001600660069006c00650054006f006f006c00420061007201000002a8ffffffff0000000000000000000000180074006f006f006c00730054006f006f006c0042006100720100000341ffffffff00000000000000000000002000720065006e0064006500720069006e00670054006f006f006c00420061007201000003b9ffffffff0000000000000000000000280073006f00750072006300650043006f006e00740072006f006c0054006f006f006c00420061007201000003f3ffffffff0000000000000000" );

class CaptureDialog: public QDialog {

public:
    QImage img;

    CaptureDialog(QWidget *parent, QImage capture, QString caption): QDialog(parent), img(capture) {

        QVBoxLayout *verticalLayout;
        QLabel *Question, *Display, *Info;
        QDialogButtonBox *DecisionButtonBox;

        setObjectName(QString::fromUtf8("CaptureDialog"));
        setWindowTitle(tr( "Frame captured"));
        verticalLayout = new QVBoxLayout(this);
        verticalLayout->setSpacing(9);

        Question = new QLabel(this);
        Question->setText(caption);
        verticalLayout->addWidget(Question);

        Display = new QLabel(this);
        Display->setPixmap(QPixmap::fromImage(img).scaledToWidth(300));
        verticalLayout->addWidget(Display);

        Info = new QLabel(this);
        Info->setText(tr("Original size: %1 x %2 px").arg(img.width()).arg(img.height()) );
        verticalLayout->addWidget(Info);

        DecisionButtonBox = new QDialogButtonBox(this);
        DecisionButtonBox->setStandardButtons(QDialogButtonBox::Cancel|QDialogButtonBox::Ok);
        verticalLayout->addWidget(DecisionButtonBox);

        QObject::connect(DecisionButtonBox, SIGNAL(accepted()), this, SLOT(accept()));
        QObject::connect(DecisionButtonBox, SIGNAL(rejected()), this, SLOT(reject()));
    }

};



GLMixer *GLMixer::getInstance() {

    if (_instance == 0) {
        _instance = new GLMixer();
        Q_CHECK_PTR(_instance);
    }

    return _instance;
}

GLMixer::GLMixer ( QWidget *parent): QMainWindow ( parent ),
    selectedSourceVideoFile(NULL), usesystemdialogs(false), maybeSave(true),
    refreshTimingTimer(0), _displayTimeAsFrame(false), _restoreLastSession(true)
{
    setupUi ( this );

    // hide logs on show
    logDockWidget->hide();

#ifndef SHM
    actionShareToRAM->setVisible(false);
    actionShmSource->setVisible(false);
#endif
#ifndef SPOUT
    actionShareToSPOUT->setVisible(false);
#endif

#ifndef OPEN_CV
    actionCameraSource->setVisible(false);
#endif

#ifdef FFGL
    pluginGLSLCodeEditor = new GLSLCodeEditorWidget();
#else
    actionFreeframeSource->setVisible(false);
#endif

    // add the show/hide menu items for the dock widgets
    toolBarsMenu->addAction(previewDockWidget->toggleViewAction());
    toolBarsMenu->addAction(sourceDockWidget->toggleViewAction());
    toolBarsMenu->addAction(vcontrolDockWidget->toggleViewAction());
    toolBarsMenu->addAction(cursorDockWidget->toggleViewAction());
    toolBarsMenu->addAction(mixingDockWidget->toggleViewAction());
    toolBarsMenu->addAction(switcherDockWidget->toggleViewAction());
    toolBarsMenu->addAction(layoutDockWidget->toggleViewAction());
    toolBarsMenu->addAction(blocnoteDockWidget->toggleViewAction());
    QAction *showlog = logDockWidget->toggleViewAction();
    toolBarsMenu->addAction(showlog);
    toolBarsMenu->addSeparator();
    toolBarsMenu->addAction(sourceToolBar->toggleViewAction());
    toolBarsMenu->addAction(viewToolBar->toggleViewAction());
    toolBarsMenu->addAction(fileToolBar->toggleViewAction());
    toolBarsMenu->addAction(toolsToolBar->toggleViewAction());
    toolBarsMenu->addAction(renderingToolBar->toggleViewAction());
    toolBarsMenu->addAction(sourceControlToolBar->toggleViewAction());

    QActionGroup *viewActions = new QActionGroup(this);
    Q_CHECK_PTR(viewActions);
    viewActions->addAction(actionMixingView);
    viewActions->addAction(actionGeometryView);
    viewActions->addAction(actionLayersView);
    viewActions->addAction(actionRenderingView);
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

    cursor_normal->setDefaultAction(actionCursorNormal);
    cursor_spring->setDefaultAction(actionCursorSpring);
    cursor_delay->setDefaultAction(actionCursorDelay);
    cursor_axis->setDefaultAction(actionCursorAxis);
    cursor_line->setDefaultAction(actionCursorLine);
    cursor_fuzzy->setDefaultAction(actionCursorFuzzy);

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
    nextSession->setShortcut(QKeySequence("Ctrl+PgDown"));
    addAction(nextSession);
    QAction *prevSession = new QAction("Previous Session", this);
    prevSession->setShortcut(QKeySequence("Ctrl+PgUp"));
    addAction(prevSession);

    // HIDDEN actions
    // for debugging and development purposes
    QAction *screenshot = new QAction("Screenshot", this);
    screenshot->setShortcut(QKeySequence("Ctrl+<,<"));
    addAction(screenshot);
    QObject::connect(screenshot, SIGNAL(triggered()), this, SLOT(screenshotView() ) );

    QAction *setGLSLFragmentShader = new QAction("setGLSLFragmentShader", this);
    setGLSLFragmentShader->setShortcut(QKeySequence("Shift+Ctrl+G,F"));
    addAction(setGLSLFragmentShader);
    QObject::connect(setGLSLFragmentShader, SIGNAL(triggered()), this, SLOT(selectGLSLFragmentShader()) );

    showlog->setShortcut(QKeySequence("Ctrl+L"));

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

    // Setup the property browsers
    specificSourcePropertyBrowser = NULL;
    layoutPropertyBrowser = new QSplitter(Qt::Vertical, this);
    SourcePropertyBrowser *sourcePropertyBrowser = RenderingManager::getPropertyBrowserWidget();
    layoutPropertyBrowser->addWidget(sourcePropertyBrowser);
//    QObject::connect(this, SIGNAL(sourceMarksModified(bool)), sourcePropertyBrowser, SLOT(updateMarksProperties(bool) ) );
    QObject::connect(sourcePropertyBrowser, SIGNAL(changed(Source*)), this, SLOT(sourceChanged(Source*) ) );
    sourceDockWidgetContentsLayout->addWidget(layoutPropertyBrowser);

    // setup the mixing toolbox
    mixingToolBox = new MixingToolboxWidget(this);
    mixingDockWidgetContentLayout->addWidget(mixingToolBox);
    QObject::connect(RenderingManager::getInstance(), SIGNAL(currentSourceChanged(SourceSet::iterator)), mixingToolBox, SLOT(connectSource(SourceSet::iterator) ) );
    QObject::connect(mixingToolBox, SIGNAL( sourceChanged(SourceSet::iterator)), RenderingManager::getInstance(), SIGNAL(currentSourceChanged(SourceSet::iterator)) );
    // bidirectional link between mixing toolbox and property manager
    QObject::connect(mixingToolBox, SIGNAL(valueChanged(QString, bool)), sourcePropertyBrowser, SLOT(valueChanged(QString, bool)) );
    QObject::connect(mixingToolBox, SIGNAL(valueChanged(QString, int)), sourcePropertyBrowser, SLOT(valueChanged(QString, int)) );
    QObject::connect(mixingToolBox, SIGNAL(valueChanged(QString, const QColor &)), sourcePropertyBrowser, SLOT(valueChanged(QString, const QColor &)) );
    QObject::connect(mixingToolBox, SIGNAL(enumChanged(QString, int)), sourcePropertyBrowser, SLOT(enumChanged(QString, int)) );
    QObject::connect(sourcePropertyBrowser, SIGNAL(propertyChanged(QString, bool)), mixingToolBox, SLOT(propertyChanged(QString, bool)) );
    QObject::connect(sourcePropertyBrowser, SIGNAL(propertyChanged(QString, int)), mixingToolBox, SLOT(propertyChanged(QString, int)) );
    QObject::connect(sourcePropertyBrowser, SIGNAL(propertyChanged(QString, const QColor &)), mixingToolBox, SLOT(propertyChanged(QString, const QColor &)) );

    // setup the layout toolbox
    layoutToolBox = new LayoutToolboxWidget(this);
    layoutDockWidgetContentLayout->addWidget(layoutToolBox);

    // Setup the session switcher toolbox
    switcherSession = new SessionSwitcherWidget(this, &settings);
    switcherDockWidgetContentsLayout->addWidget(switcherSession);
    QObject::connect(switcherSession, SIGNAL(sessionTriggered(QString)), this, SLOT(switchToSessionFile(QString)) );
    QObject::connect(this, SIGNAL(sessionSaved()), switcherSession, SLOT(updateFolder()) );
    QObject::connect(this, SIGNAL(sessionLoaded()), switcherSession, SLOT(unsuspend()));
    QObject::connect(RenderingManager::getSessionSwitcher(), SIGNAL(transitionSourceChanged(Source *)), switcherSession, SLOT(setTransitionSourcePreview(Source *)));

    QObject::connect(nextSession, SIGNAL(triggered()), switcherSession, SLOT(startTransitionToNextSession()));
    QObject::connect(prevSession, SIGNAL(triggered()), switcherSession, SLOT(startTransitionToPreviousSession()));

    // Set the docking tab vertical
    setDockOptions(dockOptions () | QMainWindow::VerticalTabs);

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

    // setup render window
    QIcon icon;
    icon.addFile(QString::fromUtf8(":/glmixer/icons/glmixer.png"), QSize(), QIcon::Normal, QIcon::Off);
    setWindowIcon(icon);
    OutputRenderWindow::getInstance()->setWindowIcon(icon);
    QObject::connect(OutputRenderWindow::getInstance(), SIGNAL(keyRightPressed()), switcherSession, SLOT(startTransitionToNextSession()));
    QObject::connect(OutputRenderWindow::getInstance(), SIGNAL(keyLeftPressed()), switcherSession, SLOT(startTransitionToPreviousSession()));

    // Setup dialogs
    mfd = new VideoFileDialog(this, "Open videos or pictures");
    Q_CHECK_PTR(mfd);
    sfd = new QFileDialog(this);
    Q_CHECK_PTR(sfd);
    sfd->setOption(QFileDialog::DontUseNativeDialog, true);
    sfd->setFilter( QDir::AllEntries | QDir::NoDotAndDotDot);
    upd = new UserPreferencesDialog(this);
    Q_CHECK_PTR(upd);

    // Create output preview widget
    outputpreview = new OutputRenderWidget(previewDockWidgetContents, mainRendering);
    Q_CHECK_PTR(outputpreview);
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
    QObject::connect(actionPause, SIGNAL(toggled(bool)), vcontrolDockWidget, SLOT(setDisabled(bool)));
#ifdef SHM
    QObject::connect(actionShareToRAM, SIGNAL(toggled(bool)), RenderingManager::getInstance(), SLOT(setFrameSharingEnabled(bool)));
#endif
#ifdef SPOUT
    QObject::connect(actionShareToSPOUT, SIGNAL(triggered(bool)), RenderingManager::getInstance(), SLOT(setSpoutSharingEnabled(bool)));
    QObject::connect(RenderingManager::getInstance(), SIGNAL(spoutSharingEnabled(bool)), actionShareToSPOUT, SLOT(setChecked(bool)));
#endif
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

    // connect recorder to disable many actions, like quitting, opening session, preferences, etc.
    QObject::connect(RenderingManager::getRecorder(), SIGNAL(activated(bool)), actionNew_Session, SLOT(setDisabled(bool)));
    QObject::connect(RenderingManager::getRecorder(), SIGNAL(activated(bool)), actionClose_Session, SLOT(setDisabled(bool)));
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
    QObject::connect(sliderZoom, SIGNAL(valueChanged(int)), RenderingManager::getRenderingWidget(), SLOT(zoom(int)));

    QObject::connect(RenderingManager::getRenderingWidget(), SIGNAL(zoomPercentChanged(int)), sliderZoom, SLOT(setValue(int)));

    QObject::connect(actionZoomIn, SIGNAL(triggered()), RenderingManager::getRenderingWidget(), SLOT(zoomIn()));
    QObject::connect(actionZoomOut, SIGNAL(triggered()), RenderingManager::getRenderingWidget(), SLOT(zoomOut()));
    QObject::connect(actionZoomReset, SIGNAL(triggered()), RenderingManager::getRenderingWidget(), SLOT(zoomReset()));
    QObject::connect(actionZoomBestFit, SIGNAL(triggered()), RenderingManager::getRenderingWidget(), SLOT(zoomBestFit()));
    QObject::connect(actionZoomCurrentSource, SIGNAL(triggered()), RenderingManager::getRenderingWidget(), SLOT(zoomCurrentSource()));
//    QObject::connect(actionToggle_fixed, SIGNAL(triggered()), RenderingManager::getInstance(), SLOT(toggleMofifiableCurrentSource()));
    QObject::connect(actionUnchangeable, SIGNAL(toggled(bool)), RenderingManager::getInstance(), SLOT(toggleUnchangeableCurrentSource(bool)));
    QObject::connect(actionAspectRatioResetOriginal, SIGNAL(triggered()), RenderingManager::getInstance(), SLOT(setOriginalAspectRatioCurrentSource()));
    QObject::connect(actionAspectRatioSetRendering, SIGNAL(triggered()), RenderingManager::getInstance(), SLOT(setRenderingAspectRatioCurrentSource()));
    QObject::connect(actionAspectRatioFixed , SIGNAL(toggled(bool)), RenderingManager::getInstance(), SLOT(toggleFixAspectRatioCurrentSource(bool)));
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

    // connect actions with selectionManager
    QObject::connect(actionSelectAll, SIGNAL(triggered()), SelectionManager::getInstance(), SLOT(selectAll()));
    QObject::connect(actionSelectInvert, SIGNAL(triggered()), SelectionManager::getInstance(), SLOT(invertSelection()));
    QObject::connect(actionSelectCurrent, SIGNAL(triggered()), SelectionManager::getInstance(), SLOT(selectCurrentSource()));
    QObject::connect(actionSelectNone, SIGNAL(triggered()), SelectionManager::getInstance(), SLOT(clearSelection()));

    QObject::connect(SelectionManager::getInstance(), SIGNAL(selectionChanged(bool)), this, SLOT(updateStatusControlActions()));
    QObject::connect(SelectionManager::getInstance(), SIGNAL(selectionChanged(bool)), actionSelectInvert, SLOT(setEnabled(bool)));
    QObject::connect(SelectionManager::getInstance(), SIGNAL(selectionChanged(bool)), actionSelectNone, SLOT(setEnabled(bool)));


    // a Timer to update sliders and counters
    frameSlider->setTracking(true);
    refreshTimingTimer = new QTimer(this);
    Q_CHECK_PTR(refreshTimingTimer);
    refreshTimingTimer->setInterval(150);
    QObject::connect(refreshTimingTimer, SIGNAL(timeout()), this, SLOT(refreshTiming()));
    QObject::connect(vcontrolDockWidget, SIGNAL(visibilityChanged(bool)), this, SLOT(updateRefreshTimerState()));
    frameForwardButton->setHidden(true);

    // start with new file
    currentSessionFileName = QString::null;
    confirmSessionFileName();

}

GLMixer::~GLMixer()
{
    delete mfd;
    delete sfd;
    delete upd;

    refreshTimingTimer->stop();
    delete refreshTimingTimer;

    mixingToolBox->close();
    delete mixingToolBox;

    switcherSession->close();
    delete switcherSession;

    outputpreview->close();
    delete outputpreview;

}


void GLMixer::exitHandler() {

    // no message handling when quit
    qInstallMsgHandler(0);

    // save window settings
    _instance->saveSettings();

    RenderingManager::deleteInstance();
    OutputRenderWindow::deleteInstance();
#ifdef SHM
    SharedMemoryManager::deleteInstance();
#endif
    if (_instance)
        delete _instance;

}

void GLMixer::closeEvent ( QCloseEvent * event ){

    qApp->closeAllWindows ();
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

#ifndef NDEBUG
    qDebug() << VideoPicture::createdVideoPictureCount << " VideoPictures created";
    qDebug() << VideoPicture::deletedVideoPictureCount << " VideoPictures deleted";
    qDebug() << VideoFile::allocatedPacketQueueCount << " PacketQueue created";
    qDebug() << VideoFile::freePacketQueueCount << " PacketQueue deleted";
    qDebug() << VideoFile::allocatedPacketListCount << " Packet List created";
    qDebug() << VideoFile::freePacketListCount << " Packet list deleted";
#endif

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

void GLMixer::on_saveLogsToFile_clicked() {

    if (logTexts->topLevelItemCount() > 0) {

        QString suggestion = QString("glmixerlogs %1%2").arg(QDate::currentDate().toString("yyMMdd")).arg(QTime::currentTime().toString("hhmmss"));
        QString fileName = getFileName(tr("Save Logs to file"),
                                       tr("GLMixer logs") + " (*.txt)",
                                       QString("txt"),
                                       suggestion);

        if ( !fileName.isEmpty() ) {
            // open file and put text into it
            QFile fileContent(fileName);
            fileContent.open(QIODevice::WriteOnly | QIODevice::Text);
            QTextStream out(&fileContent);

            QTreeWidgetItemIterator it(logTexts->topLevelItem(0));
            while (*it) {
                out << (*it)->text(0) << " : " << (*it)->text(1) << "\n";
                ++it;
            }

        }
    }
}


void GLMixer::on_copyNotes_clicked() {

    QApplication::clipboard()->setText( blocNoteEdit->toPlainText() );
}

void GLMixer::on_addDateToNotes_clicked() {

    blocNoteEdit->append(QDateTime::currentDateTime().toString("dddd d MMMM yyyy"));
}

void GLMixer::on_addListToNotes_clicked() {

    QString list;
    for(SourceList::iterator  its = RenderingManager::getInstance()->getBegin(); its != RenderingManager::getInstance()->getEnd(); its++)
        list.append(QString("-  %1\n").arg( (*its)->getName() ));

    blocNoteEdit->append(list);
}


void GLMixer::msgHandler(QtMsgType type, const char *msg)
{
    QString txt = QString(msg).remove("\"");
    // message handler
    switch (type) {
    case QtCriticalMsg:
        {
            // create message box
            QMessageBox msgBox(QMessageBox::Warning, tr("Warning"),  tr("<b>The application %1 encountered a problem.</b>").arg(QCoreApplication::applicationName()), QMessageBox::Ok);
            QStringList message = txt.split(QChar(124), QString::SkipEmptyParts);
            QString displaytext = txt;
            if (message.count() > 1 ) {
                displaytext = message[1].simplified();
                if ( !message[0].simplified().isEmpty() )
                    displaytext += tr("\n\nOrigin of the problem:\n") + message[0].simplified();
            } else if (message.count() > 0 )
                displaytext = message[0].simplified();

            // add button to show logs
            QPushButton *logButton = NULL;
            if (_instance)
                logButton = msgBox.addButton(tr("Check logs"), QMessageBox::ActionRole);

            // exec message box
            msgBox.setInformativeText(displaytext);
            msgBox.setDefaultButton(QMessageBox::Ok);
            msgBox.exec();

            // show logs if required
            if ( msgBox.clickedButton() == logButton )
                 _instance->logDockWidget->setVisible(true);

        }
        break;
    case QtFatalMsg:
        {
            QMessageBox msgBox(QMessageBox::Warning, tr("Error"),
                           tr("<b>The application %1 crashed.</b>").arg(QCoreApplication::applicationName()), QMessageBox::Ok);
            msgBox.setInformativeText(txt.simplified() + tr("\n\nThe program will stop now."));
            msgBox.setDefaultButton(QMessageBox::Ok);
            msgBox.exec();
            abort();
        }
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
 //   else QMessageBox::information(0, tr("%1 -- Debug").arg(QCoreApplication::applicationName()), txt);

}

void GLMixer::Log(int type, QString msg)
{
    // create log entry
    QTreeWidgetItem *item  = new QTreeWidgetItem();
    logTexts->addTopLevelItem( item );

    // reads the text passed and split into object|message
    QStringList message = msg.split(QChar(124), QString::SkipEmptyParts);
    if (message.count() > 1 ) {
        item->setText(0, message[1].simplified());
        item->setToolTip(0, message[1].simplified());
        item->setText(1, message[0].simplified());
    } else if (message.count() > 0 ) {
        item->setText(0, message[0].simplified());
        item->setToolTip(0, message[0].simplified());
        item->setIcon(1, QIcon(":/glmixer/icons/info.png"));
    } else {
        item->setText(0, "");
        item->setIcon(1, QIcon(":/glmixer/icons/info.png"));
    }

    // adjust color and show dialog according to message type
    switch ( (QtMsgType) type) {
    case QtWarningMsg:
         item->setBackgroundColor(0, QColor(220, 180, 50, 50));
         item->setBackgroundColor(1, QColor(220, 180, 50, 50));
         item->setIcon(1, QIcon(":/glmixer/icons/warning.png"));
         break;
    case QtCriticalMsg:
        item->setBackgroundColor(0, QColor(220, 90, 50, 50));
        item->setBackgroundColor(1, QColor(220, 90, 50, 50));
        item->setIcon(1, QIcon(":/glmixer/icons/warning.png"));
        break;
    default:
        break;
    }

    // auto scroll to new item
    logTexts->setCurrentItem( item );
    logTexts->setColumnWidth(0, logTexts->width() * 70 / 100);
}

void GLMixer::setView(QAction *a){

    // setup the rendering Widget to the requested view
    if (a == actionMixingView) {
        RenderingManager::getRenderingWidget()->setViewMode(View::MIXING);
        layoutToolBox->setViewMode(View::MIXING);
    } else if (a == actionGeometryView) {
        RenderingManager::getRenderingWidget()->setViewMode(View::GEOMETRY);
        layoutToolBox->setViewMode(View::GEOMETRY);
    } else if (a == actionLayersView) {
        RenderingManager::getRenderingWidget()->setViewMode(View::LAYER);
        layoutToolBox->setViewMode(View::LAYER);
    } else if (a == actionRenderingView) {
        RenderingManager::getRenderingWidget()->setViewMode(View::RENDERING);
        layoutToolBox->setViewMode(View::RENDERING);
    }

    // show appropriate icon
    viewIcon->setPixmap(RenderingManager::getRenderingWidget()->getView()->getIcon());
    viewLabel->setText(RenderingManager::getRenderingWidget()->getView()->getTitle());

//	// disable / enable catalog view depending on the view
//	actionShow_Catalog->setEnabled(a == actionMixingView || a == actionGeometryView);
//	RenderingManager::getRenderingWidget()->setCatalogVisible(actionShow_Catalog->isEnabled() && actionShow_Catalog->isChecked() );

    // tools available in corresponding views
    toolsToolBar->setEnabled(a != actionRenderingView);

    // get back the proper tool from former usage
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

void GLMixer::on_actionNewSource_triggered(){

    static NewSourceDialog *nsd = new NewSourceDialog(this);

    if (nsd->exec() == QDialog::Accepted) {

        switch (nsd->selectedType()) {
        case Source::VIDEO_SOURCE:
            actionMediaSource->trigger();
            break;
#ifdef OPEN_CV
        case Source::CAMERA_SOURCE:
            actionCameraSource->trigger();
            break;
#endif
        case Source::ALGORITHM_SOURCE:
            actionAlgorithmSource->trigger();
            break;
        case Source::RENDERING_SOURCE:
            actionRenderingSource->trigger();
            break;
        case Source::CAPTURE_SOURCE:
            actionCaptureSource->trigger();
            break;
        case Source::SVG_SOURCE:
            actionSvgSource->trigger();
            break;
#ifdef SHM
        case Source::SHM_SOURCE:
            actionShmSource->trigger();
            break;
#endif
#ifdef FFGL
        case Source::FFGL_SOURCE:
            actionFreeframeSource->trigger();
            break;
#endif
        case Source::WEB_SOURCE:
            actionWebSource->trigger();
            break;
        default:
            break;
        }

    }
}


void GLMixer::on_actionMediaSource_triggered(){

    bool generatePowerOfTwoRequested = false;
    QStringList fileNames = getMediaFileNames(generatePowerOfTwoRequested);

    // open all files from the list
    QStringListIterator fileNamesIt(fileNames);
    while (fileNamesIt.hasNext()){

        VideoFile *newSourceVideoFile = NULL;
        QString filename = fileNamesIt.next();

        if ( QFileInfo(filename).isFile() ){

            // if the dialog did not request power of two generation of textures
            // and if the opengl supports the extension, then open the source normally
            if ( !generatePowerOfTwoRequested && (glSupportsExtension("GL_EXT_texture_non_power_of_two") || glSupportsExtension("GL_ARB_texture_non_power_of_two") ) )
                newSourceVideoFile = new VideoFile(this);
            else
                newSourceVideoFile = new VideoFile(this, true, SWS_POINT);

            // if the video file was created successfully
            if (newSourceVideoFile){
                // can we open the file ?
                if ( newSourceVideoFile->open( filename ) ) {
                    Source *s = RenderingManager::getInstance()->newMediaSource(newSourceVideoFile);
                    // create the source as it is a valid video file (this also set it to be the current source)
                    if ( s ) {
                        RenderingManager::getInstance()->addSourceToBasket(s);
                        qDebug() << s->getName() << QChar(124).toLatin1() << tr("New media source created with file ") << filename;
                    } else {
                        qCritical() << filename <<  QChar(124).toLatin1() << tr("Could not create media source with this file.");
                        delete newSourceVideoFile;
                    }
                } else {
                    qCritical() << filename << QChar(124).toLatin1() << tr("The file could not be loaded.");
                    delete newSourceVideoFile;
                }
            }
            else
                qCritical() << filename << QChar(124).toLatin1() << tr("The file could not be opened.");

        }
    }

    if (RenderingManager::getInstance()->getSourceBasketSize() > 0)
        statusbar->showMessage( tr("%1 media source(s) created; you can now drop them.").arg( RenderingManager::getInstance()->getSourceBasketSize() ), 3000 );
}


// method called when a source is made current (either after loading, either after clicking on it).
// The goal is to have the GUI display the current state of the video file to be able to control the video playback
// and to read the correct information and configuration options
void GLMixer::connectSource(SourceSet::iterator csi){

    static QByteArray layoutPropertyBrowserState;
    if (specificSourcePropertyBrowser) {
        layoutPropertyBrowserState = layoutPropertyBrowser->saveState();
        delete specificSourcePropertyBrowser;
        specificSourcePropertyBrowser = NULL;
    }

    // whatever happens, we will drop the control on the current source
    //   (this slot is called by MainRenderWidget through signal currentSourceChanged
    //    which is sent ONLY when the current source is changed)
    if (selectedSourceVideoFile) {

        QObject::disconnect(selectedSourceVideoFile, SIGNAL(running(bool)), startButton, SLOT(setChecked(bool)));
        QObject::disconnect(selectedSourceVideoFile, SIGNAL(paused(bool)), pauseButton, SLOT(setChecked(bool)));
        QObject::disconnect(selectedSourceVideoFile, SIGNAL(running(bool)), videoFrame, SLOT(setEnabled(bool)));
        QObject::disconnect(selectedSourceVideoFile, SIGNAL(running(bool)), timingControlFrame, SLOT(setEnabled(bool)));
        QObject::disconnect(selectedSourceVideoFile, SIGNAL(playSpeedChanged(double)), playSpeedDisplay, SLOT(display(double)) );
        QObject::disconnect(selectedSourceVideoFile, SIGNAL(playSpeedFactorChanged(int)), playSpeedSlider, SLOT(setValue(int)) );
        QObject::disconnect(selectedSourceVideoFile, SIGNAL(markingChanged()), this, SLOT(updateMarks()));
        QObject::disconnect(selectedSourceVideoFile, SIGNAL(running(bool)), this, SLOT(updateRefreshTimerState()));
        QObject::disconnect(selectedSourceVideoFile, SIGNAL(seekEnabled(bool)), this, SLOT(enableSeek(bool)));

        // disconnect control buttons
        QObject::disconnect(pauseButton, SIGNAL(toggled(bool)), selectedSourceVideoFile, SLOT(pause(bool)));
        QObject::disconnect(playSpeedSlider, SIGNAL(valueChanged(int)), selectedSourceVideoFile, SLOT(setPlaySpeedFactor(int)));
        QObject::disconnect(playSpeedReset, SIGNAL(clicked()), selectedSourceVideoFile, SLOT(resetPlaySpeed()));
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

        // display source preview
        sourcePreview->setSource(*csi);
        vcontrolDockWidgetContents->setEnabled( true );

        // display property browser
        specificSourcePropertyBrowser = createSpecificPropertyBrowser((*csi), layoutPropertyBrowser);
        specificSourcePropertyBrowser->setHeaderVisible(false);
        layoutPropertyBrowser->insertWidget(1, specificSourcePropertyBrowser);
        layoutPropertyBrowser->restoreState(layoutPropertyBrowserState);

        // enable properties and actions on the current valid source
        sourceDockWidgetContents->setEnabled(true);
        currentSourceMenu->setEnabled(true);
        actionCloneSource->setEnabled(true);
        toolButtonZoomCurrent->setEnabled(true);
        mixingToolBox->setEnabled(true);
        actionAspectRatioFixed->setChecked( (*csi)->isFixedAspectRatio() );
        actionUnchangeable->setChecked( !(*csi)->isModifiable() );

        // Enable control pannels if the source is playable
        vcontrolOptionSplitter->setEnabled( (*csi)->isPlayable() );

        // except for media source, these panels are disabled
        vcontrolDockWidgetOptions->setEnabled(false);
        videoFrame->setEnabled(false);
        timingControlFrame->setEnabled(false);

        // Set the status of start button without circular call to startCurrentSource
        QObject::disconnect(startButton, SIGNAL(toggled(bool)), this, SLOT(on_startButton_toogled(bool)));
        startButton->setChecked( (*csi)->isPlaying() );
        QObject::connect(startButton, SIGNAL(toggled(bool)), this, SLOT(on_startButton_toogled(bool)));

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
                playSpeedSlider->setValue(selectedSourceVideoFile->getPlaySpeedFactor());
                playSpeedDisplay->display(selectedSourceVideoFile->getPlaySpeed());

                // CONTROL signals from GUI to VideoFile
                QObject::connect(pauseButton, SIGNAL(toggled(bool)), selectedSourceVideoFile, SLOT(pause(bool)));
                QObject::connect(playSpeedSlider, SIGNAL(valueChanged(int)), selectedSourceVideoFile, SLOT(setPlaySpeedFactor(int)));
                QObject::connect(playSpeedReset, SIGNAL(clicked()), selectedSourceVideoFile, SLOT(resetPlaySpeed()));
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
                QObject::connect(selectedSourceVideoFile, SIGNAL(running(bool)), startButton, SLOT(setChecked(bool)));
                QObject::connect(selectedSourceVideoFile, SIGNAL(paused(bool)), pauseButton, SLOT(setChecked(bool)));
                QObject::connect(selectedSourceVideoFile, SIGNAL(running(bool)), videoFrame, SLOT(setEnabled(bool)));
                QObject::connect(selectedSourceVideoFile, SIGNAL(running(bool)), timingControlFrame, SLOT(setEnabled(bool)));
                QObject::connect(selectedSourceVideoFile, SIGNAL(playSpeedChanged(double)), playSpeedDisplay, SLOT(display(double)) );
                QObject::connect(selectedSourceVideoFile, SIGNAL(playSpeedFactorChanged(int)), playSpeedSlider, SLOT(setValue(int)) );

                // Consistency and update timer control from VideoFile
                QObject::connect(selectedSourceVideoFile, SIGNAL(markingChanged()), this, SLOT(updateMarks()));
                QObject::connect(selectedSourceVideoFile, SIGNAL(running(bool)), this, SLOT(updateRefreshTimerState()));
                QObject::connect(selectedSourceVideoFile, SIGNAL(seekEnabled(bool)), this, SLOT(enableSeek(bool)));

            } // end video file
        } // end video source
    } else {  // it is not a valid source

        // disable control panel widgets
        vcontrolDockWidgetContents->setEnabled(false);
        vcontrolOptionSplitter->setEnabled( false );
        startButton->setChecked( false );

        // disable source related toolboxes
        mixingToolBox->setEnabled(false);
        sourceDockWidgetContents->setEnabled(false);

        // disable options
        currentSourceMenu->setEnabled(false);
        actionCloneSource->setEnabled(false);
        toolButtonZoomCurrent->setEnabled(false);
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
                    qDebug() << s->getName() << QChar(124).toLatin1() << tr("OpenCV device source %1 was cloned.").arg(cs->getOriginalName());
                    statusbar->showMessage( tr("The device source %1 was cloned.").arg(cs->getOriginalName()), 3000 );
                } else {
                    qDebug() << s->getName() << QChar(124).toLatin1() << tr("New OpenCV source created (device index %2).").arg(selectedCamIndex);
                    statusbar->showMessage( tr("Source created with OpenCV drivers for Camera %1").arg(selectedCamIndex), 3000 );
                }
            } else
                qCritical() << tr("Could not open OpenCV device index %2. ").arg(selectedCamIndex);

        }
    }
#endif
}

void GLMixer::on_actionWebSource_triggered(){

    // popup a question dialog to select the type of algorithm
    static WebSourceCreationDialog *webd = 0;
    if (!webd)
        webd = new WebSourceCreationDialog(this, &settings);


    if (webd->exec() == QDialog::Accepted) {

        int h = webd->getSelectedHeight();
        int c = webd->getSelectedScroll();
        int u = webd->getSelectedUpdate();
        QUrl web = webd->getSelectedUrl();

        Source *s = RenderingManager::getInstance()->newWebSource(web, h, c, u);
        if ( s ){
            RenderingManager::getInstance()->addSourceToBasket(s);
            qDebug() << s->getName() <<  QChar(124).toLatin1() << tr("New Web source created with location ")<< web.toString();
            statusbar->showMessage( tr("Source created with the web location %1.").arg( web.toString() ), 3000 );
        } else
            qCritical() << web.toString() <<  QChar(124).toLatin1() << tr("Could not create a web source with this location.");

    }

}

void GLMixer::on_actionSvgSource_triggered(){

    QString fileName = getFileName(tr("Open SVG file"), tr("Scalable Vector Graphics") + " (*.svg)");

    if ( !fileName.isEmpty() ) {

        QFileInfo file(fileName);
        if ( !file.isFile() || !file.isReadable()) {
            qCritical() << fileName <<  QChar(124).toLatin1() << tr("File does not exist or is unreadable.");
            return;
        }

        QSvgRenderer *svg = new QSvgRenderer(fileName);

        Source *s = RenderingManager::getInstance()->newSvgSource(svg);
        if ( s ){
            RenderingManager::getInstance()->addSourceToBasket(s);
            qDebug() << s->getName() <<  QChar(124).toLatin1() << tr("New vector Graphics source created with file ")<< fileName;
            statusbar->showMessage( tr("Source created with the vector graphics file %1.").arg( fileName ), 3000 );
        } else
            qCritical() << fileName <<  QChar(124).toLatin1() << tr("Could not create a vector graphics source with this file.");
    }
}


void GLMixer::on_actionShmSource_triggered(){

#ifdef SHM
    // popup a question dialog to select the shared memory block
    static SharedMemoryDialog *shmd = 0;
    if(!shmd)
        shmd = new SharedMemoryDialog(this);

    if (shmd->exec() == QDialog::Accepted) {
        Source *s = RenderingManager::getInstance()->newSharedMemorySource(shmd->getSelectedId());
        if ( s ){
            RenderingManager::getInstance()->addSourceToBasket(s);
            qDebug() << s->getName() <<  QChar(124).toLatin1() <<  tr("New shared memory source created (")<< shmd->getSelectedProcess() << ").";
            statusbar->showMessage( tr("Source created with the process %1.").arg( shmd->getSelectedProcess() ), 3000 );
        } else
            qCritical() << shmd->getSelectedProcess() <<  QChar(124).toLatin1() << tr("Could not create shared memory source.");
    }
#endif
}

#ifdef FFGL

void GLMixer::on_actionFreeframeSource_triggered(){

    // popup a question dialog to select the type of algorithm
    static FFGLSourceCreationDialog *ffgld = 0;
    if (!ffgld)
        ffgld = new FFGLSourceCreationDialog(this, &settings);


    if (ffgld->exec() == QDialog::Accepted) {

        int w = ffgld->getSelectedWidth();
        int h = ffgld->getSelectedHeight();
        QDomElement config = ffgld->getFreeframePluginConfiguration();
        if ( config.hasChildNodes() ) {

            Source *s = RenderingManager::getInstance()->newFreeframeGLSource(config, w, h);
            FFGLSource *ps = dynamic_cast<FFGLSource *> (s);

            // successful creation of a FFGLSource ?
            if ( s && ps ){

                // show status with different message depending on type : try to cast
                FFGLPluginSourceShadertoy *st = qobject_cast<FFGLPluginSourceShadertoy *> ( ps->freeframeGLPlugin() );

                // if it is a Shadertoy
                if (st) {

                    // open editor for shadertoy code
                    editShaderToyPlugin(st);

                    // shadertoy info
                    qDebug() << s->getName() << QChar(124).toLatin1() << tr("New Shadertoy plugin source created.");
                    statusbar->showMessage( tr("Shadertoy plugin source %1 created.").arg( s->getName() ), 3000 );
                }
                // if it is a Freeframe plugin
                else {
                    // freeframe info (filename)
                    qDebug() << s->getName() << QChar(124).toLatin1() << tr("New freeframe plugin source created with file ") << ps->freeframeGLPlugin()->fileName() ;
                    statusbar->showMessage( tr("Freeframe plugin source %1 created.").arg( s->getName() ), 3000 );
                }

                // add source
                RenderingManager::getInstance()->addSourceToBasket(s);
            }
            else
                qCritical() << config.tagName() <<  QChar(124).toLatin1() << tr("Could not create plugin source.");
        }
    }

}



void GLMixer::editShaderToyPlugin(FFGLPluginSource *plugin)
{
    if(!plugin)
        return;

    // test if it is a shadertoy plugin
    if( plugin->rtti() == FFGLPluginSource::SHADERTOY_PLUGIN) {

        // show the Shadertoy code editor
        pluginGLSLCodeEditor->show();
        pluginGLSLCodeEditor->raise();
        pluginGLSLCodeEditor->setFocus();

        // link plugin to GUI editor
        pluginGLSLCodeEditor->linkPlugin(qobject_cast<FFGLPluginSourceShadertoy *>(plugin));

        // update list in case the plugin changed (e.g. name)
        connect(plugin, SIGNAL(changed()), mixingToolBox, SLOT(changed()));

    }

}

#endif

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
            qDebug() << s->getName() <<  QChar(124).toLatin1() << tr("New Algorithm source created (")<< AlgorithmSource::getAlgorithmDescription(asd->getSelectedAlgorithmIndex()) << ").";
            statusbar->showMessage( tr("Source created with the algorithm %1.").arg( AlgorithmSource::getAlgorithmDescription(asd->getSelectedAlgorithmIndex())), 3000 );
        } else
            qCritical() << AlgorithmSource::getAlgorithmDescription(asd->getSelectedAlgorithmIndex()) <<  QChar(124).toLatin1() << tr("Could not create algorithm source.");
    }
}


void GLMixer::on_actionRenderingSource_triggered(){

    Source *s = RenderingManager::getInstance()->newRenderingSource();
    if ( s ){
        RenderingManager::getInstance()->addSourceToBasket(s);
        qDebug() << s->getName() <<  QChar(124).toLatin1() << tr("New rendering loopback source created.");
        statusbar->showMessage( tr("Source created with the rendering output loopback."), 3000 );
    } else
        qCritical() << tr("Could not create rendering loopback source.");
}


void GLMixer::on_actionCloneSource_triggered(){

    if ( RenderingManager::getInstance()->notAtEnd(RenderingManager::getInstance()->getCurrentSource()) ) {
        SourceSet::iterator original = RenderingManager::getInstance()->getCurrentSource();
        Source *s = RenderingManager::getInstance()->newCloneSource(original);
        if ( s ) {
            QString name = (*RenderingManager::getInstance()->getCurrentSource())->getName();

#ifdef FFGL
        // copy the Freeframe plugin stack
        for (FFGLPluginSourceStack::const_iterator it = (*original)->getFreeframeGLPluginStack()->begin();
             it != (*original)->getFreeframeGLPluginStack()->end(); ++it) {

            FFGLPluginSource *plugin = s->addFreeframeGLPlugin( (*it)->fileName() );
            // set configuration
            if (plugin)
                plugin->setConfiguration( (*it)->getConfiguration() );
        }
#endif

            RenderingManager::getInstance()->addSourceToBasket(s);
            qDebug() << s->getName() <<  QChar(124).toLatin1() << tr("New clone of source %1 created.").arg(name);
            statusbar->showMessage( tr("The current source has been cloned."), 3000);
        } else
            qCritical() << (*RenderingManager::getInstance()->getCurrentSource())->getName()<< QChar(124).toLatin1() << tr("Could not clone source %1.");
    }
}




void GLMixer::on_actionCaptureSource_triggered(){

    // capture screen
    QImage capture = RenderingManager::getInstance()->captureFrameBuffer(QImage::Format_RGB32);

    // display and request action with this capture
    CaptureDialog cd(this, capture, tr("Create a source with this image ?"));

    if (cd.exec() == QDialog::Accepted) {

        Source *s = RenderingManager::getInstance()->newCaptureSource(cd.img);
        if ( s ){
            RenderingManager::getInstance()->addSourceToBasket(s);
            qDebug() << s->getName()<< QChar(124).toLatin1() << tr("New capture source created.");
        }
        else
            qCritical() << tr("Could not create capture source.");
    }
}


void GLMixer::on_actionCopy_snapshot_triggered(){

    // capture screen
    QImage capture = RenderingManager::getInstance()->captureFrameBuffer();
    QApplication::clipboard()->setPixmap( QPixmap::fromImage(capture) );

    statusbar->showMessage( tr("Current frame captured and copied to clipboard."), 3000 );
}

void GLMixer::on_actionSave_snapshot_triggered(){

    // capture screen
    QImage capture = RenderingManager::getInstance()->captureFrameBuffer();

    // display and request action with this capture
    CaptureDialog cd(this, capture, tr("Save this image ?"));

    if (cd.exec() == QDialog::Accepted) {

        QString suggestion = QString("glmixerimage %1%2").arg(QDate::currentDate().toString("yyMMdd")).arg(QTime::currentTime().toString("hhmmss"));

        QString fileName = getFileName(tr("Save snapshot"),
                                       tr("Image") + " (*.png *.jpg *.tiff *.xpm)",
                                       QString("png"), suggestion);

        if ( !fileName.isEmpty() ) {
            if (!capture.save(fileName)) {
                qCritical() << fileName << QChar(124).toLatin1()<< tr("Could not save file.");
            } else
                qDebug() << fileName << QChar(124).toLatin1() << tr("Snapshot saved.");
        }

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
    else if ( SelectionManager::getInstance()->hasSelection() )
        // make a copy of the selection (to make sure we do not mess with pointers when removing from selection)
        todelete = SelectionManager::getInstance()->copySelection();

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


void GLMixer::on_markInSlider_sliderReleased (){

    double percent = (double)(markInSlider->value())/(double)frameSlider->maximum();
    double pos = selectedSourceVideoFile->getEnd() * percent ;
    pos += selectedSourceVideoFile->getBegin();
    selectedSourceVideoFile->setMarkIn(pos);
}

void GLMixer::on_markOutSlider_sliderReleased (){

    double percent = (double)(markOutSlider->value())/(double)frameSlider->maximum();
    double pos =  selectedSourceVideoFile->getEnd() * percent ;
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

    int f_percent = (int) ( ( selectedSourceVideoFile->getCurrentFrameTime() - selectedSourceVideoFile->getBegin() ) / ( selectedSourceVideoFile->getDuration() ) * (double)frameSlider->maximum()) ;
    frameSlider->setValue(f_percent);

    if (_displayTimeAsFrame)
        timeLineEdit->setText( selectedSourceVideoFile->getStringFrameFromTime(selectedSourceVideoFile->getCurrentFrameTime()) );
    else
        timeLineEdit->setText( selectedSourceVideoFile->getStringTimeFromtime(selectedSourceVideoFile->getCurrentFrameTime()) );

}


void GLMixer::on_frameSlider_actionTriggered (int a) {

    if (!selectedSourceVideoFile)
        return;

    if (a == QAbstractSlider::SliderMove) {

        // compute where we should jump to
        double percent = (double)(frameSlider->sliderPosition ())/ (double)frameSlider->maximum();
        double pos =  ( selectedSourceVideoFile->getDuration()  * percent ) + selectedSourceVideoFile->getBegin();

        // limit within marks of video file
        if ( pos > selectedSourceVideoFile->getMarkOut() )
            frameSlider->setSliderPosition( (int) ( ( selectedSourceVideoFile->getMarkOut() - selectedSourceVideoFile->getBegin() ) * ((double)frameSlider->maximum() / selectedSourceVideoFile->getDuration())  ) );

        else if ( pos < selectedSourceVideoFile->getMarkIn())
            frameSlider->setSliderPosition( (int) ( ( selectedSourceVideoFile->getMarkIn() - selectedSourceVideoFile->getBegin() ) * ((double)frameSlider->maximum() / selectedSourceVideoFile->getDuration())  ) );

        pos = qBound(selectedSourceVideoFile->getMarkIn(), pos, selectedSourceVideoFile->getMarkOut());

        // request seek ; we need to have the VideoFile process running to go there
        selectedSourceVideoFile->seekToPosition(pos);

        // show the time of the frame (refreshTiming disabled)
        if (_displayTimeAsFrame)
            timeLineEdit->setText( selectedSourceVideoFile->getStringFrameFromTime(pos) );
        else
            timeLineEdit->setText( selectedSourceVideoFile->getStringTimeFromtime(pos) );

    }

}

void  GLMixer::on_frameSlider_sliderPressed (){

    if (!selectedSourceVideoFile)
        return;

    // do not update slider position automatically anymore ; this interferes with user input
    refreshTimingTimer->stop();

    // disconnect the button from the VideoFile signal ; this way when we will unpause (see below), the button will keep its state
    QObject::disconnect(selectedSourceVideoFile, SIGNAL(paused(bool)), pauseButton, SLOT(setChecked(bool)));

    // pause because we want to move the slider
    selectedSourceVideoFile->pause(true);

}

void  GLMixer::on_frameSlider_sliderReleased (){

    // slider moved, frame was displayed, still paused; we un-pause if it was playing.
    selectedSourceVideoFile->pause(pauseButton->isChecked());

    // reconnect the pause button
    QObject::connect(selectedSourceVideoFile, SIGNAL(paused(bool)), pauseButton, SLOT(setChecked(bool)));

    // restart the refresh timer if it should be
    updateRefreshTimerState();

}

void GLMixer::enableSeek (bool on){

//    if (on){
//        seekForwardButton->unsetCursor();
//        seekBackwardButton->unsetCursor();
////        QApplication::restoreOverrideCursor();
//    } else {
//        seekForwardButton->setCursor(Qt::WaitCursor);
//        seekBackwardButton->setCursor(Qt::WaitCursor);
////        QApplication::setOverrideCursor(Qt::WaitCursor);
//    }

    seekBackwardButton->setEnabled(on);
    seekForwardButton->setEnabled(on);
}

void GLMixer::on_frameForwardButton_clicked(){

    if (selectedSourceVideoFile)
        selectedSourceVideoFile->seekForwardOneFrame();
}


void GLMixer::on_fastForwardButton_pressed(){

    if (selectedSourceVideoFile)
        selectedSourceVideoFile->setFastForward(true);
}

void GLMixer::on_fastForwardButton_released(){

    if (selectedSourceVideoFile)
        selectedSourceVideoFile->setFastForward(false);
}


void GLMixer::updateMarks (){

    if (!vcontrolDockWidget->isVisible())
        return;

    int i_percent = 0;
    int o_percent = (double)frameSlider->maximum();

    if (selectedSourceVideoFile){
        // adjust the marking sliders according to the source marks in and out
        i_percent = (int) ( (double)( selectedSourceVideoFile->getMarkIn() - selectedSourceVideoFile->getBegin() ) / (double)( selectedSourceVideoFile->getDuration() ) * (double)frameSlider->maximum()) ;
        o_percent = (int) ( (double)( selectedSourceVideoFile->getMarkOut() - selectedSourceVideoFile->getBegin() ) / (double)( selectedSourceVideoFile->getDuration() ) * (double)frameSlider->maximum()) ;

        if (_displayTimeAsFrame) {
            markInSlider->setToolTip( tr("Begin: ") + selectedSourceVideoFile->getStringFrameFromTime(selectedSourceVideoFile->getMarkIn()) );
            markOutSlider->setToolTip( tr("End: ") + selectedSourceVideoFile->getStringFrameFromTime(selectedSourceVideoFile->getMarkOut()) );
        } else {
            markInSlider->setToolTip( tr("Begin: ") + selectedSourceVideoFile->getStringTimeFromtime(selectedSourceVideoFile->getMarkIn()) );
            markOutSlider->setToolTip( tr("End: ") + selectedSourceVideoFile->getStringTimeFromtime(selectedSourceVideoFile->getMarkOut()) );
        }

    }

    markInSlider->setValue(i_percent);
    markOutSlider->setValue(o_percent);

    // update property for marks in / out
    emit sourceMarksModified(_displayTimeAsFrame);
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
    Title->setText(QObject::tr("Graphic Live Mixer"));
    textBrowser->setHtml(QObject::tr("<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 4.0//EN\" \"http://www.w3.org/TR/REC-html40/strict.dtd\">\n"
    "<html><head><meta name=\"qrichtext\" content=\"1\" /><style type=\"text/css\">\n"
    "p, li { white-space: pre-wrap; }\n"
    "</style></head><body style=\" font-family:'Sans Serif'; font-size:10pt; font-weight:400; font-style:normal;\">\n"
    "<p>GLMixer is a video mixing software for live performance.</p>\n"
    "<p>Author:	Bruno Herbelin<br>\n"
    "Contact:	bruno.herbelin@gmail.com<br>\n"
    "License: 	GNU GPL version 3</p>\n"
    "<p>Copyright 2009-2014 Bruno Herbelin</p>\n"
    "<p>Updates and source code at: <br>\n"
    "   	<a href=\"http://code.google.com/p/glmixer/\"><span style=\" text-decoration: underline; color:#7d400a;\">http://code.google.com/p/glmixer/</span>"
    "</a></p>"
    "<p>GLMixer is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation.</p>"
    "<p>GLMixer is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more details (see http://www.gnu.org/licenses).</p>"
    "</body></html>"));

    VERSION->setText( QString("%1").arg(QCoreApplication::applicationVersion()) );

#ifdef GLMIXER_REVISION
    SVN->setText(QString("%1").arg(GLMIXER_REVISION));
    textsvn->setText(QObject::tr("SVN repository revision:"));
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

    if (!currentSessionFileName.isEmpty()) {
        // add path to session switcher
        switcherSession->openFolder( QFileInfo(currentSessionFileName).absolutePath() );
        // message
        statusbar->showMessage( tr("Session file %1 loaded.").arg( currentSessionFileName ), 5000 );
    }
}


void GLMixer::on_actionNew_Session_triggered()
{
    on_actionClose_Session_triggered();

    actionMixingView->trigger();
}

void GLMixer::on_actionClose_Session_triggered()
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
    QString note = tr("Created on ");
    note.append(QDateTime::currentDateTime().toString("dddd d MMMM yyyy"));
    note.append(" by ");
#ifdef Q_OS_WIN
    note.append(getenv("USERNAME"));
#else
    note.append(getenv("USER"));
#endif
    note.append(".\n");
    blocNoteEdit->setPlainText(note);

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
            qWarning() << currentSessionFileName << QChar(124).toLatin1() << tr("Problem writing; ") << file.errorString();
            qCritical() << currentSessionFileName << QChar(124).toLatin1() << tr("Cannot save session file.");
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

        QDomElement notes = doc.createElement("Notes");
        QDomText text = doc.createTextNode(blocNoteEdit->toPlainText());
        notes.appendChild(text);
        root.appendChild(notes);

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
    QString suggestion = QString("glmix %1%2").arg(QDate::currentDate().toString("yyMMdd")).arg(QTime::currentTime().toString("hhmmss"));

    QString fileName = getFileName(tr("Save session"),
                                   tr("GLMixer session") + " (*.glm)",
                                   QString("glm"), suggestion);

    if ( !fileName.isEmpty() ) {
        // now we got a filename, save the file:
        currentSessionFileName = fileName;
        on_actionSave_Session_triggered();
    }
}

void GLMixer::on_actionLoad_Session_triggered()
{
    QString fileName = getFileName(tr("Open session"),
                                   tr("GLMixer session" )+" (*.glm)" );

    if (QFileInfo(fileName).isFile())
        // get the first file name selected
        switchToSessionFile( fileName );
    else
        qWarning()<< fileName << QChar(124).toLatin1() << tr("Not a session file; ");
}


void GLMixer::actionLoad_RecentSession_triggered()
{
    QAction *action = qobject_cast<QAction *>(sender());
    if (action)
        switchToSessionFile(action->data().toString());

}


void GLMixer::switchToSessionFile(QString filename){

    if (!QFileInfo(filename).isFile())
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


QString GLMixer::getRestorelastSessionFilename()
{
    // if the option to restore last session is ON, give the name of the last session file
    if (_restoreLastSession && settings.contains("lastSessionFileName")) {
        QFileInfo filename(settings.value("lastSessionFileName").toString());
        sfd->setDirectory(filename.absolutePath());
        return filename.absoluteFilePath();
    }
    else
        return QString();

}

void GLMixer::openSessionFile()
{
    // unpause if it was
    actionPause->setChecked ( false );

    // if we come from the smooth transition, disconnect the signal and enforce session switcher to show transition
    QObject::disconnect(RenderingManager::getSessionSwitcher(), SIGNAL(animationFinished()), this, SLOT(openSessionFile()) );
    RenderingManager::getSessionSwitcher()->setOverlay(1.0);
    actionToggleRenderingVisible->setEnabled(true);

    // Ok, ready to load XML ?
    QDomDocument doc;
    QString errorStr;
    int errorLine;
    int errorColumn;

    // open file
    QFile file(currentSessionFileName);
    if (!file.open(QFile::ReadOnly | QFile::Text)) {
        qWarning() << currentSessionFileName << QChar(124).toLatin1() << tr("Problem reading file; ") << file.errorString();
        qCritical() << currentSessionFileName << QChar(124).toLatin1() << tr("Cannot open file.");
        currentSessionFileName = QString();
        return;
    }
    // load content
    if (!doc.setContent(&file, true, &errorStr, &errorLine, &errorColumn)) {
        qWarning() << currentSessionFileName << QChar(124).toLatin1() << tr("XML parsing error line ") << errorLine << "(" << errorColumn << "); " << errorStr;
        qCritical() << currentSessionFileName << QChar(124).toLatin1() << tr("Cannot open file.");
        currentSessionFileName = QString();
        return;
    }
    // close file
    file.close();
    // verify it is a GLM file
    QDomElement root = doc.documentElement();
    if (root.tagName() != "GLMixer") {
        qWarning() << currentSessionFileName << QChar(124).toLatin1() << tr("This is not a GLMixer session file; ");
        currentSessionFileName = QString();
        return;
    }
    QString version = XML_GLM_VERSION;
    if ( root.hasAttribute("version") )
        version = root.attribute("version");
    if ( version != XML_GLM_VERSION ) {
        qWarning() << currentSessionFileName << QChar(124).toLatin1()<< tr("The version of the file is old (") << root.attribute("version") << tr(") : converting file to current version (") << XML_GLM_VERSION << ").";
    }

    // if we got up to here, it should be fine ; reset for a new session and apply loaded configurations
    RenderingManager::getInstance()->clearSourceSet();
    // read the source list and its configuration
    QDomElement renderConfig = root.firstChildElement("SourceList");
    if (renderConfig.isNull())
        qWarning() << currentSessionFileName << QChar(124).toLatin1() << tr("There is no source to load.");
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
        qDebug() << currentSessionFileName << QChar(124).toLatin1() << tr("Loading session.");

        // if we got up to here, it should be fine
        int errors = RenderingManager::getInstance()->addConfiguration(renderConfig, QFileInfo(currentSessionFileName).canonicalPath(), version);
        if ( errors > 0)
            qCritical() << currentSessionFileName << QChar(124).toLatin1() << errors << tr(" error(s) occurred when reading session.");

    }
    // read the views configuration
    RenderingManager::getRenderingWidget()->clearViews();
    QDomElement vconfig = root.firstChildElement("Views");
    if (vconfig.isNull())
        qDebug() << currentSessionFileName << QChar(124).toLatin1() << tr("No configuration specified.");
    else  {
        // apply the views configuration
        RenderingManager::getRenderingWidget()->setConfiguration(vconfig);
        // activate the view specified as 'current' in the xml config
        switch (vconfig.attribute("current").toInt()){
        case (View::NULLVIEW):
        case (View::MIXING):
            actionMixingView->trigger();
            break;
        case (View::GEOMETRY):
            actionGeometryView->trigger();
            break;
        case (View::LAYER):
            actionLayersView->trigger();
            break;
        case (View::RENDERING):
            actionRenderingView->trigger();
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
    // read the rendering configuration
    QDomElement rconfig = root.firstChildElement("Rendering");
    if (!rconfig.isNull()) {
        actionWhite_background->setChecked(rconfig.attribute("clearToWhite").toInt());
    }

    // read the notes text
    QString text;
    QDomElement notes = root.firstChildElement("Notes");
    if (!notes.isNull())
        text = notes.text();
    blocNoteEdit->setPlainText(text);

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

    QString fileName = getFileName(tr("Open session"),
                                   tr("GLMixer session" )+" (*.glm)" );

        if (QFileInfo(fileName).isFile()) {

        QFile file(fileName);

        if ( !file.open(QFile::ReadOnly | QFile::Text) ) {
            qWarning() << fileName << QChar(124).toLatin1()<< tr("Problem reading file; ") << file.errorString();
            qCritical() << fileName << QChar(124).toLatin1()<< tr("Cannot open file.");
            return;
        }

        if ( !doc.setContent(&file, true, &errorStr, &errorLine, &errorColumn) ) {
            qWarning() << fileName << QChar(124).toLatin1()<< tr("XML parsing error line ") << errorLine << "(" << errorColumn << "); " << errorStr;
            qCritical() << fileName << QChar(124).toLatin1() << tr("Cannot open file.");
            return;
        }

        file.close();

        QDomElement root = doc.documentElement();
        if ( root.tagName() != "GLMixer" ) {
            qWarning() << fileName << QChar(124).toLatin1()<< tr("This is not a GLMixer session file.");
            qCritical() << fileName << QChar(124).toLatin1()<< tr("Cannot open file.");
            return;
        }

        QString version = "0.0";
        if ( root.hasAttribute("version") )
            version = root.attribute("version");

        if ( version != XML_GLM_VERSION ) {
            qWarning() << fileName << QChar(124).toLatin1()<< tr("The version of the file is ") << root.attribute("version") << tr(" instead of ") << XML_GLM_VERSION;
            qCritical() << fileName << QChar(124).toLatin1() << tr("Incorrect file version. Trying to read what is compatible.");
        }

        // read the content of the source list to make sure the file is correct :
        QDomElement srcconfig = root.firstChildElement("SourceList");
        if ( srcconfig.isNull() ) {
            qWarning() << fileName << QChar(124).toLatin1() << tr("There is no source to load.");
            return;
        }

        // if we got up to here, it should be fine
        qDebug() << fileName << QChar(124).toLatin1() << tr("Adding list of sources.");
        int errors = RenderingManager::getInstance()->addConfiguration(srcconfig, QFileInfo(currentSessionFileName).canonicalPath(), version);
        if ( errors > 0)
            qCritical() << currentSessionFileName << QChar(124).toLatin1() << errors << tr(" error(s) occurred when reading session.");

        // confirm the loading of the file
        statusbar->showMessage( tr("Sources from %1 appended to %2.").arg( fileName ).arg( currentSessionFileName ), 3000 );
    }

    maybeSave = true;
}


void GLMixer::drop(QDropEvent *event)
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
                        qWarning() << urlname.absoluteFilePath()<< QChar(124).toLatin1() << "[" << ++errors << "]" << tr("File ignored (already loading another session).");
                }
                else if ( urlname.suffix() == "svg") {
                    svgFiles.append(urlname.absoluteFilePath());
                }
                else //  maybe a video ?
                    mediaFiles.append(urlname.absoluteFilePath());
            }
            else
                qWarning() << urlname.absoluteFilePath() << QChar(124).toLatin1() <<  "[" << ++errors << "]" << tr("Not a valid file; Ignoring.");
        }
    }
    else
        qWarning() << "[" << ++errors << "]" << tr("Not a valid drop; Ignoring.");

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

    } else if (!mediaFiles.isEmpty() || !svgFiles.isEmpty()) {
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
                        qWarning() << mediaFiles.at(i) << QChar(124).toLatin1()<< "[" << ++errors << "]" << tr("The source could not be created.");
                        delete newSourceVideoFile;
                    }
                } else {
                    qWarning() << mediaFiles.at(i) << QChar(124).toLatin1() << "[" << ++errors << "]" << tr("The file could not be loaded.");
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
                qDebug() << s->getName() << QChar(124).toLatin1()<< tr("New vector Graphics source created with file ")<< svgFiles.at(i);
            } else {
                qWarning() << svgFiles.at(i) << QChar(124).toLatin1() << "[" << ++errors << "]" << tr("The source could not be created.");
                delete svg;
            }
        }

        maybeSave = true;
    }

    if (errors > 0)
        qCritical() << tr("Not all the dropped files could be loaded.");

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
        restoreState(static_windowstate);

    if (settings.contains("OutputRenderWindowState"))
        OutputRenderWindow::getInstance()->restoreState( settings.value("OutputRenderWindowState").toByteArray() );
    else
        settings.setValue("defaultOutputRenderWindowState", OutputRenderWindow::getInstance()->saveState() );

    // dialogs configs
    if (settings.contains("vcontrolOptionSplitter"))
        vcontrolOptionSplitter->restoreState(settings.value("vcontrolOptionSplitter").toByteArray());
    if (settings.contains("VideoFileDialog")) {
        mfd->restoreState(settings.value("VideoFileDialog").toByteArray());
        mfd->setDirectory( mfd->history().last() );
    }
    if (settings.contains("SessionFileDialog")) {
        sfd->restoreState(settings.value("SessionFileDialog").toByteArray());
        sfd->setDirectory( sfd->history().last() );
    }

    // Cursor status
    if (settings.contains("CursorMode")) {
        switch((ViewRenderWidget::cursorMode) settings.value("CursorMode").toInt()) {
        case ViewRenderWidget::CURSOR_DELAY:
            actionCursorDelay->trigger();
            break;
        case ViewRenderWidget::CURSOR_SPRING:
            actionCursorSpring->trigger();
            break;
        case ViewRenderWidget::CURSOR_AXIS:
            actionCursorAxis->trigger();
            break;
        case ViewRenderWidget::CURSOR_LINE:
            actionCursorLine->trigger();
            break;
            break;
        case ViewRenderWidget::CURSOR_FUZZY:
            actionCursorFuzzy->trigger();
            break;
        default:
        case ViewRenderWidget::CURSOR_NORMAL:
            actionCursorNormal->trigger();
            break;
        }
    }

    if (settings.contains("CursorSpringMass"))
        cursorSpringMass->setValue(settings.value("CursorSpringMass").toInt());
    if (settings.contains("cursorLineSpeed"))
        cursorLineSpeed->setValue(settings.value("cursorLineSpeed").toInt());
    if (settings.contains("cursorLineWaitDuration"))
        cursorLineWaitDuration->setValue(settings.value("cursorLineWaitDuration").toInt());
    if (settings.contains("cursorDelayLatency"))
        cursorDelayLatency->setValue(settings.value("cursorDelayLatency").toInt());
    if (settings.contains("cursorDelayFiltering"))
        cursorDelayFiltering->setValue(settings.value("cursorDelayFiltering").toInt());
    if (settings.contains("cursorFuzzyRadius"))
        cursorFuzzyRadius->setValue(settings.value("cursorFuzzyRadius").toInt());
    if (settings.contains("cursorFuzzyFiltering"))
        cursorFuzzyFiltering->setValue(settings.value("cursorFuzzyFiltering").toInt());


    // Mixing presets
    if (settings.contains("MixingPresets"))
        mixingToolBox->restoreState(settings.value("MixingPresets").toByteArray());

    // Switcher session
    switcherSession->restoreSettings();

    // start O SC
      qDebug() << "OpenSoundControlManager " ;
    OpenSoundControlManager::getInstance()->setEnabled(true);

    qDebug() << tr("All settings restored.");
}

void GLMixer::saveSettings()
{
    // preferences
    settings.setValue("UserPreferences", getPreferences());

    // windows config
    settings.setValue("geometry", saveGeometry());
    settings.setValue("windowState", saveState());
   //  qDebug() << "windowState" << saveState().toHex();
    settings.setValue("OutputRenderWindowState", OutputRenderWindow::getInstance()->saveState() );

    // dialogs configs
    settings.setValue("vcontrolOptionSplitter", vcontrolOptionSplitter->saveState());
    settings.setValue("VideoFileDialog", mfd->saveState());
    settings.setValue("SessionFileDialog", sfd->saveState());

    // Cursor status
    settings.setValue("CursorMode", RenderingManager::getRenderingWidget()->getCursorMode() );
    settings.setValue("CursorSpringMass", cursorSpringMass->value() );
    settings.setValue("cursorLineSpeed", cursorLineSpeed->value() );
    settings.setValue("cursorLineWaitDuration", cursorLineWaitDuration->value() );
    settings.setValue("cursorDelayLatency", cursorDelayLatency->value() );
    settings.setValue("cursorDelayFiltering", cursorDelayFiltering->value() );
    settings.setValue("cursorFuzzyRadius", cursorFuzzyRadius->value() );
    settings.setValue("cursorFuzzyFiltering", cursorFuzzyFiltering->value() );

    // Mixing presets
    settings.setValue("MixingPresets", mixingToolBox->saveState());

    // last session file name
    settings.setValue("lastSessionFileName", currentSessionFileName);

    // make sure system saves settings NOW
    settings.sync();
    qDebug() << tr("Settings saved.");
}


void GLMixer::on_actionResetToolbars_triggered()
{
    restoreGeometry(settings.value("defaultGeometry").toByteArray());
    restoreState(settings.value("defaultWindowState").toByteArray());
    OutputRenderWindow::getInstance()->restoreState( settings.value("defaultOutputRenderWindowState").toByteArray() );
    restoreDockWidget(previewDockWidget);
    restoreDockWidget(sourceDockWidget);
    restoreDockWidget(vcontrolDockWidget);
    restoreDockWidget(cursorDockWidget);
    restoreDockWidget(mixingDockWidget);
    restoreDockWidget(switcherDockWidget);
    logDockWidget->hide();

    qDebug() << tr("Default layout restored.");
}

void GLMixer::on_actionPreferences_triggered()
{
    // fill in the saved preferences
    upd->showPreferences( getPreferences() );

    // show the dialog and apply preferences if it was accepted
    if (upd->exec() == QDialog::Accepted) {

        int mem = VideoFile::getMemoryUsagePolicy();

        restorePreferences( upd->getUserPreferences() );

        if (mem != VideoFile::getMemoryUsagePolicy()) {
            QMessageBox::information(this, QCoreApplication::applicationName(), "Reloading video files is necessary for the change of buffer size to take effect (reload session or restart the program). ");
        }
    }
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

        // make sure we reset preferences
        upd->restoreDefaultPreferences();

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
    int targetPeriod = 16;
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
    uint recformat = 4;
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
    mixingToolBox->setAntialiasing(antialiasing);

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
#ifdef SHM
    RenderingManager::getInstance()->setSharedMemoryColorDepth(shmdepth);
#endif
    // o. fullscreen monitor index
    uint fsmi = 0;
    stream >> fsmi;
    OutputRenderWindow::getInstance()->setFullScreenMonitor(fsmi);

    // p. options
    bool fs = false;
    stream >> fs >> _displayTimeAsFrame >> _restoreLastSession;
    RenderingManager::getRenderingWidget()->setFramerateVisible(fs);

    // q. view context menu
    int vcm = 0;
    stream >> vcm;
    if (vcm == 1)
        RenderingManager::getRenderingWidget()->setViewContextMenu(cursorMenu);
    else
        RenderingManager::getRenderingWidget()->setViewContextMenu(zoomMenu);

    // r. Memory usage policy
    int mem = VideoFile::getMemoryUsagePolicy();
    stream >> mem;
    VideoFile::setMemoryUsagePolicy(mem);

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
#ifdef SHM
    stream << (uint) RenderingManager::getInstance()->getSharedMemoryColorDepth();
#else
    stream << 0;
#endif

    // o. fullscreen monitor index
    stream << (uint) OutputRenderWindow::getInstance()->getFullScreenMonitor();

    // p. options
    stream << RenderingManager::getRenderingWidget()->getFramerateVisible() << _displayTimeAsFrame << _restoreLastSession;

    // q. view context menu
    stream << ((RenderingManager::getRenderingWidget()->getViewContexMenu() == cursorMenu) ? 1 : 0);

    // r. Memory usage policy
    stream << VideoFile::getMemoryUsagePolicy();

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
    gammaShiftSlider->setValue( GammaToScale(1.0) );
    gammaShiftText->setText( QString().setNum( 1.0, 'f', 2) );
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
    for(SourceList::iterator  its = SelectionManager::getInstance()->selectionBegin(); its != SelectionManager::getInstance()->selectionEnd(); its++) {

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


void GLMixer::on_startButton_toogled(bool on) {

    // toggle play/stop of current source
    SourceSet::iterator cs = RenderingManager::getInstance()->getCurrentSource();
    if (RenderingManager::getInstance()->isValid(cs)) {
        (*cs)->play(on);
        // update gui content from timings
        refreshTiming();
        updateRefreshTimerState();
    }

}

void GLMixer::on_actionSourcePlay_triggered(){

    // toggle play/stop of current source
    SourceSet::iterator cs = RenderingManager::getInstance()->getCurrentSource();
    if (RenderingManager::getInstance()->isValid(cs))
        (*cs)->play(!(*cs)->isPlaying());

    // loop over the selection and toggle play/stop of each source (but the current source already toggled)
    for(SourceList::iterator  its = SelectionManager::getInstance()->selectionBegin(); its != SelectionManager::getInstance()->selectionEnd(); its++)
        if (*its != *cs)
            (*its)->play(!(*its)->isPlaying());
}

void GLMixer::on_actionSourceRestart_triggered(){

    // apply action to current source
    SourceSet::iterator cs = RenderingManager::getInstance()->getCurrentSource();
    if (RenderingManager::getInstance()->isValid(cs) && selectedSourceVideoFile )
        selectedSourceVideoFile->seekBegin();

    // loop over the selection and apply action of each source (but the current source already done)
    for(SourceList::iterator  its = SelectionManager::getInstance()->selectionBegin(); its != SelectionManager::getInstance()->selectionEnd(); its++)
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
    for(SourceList::iterator  its = SelectionManager::getInstance()->selectionBegin(); its != SelectionManager::getInstance()->selectionEnd(); its++)
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
    for(SourceList::iterator  its = SelectionManager::getInstance()->selectionBegin(); its != SelectionManager::getInstance()->selectionEnd(); its++)
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
    for(SourceList::iterator  its = SelectionManager::getInstance()->selectionBegin(); its != SelectionManager::getInstance()->selectionEnd(); its++)
        if (*its != *cs && (*its)->rtti() == Source::VIDEO_SOURCE ){
            VideoFile *vf = (dynamic_cast<VideoSource *>(*its))->getVideoFile();
            vf->seekForward();
        }
}

//
// Align and distribute toolbox
//


void GLMixer::screenshotView(){

    // get screenshot from view GLWidget
    /*RenderingManager::getRenderingWidget()->makeCurrent();
    QImage s = RenderingManager::getRenderingWidget()->grabFrameBuffer();*/
    QPixmap s = QPixmap::grabWindow(RenderingManager::getRenderingWidget()->winId());
    // paint a cursor at the curent mouse coordinates
    QPainter p(&s);
    QPoint c = RenderingManager::getRenderingWidget()->mapFromGlobal( RenderingManager::getRenderingWidget()->cursor().pos() );
    p.drawPixmap(c, RenderingManager::getRenderingWidget()->cursor().pixmap());
    // create a unique filename and save to file
    QString f = QString("glmixer_%1_%2.png").arg(QDate::currentDate().toString()).arg(QTime::currentTime().toString());
    s.save( f );
    // log
    qDebug() << f << QChar(124).toLatin1() << "Saved screenshot.";
}


void GLMixer::selectGLSLFragmentShader()
{
    QString newfile = getFileName(tr("Open GLSL File"),
                                  tr("GLSL Fragment Shader") + " (*.glsl *.fsh *.txt)" );
    if ( QFileInfo(newfile).isFile())
        RenderingManager::getRenderingWidget()->setFilteringEnabled(true, newfile);
    else
        RenderingManager::getRenderingWidget()->setFilteringEnabled(RenderingManager::getRenderingWidget()->filteringEnabled());
}


QString GLMixer::getFileName(QString title, QString filter, QString saveExtention, QString suggestion)
{
    QString fileName = "";

    if (usesystemdialogs || filter.contains("bundle"))
    {
        static QDir dir(QDir::home());

        // open file or save file?
        if (!saveExtention.isNull())
            fileName = QFileDialog::getSaveFileName(this, title, dir.absolutePath() + "/" + suggestion, filter );
        else
            fileName = QFileDialog::getOpenFileName(this, title, dir.absolutePath(), filter );

        if (QFileInfo(fileName).exists())
            dir = QFileInfo(fileName).absoluteDir();

    } else {

        QFileDialog::AcceptMode mode = QFileDialog::AcceptOpen;

        sfd->setWindowTitle(title);
        sfd->setNameFilter(filter);
        sfd->selectFile(" ");

        // open file or save file?
        // if a saving extension is provided, the dialog is in save file selection mode
        if (!saveExtention.isNull()) {
            mode = QFileDialog::AcceptSave;
            sfd->setDefaultSuffix(saveExtention);
            if (!suggestion.isNull())
                sfd->selectFile(suggestion);
        }

        sfd->setAcceptMode(mode);
        sfd->setFileMode(mode == QFileDialog::AcceptOpen ? QFileDialog::ExistingFile : QFileDialog::AnyFile);

        if (sfd->exec())
            fileName = sfd->selectedFiles().front();

    }

    return fileName;
}

QStringList GLMixer::getMediaFileNames(bool &generatePowerOfTwoRequest) {

    QStringList fileNames;

    // open dialog for openning media files> system QFileDialog, or custom (mfd)
    if (usesystemdialogs) {
        static QDir dir(QDesktopServices::storageLocation(QDesktopServices::MoviesLocation));
        fileNames = QFileDialog::getOpenFileNames(this, tr("Open media files"), dir.absolutePath(), tr(VIDEOFILE_DIALOG_FORMATS) );
        if (!fileNames.isEmpty())
            dir = QFileInfo(fileNames.front()).absoluteDir();
    } else if (mfd->exec()) {
        fileNames = mfd->selectedFiles();
        generatePowerOfTwoRequest = mfd->configCustomSize();
    }

    return fileNames;
}
