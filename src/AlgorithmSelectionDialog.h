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

public slots:

	void on_AlgorithmComboBox_currentIndexChanged(int algo);
	void accept();
};

#endif /* ALGORITHMSELECTIONDIALOG_H_ */
