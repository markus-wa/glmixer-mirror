/*
 * SharedMemoryDialog.h
 *
 *  Created on: Aug 6, 2011
 *      Author: bh
 */

#ifndef SHAREDMEMORYDIALOG_H_
#define SHAREDMEMORYDIALOG_H_


#include <QtGui>
#include "ui_SharedMemoryDialog.h"

class Source;
class SourceDisplayWidget;

class SharedMemoryDialog : public QDialog, Ui::SharedMemoryDialog
{
    Q_OBJECT

public:
	SharedMemoryDialog(QWidget *parent = 0);
	virtual ~SharedMemoryDialog();

	qint64 getSelectedId();
	QString getSelectedProcess();

public Q_SLOTS:

	void done(int r);
	void setCurrent(const QItemSelection & selected, const QItemSelection & deselected);

protected:
	void showEvent(QShowEvent *);
    void timerEvent(QTimerEvent *event);

private:
	int updateListTimer;
	Source *s;
	SourceDisplayWidget *preview;
    QStandardItemModel *listExistingShmModel;
    QStandardItem *selectedItem;

	void createSource();

};

#endif /* SHAREDMEMORYDIALOG_H_ */
