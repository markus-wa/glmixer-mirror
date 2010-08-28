/*
 * glRenderWidget.cpp
 *
 *  Created on: 3 nov. 2009
 *      Author: herbelin
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

#include <QMessageBox>

#include "common.h"
#include "glRenderWidget.moc"

//
//QStringList glRenderWidget::listofextensions;

glRenderWidget::glRenderWidget(QWidget *parent, const QGLWidget * shareWidget, Qt::WindowFlags f)
: QGLWidget(QGLFormat(QGL::AlphaChannel | QGL::NoDepthBuffer), parent, shareWidget, f), aspectRatio(1.0), timer(-1), period(17)

{
	static bool testDone = false;
	if (!testDone) {
		if (!format().rgba())
		  qCritical("*** ERROR ***\n\nOpenGL Could not set RGBA buffer; cannot perform OpenGL rendering.");
		if (!format().directRendering())
		  qWarning("** WARNING **\n\nOpenGL Could not set direct rendering; rendering will be slow.");
		if (!format().doubleBuffer())
		  qWarning("** WARNING **\n\nOpenGL Could not set double buffering; rendering will be slow.");
		testDone = true;
	}

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
    glDisable(GL_CULL_FACE);
	glDepthMask(GL_FALSE);
    glDisable(GL_LIGHTING);
    glDisable(GL_NORMALIZE);

    // Enables texturing
	glActiveTexture(GL_TEXTURE0);
    glEnable(GL_TEXTURE_2D);

    // ensure alpha channel is modulated ; otherwise the source is not mixed by its alpha channel
    glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_COMBINE);
	glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_ALPHA, GL_MODULATE);
	glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_RGB, GL_MODULATE);

    // Turn blending on
    glEnable(GL_BLEND);

    // Blending Function For transparency Based On Source Alpha Value
	glBlendEquation(GL_FUNC_ADD);
	glBlendFunc(GL_SRC_ALPHA, GL_DST_ALPHA);

    // ANTIALIASING
    glEnable(GL_LINE_SMOOTH);
    glEnable(GL_POINT_SMOOTH);
    glHint(GL_LINE_SMOOTH_HINT, GL_NICEST);
    glHint(GL_POINT_SMOOTH_HINT, GL_NICEST);
    glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_FASTEST);

    // setup default background color to black
    glClearColor(0.0, 0.0, 0.0, 1.0f);
    glNormal3f(0.0f, 0.0f, 1.0f); // front face points out of the screen on z.

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
    label->setText(tr("OpenGL version %1\n\nSupported extensions:").arg((char *)glGetString(GL_VERSION)));

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


