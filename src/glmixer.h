/*
 * glv.h
 *
 *  Created on: Jul 14, 2009
 *      Author: bh
 */

#ifndef GLV_H_
#define GLV_H_

#include "ui_glmixer.h"

#include "VideoFile.h"
#include "SourceSet.h"
#include "VideoFileDialog.h"

/**

 */
class GLMixer: public QMainWindow, private Ui::GLMixer {

Q_OBJECT

public:
	GLMixer(QWidget *parent = 0);
	~GLMixer();

	void closeEvent(QCloseEvent * event);
	void openSessionFile(QString filename);

public Q_SLOTS:

	void changeWindowTitle();

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
	void on_actionAbout_triggered();
	void on_actionAbout_Qt_triggered() {
		QApplication::aboutQt();
	}
	void on_actionNew_Session_triggered();
	void on_actionSave_Session_triggered();
	void on_actionSave_Session_as_triggered();
	void on_actionLoad_Session_triggered();
	void on_actionAppend_Session_triggered();
	void on_actionSelect_Next_triggered();
	void on_actionSelect_Previous_triggered();

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

	// source config
	void connectSource(SourceSet::iterator csi);

Q_SIGNALS:
	void sourceMarksModified(bool);

protected:
    void dragEnterEvent(QDragEnterEvent *event);
    void dragMoveEvent(QDragMoveEvent *event);
    void dragLeaveEvent(QDragLeaveEvent *event);
    void dropEvent(QDropEvent *event);

private:
	QString currentStageFileName;
	VideoFile *selectedSourceVideoFile;
	VideoFileDialog *mfd;

	QTimer *refreshTimingTimer;
	bool waspaused;
	bool skipNextRefresh;

	QSettings settings;
	void readSettings();
	void saveSettings();
};

class CaptureDialog: public QDialog {
	Q_OBJECT

	QImage img;
	QString filename;
public:
	CaptureDialog(QWidget *parent, QImage capture);
public Q_SLOTS:
	void saveImage();
};

#endif /* GLV_H_ */
