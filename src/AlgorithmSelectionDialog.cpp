/*
 * AlgorithmSelectionDialog.cpp
 *
 *  Created on: Feb 28, 2010
 *      Author: Herbelin
 */

#include "AlgorithmSelectionDialog.moc"

#include "SourceDisplayWidget.h"
#include "AlgorithmSource.h"

AlgorithmSelectionDialog::AlgorithmSelectionDialog(QWidget *parent) : QDialog(parent), s(0), preview(0){

    setupUi(this);

    preview = new SourceDisplayWidget(this);
    verticalLayout->insertWidget(1, preview);

	createSource();
}

AlgorithmSelectionDialog::~AlgorithmSelectionDialog() {
	delete preview;
	if (s)
		delete s;
}

void AlgorithmSelectionDialog::createSource(){

	if(s) {
		preview->setSource(0);
		// this deletes the texture in the preview
		delete s;
	}

	// create a new source with a new texture index and the new parameters
	s = new AlgorithmSource(AlgorithmComboBox->currentIndex(), preview->getNewTextureIndex(), 0, widthSpinBox->value(), heightSpinBox->value(), getUpdatePeriod());

	// apply the source to the preview
	preview->setSource(s);
}

void AlgorithmSelectionDialog::on_AlgorithmComboBox_currentIndexChanged(int algo){
	createSource();
}


void AlgorithmSelectionDialog::on_frequencySlider_valueChanged(int v){
	s->setPeriodicity(getUpdatePeriod());
}


void  AlgorithmSelectionDialog::on_customUpdateFrequency_toggled(bool flag){

	s->setPeriodicity(0);
}

void  AlgorithmSelectionDialog::on_widthSpinBox_valueChanged(int w){
	createSource();
}

void  AlgorithmSelectionDialog::on_heightSpinBox_valueChanged(int h){
	createSource();
}


void AlgorithmSelectionDialog::on_presetsSizeComboBox_currentIndexChanged(int preset){

	if (preset == 0) {
		heightSpinBox->setReadOnly(false);
		widthSpinBox->setReadOnly(false);
	} else {
		heightSpinBox->setReadOnly(true);
		widthSpinBox->setReadOnly(true);

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
			widthSpinBox->setValue(320);
			heightSpinBox->setValue(240);
			break;
		case 9:
			widthSpinBox->setValue(640);
			heightSpinBox->setValue(480);
			break;
		case 10:
			widthSpinBox->setValue(720);
			heightSpinBox->setValue(480);
			break;
		case 11:
			widthSpinBox->setValue(768);
			heightSpinBox->setValue(576);
			break;
		case 12:
			widthSpinBox->setValue(800);
			heightSpinBox->setValue(600);
			break;
		case 13:
			widthSpinBox->setValue(1024);
			heightSpinBox->setValue(768);
			break;
		}
	}

}


int AlgorithmSelectionDialog::getSelectedAlgorithmIndex(){

	return AlgorithmComboBox->currentIndex();
}

int AlgorithmSelectionDialog::getSelectedWidth(){

	return widthSpinBox->value();
}


int AlgorithmSelectionDialog::getSelectedHeight(){

	return heightSpinBox->value();
}

unsigned long  AlgorithmSelectionDialog::getUpdatePeriod(){

	if (customUpdateFrequency->isChecked()) {
		return (unsigned long) ( 1000000.f / float(frequencySlider->value()));
	} else
		return 0;
}

