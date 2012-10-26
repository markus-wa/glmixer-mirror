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

#include "common.h"
#include "QLogStream.h"
#include "glmixer.h"
#include "RenderingManager.h"
#include "SharedMemoryManager.h"
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
    QApplication a(argc, argv);

    // this redirects qDebug qWarning etc.
    qInstallMsgHandler(GLMixer::msgHandler);
#ifndef __APPLE__
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

    // Test OpenGL support
    if (!QGLFormat::hasOpenGL() )
    	qFatal( "%s", qPrintable( QObject::tr("This system does not support OpenGL and this program cannot work without it.")) );

    // fill in the list of extensions by creating a dummy glwidget
    QGLWidget *glw = new QGLWidget();
    glw->makeCurrent();
    glewInit();
	QString allextensions = QString( (char *) glGetString(GL_EXTENSIONS));
	listofextensions = allextensions.split(" ", QString::SkipEmptyParts);
	delete glw;
    a.processEvents();

	// 1. The application GUI : it integrates the Rendering Manager QGLWidget
    GLMixer::getInstance()->setWindowTitle(a.applicationName());
    qAddPostRoutine(GLMixer::exitHandler);
    GLMixer::getInstance()->readSettings();

    if(!SharedMemoryManager::getInstance())
    	qWarning() << QObject::tr("Could not initiate shared memory manager");

	// 2. The output rendering window ; the rendering manager widget has to be existing
    OutputRenderWindow::getInstance()->setWindowTitle(QString("Output Window"));
    OutputRenderWindow::getInstance()->show();

    // 3. load eventual session file provided in argument
    QStringList params = a.arguments();
    if ( params.count() > 1) {
    	// try to read a file with the first argument
    	GLMixer::getInstance()->openSessionFile(params[1]);
    }

	// 4. show the GUI in front
    GLMixer::getInstance()->show();
    splash.finish(GLMixer::getInstance());

    return a.exec();
}

// we will need these for the correspondence between comboBox and GLenums:
GLenum blendfunctionFromInt(int i){
	switch (i) {
	case 0:
		return GL_ZERO;
	case 1:
		return GL_ONE;
	case 2:
		return GL_SRC_COLOR;
	case 3:
		return GL_ONE_MINUS_SRC_COLOR;
	case 4:
		return GL_DST_COLOR;
	case 5:
		return GL_ONE_MINUS_DST_COLOR;
	case 6:
		return GL_SRC_ALPHA;
	default:
	case 7:
		return GL_ONE_MINUS_SRC_ALPHA;
	case 8:
		return GL_DST_ALPHA;
	case 9:
		return GL_ONE_MINUS_DST_ALPHA;
	}
}

int intFromBlendfunction(GLenum e){
	switch (e) {
	case GL_ZERO:
		return 0;
	case GL_ONE:
		return 1;
	case GL_SRC_COLOR:
		return 2;
	case GL_ONE_MINUS_SRC_COLOR:
		return 3;
	case GL_DST_COLOR:
		return 4;
	case GL_ONE_MINUS_DST_COLOR:
		return 5;
	case GL_SRC_ALPHA:
		return 6;
	default:
	case GL_ONE_MINUS_SRC_ALPHA:
		return 7;
	case GL_DST_ALPHA:
		return 8;
	case GL_ONE_MINUS_DST_ALPHA:
		return 9;
	}
}

GLenum blendequationFromInt(int i){
	switch (i) {
	case 0:
		return GL_FUNC_ADD;
	case 1:
		return GL_FUNC_SUBTRACT;
	case 2:
		return GL_FUNC_REVERSE_SUBTRACT;
	case 3:
		return GL_MIN;
	case 4:
		return GL_MAX;
	}
}

int intFromBlendequation(GLenum e){
	switch (e) {
	case GL_FUNC_ADD:
		return 0;
	case GL_FUNC_SUBTRACT:
		return 1;
	case GL_FUNC_REVERSE_SUBTRACT:
		return 2;
	case GL_MIN:
		return 3;
	case GL_MAX:
		return 4;
	}
}

