/*
 * SourceDisplayWidget.cpp
 *
 *  Created on: Jan 31, 2010
 *      Author: bh
 */

#include <QPalette>

#include "SourceDisplayWidget.h"
#include "RenderingManager.h"

SourceDisplayWidget::SourceDisplayWidget(QWidget *parent) : glRenderWidget(parent, (QGLWidget *)RenderingManager::getRenderingWidget()), s(0)
{
	period = 50;
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

		glPushMatrix();
		float aspectRatio = s->getAspectRatio();
		float windowaspectratio = (float) width() / (float) height();
		if (windowaspectratio < aspectRatio)
			glScalef(SOURCE_UNIT, SOURCE_UNIT * windowaspectratio / aspectRatio, 1.f);
		else
			glScalef( SOURCE_UNIT * aspectRatio / windowaspectratio, SOURCE_UNIT, 1.f);


		glBindTexture(GL_TEXTURE_2D, s->getTextureIndex());
		s->draw();

		glPopMatrix();
	}
}

