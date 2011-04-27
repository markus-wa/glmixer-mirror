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

    void startTransitionToSession(const QModelIndex & index);
	// unblock the GUI suspended when loading new session
	void unsuspend();
    void selectSession(const QModelIndex & index);
    void nameFilterChanged(const QString &s);

    void setTransitionType(int t);
    void setTransitionMode(int m);
    void transitionSliderChanged(int t);
	void resetTransitionSlider();
	void setTransitionSourcePreview(Source *s);

    void customizeTransition();
    void saveSettings();
    void restoreSettings();

    void setAllowedAspectRatio(const standardAspectRatio ar);

Q_SIGNALS:
	void sessionTriggered(QString);

private:

	// folder toolbox
	QListWidget *createCurveIcons();
    QStandardItemModel *folderModel;
    QSortFilterProxyModel *proxyFolderModel;
    QTreeView *proxyView;
    QComboBox *folderHistory, *transitionSelection;
    QTabWidget *transitionTab;
    QSlider *transitionSlider;
    QToolButton *customButton;
    QSettings *appSettings;
    QListWidget *easingCurvePicker;
    QSpinBox *transitionDuration;
    QSize m_iconSize;
    standardAspectRatio allowedAspectRatio;

    class SourceDisplayWidget *overlayPreview;
    QLabel *currentSessionLabel, *nextSessionLabel;
    QString nextSession;
    bool nextSessionSelected, suspended;
};

#endif /* SESSIONSWITCHERWIDGET_H_ */
