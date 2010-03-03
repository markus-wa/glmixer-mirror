

#include <QtGui/QApplication>
#include <QtGui/QMessageBox>
#include <QString>

#include "common.h"
#include "glmixer.h"
#include "RenderingManager.h"
#include "OutputRenderWindow.h"

#include <iostream>

QStringList listofextensions;


void GLMixerMessageOutput(QtMsgType type, const char *msg)
{
	 switch (type) {
	 case QtDebugMsg:
		 std::cerr<<"Debug: "<<msg<<std::endl;
		 break;
	 case QtWarningMsg:
		 std::cerr<<"Warning: "<<msg<<std::endl;
		 QMessageBox::warning(0, "GLMixer Warning", QString(msg));
		 break;
	 case QtCriticalMsg:
		 std::cerr<<"Critical: "<<msg<<std::endl;
		 QMessageBox::critical(0, "GLMixer Critical Information", QString(msg));
		 abort();
		 break;
	 case QtFatalMsg:
		 std::cerr<<"Fatal: "<<msg<<std::endl;
		 QMessageBox::critical(0, "GLMixer Fatal Error", QString(msg));
		 abort();
	 }
}

bool glSupportsExtension(QString extname) {
    return listofextensions.contains(extname, Qt::CaseInsensitive);
}

QStringList glSupportedExtensions() {
	return listofextensions;
}

int main(int argc, char **argv)
{
 //   qInstallMsgHandler(GLMixerMessageOutput);
    QApplication a(argc, argv);
    a.setApplicationName("GLMixer");

    if (!QGLFormat::hasOpenGL() )
    	qCritical("*** ERROR ***\n\nThis system does not support OpenGL and this program cannot work without it.");

    QGLWidget *glw = new QGLWidget();
    glw->makeCurrent();
#ifdef GLEWAPI
    glewInit();
#endif
	QString allextensions = QString( (char *) glGetString(GL_EXTENSIONS));
	listofextensions = allextensions.split(" ", QString::SkipEmptyParts);
	delete glw;

	// 1. The rendering Manager
    RenderingManager *mrw = RenderingManager::getInstance();

	// 2. The application GUI : it integrates the Rendering Manager QGLWidget
    GLMixer glmixer_widget;

#ifdef __APPLE__
    glmixer_widget.setStyleSheet(QString::fromUtf8("font: 11pt \"Lucida Grande\";\n"));
#endif

#ifdef GLMIXER_VERSION
#ifdef GLMIXER_REVISION
    glmixer_widget.setWindowTitle(QString("GL Mixer %1 rev-%2").arg(GLMIXER_VERSION).arg(GLMIXER_REVISION));
#endif
#endif

	// 3. The output rendering window ; the rendering manager widget has to be existing
    OutputRenderWindow *orw = OutputRenderWindow::getInstance();
    orw->setGeometry(100, 100, mrw->getFrameBufferWidth(), mrw->getFrameBufferHeight());
    orw->setWindowTitle(QString("GL Mixer Rendering Window"));
    orw->show();
	
	// 4. show the GUI in front
    glmixer_widget.show();
    
	return a.exec();
}

