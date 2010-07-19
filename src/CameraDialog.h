/*
 * CameraDialog.h
 *
 *  Created on: Dec 19, 2009
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

	public Q_SLOTS:

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
