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

	void updateFolder();
    void openFolder();
    void folderChanged( const QString & text );
    void openFileFromFolder(const QModelIndex & index);
    void nameFilterChanged(const QString &s);
    void selectTransitionType(int t);

    void customizeTransition();
    void saveSettings();
    void restoreSettings();

Q_SIGNALS:
	void switchSessionFile(QString);

private:

	// folder toolbox
	QListWidget *createCurveIcons();
	void setupFolderToolbox();
    QStandardItemModel *folderModel;
    QSortFilterProxyModel *proxyFolderModel;
    QComboBox *folderHistory, *transitionSelection;
    QToolButton *customButton;
    QSettings *appSettings;
    QListWidget *easingCurvePicker;
    QSpinBox *transitionDuration;
    QSize m_iconSize;
};

#endif /* SESSIONSWITCHERWIDGET_H_ */