/*
 * RenderingView.cpp
 *
 *  Created on: Aug 31, 2012
 *      Author: bh
 */

#include "RenderingView.h"

#include "RenderingManager.h"
#include "SelectionManager.h"
#include "ViewRenderWidget.h"
#include "OutputRenderWindow.h"

RenderingView::RenderingView() : View() {
    icon.load(QString::fromUtf8(":/glmixer/icons/display.png"));
    title = " Rendering view";
    zoom = 0.9;
}

RenderingView::~RenderingView() {
	// TODO Auto-generated destructor stub
}


void RenderingView::setModelview()
{
	View::setModelview();
    glScalef(zoom, zoom, zoom);
    glGetDoublev(GL_MODELVIEW_MATRIX, modelview);
}

void RenderingView::resize(int w, int h)
{
	View::resize(w, h);
	glViewport(0, 0, viewport[2], viewport[3]);

    // Setup specific projection and view for this window
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();

	// setup the view
	double ar = (double) viewport[2] / (double) viewport[3];

    if ( ar > OutputRenderWindow::getInstance()->getAspectRatio())
         glOrtho(-SOURCE_UNIT* ar, SOURCE_UNIT*ar, -SOURCE_UNIT, SOURCE_UNIT, -MAX_DEPTH_LAYER, 1.0);
    else
         glOrtho(-SOURCE_UNIT*OutputRenderWindow::getInstance()->getAspectRatio(),
        		 OutputRenderWindow::getInstance()->getAspectRatio()*SOURCE_UNIT,
        		 -OutputRenderWindow::getInstance()->getAspectRatio()*SOURCE_UNIT/ar,
        		 OutputRenderWindow::getInstance()->getAspectRatio()*SOURCE_UNIT/ar,
        		 -MAX_DEPTH_LAYER, 1.0);

	glGetDoublev(GL_PROJECTION_MATRIX, projection);
}

void RenderingView::paint()
{
	static bool first = true;

	glPushMatrix();
	glTranslatef( - 2.f * SOURCE_UNIT * RenderingManager::getRenderingWidget()->catalogWidth() / viewport[2], 0.f, 0.f);
    glScalef( 1.0 - (float) RenderingManager::getRenderingWidget()->catalogWidth() / (float)viewport[2], 1.0 -(float) RenderingManager::getRenderingWidget()->catalogWidth() / (float)viewport[2], 1.0);

    // first the background (as the rendering black clear color) with shadow
	glPushMatrix();
    glScalef( OutputRenderWindow::getInstance()->getAspectRatio(), 1.0, 1.0);
    glCallList(ViewRenderWidget::quad_window[RenderingManager::getInstance()->clearToWhite()?1:0]);
    glPopMatrix();

	// we use the shader to render sources
	if (ViewRenderWidget::program->bind()) {
		first = true;
		// The icons of the sources (reversed depth order)
		for(SourceSet::iterator  its = RenderingManager::getInstance()->getBegin(); its != RenderingManager::getInstance()->getEnd(); its++) {

			//
			// 1. Render it into current view
			//
			ViewRenderWidget::setSourceDrawingMode(true);

            // place and scale
            glPushMatrix();
            glTranslated((*its)->getX(), (*its)->getY(), 0);
            glRotated((*its)->getRotationAngle(), 0.0, 0.0, 1.0);
            glScaled((*its)->getScaleX(), (*its)->getScaleY(), 1.f);

            // Blending Function For mixing like in the rendering window
            (*its)->beginEffectsSection();
            // bind the source texture
            (*its)->bind();

			// draw only if it is the current source
			if ( RenderingManager::getInstance()->isCurrentSource(its)
				// OR it is selected
			     || SelectionManager::getInstance()->isInSelection(*its)
			     // OR if there is no current source and no selection (i.e default case draw everything)
				 || ( !RenderingManager::getInstance()->isValid( RenderingManager::getInstance()->getCurrentSource())
                    && !SelectionManager::getInstance()->hasSelection() )  ) {

				// draw the source only if not culled and alpha not null
				if ( !(*its)->isStandby() && !(*its)->isCulled() && (*its)->getAlpha() > 0.0 ) {
					// Draw it !
					(*its)->blend();
					(*its)->draw();
				}
			}
			//
			// 2. Render it into FBO
			//
			RenderingManager::getInstance()->renderToFrameBuffer(*its, first);
			first = false;

			glPopMatrix();
		}
		ViewRenderWidget::program->release();
	}

	// if no source was rendered, clear anyway
	RenderingManager::getInstance()->renderToFrameBuffer(0, first, true);

	// post render draw (loop back and recorder)
	RenderingManager::getInstance()->postRenderToFrameBuffer();


    // finally the mask to hide the border
	glPushMatrix();
    glScalef( OutputRenderWindow::getInstance()->getAspectRatio(), 1.0, 1.0);
    glCallList(ViewRenderWidget::frame_screen_mask);
    glPopMatrix();

    // last the frame thing
    glPushMatrix();
    glScaled( OutputRenderWindow::getInstance()->getAspectRatio(), 1.0, 1.0);
    glCallList(ViewRenderWidget::frame_screen_thin);
    glPopMatrix();

    glPopMatrix();


	// DROP any source loaded
	if ( RenderingManager::getInstance()->getSourceBasketTop() )
		RenderingManager::getInstance()->dropSource();

}


bool RenderingView::mousePressEvent(QMouseEvent *event)
{
	if (!event)
		return false;

	// no current source
	RenderingManager::getInstance()->unsetCurrentSource();

	// clear selection
	SelectionManager::getInstance()->clearSelection();

	return true;
}
