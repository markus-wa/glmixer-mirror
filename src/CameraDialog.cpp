/*
 * CameraDialog.cpp
 *
 *  Created on: Dec 19, 2009
 *      Author: bh
 */

#include "CameraDialog.moc"
#include "OpencvDisplayWidget.h"

#include <QtGui/QComboBox>

CameraDialog::CameraDialog(QWidget *parent, int startTabIndex) : QDialog(parent), currentDriver(UNKNOWN)
{

    setupUi(this);


#ifdef OPEN_CV
    currentCameraIndex = -1;
    QObject::connect(opencvComboBox, SIGNAL(activated(int)), this, SLOT(setOpencvCamera(int)));

	openCVpreview = new OpencvDisplayWidget(this);
	openCVpreview->setFixedSize(200, 150);
	verticalLayout_2->insertWidget(1, openCVpreview);

	autodetectOpencvCameras();

#endif

    tabWidget->setCurrentIndex ( startTabIndex );
    on_tabWidget_currentChanged( startTabIndex );

}

CameraDialog::~CameraDialog() {
#ifdef OPEN_CV
	if (openCVpreview)
		delete openCVpreview;
#endif
}

CameraDialog::driver CameraDialog::getDriver(){
	return currentDriver;
}

void CameraDialog::accept(){

#ifdef OPEN_CV
	if (openCVpreview) {
		delete openCVpreview;
		openCVpreview = 0;
	}
#endif

	QDialog::accept();
}

void CameraDialog::on_tabWidget_currentChanged(int tabID){

	if (tabID == 0) { // OpenCV
#ifdef OPEN_CV
		currentDriver = OPENCV_CAMERA;
		openCVpreview->setCamera(currentCameraIndex);
#endif
	}


	if (tabID == 1) { // Firewire

#ifdef OPEN_CV
		// delete the opencvDisplayWidget ?
		 openCVpreview->setCamera(-1);
#endif
		currentDriver = FIREWIRE_CAMERA;
	}
}


void CameraDialog::on_opencvRefreshButton_clicked(){
#ifdef OPEN_CV
	autodetectOpencvCameras();
	openCVpreview->setCamera(currentCameraIndex);
#endif
}

#ifdef OPEN_CV
void CameraDialog::autodetectOpencvCameras(){

	// clear the list of cameras
	opencvComboBox->clear();

	// loops to get as many cameras as possible...
	int i = 0;
	CvCapture* cvcap;
	cvcap = cvCreateCameraCapture(i);
	while (cvcap) {
		opencvComboBox->addItem( QString("USB Camera %1").arg(i) );
		cvReleaseCapture(&cvcap);
		// next ?
		++i;
		cvcap = cvCreateCameraCapture(i);
	}
	cvReleaseCapture(&cvcap);

	if ( opencvComboBox->count() > 0 ) {
		currentCameraIndex = 0;
        opencvComboBox->setEnabled(true);
	}
	else {
		currentCameraIndex = -1;
		opencvComboBox->setEnabled(false);
	}
}

void CameraDialog::setOpencvCamera(int i){

	currentCameraIndex = i;

	// create the OpencvDisplayWidget
	openCVpreview->setCamera(currentCameraIndex);
}

#endif

void CameraDialog::autodetectFirewireCameras(){

}



