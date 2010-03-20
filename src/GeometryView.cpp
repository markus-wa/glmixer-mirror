/*
 * GeometryView.cpp
 *
 *  Created on: Jan 31, 2010
 *      Author: bh
 */

#include "GeometryView.h"

#include "RenderingManager.h"
#include "ViewRenderWidget.h"
#include "OutputRenderWindow.h"
#include <algorithm>

#define MINZOOM 0.1
#define MAXZOOM 3.0
#define DEFAULTZOOM 0.5


GeometryView::GeometryView() : View(), quadrant(0), currentAction(NONE)
{
	zoom = DEFAULTZOOM;
	minzoom = MINZOOM;
	maxzoom = MAXZOOM;
	maxpanx = SOURCE_UNIT*MAXZOOM*2.0;
	maxpany = SOURCE_UNIT*MAXZOOM*2.0;

    icon.load(QString::fromUtf8(":/glmixer/icons/manipulation.png"));
}


void GeometryView::paint()
{
//	        GLubyte invertTable[256][4] = { {255, 255, 255, 255},
//	        		 {254, 254, 254, 254},
//	        		 {253, 253, 253, 253},
//	        		 {252, 252, 252, 252},
//	        		 {251, 251, 251, 251},
//	        		 {250, 250, 250, 250},
//	        		 {249, 249, 249, 249},
//	        		 {248, 248, 248, 248},
//	        		 {247, 247, 247, 247},
//	        		 {246, 246, 246, 246},
//	        		 {245, 245, 245, 245},
//	        		 {244, 244, 244, 244},
//	        		 {243, 243, 243, 243},
//	        		 {242, 242, 242, 242},
//	        		 {241, 241, 241, 241},
//	        		 {240, 240, 240, 240},
//	        		 {239, 239, 239, 239},
//	        		 {238, 238, 238, 238},
//	        		 {237, 237, 237, 237},
//	        		 {236, 236, 236, 236},
//	        		 {235, 235, 235, 235},
//	        		 {234, 234, 234, 234},
//	        		 {233, 233, 233, 233},
//	        		 {232, 232, 232, 232},
//	        		 {231, 231, 231, 231},
//	        		 {230, 230, 230, 230},
//	        		 {229, 229, 229, 229},
//	        		 {228, 228, 228, 228},
//	        		 {227, 227, 227, 227},
//	        		 {226, 226, 226, 226},
//	        		 {225, 225, 225, 225},
//	        		 {224, 224, 224, 224},
//	        		 {223, 223, 223, 223},
//	        		 {222, 222, 222, 222},
//	        		 {221, 221, 221, 221},
//	        		 {220, 220, 220, 220},
//	        		 {219, 219, 219, 219},
//	        		 {218, 218, 218, 218},
//	        		 {217, 217, 217, 217},
//	        		 {216, 216, 216, 216},
//	        		 {215, 215, 215, 215},
//	        		 {214, 214, 214, 214},
//	        		 {213, 213, 213, 213},
//	        		 {212, 212, 212, 212},
//	        		 {211, 211, 211, 211},
//	        		 {210, 210, 210, 210},
//	        		 {209, 209, 209, 209},
//	        		 {208, 208, 208, 208},
//	        		 {207, 207, 207, 207},
//	        		 {206, 206, 206, 206},
//	        		 {205, 205, 205, 205},
//	        		 {204, 204, 204, 204},
//	        		 {203, 203, 203, 203},
//	        		 {202, 202, 202, 202},
//	        		 {201, 201, 201, 201},
//	        		 {200, 200, 200, 200},
//	        		 {199, 199, 199, 199},
//	        		 {198, 198, 198, 198},
//	        		 {197, 197, 197, 197},
//	        		 {196, 196, 196, 196},
//	        		 {195, 195, 195, 195},
//	        		 {194, 194, 194, 194},
//	        		 {193, 193, 193, 193},
//	        		 {192, 192, 192, 192},
//	        		 {191, 191, 191, 191},
//	        		 {190, 190, 190, 190},
//	        		 {189, 189, 189, 189},
//	        		 {188, 188, 188, 188},
//	        		 {187, 187, 187, 187},
//	        		 {186, 186, 186, 186},
//	        		 {185, 185, 185, 185},
//	        		 {184, 184, 184, 184},
//	        		 {183, 183, 183, 183},
//	        		 {182, 182, 182, 182},
//	        		 {181, 181, 181, 181},
//	        		 {180, 180, 180, 180},
//	        		 {179, 179, 179, 179},
//	        		 {178, 178, 178, 178},
//	        		 {177, 177, 177, 177},
//	        		 {176, 176, 176, 176},
//	        		 {175, 175, 175, 175},
//	        		 {174, 174, 174, 174},
//	        		 {173, 173, 173, 173},
//	        		 {172, 172, 172, 172},
//	        		 {171, 171, 171, 171},
//	        		 {170, 170, 170, 170},
//	        		 {169, 169, 169, 169},
//	        		 {168, 168, 168, 168},
//	        		 {167, 167, 167, 167},
//	        		 {166, 166, 166, 166},
//	        		 {165, 165, 165, 165},
//	        		 {164, 164, 164, 164},
//	        		 {163, 163, 163, 163},
//	        		 {162, 162, 162, 162},
//	        		 {161, 161, 161, 161},
//	        		 {160, 160, 160, 160},
//	        		 {159, 159, 159, 159},
//	        		 {158, 158, 158, 158},
//	        		 {157, 157, 157, 157},
//	        		 {156, 156, 156, 156},
//	        		 {155, 155, 155, 155},
//	        		 {154, 154, 154, 154},
//	        		 {153, 153, 153, 153},
//	        		 {152, 152, 152, 152},
//	        		 {151, 151, 151, 151},
//	        		 {150, 150, 150, 150},
//	        		 {149, 149, 149, 149},
//	        		 {148, 148, 148, 148},
//	        		 {147, 147, 147, 147},
//	        		 {146, 146, 146, 146},
//	        		 {145, 145, 145, 145},
//	        		 {144, 144, 144, 144},
//	        		 {143, 143, 143, 143},
//	        		 {142, 142, 142, 142},
//	        		 {141, 141, 141, 141},
//	        		 {140, 140, 140, 140},
//	        		 {139, 139, 139, 139},
//	        		 {138, 138, 138, 138},
//	        		 {137, 137, 137, 137},
//	        		 {136, 136, 136, 136},
//	        		 {135, 135, 135, 135},
//	        		 {134, 134, 134, 134},
//	        		 {133, 133, 133, 133},
//	        		 {132, 132, 132, 132},
//	        		 {131, 131, 131, 131},
//	        		 {130, 130, 130, 130},
//	        		 {129, 129, 129, 129},
//	        		 {128, 128, 128, 128},
//	        		 {127, 127, 127, 127},
//	        		 {126, 126, 126, 126},
//	        		 {125, 125, 125, 125},
//	        		 {124, 124, 124, 124},
//	        		 {123, 123, 123, 123},
//	        		 {122, 122, 122, 122},
//	        		 {121, 121, 121, 121},
//	        		 {120, 120, 120, 120},
//	        		 {119, 119, 119, 119},
//	        		 {118, 118, 118, 118},
//	        		 {117, 117, 117, 117},
//	        		 {116, 116, 116, 116},
//	        		 {115, 115, 115, 115},
//	        		 {114, 114, 114, 114},
//	        		 {113, 113, 113, 113},
//	        		 {112, 112, 112, 112},
//	        		 {111, 111, 111, 111},
//	        		 {110, 110, 110, 110},
//	        		 {109, 109, 109, 109},
//	        		 {108, 108, 108, 108},
//	        		 {107, 107, 107, 107},
//	        		 {106, 106, 106, 106},
//	        		 {105, 105, 105, 105},
//	        		 {104, 104, 104, 104},
//	        		 {103, 103, 103, 103},
//	        		 {102, 102, 102, 102},
//	        		 {101, 101, 101, 101},
//	        		 {100, 100, 100, 100},
//	        		 {99, 99, 99, 99},
//	        		 {98, 98, 98, 98},
//	        		 {97, 97, 97, 97},
//	        		 {96, 96, 96, 96},
//	        		 {95, 95, 95, 95},
//	        		 {94, 94, 94, 94},
//	        		 {93, 93, 93, 93},
//	        		 {92, 92, 92, 92},
//	        		 {91, 91, 91, 91},
//	        		 {90, 90, 90, 90},
//	        		 {89, 89, 89, 89},
//	        		 {88, 88, 88, 88},
//	        		 {87, 87, 87, 87},
//	        		 {86, 86, 86, 86},
//	        		 {85, 85, 85, 85},
//	        		 {84, 84, 84, 84},
//	        		 {83, 83, 83, 83},
//	        		 {82, 82, 82, 82},
//	        		 {81, 81, 81, 81},
//	        		 {80, 80, 80, 80},
//	        		 {79, 79, 79, 79},
//	        		 {78, 78, 78, 78},
//	        		 {77, 77, 77, 77},
//	        		 {76, 76, 76, 76},
//	        		 {75, 75, 75, 75},
//	        		 {74, 74, 74, 74},
//	        		 {73, 73, 73, 73},
//	        		 {72, 72, 72, 72},
//	        		 {71, 71, 71, 71},
//	        		 {70, 70, 70, 70},
//	        		 {69, 69, 69, 69},
//	        		 {68, 68, 68, 68},
//	        		 {67, 67, 67, 67},
//	        		 {66, 66, 66, 66},
//	        		 {65, 65, 65, 65},
//	        		 {64, 64, 64, 64},
//	        		 {63, 63, 63, 63},
//	        		 {62, 62, 62, 62},
//	        		 {61, 61, 61, 61},
//	        		 {60, 60, 60, 60},
//	        		 {59, 59, 59, 59},
//	        		 {58, 58, 58, 58},
//	        		 {57, 57, 57, 57},
//	        		 {56, 56, 56, 56},
//	        		 {55, 55, 55, 55},
//	        		 {54, 54, 54, 54},
//	        		 {53, 53, 53, 53},
//	        		 {52, 52, 52, 52},
//	        		 {51, 51, 51, 51},
//	        		 {50, 50, 50, 50},
//	        		 {49, 49, 49, 49},
//	        		 {48, 48, 48, 48},
//	        		 {47, 47, 47, 47},
//	        		 {46, 46, 46, 46},
//	        		 {45, 45, 45, 45},
//	        		 {44, 44, 44, 44},
//	        		 {43, 43, 43, 43},
//	        		 {42, 42, 42, 42},
//	        		 {41, 41, 41, 41},
//	        		 {40, 40, 40, 40},
//	        		 {39, 39, 39, 39},
//	        		 {38, 38, 38, 38},
//	        		 {37, 37, 37, 37},
//	        		 {36, 36, 36, 36},
//	        		 {35, 35, 35, 35},
//	        		 {34, 34, 34, 34},
//	        		 {33, 33, 33, 33},
//	        		 {32, 32, 32, 32},
//	        		 {31, 31, 31, 31},
//	        		 {30, 30, 30, 30},
//	        		 {29, 29, 29, 29},
//	        		 {28, 28, 28, 28},
//	        		 {27, 27, 27, 27},
//	        		 {26, 26, 26, 26},
//	        		 {25, 25, 25, 25},
//	        		 {24, 24, 24, 24},
//	        		 {23, 23, 23, 23},
//	        		 {22, 22, 22, 22},
//	        		 {21, 21, 21, 21},
//	        		 {20, 20, 20, 20},
//	        		 {19, 19, 19, 19},
//	        		 {18, 18, 18, 18},
//	        		 {17, 17, 17, 17},
//	        		 {16, 16, 16, 16},
//	        		 {15, 15, 15, 15},
//	        		 {14, 14, 14, 14},
//	        		 {13, 13, 13, 13},
//	        		 {12, 12, 12, 12},
//	        		 {11, 11, 11, 11},
//	        		 {10, 10, 10, 10},
//	        		 {9, 9, 9, 9},
//	        		 {8, 8, 8, 8},
//	        		 {7, 7, 7, 7},
//	        		 {6, 6, 6, 6},
//	        		 {5, 5, 5, 5},
//	        		 {4, 4, 4, 4},
//	        		 {3, 3, 3, 3},
//	        		 {2, 2, 2, 2},
//	        		 {1, 1, 1, 1},
//	        		};// Inverted color table
//	        QString s("{");
//	        for(int i = 0; i < 255; i++)
//	            {
//	            invertTable[i][0] = (GLubyte)(255 - i);
//	            invertTable[i][1] = (GLubyte)(255 - i);
//	            invertTable[i][2] = (GLubyte)(255 - i);
//	            invertTable[i][3] = (GLubyte)(255 - i);
//	            s.append( QString(" {%1, %2, %3, %4}, \n").arg(invertTable[i][0]).arg(invertTable[i][1]).arg(invertTable[i][2]).arg(invertTable[i][3]) );
//	            }
//			qDebug("%s}", qPrintable(s));
//
//	        static GLfloat lumMat[16] = { 0.30f, 0.30f, 0.30f, 0.0f,
//	                                      0.59f, 0.59f, 0.59f, 0.0f,
//	                                      0.11f, 0.11f, 0.11f, 0.0f,
//	                                      0.0f,  0.0f,  0.0f,  1.0f };
//
//	        static GLfloat mSharpen[3][3] = {  // Sharpen convolution kernel
//	             {0.0f, .4f, 0.0f},
//	             {0.4f, 0.4f, 0.4f },
//	             {0.0f, 0.4f, 0.0f }};
//
//	        static GLfloat mEmboss[3][3] = {   // Emboss convolution kernel
//	            { -2.0f, -1.0f, 0.0f },
//	            { -1.0f, 1.0f, 1.0f },
//	            { 0.0f, 1.0f, 2.0f }};


    // first the black background (as the rendering black clear color) with shadow
    glCallList(ViewRenderWidget::quad_black);

    bool first = true;
    // then the icons of the sources (reversed depth order)
	for(SourceSet::iterator  its = RenderingManager::getInstance()->getBegin(); its != RenderingManager::getInstance()->getEnd(); its++) {

		//
		// 1. Render it into current view
		//
//
//        glColorTable(GL_COLOR_TABLE, GL_RGBA, 256, GL_RGBA, GL_UNSIGNED_BYTE, invertTable);
//        glEnable(GL_COLOR_TABLE);
//
//        glPixelTransferf(GL_RED_SCALE, 1.8f);
//        glPixelTransferf(GL_GREEN_SCALE, 1.8f);
//        glPixelTransferf(GL_BLUE_SCALE, 1.8f);
//
//        glPixelTransferf(GL_RED_BIAS, -0.45f);
//        glPixelTransferf(GL_GREEN_BIAS, -0.45f);
//        glPixelTransferf(GL_BLUE_BIAS, 0.45f);

//        glConvolutionFilter2D(GL_CONVOLUTION_2D, GL_RGB, 3, 3, GL_LUMINANCE, GL_FLOAT, mSharpen);
//        glEnable(GL_CONVOLUTION_2D);
//        glMatrixMode(GL_COLOR);
//        glScalef(0.5f, 0.5f, 0.5f);
//        glMatrixMode(GL_MODELVIEW);

//        glConvolutionFilter2D(GL_CONVOLUTION_2D, GL_RGB, 3, 3, GL_LUMINANCE, GL_FLOAT, mEmboss);
//        glEnable(GL_CONVOLUTION_2D);

		// BW
//        glMatrixMode(GL_COLOR);
//        glLoadMatrixf(lumMat);
//        glMatrixMode(GL_MODELVIEW);

//		glMatrixMode(GL_COLOR);
//		glScalef(1.5f, 1.5f, 1.5f);
//		glMatrixMode(GL_MODELVIEW);

        // place and scale
        glPushMatrix();
        glTranslated((*its)->getX(), (*its)->getY(), (*its)->getDepth());
        glScaled((*its)->getScaleX(), (*its)->getScaleY(), 1.f);

	    // Blending Function For mixing like in the rendering window
        (*its)->startBlendingSection();
		// bind the source texture and update its content
		(*its)->update();
		// test for culling
        (*its)->testCulling();
        // Draw it !
        (*its)->draw();

        // draw border if active
        if ((*its)->isActive())
            glCallList(ViewRenderWidget::border_large);
        else
            glCallList(ViewRenderWidget::border_thin);

        glPopMatrix();

		//
		// 2. Render it into FBO
		//
        RenderingManager::getInstance()->renderToFrameBuffer(its, first);
        first = false;

        // back to default blending for the rest
        (*its)->endBlendingSection();

        // Reset everyting to default
//        glDisable(GL_COLOR_TABLE);
//        glMatrixMode(GL_COLOR);
//        glLoadIdentity();
//
//        glMatrixMode(GL_MODELVIEW);
//        glDisable(GL_CONVOLUTION_2D);
////        glPixelTransferi(GL_MAP_COLOR, GL_FALSE);
//        glPixelTransferf(GL_RED_SCALE, 1.0f);
//        glPixelTransferf(GL_GREEN_SCALE, 1.0f);
//        glPixelTransferf(GL_BLUE_SCALE, 1.0f);
//        glPixelTransferf(GL_RED_BIAS, 0.0f);
//        glPixelTransferf(GL_GREEN_BIAS, 0.0f);
//        glPixelTransferf(GL_BLUE_BIAS, 0.0f);

    }
	if (first)
		RenderingManager::getInstance()->clearFrameBuffer();

    // last the frame thing
    glCallList(ViewRenderWidget::frame_screen);

    RenderingManager::getInstance()->updatePreviousFrame();
}


void GeometryView::reset()
{
    glScalef(zoom * OutputRenderWindow::getInstance()->getAspectRatio(), zoom, zoom);
    glTranslatef(getPanningX(), getPanningY(), 0.0);
}


void GeometryView::resize(int w, int h)
{
    glViewport(0, 0, w, h);
    viewport[2] = w;
    viewport[3] = h;

    // Setup specific projection and view for this window
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();

    if (w > h)
         glOrtho(-SOURCE_UNIT* (double) w / (double) h, SOURCE_UNIT*(double) w / (double) h, -SOURCE_UNIT, SOURCE_UNIT, -MAX_DEPTH_LAYER, 10.0);
     else
         glOrtho(-SOURCE_UNIT, SOURCE_UNIT, -SOURCE_UNIT*(double) h / (double) w, SOURCE_UNIT*(double) h / (double) w, -MAX_DEPTH_LAYER, 10.0);

    refreshMatrices();
}


bool GeometryView::mousePressEvent(QMouseEvent *event)
{
	lastClicPos = event->pos();

	if (event->buttons() & Qt::MidButton) {
		RenderingManager::getRenderingWidget()->setCursor(Qt::SizeAllCursor);
	}
	// if at least one source icon was clicked
	else if ( getSourcesAtCoordinates(event->x(), viewport[3] - event->y()) ) {

    	// get the top most clicked source
    	SourceSet::iterator clicked = clickedSources.begin();

    	// for LEFT button clic : manipulate only the top most or the newly clicked
    	if (event->buttons() & Qt::LeftButton) {

    		// if there was no current source, its simple : just take the top most source clicked now
    		// OR
			// if the currently active source is NOT in the set of clicked sources,
			if ( RenderingManager::getInstance()->getCurrentSource() == RenderingManager::getInstance()->getEnd()
				|| clickedSources.count(*RenderingManager::getInstance()->getCurrentSource() ) == 0 )
    			//  make the top most source clicked now the newly current one
    			RenderingManager::getInstance()->setCurrentSource( (*clicked)->getId() );

			// now manipulate the current one.
    		quadrant = getSourceQuadrant(RenderingManager::getInstance()->getCurrentSource(), event->x(), viewport[3] - event->y());
			if (quadrant > 0) {
				currentAction = GeometryView::SCALE;
				if ( quadrant % 2 )
					RenderingManager::getRenderingWidget()->setCursor(Qt::SizeFDiagCursor);
				else
					RenderingManager::getRenderingWidget()->setCursor(Qt::SizeBDiagCursor);
			} else  {
				currentAction = GeometryView::MOVE;
				RenderingManager::getRenderingWidget()->setCursor(Qt::ClosedHandCursor);
			}
    	}
    	// for RIGHT button clic : switch the currently active source to the one bellow, if exists
    	else if (event->buttons() & Qt::RightButton) {

    		// if there is no source selected, select the top most
    		if ( RenderingManager::getInstance()->getCurrentSource() == RenderingManager::getInstance()->getEnd() )
    			RenderingManager::getInstance()->setCurrentSource( (*clicked)->getId() );
    		// else, try to take another one bellow it
    		else {
    			// find where the current source is in the clickedSources
    			clicked = clickedSources.find(*RenderingManager::getInstance()->getCurrentSource()) ;
    			// decrement the clicked iterator forward in the clickedSources (and jump back to end when at begining)
    			if ( clicked == clickedSources.begin() )
    				clicked = clickedSources.end();
				clicked--;

				// set this newly clicked source as the current one
    			RenderingManager::getInstance()->setCurrentSource( (*clicked)->getId() );
    		}
    	}
    } else
		// set current to none (end of list)
		RenderingManager::getInstance()->setCurrentSource( RenderingManager::getInstance()->getEnd() );

	return true;
}

bool GeometryView::mouseMoveEvent(QMouseEvent *event)
{
    int dx = event->x() - lastClicPos.x();
    int dy = lastClicPos.y() - event->y();
    lastClicPos = event->pos();

	// MIDDLE button ; panning
	if (event->buttons() & Qt::MidButton) {

		panningBy(event->x(), viewport[3] - event->y(), dx, dy);

	}
	// LEFT button : MOVE or SCALE the current source
	else if (event->buttons() & Qt::LeftButton) {
		// keep the iterator of the current source under the shoulder ; it will be used
		SourceSet::iterator cs = RenderingManager::getInstance()->getCurrentSource();
		if ( RenderingManager::getInstance()->notAtEnd(cs)) {
			// manipulate the current source according to the operation detected when clicking
			if (currentAction == GeometryView::SCALE)
				scaleSource(cs, event->x(), viewport[3] - event->y(), dx, dy);
			else if (currentAction == GeometryView::MOVE)
				grabSource(cs, event->x(), viewport[3] - event->y(), dx, dy);

		}
		return true;

//		} else if (event->buttons() & Qt::RightButton) {

	} else  { // mouse over (no buttons)

	//	SourceSet::iterator over = getSourceAtCoordinates(event->x(), viewport[3] - event->y());


	}

	return false;
}

bool GeometryView::mouseReleaseEvent ( QMouseEvent * event ){

	RenderingManager::getRenderingWidget()->setCursor(Qt::ArrowCursor);

    // enforces minimal size ; check that the rescaling did not go bellow the limits and fix it
	if ( RenderingManager::getInstance()->notAtEnd( RenderingManager::getInstance()->getCurrentSource()) ) {
		(*RenderingManager::getInstance()->getCurrentSource())->clampScale();
	}

	currentAction = GeometryView::NONE;
	return true;
}

bool GeometryView::wheelEvent ( QWheelEvent * event ){


	float previous = zoom;
	setZoom (zoom + ((float) event->delta() * zoom * minzoom) / (120.0 * maxzoom) );

	if (currentAction == GeometryView::SCALE || currentAction == GeometryView::MOVE ){
		deltazoom = 1.0 - (zoom / previous);
		// keep the iterator of the current source under the shoulder ; it will be used
		SourceSet::iterator cs = RenderingManager::getInstance()->getCurrentSource();
		if ( RenderingManager::getInstance()->notAtEnd(cs)) {
			// manipulate the current source according to the operation detected when clicking
			if (currentAction == GeometryView::SCALE)
				scaleSource(cs, event->x(), viewport[3] - event->y(), 0, 0);
			else if (currentAction == GeometryView::MOVE)
				grabSource(cs, event->x(), viewport[3] - event->y(), 0, 0);

		}
		// reset deltazoom
		deltazoom = 0;
	}



	return true;
}


bool GeometryView::mouseDoubleClickEvent ( QMouseEvent * event ){


	// for LEFT double button clic : expand the current source to the rendering area
	if ( (event->buttons() & Qt::LeftButton) && getSourcesAtCoordinates(event->x(), viewport[3] - event->y()) ) {

		if ( RenderingManager::getInstance()->getCurrentSource() != RenderingManager::getInstance()->getEnd()){

			(*RenderingManager::getInstance()->getCurrentSource())->resetScale();
		} else
			zoomBestFit();

	}

	return true;
}

void GeometryView::zoomReset() {
	setZoom(DEFAULTZOOM);
	setPanningX(0);
	setPanningY(0);
}

void GeometryView::zoomBestFit() {

	// nothing to do if there is no source
	if (RenderingManager::getInstance()->getBegin() == RenderingManager::getInstance()->getEnd()){
		zoomReset();
		return;
	}

	// 1. compute bounding box of every sources
    double x_min = 10000, x_max = -10000, y_min = 10000, y_max = -10000;
	for(SourceSet::iterator  its = RenderingManager::getInstance()->getBegin(); its != RenderingManager::getInstance()->getEnd(); its++) {
		x_min = MINI (x_min, (*its)->getX() - (*its)->getScaleX());
		x_max = MAXI (x_max, (*its)->getX() + (*its)->getScaleX());
		y_min = MINI (y_min, (*its)->getY() - (*its)->getScaleY());
		y_max = MAXI (y_max, (*its)->getY() + (*its)->getScaleY());
	}

	// 2. Apply the panning to the new center
	setPanningX	( -( x_min + ABS(x_max - x_min)/ 2.0 ) );
	setPanningY	( -( y_min + ABS(y_max - y_min)/ 2.0 )  );

	// 3. get the extend of the area covered in the viewport (the matrices have been updated just above)
    double LLcorner[3];
    double URcorner[3];
    gluUnProject(viewport[0], viewport[1], 1, modelview, projection, viewport, LLcorner, LLcorner+1, LLcorner+2);
    gluUnProject(viewport[2], viewport[3], 1, modelview, projection, viewport, URcorner, URcorner+1, URcorner+2);

	// 4. compute zoom factor to fit to the boundaries
    // initial value = a margin scale of 5%
    double scale = 0.95;
    // depending on the axis having the largest extend
    if ( ABS(x_max-x_min) > ABS(y_max-y_min))
    	scale *= ABS(URcorner[0]-LLcorner[0]) / ABS(x_max-x_min);
    else
    	scale *= ABS(URcorner[1]-LLcorner[1]) / ABS(y_max-y_min);
    // apply the scaling
	setZoom( zoom * scale );

}


bool GeometryView::getSourcesAtCoordinates(int mouseX, int mouseY) {

	// prepare variables
	clickedSources.clear();
    GLuint selectBuf[SELECTBUFSIZE] = { 0 };
    GLint hits = 0;

    // init picking
    glSelectBuffer(SELECTBUFSIZE, selectBuf);
    (void) glRenderMode(GL_SELECT);

    // picking in name 0, labels set later
    glInitNames();
    glPushName(0);

    // use the projection as it is, but remember it.
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    // setup the projection for picking
    glLoadIdentity();
    gluPickMatrix((GLdouble) mouseX, (GLdouble) mouseY, 1.0, 1.0, viewport);
    glMultMatrixd(projection);

    // rendering for select mode
    glMatrixMode(GL_MODELVIEW);

	for(SourceSet::iterator  its = RenderingManager::getInstance()->getBegin(); its != RenderingManager::getInstance()->getEnd(); its++) {
        glPushMatrix();
        // place and scale
        glTranslated((*its)->getX(), (*its)->getY(), (*its)->getDepth());
        glScaled( (*its)->getScaleX(), (*its)->getScaleY(), 1.f);
        (*its)->draw(false, GL_SELECT);
        glPopMatrix();
    }

    // compute picking . return to rendering mode
    hits = glRenderMode(GL_RENDER);

//    qDebug ("%d hits @ (%d,%d) vp (%d, %d, %d, %d)", hits, mouseX, mouseY, viewport[0], viewport[1], viewport[2], viewport[3]);

    // set the matrices back
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);

    while (hits != 0) {
    	clickedSources.insert( *(RenderingManager::getInstance()->getById (selectBuf[ (hits-1) * 4 + 3])) );
    	hits--;
    }

    return !clickedSources.empty();
}


/**
 *
 **/
void GeometryView::panningBy(int x, int y, int dx, int dy) {

    double bx, by, bz; // before movement
    double ax, ay, az; // after  movement

    gluUnProject((GLdouble) (x - dx), (GLdouble) (y - dy),
            0.0, modelview, projection, viewport, &bx, &by, &bz);
    gluUnProject((GLdouble) x, (GLdouble) y, 0.0,
            modelview, projection, viewport, &ax, &ay, &az);

    // apply panning
    setPanningX(getPanningX() + ax - bx);
    setPanningY(getPanningY() + ay - by);
}

/**
 *
 **/
void GeometryView::grabSource(SourceSet::iterator currentSource, int x, int y, int dx, int dy) {

    double bx, by, bz; // before movement
    double ax, ay, az; // after  movement

    gluUnProject((GLdouble) (x - dx), (GLdouble) (y - dy),
            0.0, modelview, projection, viewport, &bx, &by, &bz);
    gluUnProject((GLdouble) x, (GLdouble) y, 0.0,
            modelview, projection, viewport, &ax, &ay, &az);

    ax += (ax + getPanningX()) * deltazoom;
    ay += (ay + getPanningY()) * deltazoom;

    double ix = (*currentSource)->getX() + (ax - bx);
    double iy = (*currentSource)->getY() + (ay - by);

    // move source
    (*currentSource)->moveTo(ix, iy);

}

/**
 *
 **/
void GeometryView::scaleSource(SourceSet::iterator currentSource, int X, int Y, int dx, int dy) {

    double bx, by, bz; // before movement
    double ax, ay, az; // after  movement

    // make proportionnal scaling

    gluUnProject((GLdouble) (X - dx), (GLdouble) (Y - dy),
            1.0, modelview, projection, viewport, &bx, &by, &bz);
    gluUnProject((GLdouble) X, (GLdouble) Y, 1.0,
            modelview, projection, viewport, &ax, &ay, &az);


    double w = ((*currentSource)->getScaleX());
    double x = (*currentSource)->getX();
    double h = ((*currentSource)->getScaleY());
    double y = (*currentSource)->getY();
    double sx = 1.0, sy = 1.0;
    double xp = x, yp = y;

    ax += (ax + getPanningX()) * deltazoom;
    ay += (ay + getPanningY()) * deltazoom;

    if ( quadrant == 2 || quadrant == 3) {  // RIGHT
            sx = (ax - x + w) / ( bx - x + w);
            xp = x + w * (sx - 1.0);
    } else {                                // LEFT
            sx = (ax - x - w) / ( bx - x - w);
            xp = x - w * (sx - 1.0);
    }

    if ( quadrant < 3 ){                    // TOP
            sy = (ay - y + h) / ( by - y + h);
            yp = y + h * (sy - 1.0);
    } else {                                // BOTTOM
            sy = (ay - y - h) / ( by - y - h);
            yp = y - h * (sy - 1.0);
    }

    (*currentSource)->scaleBy(sx, sy);
    (*currentSource)->moveTo(xp, yp);
}


/**
 *
 **/
char GeometryView::getSourceQuadrant(SourceSet::iterator currentSource, int X, int Y) {
    //      ax
    //      ^
    //  ----|----
    //  | 3 | 4 |
    //  ----+---- > ay
    //  | 2 | 1 |
    //  ---------
    char quadrant = 0;
    double ax, ay, az;

    gluUnProject((GLdouble) X, (GLdouble) Y, 0.0,
            modelview, projection, viewport, &ax, &ay, &az);

    double w = ((*currentSource)->getScaleX());
    double x = (*currentSource)->getX();
    double h = ((*currentSource)->getScaleY());
    double y = (*currentSource)->getY();

    if (( x > ax + 0.8 * ABS(w) ) && ( y < ay - 0.8 * ABS(h) ) ) // RIGHT BOTTOM
//        quadrant = 1;
        quadrant = h > 0 ? (w > 0 ? (1) : (2)) : (w > 0 ? (4) : (3));
    else if  (( x > ax + 0.8 * ABS(w)) && ( y > ay + 0.8 * ABS(h) ) ) // RIGHT TOP
//        quadrant = 4;
        quadrant = h > 0 ? (w > 0 ? (4) : (3)) : (w > 0 ? (1) : (2));
    else if  (( x < ax - 0.8 * ABS(w)) && ( y < ay - 0.8 * ABS(h) ) ) // LEFT BOTTOM
//        quadrant = 2;
    	quadrant = h > 0 ? (w > 0 ? (2) : (1)) : (w > 0 ? (3) : (4));
    else if  (( x < ax - 0.8 * ABS(w)) && ( y > ay + 0.8 * ABS(h) ) ) // LEFT TOP
//        quadrant = 3;
    	quadrant = h > 0 ? (w > 0 ? (3) : (4)) : (w > 0 ? (2) : (1));

    return quadrant;
}


