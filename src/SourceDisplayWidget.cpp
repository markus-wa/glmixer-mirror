/*
 * SourceDisplayWidget.cpp
 *
 *  Created on: Jan 31, 2010
 *      Author: bh
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

#include <QPalette>

#include "SourceDisplayWidget.moc"
#include "Source.h"
#include "RenderingManager.h"
#include "ViewRenderWidget.h"

SourceDisplayWidget::SourceDisplayWidget(QWidget *parent) : glRenderWidget(parent, (QGLWidget *)RenderingManager::getRenderingWidget()), s(0)
{
	use_aspect_ratio = true;
}


void SourceDisplayWidget::initializeGL()
{
	glRenderWidget::initializeGL();

#ifdef __APPLE__
	setBackgroundColor(Qt::black);
#else
	setBackgroundColor(palette().color(QPalette::Window));
#endif
    glDisable(GL_BLEND);

}

void SourceDisplayWidget::setSource(Source *sourceptr) {
	s = sourceptr;

	if (s && !use_aspect_ratio)
		// adjust source AR to fill the area
		s->setAspectRatio( double (width()) / double (height()) );

}

void SourceDisplayWidget::playSource(bool on) { s->play(on); }

void SourceDisplayWidget::paintGL()
{
	glRenderWidget::paintGL();

	if (!isEnabled())
		return;

	if (s) {

	    glMatrixMode(GL_MODELVIEW);
	    glLoadIdentity();

	    glColor4f(s->getColor().redF(), s->getColor().greenF(), s->getColor().blueF(), 1.0);
		glScaled(1.0, s->verticalFlip() ? -1.0 : 1.0, 1.0);
		// update the texture of the source
		s->update();

		// paint it with respect of its aspect ratio
		float aspectRatio = s->getAspectRatio();
		float windowaspectratio = (float) width() / (float) height();
		if (windowaspectratio < aspectRatio)
			glScalef(1.0, windowaspectratio / aspectRatio, 1.f);
		else
			glScalef( aspectRatio / windowaspectratio, 1.0, 1.f);

		glCallList(ViewRenderWidget::quad_texured);
	}
}


GLuint SourceDisplayWidget::getNewTextureIndex() {
    GLuint textureIndex;
	makeCurrent();
	glGenTextures(1, &textureIndex);
	return textureIndex;
}

