/*
 *  main.cpph
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
 *   Copyright 2009, 2017 Bruno Herbelin
 *
 */

#include <QApplication>
#include <QEvent>
#include <QStringList>
#include <QFileInfo>
#include <QDir>

#define GLMIXER_LOGFILE "glmixer_log_"

// Create a subclass of QApplication so that we can customize what we
// do when the operating system sends us an event (such as opening a file)
class GLMixerApp : public QApplication
{

  Q_OBJECT

public:

    GLMixerApp(int& argc, char** argv);

    void setFilenameToOpen(QString filename);
    void requestOpenFile();

    void killOtherInstances();

#ifdef GLM_LOGS
    static QString getLogFileName(QString pid = QString::null);
    bool hasCrashLogs();
    void openCrashLogs();
    void deleteCrashLogs();
#endif

signals:
    void filenameToOpen(QString);

protected:
    bool event (QEvent *event);
    QStringList otherInstances();

private:
    QString _filename;
    QStringList _otherinstances;
    QFileInfoList _crashedlogfiles;
};
