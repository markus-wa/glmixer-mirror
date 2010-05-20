/*
 * SourceDisplayWidget.cpp
 *
 *  Created on: Jan 31, 2010
 *      Author: bh
 */

#include <QPalette>

#include "SourceDisplayWidget.moc"
#include "Source.h"
#include "RenderingManager.h"

SourceDisplayWidget::SourceDisplayWidget(QWidget *parent) : glRenderWidget(parent, (QGLWidget *)RenderingManager::getRenderingWidget()), s(0)
{
	// setup for 50Hz
	period = 20;
}


void SourceDisplayWidget::initializeGL()
{
	glRenderWidget::initializeGL();

	setBackgroundColor(palette().color(QPalette::Window));
    glDisable(GL_BLEND);

}

void SourceDisplayWidget::paintGL()
{
	glRenderWidget::paintGL();

	if (s) {

	    glMatrixMode(GL_MODELVIEW);
	    glLoadIdentity();
		// update the texture of the source
		s->update();

		// paint it with respect of its aspect ratio
		float aspectRatio = s->getAspectRatio();
		float windowaspectratio = (float) width() / (float) height();
		if (windowaspectratio < aspectRatio)
			glScalef(1.0, windowaspectratio / aspectRatio, 1.f);
		else
			glScalef( aspectRatio / windowaspectratio, 1.0, 1.f);

		s->draw();
	}
}


int SourceDisplayWidget::getNewTextureIndex() {
    GLuint textureIndex;
	makeCurrent();
	glGenTextures(1, &textureIndex);
	return textureIndex;
}

