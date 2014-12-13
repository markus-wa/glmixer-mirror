/*
 * SessionSwitcherWidget.h
 *
 *  Created on: Oct 1, 2010
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
 *   Copyright 2009, 2012 Bruno Herbelin
 *
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
    ~SessionSwitcherWidget();

public Q_SLOTS:

    void updateFolder();
    void openFolder(QString directory = QString::null);
    void discardFolder();
    void folderChanged( const QString & text );

    void startTransitionToSession(const QModelIndex & index);
    void startTransitionToNextSession();
    void startTransitionToPreviousSession();
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
    void setAvailable();

Q_SIGNALS:
	void sessionTriggered(QString);

private:

	// folder toolbox
	QListWidget *createCurveIcons();
    QStandardItemModel *folderModel;
    QSortFilterProxyModel *proxyFolderModel;
    class SearchingTreeView *proxyView;
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


class FolderModelFiller : public QThread
 {
     Q_OBJECT

     void run();

     QStandardItemModel *model;
     QString path;
     standardAspectRatio allowedAspectRatio;

public:
     FolderModelFiller(QObject *parent, QStandardItemModel *m, const QString &p, const standardAspectRatio allowedAR);
 };

#endif /* SESSIONSWITCHERWIDGET_H_ */
