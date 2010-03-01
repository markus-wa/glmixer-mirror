/*
 * AlgorithmSelectionDialog.cpp
 *
 *  Created on: Feb 28, 2010
 *      Author: Herbelin
 */

#include "AlgorithmSelectionDialog.moc"

AlgorithmSelectionDialog::AlgorithmSelectionDialog(QWidget *parent) : QDialog(parent){

    setupUi(this);

}

AlgorithmSelectionDialog::~AlgorithmSelectionDialog() {
	// TODO Auto-generated destructor stub
}


void AlgorithmSelectionDialog::on_AlgorithmComboBox_currentIndexChanged(int algo){

}


void AlgorithmSelectionDialog::on_frequencySlider_valueChanged(int v){

}


void  AlgorithmSelectionDialog::on_customUpdateFrequency_toggled(bool flag){

}

void  AlgorithmSelectionDialog::on_widthSpinBox_valueChanged(int w){


}
void  AlgorithmSelectionDialog::on_heightSpinBox_valueChanged(int h){


}

void AlgorithmSelectionDialog::accept(){


	QDialog::accept();
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

