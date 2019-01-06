/*
 * SessionSwitcherWidget.cpp
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

#include <QDomDocument>

#include "common.h"
#include "glmixer.h"
#include "SessionSwitcher.h"
#include "OutputRenderWindow.h"
#include "SourceDisplayWidget.h"

#include "SessionSwitcherWidget.moc"

QString stringFromAspectRatio(standardAspectRatio ar){
    QString aspectRatio;

    switch(ar) {
    case ASPECT_RATIO_FREE:
        aspectRatio = "free";
        break;
    case ASPECT_RATIO_16_10:
        aspectRatio = "16:10";
        break;
    case ASPECT_RATIO_16_9:
        aspectRatio = "16:9";
        break;
    case ASPECT_RATIO_3_2:
        aspectRatio = "3:2";
        break;
    default:
    case ASPECT_RATIO_4_3:
        aspectRatio = "4:3";
        break;
    }

    return aspectRatio;
}

bool fillItemData(QStandardItemModel *model, int row, QFileInfo fileinfo)
{
    if (!model)
        return false;

    QModelIndex i;
    i = model->index(row, 0);
    if (!i.isValid())
        return false;

    // read content of the file
    QString filename = fileinfo.absoluteFilePath();
    QFile file(filename);
    if ( !file.open(QFile::ReadOnly | QFile::Text) )
        return false;

    QDomDocument doc;
    if ( !doc.setContent(&file, true) )
        return false;

    QDomElement root = doc.documentElement();
    if ( root.tagName() != "GLMixer" )
        return false;

    QDomElement srcconfig = root.firstChildElement("SourceList");
    if ( srcconfig.isNull() )
        return false;

    // get number of sources in the session
    int nbElem = srcconfig.childNodes().count();

    // get aspect ratio
    standardAspectRatio ar = (standardAspectRatio) srcconfig.attribute("aspectRatio", "0").toInt();
    QString aspectRatio = stringFromAspectRatio(ar);

    // read notes
    QString tooltip = filename;
    QDomElement notes = root.firstChildElement("Notes");
    if ( !notes.isNull() && !notes.text().isEmpty())
        tooltip = notes.text().split("\n").first();

    file.close();

    // fill the line
    i = model->index(row, 0);
    model->setData(i, fileinfo.completeBaseName());
    model->setData(i, filename, Qt::UserRole);
    model->setData(i, (int) ar, Qt::UserRole+1);
    model->itemFromIndex (i)->setFlags (Qt::ItemIsSelectable | Qt::ItemIsEnabled | Qt::ItemIsEditable );
    model->itemFromIndex (i)->setToolTip(tooltip);

    i = model->index(row, 1);
    model->setData(i, nbElem);
    model->setData(i, filename, Qt::UserRole);
    model->itemFromIndex (i)->setFlags (Qt::ItemIsSelectable | Qt::ItemIsEnabled);
    model->itemFromIndex (i)->setToolTip(tooltip);

    i = model->index(row, 2);
    model->setData(i, aspectRatio);
    model->setData(i, filename, Qt::UserRole);
    model->itemFromIndex (i)->setFlags (Qt::ItemIsSelectable | Qt::ItemIsEnabled);
    model->itemFromIndex (i)->setToolTip(tooltip);

    i = model->index(row, 3);
    model->setData(i, fileinfo.lastModified().toString("yy.MM.dd hh:mm"));
    model->setData(i, filename, Qt::UserRole);
    model->itemFromIndex (i)->setFlags (Qt::ItemIsSelectable | Qt::ItemIsEnabled);
    model->itemFromIndex (i)->setToolTip(tooltip);

    // all good
    return true;
}


void FolderModelFiller::run()
{
    if (model) {

        // empty list
        if (model->rowCount() > 0)
            model->removeRows(0, model->rowCount());

        // here test for folder to exist with QFile Info
        QFileInfo folder(path);
        fillFolder(folder, depth);

    }
}


void FolderModelFiller::fillFolder(QFileInfo folder, int depth)
{
    // recursive limit
    if (depth > MAX_RECURSE_FOLDERS)
        return;

    if (folder.exists() && folder.isDir()) {

        QDir dir(folder.absoluteFilePath());

        // recursive for subfolders
        QFileInfoList dirList = dir.entryInfoList(QDir::Dirs | QDir::Readable | QDir::NoDotAndDotDot);
        for (int d = 0; d < dirList.size(); ++d) {
            fillFolder( dirList.at(d), depth + 1);
        }

        // fill list of glms
        QFileInfoList fileList = dir.entryInfoList(QStringList("*.glm"), QDir::Files | QDir::Readable );
        for (int f = 0; f < fileList.size(); ++f) {
            // create a new line
            model->insertRow(0);
            // fill the line
            if ( !fillItemData(model, 0, fileList.at(f) ) ) {
                qWarning() << fileList.at(f).absoluteFilePath() << QChar(124).toLatin1()
                           << tr("Ignoring invalid glm file.");
                // undo the new line on error
                model->removeRow(0);
            }
        }

    }
}


FolderModelFiller::FolderModelFiller(QObject *parent, QStandardItemModel *m, QString p, int d)
    : QThread(parent), model(m), path(p), depth(d)
{

}


SessionSwitcherWidget::SessionSwitcherWidget(QWidget *parent, QSettings *settings) : QWidget(parent),
    appSettings(settings), m_iconSize(48,48), nextSessionSelected(false), suspended(false), recursive(false)
{
    QGridLayout *g;

    /**
     * transition control & tab
     */
    transitionSelection = new QComboBox(this);
    transitionSelection->addItem("Instantaneous");
    transitionSelection->addItem("Fade to black");
    transitionSelection->addItem("Fade to custom color");
    transitionSelection->addItem("Fade with last frame");
    transitionSelection->addItem("Fade with image file");
    transitionSelection->setToolTip(tr("Select the transition mode"));
    transitionSelection->setCurrentIndex(-1);
    transitionSelection->setSizePolicy(QSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred));

    customButton = new QToolButton(this);
    customButton->setIcon( QIcon() );
    customButton->setVisible(false);

    transitionTab = new QTabWidget(this);
    transitionTab->setToolTip(tr("How you control the transition"));
    transitionTab->setSizePolicy(QSizePolicy(QSizePolicy::Preferred, QSizePolicy::Maximum));

    // put all together in the Transition Box
    transitionBox = new QWidget(this);
    g = new QGridLayout(this);
    g->setContentsMargins(0, 0, 0, 0);
    g->addWidget(transitionSelection, 0, 0);
    g->addWidget(customButton, 0, 1);
    g->addWidget(transitionTab, 1, 0, 1, 2);
    transitionBox->setLayout(g);

    /**
     * Tab automatic
     */
    transitionDuration = new QSpinBox(this);
    transitionDuration->setSingleStep(200);
    transitionDuration->setMinimum(100);
    transitionDuration->setMaximum(5000);
    transitionDuration->setValue(1000);
    transitionDuration->setSuffix(" ms");
    transitionDuration->setToolTip(tr("How long is the transition (miliseconds)"));
    QLabel *transitionDurationLabel;
    transitionDurationLabel = new QLabel(tr("Duration "));
    transitionDurationLabel->setBuddy(transitionDuration);

    // create the curves into the transition easing curve selector
    easingCurvePicker = createCurveIcons();
    easingCurvePicker->setViewMode(QListView::IconMode);
    easingCurvePicker->setWrapping (false);
    easingCurvePicker->setIconSize(m_iconSize);
    easingCurvePicker->setFixedHeight(m_iconSize.height()+26);
    easingCurvePicker->setCurrentRow(3);
    easingCurvePicker->setToolTip(tr("How the transition is done through time."));

    transitionTab->addTab( new QWidget(), "Automatic");
    g = new QGridLayout(this);
    g->setContentsMargins(6, 6, 6, 6);
    g->addWidget(transitionDurationLabel, 1, 0);
    g->addWidget(transitionDuration, 1, 1);
    g->addWidget(easingCurvePicker, 2, 0, 1, 2);
    transitionTab->widget(0)->setLayout(g);

    /**
     * Tab manual
     */
    currentSessionLabel = new QLabel(this);
    currentSessionLabel->setText(tr("100%"));
    currentSessionLabel->setSizePolicy( QSizePolicy::Preferred, QSizePolicy::Preferred);
    currentSessionLabel->setAlignment(Qt::AlignLeading|Qt::AlignLeft|Qt::AlignBottom);
    currentSessionLabel->setToolTip(tr("Percent of current session visible."));

    nextSessionLabel = new QLabel(this);
    nextSessionLabel->setText(tr("0%"));
    nextSessionLabel->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
    nextSessionLabel->setAlignment(Qt::AlignBottom|Qt::AlignRight|Qt::AlignTrailing);
    nextSessionLabel->setToolTip(tr("Percent of destination session visible."));

    overlayLabel = new QLabel(this);
    overlayLabel->setText(tr("Select destination\n(double clic session)"));
    overlayLabel->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
    overlayLabel->setAlignment(Qt::AlignCenter);
    overlayLabel->setToolTip(tr("Double clic on a session to activate destination."));

    transitionSlider = new QSlider(this);
    transitionSlider->setMinimum(-100);
    transitionSlider->setMaximum(101);
    transitionSlider->setValue(-100);
    transitionSlider->setOrientation(Qt::Horizontal);
    transitionSlider->setTickPosition(QSlider::TicksAbove);
    transitionSlider->setTickInterval(100);
    transitionSlider->setEnabled(false);
    transitionSlider->setToolTip(tr("Slide to the right to open destination."));

    transitionTab->addTab( new QWidget(), "Manual");
    g = new QGridLayout(this);
    g->setContentsMargins(6, 6, 6, 6);
    g->addWidget(currentSessionLabel, 0, 0);
    g->addWidget(overlayLabel, 0, 1);
    g->addWidget(nextSessionLabel, 0, 2);
    g->addWidget(transitionSlider, 1, 0, 1, 3);
    transitionTab->widget(1)->setLayout(g);

    /**
     * Folder view
     */
    folderModel = new QStandardItemModel(0, 4, this);
    folderModel->setHeaderData(0, Qt::Horizontal, QObject::tr("Filename"));
    folderModel->setHeaderData(1, Qt::Horizontal, QString("n"));
    folderModel->setHeaderData(2, Qt::Horizontal, QObject::tr("W:H"));
    folderModel->setHeaderData(3, Qt::Horizontal, QObject::tr("Date"));

    sortingColumn = 3;
    sortingOrder = Qt::AscendingOrder;

    QToolButton *dirButton = new QToolButton(this);
    dirButton->setToolTip(tr("Add a folder to the list"));
    QIcon icon;
    icon.addFile(QString::fromUtf8(":/glmixer/icons/folderadd.png"), QSize(16,16), QIcon::Normal, QIcon::Off);
    dirButton->setIcon(icon);

    QToolButton *dirDeleteButton = new QToolButton(this);
    dirDeleteButton->setToolTip(tr("Remove folder from the list"));
    QIcon icon2;
    icon2.addFile(QString::fromUtf8(":/glmixer/icons/fileclose.png"), QSize(16,16), QIcon::Normal, QIcon::Off);
    dirDeleteButton->setIcon(icon2);

    dirRecursiveButton = new QToolButton(this);
    dirRecursiveButton->setCheckable(true);
    dirRecursiveButton->setToolTip(tr("Search in subfolders (max depth %1)").arg(MAX_RECURSE_FOLDERS));
    QIcon icon3;
    icon3.addFile(QString::fromUtf8(":/glmixer/icons/foldersearch.png"), QSize(16,16), QIcon::Normal);
    dirRecursiveButton->setIcon(icon3);

    folderHistory = new QComboBox(this);
    folderHistory->setToolTip(tr("List of folders containing session files"));
    folderHistory->setValidator(new folderValidator(this));
    folderHistory->setInsertPolicy (QComboBox::InsertAtTop);
    folderHistory->setMaxCount(MAX_RECENT_FOLDERS);
    folderHistory->setSizeAdjustPolicy(QComboBox::AdjustToContents);
    folderHistory->setDuplicatesEnabled(false);
    folderHistory->setLayoutDirection(Qt::RightToLeft);

    // put all together in the Transition Box
    folderBox = new QWidget(this);
    g = new QGridLayout(this);
    g->setContentsMargins(0, 0, 0, 0);
    g->addWidget(dirButton, 0, 0);
    g->addWidget(folderHistory, 0, 1);
    g->addWidget(dirDeleteButton, 0, 2);
    g->addWidget(dirRecursiveButton, 0, 3);
    folderBox->setLayout(g);

    /**
     * Proxy View: list sessions
     */
    proxyView = new QTreeView(this);
    proxyView->setModel(folderModel);
    proxyView->setSortingEnabled(true);
    proxyView->setEditTriggers(QAbstractItemView::NoEditTriggers);
    proxyView->setSelectionMode(QAbstractItemView::SingleSelection);
    proxyView->setRootIsDecorated(false);
    proxyView->setAlternatingRowColors(true);
    proxyView->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    proxyView->setStyleSheet(QString::fromUtf8("QToolTip {\n"
        "	font: 8pt \"%1\";\n"
        "}").arg(getMonospaceFont()));

    /**
     * Proxy View context & actions
     */
    proxyView->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(proxyView, SIGNAL(customContextMenuRequested(const QPoint &)), SLOT(ctxMenu(const QPoint &)));

    loadAction = new QAction(QIcon(":/glmixer/icons/fileopen.png"), tr("Open"), proxyView);
    proxyView->addAction(loadAction);
    QObject::connect(loadAction, SIGNAL(triggered()), this, SLOT(openSession()) );

    renameSessionAction = new QAction(QIcon(":/glmixer/icons/rename.png"), tr("Rename"), proxyView);
    proxyView->addAction(renameSessionAction);
    QObject::connect(renameSessionAction, SIGNAL(triggered()), this, SLOT(renameSession()) );

    deleteSessionAction = new QAction(QIcon(":/glmixer/icons/fileclose.png"), tr("Delete"), proxyView);
    proxyView->addAction(deleteSessionAction);
    QObject::connect(deleteSessionAction, SIGNAL(triggered()), this, SLOT(deleteSession()) );

    openUrlAction = new QAction(QIcon(":/glmixer/icons/folderopen.png"), tr("Show file in browser"), proxyView);
    proxyView->addAction(openUrlAction);
    QObject::connect(openUrlAction, SIGNAL(triggered()), this, SLOT(browseFolder()) );

    /**
     * control box
     */
    QPushButton *nextButton = new QPushButton(QIcon(":/glmixer/icons/media-skip-forward.png"), tr(" Next"), this);
    nextButton->setSizePolicy(QSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred));
    nextButton->setToolTip(tr("Open next session in list."));
    nextButton->setIconSize(QSize(24, 24));
    QPushButton *prevButton = new QPushButton(QIcon(":/glmixer/icons/media-skip-backward.png"), tr(" Previous"), this);
    prevButton->setSizePolicy(QSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred));
    prevButton->setToolTip(tr("Open previous session in list."));
    prevButton->setIconSize(QSize(24, 24));

    controlBox = new QWidget(this);
    g = new QGridLayout(this);
    g->setContentsMargins(0, 0, 0, 0);
    g->addWidget(prevButton, 0, 0);
    g->addWidget(nextButton, 0, 1);
    controlBox->setLayout(g);
    controlBox->setVisible(false);

    /**
     * global layout
     */
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->addWidget(controlBox);
    mainLayout->addWidget(transitionBox);
    mainLayout->addWidget(proxyView);
    mainLayout->addWidget(folderBox);
    setLayout(mainLayout);

    /**
     * connections
     */
    connect(dirButton, SIGNAL(clicked()), SLOT(openFolder()));
    connect(dirDeleteButton, SIGNAL(clicked()), SLOT(discardFolder()));
    connect(dirRecursiveButton, SIGNAL(clicked(bool)), SLOT(setRecursiveFolder(bool)));
    connect(customButton, SIGNAL(clicked()), SLOT(customizeTransition()));
    connect(folderHistory, SIGNAL(activated(const QString &)), SLOT(folderChanged(const QString &)));
    connect(transitionSelection, SIGNAL(currentIndexChanged(int)), SLOT(setTransitionType(int)));
    connect(transitionDuration, SIGNAL(valueChanged(int)), RenderingManager::getSessionSwitcher(), SLOT(setTransitionDuration(int)));
    connect(easingCurvePicker, SIGNAL(currentRowChanged (int)), RenderingManager::getSessionSwitcher(), SLOT(setTransitionCurve(int)));
    connect(transitionSlider, SIGNAL(valueChanged(int)), SLOT(transitionSliderChanged(int)));
    connect(transitionTab, SIGNAL(currentChanged(int)), SLOT(setTransitionMode(int)));
    connect(nextButton, SIGNAL(clicked()), SLOT(startTransitionToNextSession()));
    connect(prevButton, SIGNAL(clicked()), SLOT(startTransitionToPreviousSession()));
    connect(proxyView->header(), SIGNAL(sortIndicatorChanged(int, Qt::SortOrder)), SLOT(sortingChanged(int, Qt::SortOrder)));

}

SessionSwitcherWidget::~SessionSwitcherWidget()
{
//    delete proxyFolderModel;
}

void SessionSwitcherWidget::reloadFolder()
{
    QFileInfo sessionFolder( folderHistory->currentText() );

    if ( sessionFolder.exists() )
        folderChanged( sessionFolder.absoluteFilePath() );

}

void SessionSwitcherWidget::browseFolder()
{
    QFileInfo sessionFolder( folderHistory->currentText() );

    // in case of recursive, browse folder should open the item folder
    if (proxyView->currentIndex().isValid()) {
        QFileInfo sessionFile( folderModel->data(proxyView->currentIndex(), Qt::UserRole).toString() );
        if (sessionFile.isFile())
            sessionFolder = QFileInfo( sessionFile.absolutePath() );
    }

    if ( sessionFolder.exists() ) {

        QUrl sessionURL = QUrl::fromLocalFile( sessionFolder.absoluteFilePath() ) ;

        if ( sessionURL.isValid() )
            QDesktopServices::openUrl(sessionURL);
    }
}


void SessionSwitcherWidget::setRecursiveFolder(bool on)
{
    recursive = on;
    reloadFolder();
}

void SessionSwitcherWidget::setViewSimplified(bool on)
{
    proxyView->setHeaderHidden(on);
    proxyView->header()->setSectionHidden(1, on );
    proxyView->header()->setSectionHidden(2, on );
    proxyView->header()->setSectionHidden(3, on );

    controlBox->setVisible(on);
    transitionBox->setHidden(on);
    folderBox->setHidden(on);
}

void SessionSwitcherWidget::openSession()
{
    if (!proxyView->currentIndex().isValid())
        return;

    startTransitionToSession(proxyView->currentIndex());
}

void SessionSwitcherWidget::deleteSession()
{
    if (!proxyView->currentIndex().isValid())
        return;

    QFileInfo sessionFile( folderModel->data(proxyView->currentIndex(), Qt::UserRole).toString() );
    if ( sessionFile.isFile() ) {

        // request confirmation if session is open
        if ( GLMixer::getInstance()->getCurrentSessionFilename() == sessionFile.absoluteFilePath() )  {
            QMessageBox::information(this, tr("%1").arg(QCoreApplication::applicationName()), tr("You cannot remove session %1 when it is openned.").arg(sessionFile.fileName()));
        }
        else {
            QString msg = tr("Do you really want to delete this file?\n\n%1").arg(sessionFile.absoluteFilePath());
            if ( QMessageBox::question(this, tr("%1 - Are you sure?").arg(QCoreApplication::applicationName()), msg,
                QMessageBox::Yes | QMessageBox::No, QMessageBox::Yes) == QMessageBox::Yes) {
                // delete file
                QDir sessionDir(sessionFile.canonicalPath());
                sessionDir.remove(sessionFile.fileName());
                reloadFolder();
            }
        }
    }
}

void SessionSwitcherWidget::renameSession()
{
    if (!proxyView->currentIndex().isValid())
        return;

    QFileInfo sessionFile( folderModel->data(proxyView->currentIndex(), Qt::UserRole).toString() );

    if ( sessionFile.isFile() ) {

        // grab edit signal
        connect(folderModel, SIGNAL(itemChanged(QStandardItem *)), this, SLOT(sessionNameChanged(QStandardItem *)));
        // trigger edit of first column in current index
        QModelIndex index = proxyView->model()->index( proxyView->currentIndex().row(), 0);

        if (index.isValid())
            proxyView->edit(index);

    }
}

void SessionSwitcherWidget::sessionNameChanged( QStandardItem * item )
{
    if (!item)
        return;

    // discard edit signal
    disconnect(folderModel, SIGNAL(itemChanged(QStandardItem *)), this, SLOT(sessionNameChanged(QStandardItem *)));

    // access session file
    QFileInfo sessionFile( item->data(Qt::UserRole).toString() );
    if ( sessionFile.isFile() ) {

        // set folder and filename
        QDir sessionDir(sessionFile.canonicalPath());
        QFileInfo newSessionFile(sessionDir, QString("%1.glm").arg(item->text()));

        // actual renaming of the file
        if ( sessionDir.rename( sessionFile.fileName(), newSessionFile.fileName() ) ) {

            qDebug() << sessionFile.absoluteFilePath() << QChar(124).toLatin1()
                     << tr("Session file renamed to '%1'").arg(newSessionFile.fileName());

            // inform main GUI
            emit sessionRenamed(sessionFile.absoluteFilePath(), newSessionFile.absoluteFilePath());

            // update the file item
            if (folderModelAccesslock.tryLock(100)) {
                // update the item data
                if ( !fillItemData(folderModel, item->row(), newSessionFile) ) {
                    qWarning() << newSessionFile.absoluteFilePath() << QChar(124).toLatin1()
                               << tr("failed update data renamed session file.");
                }

                folderModelAccesslock.unlock();
            }
        }
        else {
            // cancel item edit
            item->setText( sessionFile.completeBaseName() );
            qWarning() << sessionFile.absoluteFilePath() << QChar(124).toLatin1()
                       << tr("Failed to rename session file.");
        }

    }

    proxyView->sortByColumn(sortingColumn, sortingOrder);
}

void SessionSwitcherWidget::fileChanged(const QString & filename )
{
    QFileInfo fileinfo(filename);

    if ( fileinfo.exists() && fileinfo.isFile() ) {

        // the folder is already the good one, change only the file item
        if (folderModelAccesslock.tryLock(100)) {

            // look for item in the list
            QList<QStandardItem *> items = folderModel->findItems( fileinfo.completeBaseName() );

            // if couldn't find file in the list,
            if (items.isEmpty()) {
                // done with folder Model
                folderModelAccesslock.unlock();
                // try loading the folder
                openFolder(fileinfo.absolutePath());
            }
            // else, update modified fields for all files found
            else {

                // loop over the list of items with the given name
                QModelIndex index;
                QStandardItem *item = NULL;
                while( !items.isEmpty()){
                    item = items.takeFirst();
                    index = item->index();

                    // if we found an item with the same filename and path
                    if ( index.isValid() && fileinfo.absoluteFilePath() == folderModel->data(index, Qt::UserRole).toString() ) {

                        // update the item data
                        if ( !fillItemData(folderModel, item->row(), fileinfo) )
                            qWarning() << fileinfo.absoluteFilePath() << QChar(124).toLatin1()
                                       << tr("Ignoring invalid glm file.");

                        break;
                    }
                }

                // done with folder Model
                folderModelAccesslock.unlock();

                // restore selection in tree view
                if (index.isValid())
                    proxyView->setCurrentIndex( index );
            }
        }
    }
    else
        proxyView->setCurrentIndex( QModelIndex () );
}

void SessionSwitcherWidget::folderChanged(const QString & foldername )
{
    // remove (if exist) the path from list and reorder it
    QStringList folders = appSettings->value("recentFolderList").toStringList();
    folders.removeAll(foldername);
    folders.prepend(foldername);
    // limit list size
    while (folders.size() > MAX_RECENT_FOLDERS)
        folders.removeLast();
    appSettings->setValue("recentFolderList", folders);

    if ( folderModelAccesslock.tryLock(100) ) {

        // Threaded version of fillFolderModel(folderModel, text);
        FolderModelFiller *workerThread = new FolderModelFiller(this, folderModel, foldername, recursive ? 0 : MAX_RECURSE_FOLDERS);
        if (!workerThread) {
            folderModelAccesslock.unlock();
            return;
        }

        // do not interfere with auto sorting while loading
        setEnabled(false);

        // todo when done
        connect(workerThread, SIGNAL(finished()), this, SLOT(restoreFolderView()));
        connect(workerThread, SIGNAL(finished()), workerThread, SLOT(deleteLater()));

        // do the job in parallel
        workerThread->start();
        // NB: the lock will be released in restoreFolderView after thread is finished
    }
}


void SessionSwitcherWidget::restoreFolderView()
{
    // restore sorting
    proxyView->sortByColumn(sortingColumn, sortingOrder);
    setEnabled(true);

    // restore availability
    folderModelAccesslock.unlock();

    // inform user
    qDebug() << folderHistory->currentText() << QChar(124).toLatin1() << tr("Session switcher ready with %1 session files.").arg(folderModel->rowCount());
}

bool SessionSwitcherWidget::openFolder(QString directory)
{
    // open file dialog if no directory is given
    QString dirName;
    if ( directory.isNull() )
        dirName = QFileDialog::getExistingDirectory(0, QObject::tr("Select a directory"), QDir::currentPath(), QFileDialog::ShowDirsOnly );
    else
        dirName = directory;

    // make sure we have a valid directory
    QFileInfo sessionFolder (dirName);
    if ( !sessionFolder.isDir() )
        return false;

    // nothing to do is its already the current directory
    QFileInfo currentFolder( folderHistory->currentText() );
    if ( sessionFolder == currentFolder )
        return false;

    // find the index of the given directory
    int index = folderHistory->findText(dirName);

    // if not found, then insert it
    if ( index < 0 ) {
        folderHistory->insertItem(0, sessionFolder.absoluteFilePath());
        index = 0;
    }

    // change the current index (this triggers the reloading of the list)
    folderHistory->setCurrentIndex( index );

    // display it
    reloadFolder();

    return true;
}

void SessionSwitcherWidget::discardFolder()
{
    if (folderHistory->count() > 0) {
        // remove the folder from the settings
        QStringList folders = appSettings->value("recentFolderList").toStringList();
        folders.removeAll( folderHistory->currentText() );
        appSettings->setValue("recentFolderList", folders);

        // remove the item
        folderHistory->removeItem(folderHistory->currentIndex());

        // display another
        reloadFolder();
    }
}


void SessionSwitcherWidget::startTransitionToSession(const QModelIndex & index)
{
    // allow activation of session only if item is enabled
    if ( index.isValid() && folderModel->flags(index) & Qt::ItemIsEnabled )
    {
        // transfer info to glmixer
        emit sessionTriggered(folderModel->data(index, Qt::UserRole).toString());

        // make sure no other events are accepted until the end of the transition
        disconnect(proxyView, SIGNAL(activated(QModelIndex)), this, SLOT(startTransitionToSession(QModelIndex) ));
        proxyView->setEnabled(false);
        QTimer::singleShot( transitionSelection->currentIndex() > 0 ? transitionDuration->value() : 100, this, SLOT(restoreTransition()));

    }
}

void SessionSwitcherWidget::startTransitionToNextSession()
{
    QModelIndex index = proxyView->currentIndex();
    if (index.isValid())
        index = proxyView->indexBelow(index);
    else
        index = folderModel->item(0)->index();

    if (index.isValid()) {
        startTransitionToSession(index);
        proxyView->setCurrentIndex(index);
    }
}

void SessionSwitcherWidget::startTransitionToPreviousSession()
{
    QModelIndex index = proxyView->currentIndex();
    if (index.isValid())
        index = proxyView->indexAbove(index);
    else
        index = folderModel->item(0)->index();

    if (index.isValid()) {
        startTransitionToSession(index);
        proxyView->setCurrentIndex(index);
    }
}

void SessionSwitcherWidget::setTransitionDestinationSession(const QModelIndex & index)
{
    if (!index.isValid())
        return;

    // read file name
    nextSession = folderModel->data(index, Qt::UserRole).toString();
    transitionSlider->setEnabled(true);
    currentSessionLabel->setEnabled(true);
    nextSessionLabel->setEnabled(true);

    // display that we can do transition to new selected session
    nextSessionLabel->setText(QString("0%"));
    overlayLabel->setText(QString("Current destination is:\n'%1'").arg(QFileInfo(nextSession).baseName()));
}

void SessionSwitcherWidget::setTransitionType(int t)
{
    SessionSwitcher::transitionType tt = (SessionSwitcher::transitionType) CLAMP(SessionSwitcher::TRANSITION_NONE, t, SessionSwitcher::TRANSITION_CUSTOM_MEDIA);
    RenderingManager::getSessionSwitcher()->setTransitionType( tt );

    // revert to default status
    customButton->setVisible(false);
    customButton->setStyleSheet("QToolButton { padding: 1px;}");
    transitionTab->setVisible(tt != SessionSwitcher::TRANSITION_NONE);
    transitionTab->widget(1)->setEnabled(true);

    // hack ; NONE transition type should emulate automatic transition mode
    setTransitionMode(tt == SessionSwitcher::TRANSITION_NONE ? 0 : transitionTab->currentIndex());

    // adjust GUI depending on transition
    if ( tt == SessionSwitcher::TRANSITION_CUSTOM_COLOR ) {
        QPixmap c = QPixmap(16, 16);
        c.fill(RenderingManager::getSessionSwitcher()->transitionColor());
        customButton->setIcon( QIcon(c) );
        customButton->setVisible(true);
        customButton->setToolTip("Choose color");

    } else if ( tt == SessionSwitcher::TRANSITION_CUSTOM_MEDIA ) {
        customButton->setIcon(QIcon(QString::fromUtf8(":/glmixer/icons/folderopen.png")));
        customButton->setVisible(true);

        if ( !QFileInfo(RenderingManager::getSessionSwitcher()->transitionMedia()).exists() ) {
            customButton->setStyleSheet("QToolButton { border: 1px solid red; }");
            customButton->setToolTip("Choose Image");
        }
        else
            customButton->setToolTip(QString("%1").arg(RenderingManager::getSessionSwitcher()->transitionMedia()));
    }
    else if ( tt == SessionSwitcher::TRANSITION_LAST_FRAME ) {
        // manual transition impossible in this mode
        transitionTab->widget(1)->setEnabled(false);

    }


}


void SessionSwitcherWidget::customizeTransition()
{
    if (RenderingManager::getSessionSwitcher()->getTransitionType() == SessionSwitcher::TRANSITION_CUSTOM_COLOR ) {

        QColor color = QColorDialog::getColor(RenderingManager::getSessionSwitcher()->transitionColor(), parentWidget());
        if (color.isValid()) {
            RenderingManager::getSessionSwitcher()->setTransitionColor(color);
            setTransitionType( (int) SessionSwitcher::TRANSITION_CUSTOM_COLOR);
        }
    }
    else if (RenderingManager::getSessionSwitcher()->getTransitionType() == SessionSwitcher::TRANSITION_CUSTOM_MEDIA ) {

        QString fileName = GLMixer::getInstance()->getFileName(tr("Open Image"), tr("Image") + " (*.png *.jpg)");

        // media file dialog returns a list of filenames :
        if (QFileInfo(fileName).exists()) {

            RenderingManager::getSessionSwitcher()->setTransitionMedia(fileName);
            customButton->setStyleSheet("");
            customButton->setToolTip(QString("%1").arg(RenderingManager::getSessionSwitcher()->transitionMedia()));
        }
        // no valid file name was given
        else {
            // not a valid file ; show a warning only if the QFileDialog did not return null (cancel)
            if (!fileName.isEmpty())
                qCritical() << fileName << QChar(124).toLatin1() << QObject::tr("File does not exist.");
            // if no valid oldfile neither; show icon in red
            if (RenderingManager::getSessionSwitcher()->transitionMedia().isEmpty())
                customButton->setStyleSheet("QToolButton { border: 1px solid red }");
        }
    }
    // remember
    saveSettings();
}


void SessionSwitcherWidget::saveSettings()
{
    appSettings->setValue("transitionSelection", transitionSelection->currentIndex());
    appSettings->setValue("transitionDuration", transitionDuration->value());
    appSettings->setValue("transitionCurve", easingCurvePicker->currentRow());

    if (RenderingManager::getSessionSwitcher()) {
        QVariant variant = RenderingManager::getSessionSwitcher()->transitionColor();
        appSettings->setValue("transitionColor", variant);
        appSettings->setValue("transitionMedia", RenderingManager::getSessionSwitcher()->transitionMedia());
    }

    appSettings->setValue("transitionSortingColumn", sortingColumn);
    appSettings->setValue("transitionSortingOrder", sortingOrder);
    appSettings->setValue("transitionHeader", proxyView->header()->saveState());

    appSettings->setValue("recentFolderRecursive", recursive);
    appSettings->setValue("recentFolderLast", folderHistory->currentText());
}

void SessionSwitcherWidget::restoreSettings()
{
    // list of folders
    QStringList folders(QDir::currentPath());
    int lastFolderIndex = -1;
    if ( appSettings->contains("recentFolderList") )
        folders = appSettings->value("recentFolderList").toStringList();
    if ( appSettings->contains("recentFolderLast") )
        lastFolderIndex = folders.indexOf(appSettings->value("recentFolderLast").toString());
    folderHistory->addItems( folders );
    folderHistory->setCurrentIndex(lastFolderIndex);
    // recursive ?
    recursive = appSettings->value("recentFolderRecursive", "false").toBool();
    dirRecursiveButton->setChecked(recursive);

    // transition configs
    RenderingManager::getSessionSwitcher()->setTransitionColor( appSettings->value("transitionColor").value<QColor>());
    QString mediaFileName = appSettings->value("transitionMedia", "").toString();
    if (QFileInfo(mediaFileName).exists())
        RenderingManager::getSessionSwitcher()->setTransitionMedia(mediaFileName);

    transitionDuration->setValue(appSettings->value("transitionDuration", "1000").toInt());
    transitionSelection->setCurrentIndex(appSettings->value("transitionSelection", "0").toInt());
    easingCurvePicker->setCurrentRow(appSettings->value("transitionCurve", "3").toInt());

    // list of sessions
    // saved settings
    if ( appSettings->contains("transitionHeader") )
        proxyView->header()->restoreState( appSettings->value("transitionHeader").toByteArray() );
    // enforce correct settings
    proxyView->header()->setDragEnabled(false);
    proxyView->header()->setMinimumSectionSize (40);
    proxyView->header()->setStretchLastSection(false);
    proxyView->header()->setResizeMode(0, QHeaderView::Stretch);
    proxyView->header()->setResizeMode(1, QHeaderView::Fixed);
    proxyView->header()->resizeSection(1, 40);
    proxyView->header()->setResizeMode(2, QHeaderView::Fixed);
    proxyView->header()->resizeSection(2, 55);
    proxyView->header()->setResizeMode(3, QHeaderView::ResizeToContents);
    setViewSimplified(false);

    // order of transition
    sortingColumn = appSettings->value("transitionSortingColumn", "3").toInt();
    sortingOrder = (Qt::SortOrder) appSettings->value("transitionSortingOrder", "0").toInt();

    // next time parameter changes, save settings
    connect(transitionSelection, SIGNAL(currentIndexChanged(int)), this, SLOT(saveSettings()));
    connect(transitionDuration, SIGNAL(valueChanged(int)), this, SLOT(saveSettings()));
    connect(easingCurvePicker, SIGNAL(currentRowChanged (int)), this, SLOT(saveSettings()));
    connect(dirRecursiveButton, SIGNAL(clicked()), SLOT(saveSettings()));
}

QListWidget *SessionSwitcherWidget::createCurveIcons()
{
    // default icon size is 48x48
    int zoom = 3; // magnification factor for creation of icon (must be > 0)
    qreal margin = 5.0;  // margins (top, boton, right & left)

    QPixmap pix(m_iconSize * zoom); // pix size is (48x48) * zoom
    QPainter painter(&pix);
    painter.setRenderHint(QPainter::Antialiasing, true);
    margin *= (float) zoom;
    qreal xAxis = (float) pix.width() - margin;
    qreal yAxis = margin;
    qreal curveScale = (float) pix.width() - margin * 2.0;
    qreal inc = 1.0 / curveScale;

    // ready to create list widget
    QListWidget *easingCurvePicker = new QListWidget;

    // loop on every curve type
    // Skip QEasingCurve::Custom
    for (int i = 0; i <= QEasingCurve::OutInBounce; ++i) {
        // background & axis
        painter.fillRect(QRect(QPoint(0, 0), pix.size()), QApplication::palette().base());
        painter.setPen(QPen(QApplication::palette().alternateBase().color(), zoom));
        painter.drawLine(0, xAxis, pix.width(),  xAxis);
        painter.drawLine(yAxis, 0, yAxis, pix.height());

        // curve
        QEasingCurve curve((QEasingCurve::Type)i);

        // start point
        painter.setPen(QPen(Qt::red, zoom*2));
        QPoint start(yAxis, xAxis - curveScale * curve.valueForProgress(0));
        painter.drawRect(start.x() - 1, start.y() - 1, 3, 3);

        // end point
        painter.setPen(QPen(Qt::blue, zoom*2));
        QPoint end(yAxis + curveScale, xAxis - curveScale * curve.valueForProgress(1));
        painter.drawRect(end.x() - 1, end.y() - 1, 3, 3);

        // curve points
        QPainterPath curvePath;
        curvePath.moveTo(start);
        for (qreal t = 0; t < 1.0 + inc; t += inc)
            curvePath.lineTo( QPoint(yAxis + curveScale * t, xAxis - curveScale * curve.valueForProgress(t)) );
        painter.strokePath(curvePath, QPen(QApplication::palette().text().color(), zoom));

        // add icon
        QListWidgetItem *item = new QListWidgetItem;
        item->setIcon(QIcon(pix));
        // scaling not needed (improves rendering quality in retina displays)
        // item->setIcon(QIcon(pix.scaled(m_iconSize, Qt::IgnoreAspectRatio, Qt::SmoothTransformation)));
        easingCurvePicker->addItem(item);
    }

    return easingCurvePicker;
}



void SessionSwitcherWidget::setAllowedAspectRatio(const standardAspectRatio ar)
{
    if (folderModelAccesslock.tryLock(100)) {

        // quick redisplay of folder list
        for (int r = 0; r < folderModel->rowCount(); ++r )
        {
            // items are always selectable
            Qt::ItemFlags flags = Qt::ItemIsSelectable;
            // read aspect ratio of item
            standardAspectRatio sar = (standardAspectRatio) folderModel->data(folderModel->index(r, 0), Qt::UserRole+1).toInt();
            // disable items which are not in the given aspect ratio
            if (ar == ASPECT_RATIO_FREE || sar == ar)
                flags |= Qt::ItemIsEnabled;
            // apply flag
            folderModel->itemFromIndex(folderModel->index(r, 0))->setFlags (flags);
            folderModel->itemFromIndex(folderModel->index(r, 1))->setFlags (flags);
            folderModel->itemFromIndex(folderModel->index(r, 2))->setFlags (flags);
            folderModel->itemFromIndex(folderModel->index(r, 3))->setFlags (flags);
        }

        folderModelAccesslock.unlock();
    }

}


void SessionSwitcherWidget::resetTransitionSlider()
{
    // enable / disable transition slider
    transitionSlider->setEnabled(nextSessionSelected);
    currentSessionLabel->setEnabled(nextSessionSelected);
    nextSessionLabel->setEnabled(nextSessionSelected);
    // enable / disable changing session
    proxyView->setEnabled(!nextSessionSelected);
    // clear the selection
    proxyView->clearSelection();

    if(!nextSessionSelected)
        overlayLabel->setText(tr("Select destination\n(double clic session)"));

}

void  SessionSwitcherWidget::setTransitionMode(int m)
{
    resetTransitionSlider();

    // mode is manual (and not with instantaneous transition selected)
    if ( m == 1  && transitionSelection->currentIndex() > 0) {
        RenderingManager::getSessionSwitcher()->manual_mode = true;
        // adjust slider to represent current transparency
        transitionSlider->setValue(RenderingManager::getSessionSwitcher()->overlay() * 100.f - (nextSessionSelected?0.f:100.f));
        // single clic to select next session
        proxyView->setToolTip("Double click on a session to choose target session");
        disconnect(proxyView, SIGNAL(activated(QModelIndex)), this, SLOT(startTransitionToSession(QModelIndex) ));
        connect(proxyView, SIGNAL(activated(QModelIndex)), this, SLOT(setTransitionDestinationSession(QModelIndex) ));

    }
    // mode is automatic
    else {
        RenderingManager::getSessionSwitcher()->manual_mode = false;
        // enable changing session
        proxyView->setEnabled(true);
        //  activate transition to next session (double clic or Return)
        proxyView->setToolTip("Double click on a session to initiate the transition");
        connect(proxyView, SIGNAL(activated(QModelIndex)), this, SLOT(startTransitionToSession(QModelIndex) ));
        disconnect(proxyView, SIGNAL(activated(QModelIndex)), this, SLOT(setTransitionDestinationSession(QModelIndex) ));

        easingCurvePicker->scrollToItem(easingCurvePicker->item( easingCurvePicker->currentRow() ), QAbstractItemView::PositionAtCenter );
    }

}

void SessionSwitcherWidget::restoreTransition()
{
    setTransitionMode(transitionTab->currentIndex());

    // restore focus and selection in tree view
    proxyView->setFocus();
    proxyView->setCurrentIndex(proxyView->currentIndex());
}

void SessionSwitcherWidget::transitionSliderChanged(int t)
{
    // apply transition
    RenderingManager::getSessionSwitcher()->setTransparency( ABS(t) );

    if (suspended) {
        transitionSlider->setValue(0);
        return;
    }

    if ( nextSessionSelected ) {

        // display that we can do transition to new selected session
        nextSessionLabel->setText(QString("%1%").arg(ABS(t)));

        // prevent coming back to previous
        if (t < 0) {
            transitionSlider->setValue(0);
        } else

            // detect end of transition
            if ( t > 100 ) {
                // reset
                nextSessionSelected = false;
                RenderingManager::getSessionSwitcher()->endTransition();
                // no target
                transitionSlider->setValue(-100);
                resetTransitionSlider();

                // ensure correct re-display
                RenderingManager::getSessionSwitcher()->setTransitionType(RenderingManager::getSessionSwitcher()->getTransitionType());
                nextSessionLabel->setText(QString("0%"));
            }


    } else {

        // detect change of session
        if ( t >= 0 ){
            nextSessionSelected = true;
            suspended = true;
            resetTransitionSlider();
            // request to load session file
            emit sessionTriggered(nextSession);

        }
        // show percent of mixing
        currentSessionLabel->setText(QString("%1%").arg(ABS(t)));
    }
}

void SessionSwitcherWidget::unsuspend()
{
    suspended = false;
}


void SessionSwitcherWidget::ctxMenu(const QPoint &pos)
{
    static QMenu *contextmenu_item = NULL;
    if (contextmenu_item == NULL) {
        contextmenu_item = new QMenu(this);
        contextmenu_item->addAction(loadAction);
        contextmenu_item->addAction(renameSessionAction);
        contextmenu_item->addAction(deleteSessionAction);
        contextmenu_item->addAction(openUrlAction);
    }
    static QMenu *contextmenu_background = NULL;
    if (contextmenu_background == NULL) {
        contextmenu_background = new QMenu(this);
        contextmenu_background->addAction(openUrlAction);
    }

    QModelIndex index = proxyView->indexAt(pos);
    if (index.isValid()) {
        contextmenu_item->popup(proxyView->viewport()->mapToGlobal(pos));
    }
    else {
        proxyView->reset();
        contextmenu_background->popup(proxyView->viewport()->mapToGlobal(pos));
    }
}

void SessionSwitcherWidget::showEvent(QShowEvent *e){

    reloadFolder();

    QWidget::showEvent(e);
}


void SessionSwitcherWidget::sortingChanged(int c, Qt::SortOrder o) {

    sortingColumn = c;
    sortingOrder = o;

    appSettings->setValue("transitionSortingColumn", sortingColumn);
    appSettings->setValue("transitionSortingOrder", sortingOrder);
}
