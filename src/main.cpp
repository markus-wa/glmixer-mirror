/*
 *  main.cpp
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

#include "main.moc"
#include "common.h"
#include "glmixer.h"
#include "RenderingManager.h"
#include "OutputRenderWindow.h"
#ifdef GLM_SHM
#include "SharedMemoryManager.h"
#endif

#ifdef GLM_LOGS
#ifndef Q_OS_MAC
#include "QLogStream.h"
#endif
#endif

#include <QTextCodec>


GLMixerApp::GLMixerApp(int& argc, char** argv): QApplication(argc, argv), _filename(QString::null)
{
   // -1. sets global application name ; this is used application wide (e.g. QSettings)
    setOrganizationName("bhbn");
    setOrganizationDomain("bhbn.free.fr");
    setApplicationName("GLMixer");

#ifdef GLMIXER_VERSION
    setApplicationVersion( QString("%1").arg(GLMIXER_VERSION, 2, 'f', 1 ) );
#else
    setApplicationVersion( "Beta" );
#endif

#ifndef Q_OS_WIN32
    QTextCodec::setCodecForCStrings(QTextCodec::codecForName("utf8"));
    QTextCodec::setCodecForTr(QTextCodec::codecForName("utf8"));
#endif

    // list other instances already running
    _otherinstances = otherInstances();

#ifdef GLM_LOGS
    // fill-in list of crashed logs :
    // 1. list log files existing in temp
    QDir tmpdir = QDir::temp();
    _crashedlogfiles = tmpdir.entryInfoList(QStringList(QString("%1*.txt").arg(GLMIXER_LOGFILE)), QDir::Files);
    // 2. ignore my own pid
    _crashedlogfiles.removeAll( QFileInfo(getLogFileName()) );
    // 3. ignore log files of running processes
    foreach (QString pid, _otherinstances)
        _crashedlogfiles.removeAll( QFileInfo(getLogFileName(pid)) );
#endif
}

void GLMixerApp::requestOpenFile()
{
    // if a filename was given
    if (!_filename.isNull()) {
        // request to GLMixer to open the file (connected)
        emit filenameToOpen(_filename);
        // clear file name
        _filename = QString();
    }
}

void GLMixerApp::setFilenameToOpen(QString filename)
{
    // update the file name only if not already set
    if (_filename.isNull())
        _filename = filename;
}

bool GLMixerApp::event(QEvent *event)
{
    // The system requested us to open a file
    if (event->type() == QEvent::FileOpen)  {
        // Get the path of the file that we want to open
        _filename = static_cast<QFileOpenEvent *> (event)->file();
        // inform about file to open
        emit filenameToOpen(_filename);
    }
    // The system requested us to do another thing, so we just follow the rules
    else
        return QApplication::event (event);

    return true;
}


void GLMixerApp::killOtherInstances() {

    qint64 pid = applicationPid();
    // Generate platform specific command to Kill all other glmixer processes
#ifdef Q_OS_WIN
    // Kill all glmixer processes with a different PID
    QProcess::execute(QString("taskkill /F /FI \"PID ne %1\" /im glmixer.exe").arg(pid));
#else
    // unix bash: find all glmixer processes with a different PID and kill them
#ifdef Q_OS_MAC
    QString cmd = QString("pgrep -x glmixer | grep -vE %1 | xargs kill").arg(pid);
#else
    QString cmd = QString("pgrep -x glmixer | grep -vE %1 | xargs -r kill").arg(pid);
#endif
    QProcess::execute("bash", QStringList() << "-c" << cmd);
#endif

#ifdef GLM_LOGS
    // as we killed the processes, the corresponding logs shall be removed
    foreach (QString pid, _otherinstances) {
        QFileInfo log (QFileInfo(getLogFileName(pid)));
        if (log.exists())
            log.dir().remove(log.fileName());
    }
#endif

    // no more other instances!
    _otherinstances.clear();
}


QStringList GLMixerApp::otherInstances() {

    QStringList listpid;
    QProcess pgrep;

    // platform dependent function for reading list of glmixer processes
#ifdef Q_OS_WIN
    pgrep.start("tasklist /fi glmixer");
#else
    // unix bash
    pgrep.start("pgrep", QStringList() << "glmixer");
#endif

    // let process finish
    pgrep.waitForFinished();

    // fill the list of pid
    listpid = QString(pgrep.readAll()).split("\n", QString::SkipEmptyParts);

    // remove my own PID
    listpid.removeAll(QString::number(applicationPid()));

    return listpid;
}

#ifdef GLM_LOGS
QString GLMixerApp::getLogFileName(QString pid)
{
    // if no argument given, use the application pid
    if (pid.isNull())
        pid = QString::number(QApplication::applicationPid());

    // build a standard log filename
    return QDir::tempPath() + QString("/%1%2.txt").arg(GLMIXER_LOGFILE).arg(pid);
}

void GLMixerApp::deleteCrashLogs() {

    // delete all previous log files
    while( !_crashedlogfiles.empty() ) {
        QFileInfo log = _crashedlogfiles.takeFirst();
        if (log.exists())
            log.dir().remove(log.fileName());
    }
}

void GLMixerApp::openCrashLogs() {

    // move all previous log files to home dir and open them
    while( !_crashedlogfiles.empty() ) {
        QFileInfo log = _crashedlogfiles.takeFirst();
        if (log.exists()) {
            // find an available filename for copy
            QFileInfo copylog = QFileInfo(QDir::home(), log.fileName());
            while (copylog.exists())
                copylog.setFile(QDir::home(), copylog.fileName().prepend("_"));
            // move the file
            QFile::copy( log.absoluteFilePath(), copylog.absoluteFilePath() );
            log.dir().remove(log.fileName());
            // open the copy
            QDesktopServices::openUrl( QUrl::fromLocalFile(copylog.absoluteFilePath()) );
        }
    }
}

bool GLMixerApp::hasCrashLogs() {

    return !_crashedlogfiles.empty();
}
#endif

// DEBUG
//void testRegExp(QString pattern, Qt::CaseSensitivity cs, QRegExp::PatternSyntax syntax,  QString text)
//{
//    QRegExp regex(pattern, cs, syntax);
//    bool matches = regex.exactMatch(text);
//    qDebug("'%s'.exactMatch('%s') = %d",
//           qPrintable(pattern), qPrintable(text), matches);
//}

int main(int argc, char **argv)
{
    bool crashrecover = false;
    int returnvalue = -1;

    //
    // 0. Create the Qt application and treat arguments
    //
    GLMixerApp a(argc, argv);

    // get the arguments into a list
    QStringList cmdline_args = a.arguments();

    // Request for VERSION argument
    int idx = cmdline_args.indexOf(QRegExp("^(\\-v|\\-{2,2}version)"), 1);
    if ( idx > -1) {
        qDebug("%s Version %s", qPrintable(a.applicationName()), qPrintable(a.applicationVersion()));
        cmdline_args.removeAt(idx);
        returnvalue = EXIT_SUCCESS;
    }

    // Request for HELP argument
    idx = cmdline_args.indexOf(QRegExp("^(\\-h|\\-{2,2}help)"), 1);
    if ( idx > -1) {
        qDebug("%s [-v|--version] [-h|--help] [GLM SESSION FILE]", qPrintable(cmdline_args.at(0)) );
        cmdline_args.removeAt(idx);
        returnvalue = EXIT_SUCCESS;
    }

    // invalid Request argument
    foreach (const QString &argument, cmdline_args.filter(QRegExp("^\\-{1,2}"))) {
        qDebug("%s : invalid arguments (ignored).", qPrintable(argument));
        returnvalue = EXIT_FAILURE;
    }

    // exit if already delt with one of the requests above (return value was set)
    if ( returnvalue != -1)
        exit(returnvalue);

    // maybe argument is a filename ?
    if ( cmdline_args.count() > 1 ) {
        if (QFileInfo(cmdline_args.at(1)).isFile())
            a.setFilenameToOpen( cmdline_args.at(1) );
        else
            qDebug("%s : invalid arguments (not a file name).", qPrintable(cmdline_args.at(1)));
    }


    //
    // 1. Start of the GUI interface section.
    //
    // Show a splash screen to wait
    QPixmap pixmap(":/glmixer/images/glmixer_splash.png");
    QSplashScreen splash(pixmap);
#ifdef GLMIXER_REVISION
    splash.showMessage(QString("r%1 (%2)").arg(GLMIXER_REVISION).arg(COMPILE_YEAR));
#endif
    splash.show();
    a.processEvents();

#ifdef GLM_LOGS
    // Redirect qDebug, qWarning and qFatal to GUI and logger
    qInstallMsgHandler(GLMixer::msgHandler);
    // this cleans up after the application ends
    qAddPostRoutine(GLMixer::exitHandler);
#endif

//    QTranslator translator;
//    translator.load(QString("trans_") + QLocale::system().name());
//    a.installTranslator(&translator);
//    a.processEvents();

    initApplicationFonts();

    //
    // 2. Test OpenGL support and initialize list of GL extensions
    //
    if (!QGLFormat::hasOpenGL() )
        qFatal( "%s", qPrintable( QObject::tr("This system does not support OpenGL and this program cannot work without it.")) );
    initListOfExtension();
    a.processEvents();

    //
    // 3. Start the application GUI
    //
    GLMixer::getInstance()->readSettings( a.applicationDirPath() );

    // enable openning of file from system message
    QObject::connect(&a, SIGNAL(filenameToOpen(QString)), GLMixer::getInstance(), SLOT(switchToSessionFile(QString)));
    a.processEvents();

    // terminate other instance in single instance mode
    if (GLMixer::isSingleInstanceMode())
        a.killOtherInstances();
    a.processEvents();

    // if there are remaining logs, it is because of a crash
#ifdef GLM_LOGS
    crashrecover = a.hasCrashLogs();
    if (crashrecover){
        int ret = QMessageBox::Ignore;
        QMessageBox msgBox;
        msgBox.setText(("It looks like GLMixer crashed !"));
        msgBox.setInformativeText(("Do you want to open the log files ?"));
        msgBox.setIconPixmap( QPixmap(QString::fromUtf8(":/glmixer/icons/question.png")) );
        msgBox.setStandardButtons(QMessageBox::Open | QMessageBox::Ignore | QMessageBox::Cancel);
        msgBox.setDefaultButton(QMessageBox::Ignore);
        ret = msgBox.exec();

        if (ret == QMessageBox::Open)
            a.openCrashLogs();
        else if (ret == QMessageBox::Ignore)
            a.deleteCrashLogs();
        else
            return -1;
    }
#endif


#ifdef GLM_SHM
    if(!SharedMemoryManager::getInstance())
        qWarning() << QObject::tr("Could not initiate shared memory manager");
    a.processEvents();
#endif

    // The output rendering window ; the rendering manager widget has to be existing
    OutputRenderWindow::getInstance()->setWindowTitle(QObject::tr("GLMixer - Output"));
    QIcon icon;
    icon.addFile(QString::fromUtf8(":/glmixer/icons/glmixer.png"), QSize(), QIcon::Normal, QIcon::Off);
    OutputRenderWindow::getInstance()->setWindowIcon(icon);
    OutputRenderWindow::getInstance()->show();
    a.processEvents();

    // Show the GUI in front
    GLMixer::getInstance()->show();
    a.processEvents();

    // all done
    splash.finish(GLMixer::getInstance());
    a.processEvents();

    //
    // 4. load eventual session file provided in argument or restore last session
    //
    if (!crashrecover)
        a.setFilenameToOpen( GLMixer::getInstance()->getRestorelastSessionFilename() );
    a.requestOpenFile();

    // start application loop
    returnvalue = a.exec();

    //
    //  All done, exit properly
    //
    // save GUI settings
    GLMixer::getInstance()->saveSettings();

    // delete static objects
    RenderingManager::deleteInstance();
    OutputRenderWindow::deleteInstance();
#ifdef GLM_SHM
    SharedMemoryManager::deleteInstance();
#endif
    a.processEvents();

    GLMixer::deleteInstance();

    return returnvalue;
}
