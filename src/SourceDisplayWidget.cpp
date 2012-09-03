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
 *   Copyright 2009, 2012 Bruno Herbelin
 *
 */

#include <QPalette>

#include "SourceDisplayWidget.moc"
#include "Source.h"
#include "RenderingManager.h"
#include "ViewRenderWidget.h"

SourceDisplayWidget::SourceDisplayWidget(QWidget *parent) : glRenderWidget(parent, (QGLWidget *)RenderingManager::getRenderingWidget()),
	s(0), _bgTexture(0)
{
	use_aspect_ratio = true;
	use_filters = false;
}


void SourceDisplayWidget::initializeGL()
{
	glRenderWidget::initializeGL();

	setBackgroundColor(Qt::black);

	glGenTextures(1, &_bgTexture);
	glBindTexture(GL_TEXTURE_2D, _bgTexture);
	QImage p(":/glmixer/textures/transparencygrid.png");
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8,  p.width(), p. height(), 0, GL_RGBA, GL_UNSIGNED_BYTE,  p.bits());
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
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

	// draw the background
	glLoadIdentity();
	glScalef(aspectRatio, aspectRatio, 1.f);
	glBindTexture(GL_TEXTURE_2D, _bgTexture);
	glCallList(ViewRenderWidget::quad_texured);

	if (s) {
		// update the texture of the source
		s->update();

		// draw the source
		glLoadIdentity();
	    glColor4f(s->getColor().redF(), s->getColor().greenF(), s->getColor().blueF(), 1.0);
		glScalef( s->getAspectRatio(), s->isVerticalFlip() ? -1.0 : 1.0, 1.f);
		glCallList(ViewRenderWidget::quad_texured);

	}
}

void SourceDisplayWidget::resizeGL(int w, int h)
{
    glViewport(0, 0, w, h);
	aspectRatio = (float) w / (float) h;

    // Setup specific projection and view for this window
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluOrtho2D(-aspectRatio, aspectRatio, -1.0, 1.0);

    glMatrixMode(GL_MODELVIEW);

}

GLuint SourceDisplayWidget::getNewTextureIndex() {
    GLuint textureIndex;
	makeCurrent();
	glGenTextures(1, &textureIndex);
	return textureIndex;
}

