/*
 * SharedMemoryDialog.h
 *
 *  Created on: Aug 6, 2011
 *      Author: bh
 */

#ifndef SHAREDMEMORYDIALOG_H_
#define SHAREDMEMORYDIALOG_H_


#include <QDialog>
#include "ui_SharedMemoryDialog.h"

class SharedMemoryDialog : public QDialog, Ui::SharedMemoryDialog
{
    Q_OBJECT

public:
	SharedMemoryDialog(QWidget *parent = 0);
	virtual ~SharedMemoryDialog();



};

#endif /* SHAREDMEMORYDIALOG_H_ */
