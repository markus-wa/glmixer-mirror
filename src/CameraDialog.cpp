/*
 * CameraDialog.cpp
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

#include "CameraDialog.moc"
#include "Source.h"
#include "SourceDisplayWidget.h"

#ifdef OPEN_CV
#include "OpencvSource.h"
#endif

#include <QtGui>

#define CAMERA_PREVIEW 1

CameraDialog::CameraDialog(QWidget *parent, int startTabIndex) : QDialog(parent), s(0), preview(0)
{
    setupUi(this);

#ifndef CAMERA_PREVIEW
    preview = 0;
    showPreview->setEnabled(false);
#else
    preview = new SourceDisplayWidget(this);
    preview->hide();
    QObject::connect(showPreview, SIGNAL(toggled(bool)), this, SLOT(setPreviewEnabled(bool)));
#endif

#ifdef OPEN_CV
    currentCameraIndex = -1;
    QObject::connect(opencvComboBox, SIGNAL(activated(int)), this, SLOT(setOpencvCamera(int)));
#endif

}

void CameraDialog::createSource(){

	if (!preview)
		return;

	if(s) {
		preview->setSource(0);
		// this deletes the texture in the preview
		delete s;
		s = 0;
	}

#ifdef OPEN_CV


	if (currentCameraIndex >= 0) {

		s = OpencvSource::getExistingSourceForCameraIndex(currentCameraIndex);
		if ( !s ) {
			try {
				s = (Source *) new OpencvSource(currentCameraIndex, preview->getNewTextureIndex(), 0);
			} catch (NoCameraIndexException) {
				s = 0;
			}
		}
	}
#endif

	// apply the source to the preview
	if (s)
		preview->setSource(s);
}


void CameraDialog::showEvent(QShowEvent *e){

	QWidget::showEvent(e);

    showPreview->setChecked(false);
    opencvComboBox->setCurrentIndex(0);

}

void CameraDialog::accept(){

	if (preview)
		preview->setSource(0);
	if (s) {
		delete s;
		s = 0;
	}
	QDialog::accept();
}

void CameraDialog::setPreviewEnabled(bool on){

	// remove the top item
	verticalLayout->itemAt(0)->widget()->hide();
	verticalLayout->removeItem(verticalLayout->itemAt(0));

	// add a top idem according to preview mode
	if(on) {
	    verticalLayout->insertWidget(0, preview);
		createSource();
	} else {
	    verticalLayout->insertWidget(0, nopreview);
	}

	verticalLayout->itemAt(0)->widget()->show();

}

#ifdef OPEN_CV

void CameraDialog::setOpencvCamera(int i){

	currentCameraIndex = i-1;

	// create the source
	if (showPreview->isChecked())
		createSource();
}

#endif



