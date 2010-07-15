/*
 * UserPreferencesDialog.h
 *
 *  Created on: Jul 16, 2010
 *      Author: bh
 */

#ifndef USERPREFERENCESDIALOG_H_
#define USERPREFERENCESDIALOG_H_

#include <QDialog>
#include "ui_UserPreferencesDialog.h"

class UserPreferencesDialog : public QDialog, Ui_UserPreferencesDialog
{
    Q_OBJECT

public:
	UserPreferencesDialog(QWidget *parent = 0);
	virtual ~UserPreferencesDialog();

	void restoreState(const QByteArray & state);
	QByteArray state();

};

#endif /* USERPREFERENCESDIALOG_H_ */
