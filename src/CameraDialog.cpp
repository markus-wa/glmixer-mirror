/*
 * CameraDialog.cpp
 *
 *  Created on: Dec 19, 2009
 *      Author: bh
 */

#include "CameraDialog.moc"
#include "Source.h"
#include "SourceDisplayWidget.h"

#ifdef OPEN_CV
#include "OpencvSource.h"
#endif

//#include <QtGui/QComboBox>

CameraDialog::CameraDialog(QWidget *parent, int startTabIndex) : QDialog(parent), currentDriver(UNKNOWN), s(0)
{

    setupUi(this);

    preview = new SourceDisplayWidget(this);
    verticalLayout->insertWidget(1, preview);

#ifdef OPEN_CV
    currentCameraIndex = -1;
    QObject::connect(opencvComboBox, SIGNAL(activated(int)), this, SLOT(setOpencvCamera(int)));

	autodetectOpencvCameras();
// TODO : ifnot def opencv, disable tab
#endif

    tabWidget->setCurrentIndex ( startTabIndex );
    on_tabWidget_currentChanged( startTabIndex );

}

CameraDialog::~CameraDialog() {
	delete preview;
	//	if (s)
	//		delete s;
}

CameraDialog::driver CameraDialog::getDriver(){
	return currentDriver;
}


void CameraDialog::createSource(){

	if(s) {
		preview->setSource(0);
		// this deletes the texture in the preview
		delete s;
		s = 0;
	}

	switch (currentDriver) {
	case OPENCV_CAMERA:
#ifdef OPEN_CV
		if (currentCameraIndex >= 0)
			s = (Source *) new OpencvSource(currentCameraIndex, preview->getNewTextureIndex(), 0);
#endif
		break;
	case FIREWIRE_CAMERA:
		break;
	case UNKNOWN:
	default:
		break;
	}

	// apply the source to the preview
	if (s)
		preview->setSource(s);
}

void CameraDialog::on_tabWidget_currentChanged(int tabID){

	if (tabID == 0) { // OpenCV
		currentDriver = OPENCV_CAMERA;
	}
	else if (tabID == 1) { // Firewire
		currentDriver = FIREWIRE_CAMERA;
	}

	createSource();
}

void CameraDialog::accept(){

	preview->setSource(0);
	if (s)
		delete s;

	QDialog::accept();
}

#ifdef OPEN_CV

void CameraDialog::on_opencvRefreshButton_clicked(){
	autodetectOpencvCameras();
	createSource();
}

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

	// create the source
	createSource();
}

#endif

void CameraDialog::autodetectFirewireCameras(){

}



