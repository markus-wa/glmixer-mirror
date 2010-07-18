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

class UserPreferencesDialog : public QDialog, Ui::UserPreferencesDialog
{
    Q_OBJECT

public:
	UserPreferencesDialog(QWidget *parent = 0);
	virtual ~UserPreferencesDialog();

public Q_SLOTS:
	void restoreDefaultPreferences();
	void showPreferences(const QByteArray & state);
	QByteArray getUserPreferences() const;

private:
	void sizeToSelection(QSize s);
	QSize selectionToSize() const;

};

#endif /* USERPREFERENCESDIALOG_H_ */
