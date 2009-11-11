/*
 * glRenderWidget.h
 *
 *  Created on: 3 nov. 2009
 *      Author: herbelin
 */

#ifndef GLRENDERWIDGET_H_
#define GLRENDERWIDGET_H_


#define UNIT 1.0

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

    // Events management
    virtual void timerEvent(QTimerEvent *) { update(); }
    virtual void keyPressEvent(QKeyEvent * event );
    virtual void mouseDoubleClickEvent ( QMouseEvent * event );
    virtual void closeEvent ( QCloseEvent * event );

    // OpenGL informations
    static bool glSupportsExtension(QString extname);
    static void showGlExtensionsInformationDialog(QString iconfile = "");

public slots:
	void setFullScreen(bool on);

signals:
	void windowClosed();

private:

    static QStringList listofextensions;
};

#endif /* GLRENDERWIDGET_H_ */
