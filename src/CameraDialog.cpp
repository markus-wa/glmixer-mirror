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

//#include <QtGui/QComboBox>

CameraDialog::CameraDialog(QWidget *parent, int startTabIndex) : QDialog(parent), s(0)
{
    setupUi(this);

    preview = new SourceDisplayWidget(this);
    verticalLayout->insertWidget(0, preview);

#ifdef OPEN_CV
    currentCameraIndex = -1;
    QObject::connect(opencvComboBox, SIGNAL(activated(int)), this, SLOT(setOpencvCamera(int)));
#endif

}

CameraDialog::~CameraDialog() {
	delete preview;
	//	if (s)
	//		delete s;
}


void CameraDialog::createSource(){

	if(s) {
		preview->setSource(0);
		// this deletes the texture in the preview
		delete s;
		s = 0;
	}

#ifdef OPEN_CV
		try {
			if (currentCameraIndex >= 0)
				s = (Source *) new OpencvSource(currentCameraIndex, preview->getNewTextureIndex(), 0);
		} catch (NoCameraIndexException){
			s = 0;
		}
#endif

	// apply the source to the preview
	if (s)
		preview->setSource(s);
}


void CameraDialog::accept(){

	preview->setSource(0);
	if (s)
		delete s;

	QDialog::accept();
}

#ifdef OPEN_CV

void CameraDialog::setOpencvCamera(int i){

	currentCameraIndex = i;

	// create the source
	createSource();
}

#endif



