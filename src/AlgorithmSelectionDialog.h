/*
 * AlgorithmSelectionDialog.h
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
 *   Copyright 2009, 2010 Bruno Herbelin
 *
 */

#ifndef ALGORITHMSELECTIONDIALOG_H_
#define ALGORITHMSELECTIONDIALOG_H_

#include <QDialog>
#include "ui_AlgorithmSelectionDialog.h"

class AlgorithmSource;
class SourceDisplayWidget;

class AlgorithmSelectionDialog  : public QDialog, Ui::AlgorithmSelectionDialog
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

public Q_SLOTS:

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