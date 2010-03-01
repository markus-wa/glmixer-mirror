/*
 * AlgorithmSelectionDialog.h
 *
 *  Created on: Feb 28, 2010
 *      Author: Herbelin
 */

#ifndef ALGORITHMSELECTIONDIALOG_H_
#define ALGORITHMSELECTIONDIALOG_H_

#include <QDialog>
#include "ui_AlgorithmSelectionDialog.h"

class AlgorithmSelectionDialog  : public QDialog, Ui_AlgorithmSelectionDialog
{
    Q_OBJECT

public:
	AlgorithmSelectionDialog(QWidget *parent = 0);
	virtual ~AlgorithmSelectionDialog();

	int getSelectedAlgorithmIndex();
	int getSelectedWidth();
	int getSelectedHeight();
	unsigned long  getUpdatePeriod();

public slots:

	void on_AlgorithmComboBox_currentIndexChanged(int algo);
	void on_customUpdateFrequency_toggled(bool flag);
	void on_frequencySlider_valueChanged(int v);
	void on_widthSpinBox_valueChanged(int w);
	void on_heightSpinBox_valueChanged(int h);
	void accept();
};

#endif /* ALGORITHMSELECTIONDIALOG_H_ */
