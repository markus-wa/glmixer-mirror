/*
 * AlgorithmSelectionDialog.cpp
 *
 *  Created on: Feb 28, 2010
 *      Author: Herbelin
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
 *   Copyright 2009, 2012 Bruno Herbelin
 *
 */

#include "AlgorithmSelectionDialog.moc"

#include "SourceDisplayWidget.h"
#include "AlgorithmSource.h"


AlgorithmSelectionDialog::AlgorithmSelectionDialog(QWidget *parent) : QDialog(parent), s(0), preview(0){

    setupUi(this);

    preview = new SourceDisplayWidget(this);
    verticalLayout->insertWidget(0, preview);

    AlgorithmComboBox->clear();
    // create sources with selected algo
    for (int i = 0; i < AlgorithmSource::NONE; ++i)
    	AlgorithmComboBox->addItem(AlgorithmSource::getAlgorithmDescription(i));

}

AlgorithmSelectionDialog::~AlgorithmSelectionDialog() {
	delete preview;
}

void AlgorithmSelectionDialog::showEvent(QShowEvent *e){

	createSource();

	QWidget::showEvent(e);
}

void AlgorithmSelectionDialog::done(int r){

	if (preview)
		preview->setSource(0);
	if (s) {
		delete s;
		s = 0;
	}
	QDialog::done(r);
}


void AlgorithmSelectionDialog::createSource(){

	if(s) {
		// remove source from preview: this deletes the texture in the preview
		preview->setSource(0);
		// delete the source:
		delete s;
	}

	GLuint tex = preview->getNewTextureIndex();
	try {
		// create a new source with a new texture index and the new parameters
		s = new AlgorithmSource(AlgorithmComboBox->currentIndex(), tex, 0, widthSpinBox->value(), heightSpinBox->value(),
								getSelectedVariability(), getUpdatePeriod(), !ignoreAlphaCheckbox->isChecked());

	} catch (AllocationException &e){
		qCritical() << "AlgorithmSelectionDialog|" << e.message();
		// free the OpenGL texture
		glDeleteTextures(1, &tex);
		// return an invalid pointer
		s = 0;
	}

	// apply the source to the preview
	preview->setSource(s);
    preview->playSource(true);
}

void AlgorithmSelectionDialog::on_AlgorithmComboBox_currentIndexChanged(int algo){
	createSource();
}


void AlgorithmSelectionDialog::on_frequencySlider_valueChanged(int v){
	s->setPeriodicity(getUpdatePeriod());
}

void AlgorithmSelectionDialog::on_variabilitySlider_valueChanged(int v){
	s->setVariability( double ( v ) / 100.0);
}


void  AlgorithmSelectionDialog::on_customUpdateFrequency_toggled(bool flag){
	if (!flag)
		frequencySlider->setValue(40);
}

void AlgorithmSelectionDialog::on_ignoreAlphaCheckbox_toggled(bool on){
	s->setIgnoreAlpha(!on);
}

void  AlgorithmSelectionDialog::on_widthSpinBox_valueChanged(int w){
	createSource();
}

void  AlgorithmSelectionDialog::on_heightSpinBox_valueChanged(int h){
	createSource();
}


void AlgorithmSelectionDialog::on_presetsSizeComboBox_currentIndexChanged(int preset){

	if (preset == 0) {
		heightSpinBox->setEnabled(true);
		widthSpinBox->setEnabled(true);
		h->setEnabled(true);
		w->setEnabled(true);
	} else {
		heightSpinBox->setEnabled(false);
		widthSpinBox->setEnabled(false);
		h->setEnabled(false);
		w->setEnabled(false);

		switch (preset) {
		case 1:
			heightSpinBox->setValue(2);
			widthSpinBox->setValue(2);
			break;
		case 2:
			heightSpinBox->setValue(8);
			widthSpinBox->setValue(8);
			break;
		case 3:
			heightSpinBox->setValue(16);
			widthSpinBox->setValue(16);
			break;
		case 4:
			heightSpinBox->setValue(32);
			widthSpinBox->setValue(32);
			break;
		case 5:
			heightSpinBox->setValue(64);
			widthSpinBox->setValue(64);
			break;
		case 6:
			heightSpinBox->setValue(128);
			widthSpinBox->setValue(128);
			break;
		case 7:
			heightSpinBox->setValue(256);
			widthSpinBox->setValue(256);
			break;
		case 8:
			widthSpinBox->setValue(160);
			heightSpinBox->setValue(120);
			break;
		case 9:
			widthSpinBox->setValue(320);
			heightSpinBox->setValue(240);
			break;
		case 10:
			widthSpinBox->setValue(640);
			heightSpinBox->setValue(480);
			break;
		case 11:
			widthSpinBox->setValue(720);
			heightSpinBox->setValue(480);
			break;
		case 12:
			widthSpinBox->setValue(768);
			heightSpinBox->setValue(576);
			break;
		case 13:
			widthSpinBox->setValue(800);
			heightSpinBox->setValue(600);
			break;
		case 14:
			widthSpinBox->setValue(1024);
			heightSpinBox->setValue(768);
			break;
		}
	}

}


int AlgorithmSelectionDialog::getSelectedAlgorithmIndex(){

	return AlgorithmComboBox->currentIndex();
}


double AlgorithmSelectionDialog::getSelectedVariability(){

	return double(variabilitySlider->value()) / 100.0;
}


int AlgorithmSelectionDialog::getSelectedWidth(){

	return widthSpinBox->value();
}


int AlgorithmSelectionDialog::getSelectedHeight(){

	return heightSpinBox->value();
}

unsigned long  AlgorithmSelectionDialog::getUpdatePeriod(){

	if (!customUpdateFrequency->isChecked())
		return 0;
	else
		return (unsigned long) ( 1000000.0 / double(frequencySlider->value()));

}

bool AlgorithmSelectionDialog::getIngoreAlpha(){

	return !ignoreAlphaCheckbox->isChecked();
}
