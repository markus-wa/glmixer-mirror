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

class AlgorithmSource;
class SourceDisplayWidget;

class AlgorithmSelectionDialog  : public QDialog, Ui_AlgorithmSelectionDialog
{
    Q_OBJECT

public:
	AlgorithmSelectionDialog(QWidget *parent = 0);
	virtual ~AlgorithmSelectionDialog();

	int getSelectedAlgorithmIndex();
	int getSelectedWidth();
	int getSelectedHeight();
	double getSelectedVariability();
	unsigned long  getUpdatePeriod();

public slots:

	void on_AlgorithmComboBox_currentIndexChanged(int algo);
	void on_customUpdateFrequency_toggled(bool flag);
	void on_frequencySlider_valueChanged(int v);
	void on_variabilitySlider_valueChanged(int v);
	void on_widthSpinBox_valueChanged(int w);
	void on_heightSpinBox_valueChanged(int h);
	void on_presetsSizeComboBox_currentIndexChanged(int preset);

private:
	AlgorithmSource *s;
	SourceDisplayWidget *preview;

	void createSource();

	static int _algo, _width, _height, _preset, _variability;
	static unsigned long _update;
};

#endif /* ALGORITHMSELECTIONDIALOG_H_ */
