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
#include "OpencvDisplayWidget.h"
#endif

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
		void autodetectOpencvCameras();
		void setOpencvCamera(int i);
		inline int indexOpencvCamera() {return currentCameraIndex;}

	private:
		int currentCameraIndex;
		OpencvDisplayWidget *openCVpreview;
#endif

	private:
		driver currentDriver;
};

#endif /* CAMERADIALOG_H_ */
