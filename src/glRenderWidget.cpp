/*
 * glRenderWidget.cpp
 *
 *  Created on: 3 nov. 2009
 *      Author: herbelin
 */

#include <QMessageBox>
#include "glRenderWidget.moc"


QStringList glRenderWidget::listofextensions;

glRenderWidget::glRenderWidget(QWidget *parent, const QGLWidget * shareWidget)
: QGLWidget(QGLFormat(QGL::AlphaChannel), parent, shareWidget)

{
	if (!format().depth())
	  qWarning("Could not get depth buffer; results will be suboptimal");
	if (!format().rgba())
	  qWarning("Could not set rgba buffer; results will be suboptimal");
	if (!format().directRendering())
	  qWarning("Could not set direct rendering; results will be suboptimal");
	if (!format().doubleBuffer())
	  qWarning("Could not set double buffering; results will be suboptimal");


	if (listofextensions.isEmpty()) {
	  makeCurrent();
	  QString allextensions = QString( (char *) glGetString(GL_EXTENSIONS));
	  listofextensions = allextensions.split(" ", QString::SkipEmptyParts);
	}

	update();
}


glRenderWidget::~glRenderWidget() {

}

void glRenderWidget::initializeGL()
{
    // Enables smooth color shading
    glShadeModel(GL_FLAT);

    // disable depth and lighting by default
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_LIGHTING);

    // Enables texturing
    glEnable(GL_TEXTURE_2D);
    // Pure texture color (no lighting)
    glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);

    // Turn blending on
    glEnable(GL_BLEND);

    // Blending Function For transparency Based On Source Alpha Value
//    glBlendFunc(GL_SRC_ALPHA, GL_ONE);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // ANTIALIASING
    glEnable(GL_LINE_SMOOTH);
    glHint(GL_LINE_SMOOTH_HINT, GL_NICEST);

     // setup color
     glClearColor(0.0, 0.0, 0.0, 1.0f);
//     qglClearColor(QColor::fromRgb(1.0, 0.0, 0.0).dark());

}

void glRenderWidget::resizeGL(int w, int h)
{
    glViewport(0, 0, w, h);

    // Setup specific projection and view for this window
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluOrtho2D(-UNIT, UNIT, -UNIT, UNIT);

    glMatrixMode(GL_MODELVIEW);
}

void glRenderWidget::paintGL()
{
    glClear(GL_COLOR_BUFFER_BIT);

}

void glRenderWidget::setFullScreen(bool on) {

	// this is valid only for WINDOW widgets
	if (windowFlags() & Qt::Window) {

		// if ask fullscreen and already fullscreen
		if (on && (windowState() & Qt::WindowFullScreen))
			return;

		// if ask NOT fullscreen and already NOT fullscreen
		if (!on && !(windowState() & Qt::WindowFullScreen))
			return;

		// other cases ; need to switch fullscreen <-> not fullscreen
		setWindowState(windowState() ^ Qt::WindowFullScreen);
		update();
	}

}

void glRenderWidget::mouseDoubleClickEvent ( QMouseEvent * event ){

	// switch fullscreen / window
	if (windowFlags() & Qt::Window) {
		setWindowState(windowState() ^ Qt::WindowFullScreen);
		update();
	}

}

void glRenderWidget::keyPressEvent(QKeyEvent * event ){

	switch (event->key()) {
	     case Qt::Key_Escape:
	    	 setFullScreen(false);
			 break;
	     case Qt::Key_Enter:
	     case Qt::Key_Space:
	    	 setFullScreen(true);
			 event->accept();
	    	 break;
	     default:
	    	 QGLWidget::keyPressEvent(event);
	}
}


void glRenderWidget::closeEvent ( QCloseEvent * event ){

	emit windowClosed();
	event->accept();

}

bool glRenderWidget::glSupportsExtension(QString extname) {

    if (listofextensions.isEmpty()) {
    	glRenderWidget *tmp = new glRenderWidget();
        delete tmp;
    }

    return listofextensions.contains(extname, Qt::CaseInsensitive);
}



void glRenderWidget::showGlExtensionsInformationDialog(QString iconfile){

    QDialog *openglExtensionsDialog;
    QVBoxLayout *verticalLayout;
    QLabel *label;
    QListView *extensionsListView;
    QDialogButtonBox *buttonBox;

    openglExtensionsDialog = new QDialog(0);
    if (!iconfile.isEmpty()){
		QIcon icon;
		icon.addFile(iconfile, QSize(), QIcon::Normal, QIcon::Off);
		openglExtensionsDialog->setWindowIcon(icon);
    }
    openglExtensionsDialog->resize(442, 358);
    verticalLayout = new QVBoxLayout(openglExtensionsDialog);
    label = new QLabel(openglExtensionsDialog);

    verticalLayout->addWidget(label);

    extensionsListView = new QListView(openglExtensionsDialog);

    verticalLayout->addWidget(extensionsListView);

    buttonBox = new QDialogButtonBox(openglExtensionsDialog);
    buttonBox->setOrientation(Qt::Horizontal);
    buttonBox->setStandardButtons(QDialogButtonBox::Ok);

    verticalLayout->addWidget(buttonBox);

    openglExtensionsDialog->setWindowTitle(tr("OpenGL Extensions"));
    label->setText(tr("Supported OpenGL extensions:"));

    if (listofextensions.isEmpty()) {
    	glRenderWidget *tmp = new glRenderWidget();
        delete tmp;
    }
    QAbstractItemModel *model = new QStringListModel(listofextensions);
    extensionsListView->setModel(model);

    QObject::connect(buttonBox, SIGNAL(accepted()), openglExtensionsDialog, SLOT(accept()));
    QObject::connect(buttonBox, SIGNAL(rejected()), openglExtensionsDialog, SLOT(reject()));

    openglExtensionsDialog->exec();

    delete verticalLayout;
    delete label;
    delete extensionsListView;
    delete buttonBox;
    delete model;
    delete openglExtensionsDialog;
}


