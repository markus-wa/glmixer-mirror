/*
 * UserPreferencesDialog.cpp
 *
 *  Created on: Jul 16, 2010
 *      Author: bh
 */

#include "UserPreferencesDialog.moc"

UserPreferencesDialog::UserPreferencesDialog(QWidget *parent): QDialog(parent)
{
    setupUi(this);

}

UserPreferencesDialog::~UserPreferencesDialog()
{
	// TODO Auto-generated destructor stub
}



void UserPreferencesDialog::restoreState(const QByteArray & state){


}

QByteArray UserPreferencesDialog::state(){

	return QByteArray("nothing");
}
