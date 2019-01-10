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
#define MAX_RECURSE_FOLDERS 5

#include <QtGui>
#include "RenderingManager.h"

class SessionSwitcherWidget  : public QWidget {

    Q_OBJECT

public:
    SessionSwitcherWidget(QWidget *parent, QSettings *settings);
    virtual ~SessionSwitcherWidget();

public slots:

    bool openFolder(QString directory = QString::null);
    void folderChanged(const QString &foldername);
    void discardFolder();
    void reloadFolder();
    void browseFolder();
    void setRecursiveFolder(bool);
    void setViewSimplified(bool);

    void updateAndSelectFile(const QString &filename);
    void sessionNameChanged(QStandardItem *item);
    void sortingChanged(int, Qt::SortOrder);
    void openSession();
    void deleteSession();
    void renameSession();

    void startTransitionToSession(const QModelIndex & index);
    void startTransitionToNextSession();
    void startTransitionToPreviousSession();
    // unblock the GUI suspended when loading new session
    void unsuspend();
    void setTransitionDestinationSession(const QModelIndex & index);

    void setTransitionType(int t);
    void setTransitionMode(int m);
    void transitionSliderChanged(int t);
    void resetTransitionSlider();

    void customizeTransition();
    void saveSettings();
    void restoreSettings();

    void enableOnlyRenderingAspectRatio(bool);
    void restoreTransition();
    void restoreFolderView();

    // contex menu
    void ctxMenu(const QPoint &pos);

signals:
    void sessionTriggered(QString);
    void sessionRenamed(QString before, QString after);

protected:

    void showEvent(QShowEvent *);
    QStandardItem *selectFile(const QString &filename);

private:

    // folder and list of sessions
    QStandardItemModel *folderModel;
    QTreeView *proxyView;
    QMutex folderModelAccesslock;

    // GUI
    QComboBox *folderHistory, *transitionSelection;
    QTabWidget *transitionTab;
    QSlider *transitionSlider;
    QToolButton *customButton;
    QListWidget *easingCurvePicker;
    QSpinBox *transitionDuration;
    QWidget *transitionBox;
    QWidget *folderBox;
    QWidget *controlBox;

    QSize m_iconSize;
    QListWidget *createCurveIcons();

    QLabel *overlayLabel;
    QLabel *currentSessionLabel, *nextSessionLabel;
    QString nextSession;
    QToolButton *dirRecursiveButton;
    bool nextSessionSelected, suspended, recursive;

    // sorting stuff
    standardAspectRatio allowedAspectRatio;
    Qt::SortOrder sortingOrder;
    int sortingColumn;

    QSettings *appSettings;
    QAction *loadAction, *renameSessionAction, *deleteSessionAction, *openUrlAction;
};


class FolderModelFiller : public QThread
 {
     Q_OBJECT

     void fillFolder(QFileInfo folder, int depth = 0);
     void run();

     QStandardItemModel *model;
     QString path;
     int depth;

public:
     FolderModelFiller(QObject *parent, QStandardItemModel *m, QString p, int d);

};

#endif /* SESSIONSWITCHERWIDGET_H_ */
