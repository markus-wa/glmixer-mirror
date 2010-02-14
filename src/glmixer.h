/*
 * glv.h
 *
 *  Created on: Jul 14, 2009
 *      Author: bh
 */

#ifndef GLV_H_
#define GLV_H_

#include "ui_glmixer.h"

#include "VideoFileDisplayWidget.h"
#include "RenderingManager.h"
#include "VideoFile.h"

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
		void on_actionMediaSource_activated();
		void on_actionCameraSource_activated();
		void on_actionRenderingSource_activated();
		void on_actionCaptureSource_activated();
		void on_actionSaveCapture_activated();
        void on_actionFormats_and_Codecs_activated();
        void on_actionOpenGL_extensions_activated();
        void on_markInSlider_sliderReleased ();
        void on_markOutSlider_sliderReleased ();
        void on_frameSlider_sliderPressed ();
        void on_frameSlider_sliderReleased ();
        void on_frameSlider_sliderMoved (int);
        void on_frameSlider_actionTriggered (int);
        void on_actionShow_frames_toggled(bool);
        void on_actionShowFPS_toggled(bool);
        void on_actionAbout_activated();
        void on_actionMixingView_activated();
        void on_actionGeometryView_activated();
        void on_actionAbout_Qt_activated() { QApplication::aboutQt (); }

	// GUI interaction
        void updateRefreshTimerState();
        void updateMarks();
        void pauseAfterFrame();
        void refreshTiming();
        void displayLogMessage(QString msg);
        void displayErrorMessage(QString msg);

	// source information
        void controlSource(SourceSet::iterator csi);

    private:

//        videoFileDisplayWidget *videowidget;
        VideoFile *selectedSourceVideoFile;

        QTimer *refreshTimingTimer;
        bool waspaused;
        bool skipNextRefresh;
};

#endif /* GLV_H_ */
