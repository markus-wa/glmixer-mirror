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

/**

 */
class GLMixer: public QMainWindow, private Ui::GLMixer {

Q_OBJECT

public:
	GLMixer(QWidget *parent = 0);
	~GLMixer();

	void closeEvent(QCloseEvent * event);

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
	void on_frameSlider_sliderPressed();
	void on_frameSlider_sliderReleased();
	void on_frameSlider_sliderMoved(int);
	void on_frameSlider_actionTriggered(int);
	void on_actionShow_frames_toggled(bool);
	void on_actionShowFPS_toggled(bool);
	void on_actionAbout_triggered();
	void on_actionMixingView_triggered();
	void on_actionGeometryView_triggered();
	void on_actionLayersView_triggered();
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
	void updateRefreshTimerState();
	void updateMarks();
	void pauseAfterFrame();
	void refreshTiming();
	void displayLogMessage(QString msg);
	void displayErrorMessage(QString msg);

	// source config
	void connectSource(SourceSet::iterator csi);

Q_SIGNALS:
	void sourceMarksModified(bool);

private:
	QString currentStageFileName;
	VideoFile *selectedSourceVideoFile;

	QTimer *refreshTimingTimer;
	bool waspaused;
	bool skipNextRefresh;
};

class CaptureDialog: public QDialog {
	Q_OBJECT

	QImage img;
public:
	CaptureDialog(QWidget *parent, QImage capture);
public Q_SLOTS:
	void saveImage();
};

#endif /* GLV_H_ */
