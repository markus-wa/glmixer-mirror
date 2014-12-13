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

#include <QtGui/QApplication>
#include <QString>
#include <QTextCodec>

#include "common.h"
#include "QLogStream.h"
#include "glmixer.h"
#include "RenderingManager.h"
#include "OutputRenderWindow.h"
#ifdef SHM
#include "SharedMemoryManager.h"
#endif



int main(int argc, char **argv)
{
    QApplication a(argc, argv);
    QTextCodec::setCodecForCStrings(QTextCodec::codecForName("utf8"));
    QTextCodec::setCodecForTr(QTextCodec::codecForName("utf8"));

    // this redirects qDebug qWarning etc.
    qInstallMsgHandler(GLMixer::msgHandler);
#ifndef Q_OS_MAC
    // these redirect both cout/cerr (seems to crash under OSX :( )
    QLogStream qout(std::cout, GLMixer::msgHandler, QtDebugMsg);
    QLogStream qerr(std::cerr, GLMixer::msgHandler, QtWarningMsg);
#endif

    // -1. sets global application name ; this is used application wide (e.g. QSettings)
    a.setOrganizationName("bhbn");
    a.setOrganizationDomain("bhbn.free.fr");
    a.setApplicationName("GLMixer");

#ifdef GLMIXER_VERSION
    a.setApplicationVersion( QString("%1").arg(GLMIXER_VERSION, 2, 'f', 1 ) );
#else
    a.setApplicationVersion( "Beta" );
#endif

    // 0. A splash screen to wait
    QPixmap pixmap(":/glmixer/images/glmixer_splash.png");
    QSplashScreen splash(pixmap);
    splash.show();
    a.processEvents();

    QTranslator translator;
    translator.load(QString("trans_") + QLocale::system().name());
    a.installTranslator(&translator);
    a.processEvents();

    // 0. Test OpenGL support
    if (!QGLFormat::hasOpenGL() )
    	qFatal( "%s", qPrintable( QObject::tr("This system does not support OpenGL and this program cannot work without it.")) );
    initListOfExtension();
    a.processEvents();

    // 1. The application GUI : it integrates the Rendering Manager QGLWidget
    qAddPostRoutine(GLMixer::exitHandler);
    GLMixer::getInstance()->readSettings();
    a.processEvents();

#ifdef SHM
    if(!SharedMemoryManager::getInstance())
    	qWarning() << QObject::tr("Could not initiate shared memory manager");
    a.processEvents();
#endif

	// 2. The output rendering window ; the rendering manager widget has to be existing
    OutputRenderWindow::getInstance()->setWindowTitle(QObject::tr("Output Window"));
    OutputRenderWindow::getInstance()->show();
    a.processEvents();

    // 3. show the GUI in front
    GLMixer::getInstance()->show();
    splash.finish(GLMixer::getInstance());
    a.processEvents();

    // 4. load eventual session file provided in argument or restore last session
    QString filename = QString::null;
    if ( a.arguments().count()>1 )
        filename = a.arguments()[1];
    else
        filename = GLMixer::getInstance()->getRestorelastSessionFilename();
    GLMixer::getInstance()->switchToSessionFile(filename);

    // start
    return a.exec();
}


