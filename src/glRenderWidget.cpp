/*
 * glRenderWidget.cpp
 *
 *  Created on: 3 nov. 2009
 *      Author: herbelin
 */

#include <QMessageBox>

#include "common.h"
#include "glRenderWidget.moc"


bool glRenderWidget::showFps_ = false;
//
//QStringList glRenderWidget::listofextensions;

glRenderWidget::glRenderWidget(QWidget *parent, const QGLWidget * shareWidget, Qt::WindowFlags f)
: QGLWidget(QGLFormat(QGL::AlphaChannel | QGL::NoDepthBuffer), parent, shareWidget, f), aspectRatio(1.0), timer(-1), period(16)

{
	if (!format().rgba())
	  qCritical("*** ERROR ***\n\nOpenGL Could not set RGBA buffer; cannot perform OpenGL rendering.");
	if (!format().directRendering())
	  qWarning("** WARNING **\n\nOpenGL Could not set direct rendering; rendering will be slow.");
	if (!format().doubleBuffer())
	  qWarning("** WARNING **\n\nOpenGL Could not set double buffering; rendering will be slow.");


	fpsTime_.start();
	fpsCounter_	= 0;
	f_p_s_		= 1 / period;
	fpsString_	= tr("%1Hz", "Frames per seconds, in Hertz").arg("?");

	update();
}


glRenderWidget::~glRenderWidget() {

}

void glRenderWidget::initializeGL()
{
    // Set flat color shading without dithering
    glShadeModel(GL_FLAT);
    glDisable(GL_DITHER);

    // disable depth and lighting by default
    glDisable(GL_DEPTH_TEST);
	glDepthMask(GL_FALSE);
    glDisable(GL_LIGHTING);
    glDisable(GL_NORMALIZE);

    // Enables texturing
    glEnable(GL_TEXTURE_2D);
    // Pure texture color (no lighting)
    glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
    // This hint can improve the speed of texturing when perspective- correct texture coordinate interpolation isn't needed
    glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_FASTEST);

    // ensure alpha channel is modulated
    glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_COMBINE);
	glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_ALPHA, GL_MODULATE);

    // Turn blending on
    glEnable(GL_BLEND);

    // Blending Function For transparency Based On Source Alpha Value
	glBlendEquation(GL_FUNC_ADD);
	glBlendFunc(GL_SRC_ALPHA, GL_DST_ALPHA);

    // ANTIALIASING
    glEnable(GL_LINE_SMOOTH);
    glHint(GL_LINE_SMOOTH_HINT, GL_NICEST);

    // setup default background color to black
    glClearColor(0.0, 0.0, 0.0, 1.0f);

}


void glRenderWidget::setBackgroundColor(const QColor &c){

    makeCurrent();
    qglClearColor(c);
}

void glRenderWidget::resizeGL(int w, int h)
{
    glViewport(0, 0, w, h);
	aspectRatio = (float) w / (float) h;

    // Setup specific projection and view for this window
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluOrtho2D(-1.0, 1.0, -1.0, 1.0);

    glMatrixMode(GL_MODELVIEW);
	
	update();
}

void glRenderWidget::paintGL()
{
    glClear(GL_COLOR_BUFFER_BIT);

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

	// FPS computation
	if (++fpsCounter_ == 20)
	{
		f_p_s_ = 1000.0 * 20.0 / fpsTime_.restart();
		fpsCounter_ = 0;
	}

	if (showFps_) {
		fpsString_ = tr("%1Hz", "Frames per seconds, in Hertz").arg(f_p_s_, 0, 'f', ((f_p_s_ < 10.0)?1:0));
		displayFPS();
	} 
}



void glRenderWidget::showEvent ( QShowEvent * event ) {
	QGLWidget::showEvent(event);
	if(timer == -1)
		timer = startTimer(period);
}

void glRenderWidget::hideEvent ( QHideEvent * event ) {
	QGLWidget::hideEvent(event);
	if(timer > 0) {
		killTimer(timer);
		timer = -1;
	}
}

void glRenderWidget::displayFPS()
{
	qglColor(Qt::darkGreen);
	renderText(10, int(1.5*((QApplication::font().pixelSize()>0)?QApplication::font().pixelSize():QApplication::font().pointSize())), fpsString_, QFont());
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

    QAbstractItemModel *model = new QStringListModel(glSupportedExtensions());
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


