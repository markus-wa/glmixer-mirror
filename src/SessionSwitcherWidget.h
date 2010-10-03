/*
 * SessionSwitcherWidget.h
 *
 *  Created on: Oct 1, 2010
 *      Author: bh
 */

#ifndef SESSIONSWITCHERWIDGET_H_
#define SESSIONSWITCHERWIDGET_H_

#define MAX_RECENT_FOLDERS 10

#include <QtGui>

class SessionSwitcherWidget  : public QWidget {

    Q_OBJECT

public:
	SessionSwitcherWidget(QWidget *parent, QSettings *settings);

public Q_SLOTS:
    void nameFilterChanged(const QString &s);
    void openFolder();
    void folderChanged( const QString & text );
    void openFileFromFolder(const QModelIndex & index);
    void selectTransitionType(int t);

    void customizeTransition();

Q_SIGNALS:
	void switchSessionFile(QString);

private:

	// folder toolbox
	void setupFolderToolbox();
    class QStandardItemModel *folderModel;
    class QSortFilterProxyModel *proxyFolderModel;
    class QComboBox *folderHistory, *transitionSelection;
    class QToolButton *customButton;
    class QSettings *appSettings;

};

#endif /* SESSIONSWITCHERWIDGET_H_ */
