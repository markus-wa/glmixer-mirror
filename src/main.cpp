

#include <QtGui/QApplication>
#include <QtGui/QMessageBox>
#include <QString>
#include "glmixer.h"
#include "RenderingManager.h"
#include "OutputRenderWindow.h"

#include <iostream>

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

int main(int argc, char **argv)
{
    qInstallMsgHandler(GLMixerMessageOutput);
    QApplication a(argc, argv);
    a.setApplicationName("GLMixer");

    if (!QGLFormat::hasOpenGL() )
    	qCritical("This system does not support OpenGL and this program cannot work without it.");

	// 1. The rendering Manager
    RenderingManager *mrw = RenderingManager::getInstance();

	// 2. The application GUI : it integrates the Rendering Manager QGLWidget
    GLMixer glmixer_widget;

#ifdef __APPLE__
    glmixer_widget.setStyleSheet(QString::fromUtf8("font: 11pt \"Lucida Grande\";\n"));
#else
    glmixer_widget.setStyleSheet(QString::fromUtf8("font: 9pt \"Bitstream Vera Sans\";\n"));
#endif

#ifdef GLMIXER_VERSION
    glmixer_widget.setWindowTitle(QString("GL Mixer %1").arg(GLMIXER_VERSION));
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

