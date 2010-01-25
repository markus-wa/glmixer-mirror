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
#endif
	}


	if (tabID == 1) { // Firewire

#ifdef OPEN_CV
		// delete the opencvDisplayWidget ?
#endif
		currentDriver = FIREWIRE_CAMERA;
	}
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
		opencvComboBox->addItem( QString("Camera %1").arg(i) );
		cvReleaseCapture(&cvcap);
		// next ?
		++i;
		cvcap = cvCreateCameraCapture(i);
	}

	if ( opencvComboBox->count() > 0 ) {
		setOpencvCamera(0);
        QObject::connect(opencvComboBox, SIGNAL(currentIndexChanged(int)), this, SLOT(setOpencvCamera(int)));
	}
	else
		opencvComboBox->setEnabled(false);
}

void CameraDialog::setOpencvCamera(int i){

	currentCameraIndex = i;

	// create the OpencvDisplayWidget
	openCVpreview->setCamera(currentCameraIndex);

	update();
}

#endif

void CameraDialog::autodetectFirewireCameras(){

}



