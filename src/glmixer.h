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
    @author Bruno Herbelin <bruno.herbelin@gmail.com>
*/
class  GLMixer: public QMainWindow, private Ui::GLMixer
{
        Q_OBJECT

    public:
        GLMixer ( QWidget *parent = 0 );
        ~GLMixer();

        void closeEvent ( QCloseEvent * event );


    public slots:

    // menu and actions
		void on_blendingPresetsComboBox_currentIndexChanged(int);
		void on_actionMediaSource_triggered();
		void on_actionCameraSource_triggered();
		void on_actionRenderingSource_triggered();
		void on_actionCaptureSource_triggered();
		void on_actionAlgorithmSource_triggered();
		void on_actionCloneSource_triggered();
		void on_actionDeleteSource_triggered();
		void on_actionSaveCapture_triggered();
        void on_actionFormats_and_Codecs_triggered();
        void on_actionOpenGL_extensions_triggered();
        void on_markInSlider_sliderReleased ();
        void on_markOutSlider_sliderReleased ();
        void on_frameSlider_sliderPressed ();
        void on_frameSlider_sliderReleased ();
        void on_frameSlider_sliderMoved (int);
        void on_frameSlider_actionTriggered (int);
        void on_actionShow_frames_toggled(bool);
        void on_actionShowFPS_toggled(bool);
        void on_actionAbout_triggered();
        void on_actionMixingView_triggered();
        void on_actionGeometryView_triggered();
        void on_actionLayersView_triggered();
        void on_actionAbout_Qt_triggered() { QApplication::aboutQt (); }
        void on_actionNew_Session_triggered();
        void on_preFilteringBox_toggled(bool);

	// GUI interaction
        void updateRefreshTimerState();
        void updateMarks();
        void pauseAfterFrame();
        void refreshTiming();
        void displayLogMessage(QString msg);
        void displayErrorMessage(QString msg);

	// source config
        void connectSource(SourceSet::iterator csi);
        void blendingChanged();
        void colorsChanged();

    private:

//        videoFileDisplayWidget *videowidget;
        VideoFile *selectedSourceVideoFile;

        QTimer *refreshTimingTimer;
        bool waspaused;
        bool skipNextRefresh;
};

#endif /* GLV_H_ */
