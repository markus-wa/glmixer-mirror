/*
 * glRenderWidget.h
 *
 *  Created on: 3 nov. 2009
 *      Author: herbelin
 */

#ifndef GLRENDERWIDGET_H_
#define GLRENDERWIDGET_H_


#include <QStringList>
#include <QtOpenGL>

class glRenderWidget  : public QGLWidget
{
    Q_OBJECT

public:
	glRenderWidget(QWidget *parent = 0, const QGLWidget * shareWidget = 0);
	virtual ~glRenderWidget();

    // OpenGL implementation
    virtual void initializeGL();
    virtual void resizeGL(int w, int h);
    virtual void paintGL();
    void setBackgroundColor(const QColor &c);

    // Update events management
    virtual void timerEvent( QTimerEvent *) { update(); }
    virtual void showEvent ( QShowEvent * event ) { QGLWidget::showEvent(event); timer = startTimer(16);}
    virtual void hideEvent ( QHideEvent * event ) { QGLWidget::hideEvent(event);  killTimer(timer);}

    // OpenGL informations
    static bool glSupportsExtension(QString extname);
    static void showGlExtensionsInformationDialog(QString iconfile = "");

protected:
    int timer;
    static QStringList listofextensions;
};

#endif /* GLRENDERWIDGET_H_ */