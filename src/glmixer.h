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
 *   Copyright 2009, 2010 Bruno Herbelin
 *
 */

#ifndef GLV_H_
#define GLV_H_

#include "ui_glmixer.h"

#include "VideoFile.h"
#include "SourceSet.h"

#define MAX_RECENT_FILES 7

/**

 */
class GLMixer: public QMainWindow, private Ui::GLMixer {

Q_OBJECT

public:
	GLMixer(QWidget *parent = 0);
	~GLMixer();


public Q_SLOTS:

	// menu and actions
	void on_actionMediaSource_triggered();
	void on_actionCameraSource_triggered();
	void on_actionRenderingSource_triggered();
	void on_actionCaptureSource_triggered();
	void on_actionAlgorithmSource_triggered();
	void on_actionCloneSource_triggered();
	void on_actionDeleteSource_triggered();
	void on_actionFormats_and_Codecs_triggered();
	void on_actionOpenGL_extensions_triggered();
	void on_markInSlider_sliderReleased();
	void on_markOutSlider_sliderReleased();
	void on_frameForwardButton_clicked();
	void on_frameSlider_sliderPressed();
	void on_frameSlider_sliderReleased();
	void on_frameSlider_sliderMoved(int);
	void on_frameSlider_actionTriggered(int);
	void on_actionShow_frames_toggled(bool);
	void on_actionShowFPS_toggled(bool);
	void on_actionFree_aspect_ratio_toggled(bool);
	void on_actionAbout_triggered();
	void on_actionPreferences_triggered();

	void on_actionNew_Session_triggered();
	void on_actionSave_Session_triggered();
	void on_actionSave_Session_as_triggered();
	void on_actionLoad_Session_triggered();
	void on_actionLoad_RecentSession_triggered();
	void on_actionAppend_Session_triggered();
	void on_actionSelect_Next_triggered();
	void on_actionSelect_Previous_triggered();
	void on_actionResetToolbars_triggered();

	// GUI interaction
	void setView(QAction *a);
	void setTool(QAction *a);
	void setCursor(QAction *a);
	void updateRefreshTimerState();
	void updateMarks();
	void unpauseBeforeSeek();
	void pauseAfterFrame();
	void pauseAfterSeek();
	void refreshTiming();
	void displayInfoMessage(QString msg);
	void displayWarningMessage(QString msg);
	void newSession();
	void openSessionFile(QString filename = QString());
	void switchToSessionFile(QString filename);
	void confirmSessionFileName();

	// source config
	void connectSource(SourceSet::iterator csi);

Q_SIGNALS:
	void sourceMarksModified(bool);

protected:
    void dragEnterEvent(QDragEnterEvent *event);
    void dragMoveEvent(QDragMoveEvent *event);
    void dragLeaveEvent(QDragLeaveEvent *event);
    void dropEvent(QDropEvent *event);
	void closeEvent(QCloseEvent * event);

	bool restorePreferences(const QByteArray & state);
	QByteArray getPreferences() const;

private:
	QErrorMessage *errorMessageDialog;
	QString currentSessionFileName;
	VideoFile *selectedSourceVideoFile;
	QFileDialog *sfd;
	class VideoFileDialog *mfd;
	class OutputRenderWidget *outputpreview;

	QTimer *refreshTimingTimer;
	bool waspaused;
	bool skipNextRefresh;

	QSettings settings;
	void readSettings();
	void saveSettings();
	QAction *recentFileActs[MAX_RECENT_FILES];

};


class CaptureDialog: public QDialog {
	Q_OBJECT

	QImage img;
	QString filename;
public:
	CaptureDialog(QWidget *parent, QImage capture);
public Q_SLOTS:
	QString saveImage();
};

#endif /* GLV_H_ */
