/*
 * UserPreferencesDialog.h
 *
 *  Created on: Jul 16, 2010
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

#ifndef USERPREFERENCESDIALOG_H_
#define USERPREFERENCESDIALOG_H_

#define QSETTING_PREFERENCE_VERSION 2
#define MAGIC_NUMBER 0x1D9D0CB

#include <QDialog>
#include "ui_UserPreferencesDialog.h"

class UserPreferencesDialog : public QDialog, Ui::UserPreferencesDialog
{
    Q_OBJECT

public:
	UserPreferencesDialog(QWidget *parent = 0);
	virtual ~UserPreferencesDialog();

	void setModeMinimal(bool on);

public Q_SLOTS:
	void restoreDefaultPreferences();
	void showPreferences(const QByteArray & state);
	QByteArray getUserPreferences() const;
	void on_updatePeriod_valueChanged(int period);
	void on_recordingUpdatePeriod_valueChanged(int period);
	void on_recordingFolderButton_clicked();
	void recordingFolderPathChanged(const QString &);

private:

	class Source *defaultSource;

};

#endif /* USERPREFERENCESDIALOG_H_ */
