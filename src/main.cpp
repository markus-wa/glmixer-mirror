

#include <QtGui/QApplication>
#include <QtGui/QMessageBox>
#include <QString>
#include "glmixer.h"
#include "MainRenderWidget.h"

int main(int argc, char **argv)
{
    QApplication a(argc, argv);
    a.setApplicationName("GLMixer");

    if (!QGLFormat::hasOpenGL() ) {
    	QMessageBox::critical(0, "OpenGL is needed",
                 "This system does not support OpenGL and this program cannot work without it.");
        return -1;
    }

    MainRenderWidget *mrw = MainRenderWidget::getInstance();
    mrw->setParent(NULL, Qt::Window | Qt::CustomizeWindowHint | Qt::WindowTitleHint);
    mrw->setGeometry(100, 100, 1024, 768);
    mrw->setWindowTitle(QString("GL Mixer Rendering Window"));
    mrw->show();

    GLMixer glmixer_widget(mrw);

#ifdef __APPLE__
    glmixer_widget.setStyleSheet(QString::fromUtf8("font: 11pt \"Lucida Grande\";\n"));
#else
    glmixer_widget.setStyleSheet(QString::fromUtf8("font: 9pt \"Bitstream Vera Sans\";\n"));
#endif

#ifdef GLMIXER_VERSION
    glmixer_widget.setWindowTitle(QString("GL Mixer %1").arg(GLMIXER_VERSION));
#endif
    glmixer_widget.show();

    mrw->startRender();

    return a.exec();
}

