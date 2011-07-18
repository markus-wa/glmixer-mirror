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
 *   Copyright 2009, 2010 Bruno Herbelin
 *
 */

#include <QtGui/QApplication>
#include <QString>

#include "common.h"
#include "glmixer.h"
#include "RenderingManager.h"
#include "OutputRenderWindow.h"

QStringList listofextensions;

bool glSupportsExtension(QString extname) {
    return listofextensions.contains(extname, Qt::CaseInsensitive);
}

QStringList glSupportedExtensions() {
	return listofextensions;
}

int main(int argc, char **argv)
{
//    qInstallMsgHandler(GLMixerMessageOutput);
    QApplication a(argc, argv);

    // -1. sets global application name ; this is used application wide (e.g. QSettings)
    a.setOrganizationName("bhbn");
    a.setOrganizationDomain("bhbn.free.fr");
    a.setApplicationName("GLMixer");

#ifdef GLMIXER_VERSION
    a.setApplicationVersion( QString("%1").arg(GLMIXER_VERSION, 2, 'f', 1 ) );
#else
    a.setApplicationVersion( "Beta" );
#endif

#ifdef __APPLE__
    // add local bundled lib directory as library path (Qt Plugins)
    QDir dir(QApplication::applicationDirPath());
	dir.cdUp();
	dir.cd("lib");
	QApplication::setLibraryPaths(QStringList(dir.absolutePath()));
#endif

    // 0. A splash screen to wait
    QPixmap pixmap(":/glmixer/images/glmixer_splash.png");
    QSplashScreen splash(pixmap);
    splash.show();
    a.processEvents();

    if (!QGLFormat::hasOpenGL() ) {
    	qFatal( "This system does not support OpenGL and this program cannot work without it.");
    	a.processEvents();
    }

    // fill in the list of extensions by creating a dummy glwidget
    QGLWidget *glw = new QGLWidget();
    glw->makeCurrent();
#ifdef GLEWAPI
    glewInit();
#endif
	QString allextensions = QString( (char *) glGetString(GL_EXTENSIONS));
	listofextensions = allextensions.split(" ", QString::SkipEmptyParts);
	delete glw;
    a.processEvents();

	// 1. The application GUI : it integrates the Rendering Manager QGLWidget
    GLMixer glmixer_widget;
    glmixer_widget.setWindowTitle(a.applicationName());

    qInstallMsgHandler(GLMixer::MessageOutput);

	// 2. The output rendering window ; the rendering manager widget has to be existing
    OutputRenderWindow::getInstance()->setWindowTitle(QString("Output Window"));
    OutputRenderWindow::getInstance()->show();
	
	// 3. show the GUI in front
    glmixer_widget.show();

    // 4. load eventual session file provided in argument
    QStringList params = a.arguments();
    if ( params.count() > 1) {
    	// try to read a file with the first argument
    	glmixer_widget.openSessionFile(params[1]);
    }
    splash.finish(&glmixer_widget);

    return a.exec();
}

