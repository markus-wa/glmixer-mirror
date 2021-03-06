/*
 * glv.h
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

#ifndef GLV_H_
#define GLV_H_

#include "SourceSet.h"
#include "ui_glmixer.h"

#define MAX_RECENT_FILES 7

/**

 */
class GLMixer: public QMainWindow, private Ui::GLMixer {

Q_OBJECT

public:
    static GLMixer *getInstance();
    static void deleteInstance();
    static bool isSingleInstanceMode();

#ifdef GLM_LOGS
    // message handler
    static void msgHandler(QtMsgType type, const char *msg);
    // exit handler
    static void exitHandler();
#endif

    // catch keyboard events
    void keyPressEvent ( QKeyEvent * event );
    void keyReleaseEvent ( QKeyEvent * event );
    void timerEvent ( QTimerEvent * event );

    // gets
    bool useSystemDialogs() const;
    QString getCurrentSessionFilename() const;
    QString getRestorelastSessionFilename();
    QString getNotes() const;

public slots:

    // menu and actions
    void on_actionNewSource_triggered();
    void on_actionMediaSource_triggered();
    void on_actionBasketSource_triggered();
    void on_actionCameraSource_triggered();
    void on_actionRenderingSource_triggered();
    void on_actionCaptureSource_triggered();
    void on_actionAlgorithmSource_triggered();
    void on_actionSvgSource_triggered();
    void on_actionWebSource_triggered();
    void on_actionShmSource_triggered();
    void on_actionCloneSource_triggered();
    void on_actionFreeframeSource_triggered();
    void on_actionStreamSource_triggered();
    void on_actionDeleteSource_triggered();
    void on_actionEditSource_triggered();
    void on_actionFormats_and_Codecs_triggered();
    void on_actionOpenGL_extensions_triggered();
    void on_frameForwardButton_clicked();
    void on_fastForwardButton_pressed();
    void on_fastForwardButton_released();
    void on_actionAbout_triggered();
    void on_actionWebsite_triggered();
    void on_actionTutorials_triggered();
    void on_actionPreferences_triggered();
    void on_actionOSCTranslator_triggered();
    void on_actionNew_Session_triggered();
    void on_actionClose_Session_triggered();
    void on_actionSave_Session_triggered();
    void on_actionSave_Session_as_triggered();
    void on_actionLoad_Session_triggered();
    void on_actionAppend_Session_triggered();
    void on_actionReload_Session_triggered();
    void on_actionSelect_Next_triggered();
    void on_actionSelect_Previous_triggered();
    void on_actionResetToolbars_triggered();
    void on_controlOptionsButton_clicked();
    void on_actionSave_snapshot_triggered();
    void on_actionCopy_snapshot_triggered();
    void on_output_alpha_valueChanged(int);
    void on_copyNotes_clicked();
    void on_addDateToNotes_clicked();
    void on_addListToNotes_clicked();
    void on_timeLineEdit_clicked();

    void on_actionSourcePlay_triggered();
    void on_actionSourceRestart_triggered();
    void on_actionSourceSeekBackward_triggered();
    void on_actionSourcePause_triggered();
    void on_actionSourceSeekForward_triggered();
    void on_actionImportSettings_triggered();
    void on_actionExportSettings_triggered();

    // Clipboard
    void on_actionCopy_triggered();
    void on_actionCut_triggered();
    void on_actionPaste_triggered();
    void CliboardDataChanged();

    // GUI interaction
    void setView(QAction *a);
    void setTool(QAction *a);
    void setCursor(QAction *a);
    void setAspectRatio(QAction *a);
    void enableSeek(bool);
    void refreshTiming();
    void switchToSessionFile(QString filename);
    void renameSessionFile(QString oldfilename, QString newfilename);
    void actionLoad_RecentSession_triggered();
    void confirmSessionFileName();
    void updateStatusControlActions();
    void startButton_toogled(bool);
    void replaceCurrentSource();
    void undoChanged(bool, bool);
    void updateWorkspaceActions();
    void setBusy(bool busy = true);
    void resetCurrentCursor();
    void disable() { setDisabled(true); }
    void toggleRender();

    // source config
    void connectSource(SourceSet::iterator csi);
    void sessionChanged();
    void openSessionFile();
    void newSource(Source::RTTI type);
    void newSession();
    void closeSession();
    void saveSession(bool close = false, bool quit = false);
    void postSaveSession();
    void postNewSession();

    // app settings
    void readSettings(QString pathtobin = QString::null);
    void saveSettings();

    // interaction
    void drop(QDropEvent *event);
    QString getFileName(QString title, QString filters, QString saveExtention = QString(), QString suggestion = QString());
    QStringList getMediaFileNames(bool &smartScalingRequest, bool &hwDecodingRequest);
    QString getMaskFileName(QString suggestion);

    // timer display
    void setDisplayTimeEnabled(bool on);
    // performance mode
    void setPerformanceModeEnabled(bool on);

    // hidden actions
    void screenshotView();  // "Ctrl+<,<"
    void selectGLSLFragmentShader();  // "Shift+Ctrl+G,F"
    void startSessionTestingBot();  // "Shift+Ctrl+T,B"

#ifdef GLM_FFGL
    void editShaderToyPlugin(FFGLPluginSource *);
    void editShaderToySource(Source *);
#endif
#ifdef GLM_SESSION
    void openNextSession();
    void openPreviousSession();
#endif
#ifdef GLM_LOGS
    void saveLogsToFile();
#endif


signals:
    void sessionLoaded();
    void filenameChanged(const QString &);
    void keyPressed(int, bool);
    void status(QString, int);

protected:
    void closeEvent(QCloseEvent * event);
    void restorePreferences(const QByteArray & state);
    bool selectAspectRatio(int);
    QByteArray getPreferences() const;

private:
    GLMixer(QWidget *parent = 0);
    ~GLMixer();
    static GLMixer *_instance;
    static bool  _singleInstanceMode;

    QString currentSessionFileName;
    QLabel *infobar;
    bool usesystemdialogs, maybeSave;
    Source *previousSource;
    class VideoFile *currentVideoFile;
    class QFileDialog *sfd;
    class VideoFileDialog *mfd;
    class OutputRenderWidget *outputpreview;
    class UserPreferencesDialog *upd;
    class MixingToolboxWidget *mixingToolBox;
    class LayoutToolboxWidget *layoutToolBox;
    class PropertyBrowser *specificSourcePropertyBrowser;
    class QSplitter *layoutPropertyBrowser;

    bool _displayTimeAsFrame, _restoreLastSession, _saveExitSession;
    bool _disableOutputWhenRecord;
    bool _displayTimerEnabled;
    QElapsedTimer _displayTimer;

    QSettings *_settings;
    QAction *recentFileActs[MAX_RECENT_FILES];

#ifdef GLM_SESSION
    class SessionSwitcherWidget *switcherSession;
#endif
#ifdef GLM_TAG
    class TagsManager *tagsManager;
#endif
#ifdef GLM_SNAPSHOT
    class SnapshotManagerWidget *snapshotManager;
#endif
#ifdef GLM_HISTORY
    class HistoryRecorderWidget *actionHistoryView;
#endif
#ifdef GLM_FFGL
    class GLSLCodeEditorWidget *pluginGLSLCodeEditor;
#endif
#ifdef GLM_LOGS
    static QFile *logFile;
    static QTextStream logStream;
    static class LoggingWidget *logsWidget;
#endif
};


class SessionSaver : public QThread
 {
     Q_OBJECT

     void run();
     QString _filename;

public:
     SessionSaver(QString filename);

};

class ImageSaver : public QThread
 {
     Q_OBJECT

     void run();
     QImage _image;
     QString _filename;

public:
     ImageSaver(QImage image, QString filename);

     static void saveImage(QImage image, QString filename);

};

#endif /* GLV_H_ */
