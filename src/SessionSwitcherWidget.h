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
#include "RenderingManager.h"

class SessionSwitcherWidget  : public QWidget {

    Q_OBJECT

public:
	SessionSwitcherWidget(QWidget *parent, QSettings *settings);

public Q_SLOTS:

	void updateFolder();
    void openFolder();
    void discardFolder();
    void folderChanged( const QString & text );
    void openFileFromFolder(const QModelIndex & index);
    void nameFilterChanged(const QString &s);
    void selectTransitionType(int t);

    void customizeTransition();
    void saveSettings();
    void restoreSettings();

    void setAllowedAspectRatio(const standardAspectRatio ar);

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
    standardAspectRatio allowedAspectRatio;
};

#endif /* SESSIONSWITCHERWIDGET_H_ */
