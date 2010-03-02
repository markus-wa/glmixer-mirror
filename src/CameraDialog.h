/*
 * CameraDialog.h
 *
 *  Created on: Dec 19, 2009
 *      Author: bh
 */

#ifndef CAMERADIALOG_H_
#define CAMERADIALOG_H_

#include <QDialog>
#include "ui_CameraDialog.h"

#ifdef OPEN_CV
//#include "OpencvDisplayWidget.h"
class OpencvSource;
#endif

class Source;
class SourceDisplayWidget;

class CameraDialog : public QDialog, Ui_CameraDialog
{
    Q_OBJECT

	public:

		typedef enum { UNKNOWN = 0, OPENCV_CAMERA, FIREWIRE_CAMERA} driver;

		CameraDialog(QWidget *parent = 0, int startTabIndex = 0);
		virtual ~CameraDialog();

		driver getDriver();

	public slots:

		void on_tabWidget_currentChanged(int tabID);
		void autodetectFirewireCameras();
		void accept();

#ifdef OPEN_CV
		void setOpencvCamera(int i);
		void on_opencvRefreshButton_clicked();

	public:
		void autodetectOpencvCameras();
		inline int indexOpencvCamera() const {return currentCameraIndex;}

	private:
		int currentCameraIndex;
#endif

	private:
		driver currentDriver;
		Source *s;
		SourceDisplayWidget *preview;

		void createSource();
};

#endif /* CAMERADIALOG_H_ */
