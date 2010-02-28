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

void AlgorithmSelectionDialog::accept(){


	QDialog::accept();
}
