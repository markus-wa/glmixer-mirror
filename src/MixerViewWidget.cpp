/*
 * MixerViewWidget.cpp
 *
 *  Created on: Nov 9, 2009
 *      Author: bh
 */

#include <MixerViewWidget.moc>

#include "MainRenderWidget.h"

GLuint MixerViewWidget::squareDisplayList = 0;

MixerViewWidget::MixerViewWidget( QWidget * parent, const QGLWidget * shareWidget)
	: glRenderWidget(parent, shareWidget)
{
	this->startTimer(20);

	makeCurrent();
	// create display list if never created
	if (!squareDisplayList){
		squareDisplayList = glGenLists(1);
		glNewList(squareDisplayList, GL_COMPILE);
		{

		    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
			//qglColor(QColor::fromRgb(100, 100, 100).lighter());
			glBegin(GL_QUADS); // begin drawing a square

			// Front Face (note that the texture's corners have to match the quad's corners)
			glNormal3f(0.0f, 0.0f, 1.0f); // front face points out of the screen on z.
			glTexCoord2f(0.0f, 1.0f);

			glVertex3f(-1.0f, -1.0f, 0.0f); // Bottom Left
			glTexCoord2f(1.0f, 1.0f);
			glVertex3f(1.0f, -1.0f, 0.0f); // Bottom Right
			glTexCoord2f(1.0f, 0.0f);
			glVertex3f(1.0f, 1.0f, 0.0f); // Top Right
			glTexCoord2f(0.0f, 0.0f);
			glVertex3f(-1.0f, 1.0f, 0.0f); // Top Left

			glEnd();
		}
		glEndList();
	}

}

MixerViewWidget::~MixerViewWidget() {
	// TODO Auto-generated destructor stub
}

void MixerViewWidget::paintGL()
{
	glRenderWidget::paintGL();

	VideoSource *s = MainRenderWidget::getInstance()->getSource();

	if (s) {
//	    glBindTexture(GL_TEXTURE_2D, s->getFboTexture());
		glBindTexture(GL_TEXTURE_2D, MainRenderWidget::getInstance()->fbo->texture());

		glCallList(MixerViewWidget::squareDisplayList);
	}
}
