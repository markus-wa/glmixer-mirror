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

SourceDisplayWidget::SourceDisplayWidget(QWidget *parent, enum backgroundType bg) : glRenderWidget(parent, (QGLWidget *)RenderingManager::getRenderingWidget()),
	s(0), background(bg), _bgTexture(0)
{
	function = GL_ONE_MINUS_SRC_ALPHA;
	equation = GL_FUNC_ADD;

}


void SourceDisplayWidget::initializeGL()
{
	glRenderWidget::initializeGL();

	if (background == GRID)
		setBackgroundColor( QColor(102, 102, 102) );
	else if (background == WHITE)
		setBackgroundColor(Qt::white);
	else
		setBackgroundColor(Qt::black);

	glGenTextures(1, &_bgTexture);
	glBindTexture(GL_TEXTURE_2D, _bgTexture);
	QImage p(":/glmixer/textures/transparencygrid.png");
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8,  p.width(), p. height(), 0, GL_RGBA, GL_UNSIGNED_BYTE,  p.bits());
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
}

void SourceDisplayWidget::setSource(Source *sourceptr)
{
	s = sourceptr;
}

void SourceDisplayWidget::playSource(bool on)
{
	s->play(on);
}

void SourceDisplayWidget::paintGL()
{
	glRenderWidget::paintGL();

	if (!isEnabled())
		return;

	// reset
	glLoadIdentity();
	glBlendEquationSeparate(GL_FUNC_ADD, GL_MAX);
	glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_ZERO);

	if ( background == GRID) {
		// draw the background
		glScalef(2.f * aspectRatio, 2.f * aspectRatio, 1.f);
		glBindTexture(GL_TEXTURE_2D, _bgTexture);
		glCallList(ViewRenderWidget::quad_texured);
		glLoadIdentity();
	}


    if (s) {
        // update the texture of the source
        s->bind();

		// adjust size to show all the square and ensure aspect ratio is preserved
		if ( s->getAspectRatio() > aspectRatio )
			glScalef( 1.f, aspectRatio / s->getAspectRatio(), 1.f);
		else
			glScalef( s->getAspectRatio() / aspectRatio, 1.f, 1.f);
		// flip vertical if requested
		glScalef( aspectRatio, s->isVerticalFlip() ? -1.0 : 1.0, 1.f);
		// use source color
	    glColor4f(s->getColor().redF(), s->getColor().greenF(), s->getColor().blueF(), 1.0);

	    // blending
		glBlendEquationSeparate(equation, GL_MAX);
		glBlendFuncSeparate(GL_SRC_ALPHA, function, GL_ONE_MINUS_SRC_ALPHA, GL_ZERO);

	    // draw a quad with the texture
		glCallList(ViewRenderWidget::quad_texured);
		// revert color
	    glColor4f(1.0, 1.0, 1.0, 1.0);

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

void SourceDisplayWidget::setBlendingFunction(int functionindex)
{
	function = blendfunctionFromInt(functionindex);
}

void SourceDisplayWidget::setBlendingEquation(int equationindex)
{
	equation = blendequationFromInt(equationindex);
}

