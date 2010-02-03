/*
 * Source.cpp
 *
 *  Created on: Jun 29, 2009
 *      Author: bh
 */

 
#include "Source.h"
#include "glRenderWidget.h"
#include "MainRenderWidget.h"

GLuint Source::lastid = 1;
//GLuint Source::squareDisplayList = 0;
//GLuint Source::halfDisplayList = 0;
//GLuint Source::selectDisplayList = 0;
//GLuint Source::lineDisplayList[2] =  { 0, 0 };

Source::Source(QGLWidget *context) :
	glcontext(context), x(0.0), y(0.0), z(0.0), alphax(0.0), alphay(0.0),
			texalpha(1.0), texcolor(1.0) {
	// give it a unique identifying name
	// TODO CHANGE the way ids are used
	id = lastid++;

	// TODO read aspect ratio from image in VideoFile
	aspectratio = 1.0;
	scalex = aspectratio * SOURCE_UNIT;
	scaley = SOURCE_UNIT;

	// TODO do not use ids for depth
	z = -(double) id;

	if (!context)
		// TODO : through exception
		return;
	glcontext->makeCurrent();

	glGenTextures(1, &textureIndex);
	glBindTexture(GL_TEXTURE_2D, textureIndex);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

	// TODO : use fbo in MainRenderWidget using a different attachment point per source
	//attachmentPoint = GL_COLOR_ATTACHMENT0_EXT;

//	// create display list if never created
//	if (!squareDisplayList)
//		squareDisplayList = buildQuadList();
//	if (!halfDisplayList)
//		halfDisplayList = buildHalfList();
//	if (!selectDisplayList)
//		selectDisplayList = buildSelectList();
//
//	if (lineDisplayList[0] == 0){
//		lineDisplayList[0] = buildLineList();
//		lineDisplayList[1] = lineDisplayList[0] + 1;
//	}



	// set attributes and children
	dom.setAttribute("id", id);
	QDomElement coordinates;
	coordinates.setAttribute("x", x);
	coordinates.setAttribute("y", y);
	coordinates.setAttribute("z", z);
	dom.appendChild(coordinates);

}


Source::~Source() {

	glDeleteTextures(1, &textureIndex);
}

//
//Source::Source(Source *clone, double d) {
//
//    // clone everything
//    id = clone->id;
//    x = clone->x;
//    y = clone->y;
//    scalex = clone->scalex;
//    scaley = clone->scaley;
//    alphax = clone->alphax;
//    alphay = clone->alphay;
//    aspectratio = clone->aspectratio;
//    texalpha = clone->texalpha;
//    texcolor = clone->texcolor;
//    active = clone->active;
//
//    // new depth (if in correct value range)
//    if (d > MIN_DEPTH_LAYER && d < MAX_DEPTH_LAYER)
//        z = d;
//    else
//        z = clone->z;
//
//}

void Source::scaleBy(float fx, float fy) {
	scalex *= fx;
	scaley *= fy;
}

void Source::setAlphaCoordinates(double x, double y, double max) {

	// set new alpha coordinates
	alphax = x;
	alphay = y;

	// Compute distance to the center
	double d = ((x * x) + (y * y)) / (SOURCE_UNIT * SOURCE_UNIT * max * max); // QUADRATIC
	// adjust alpha acording to distance to center
	if (d < 1.0)
		texalpha = 1.0 - d;
	else
		texalpha = 0.0;
}


void Source::draw(bool withalpha, GLenum mode) const {
    // set id in select mode, avoid texturing if not rendering.
    if (mode == GL_SELECT)
        glLoadName(id);
    else {

        glBindTexture(GL_TEXTURE_2D, textureIndex);
        // ensure alpha channel is modulated
        glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_COMBINE);
		glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_ALPHA, GL_MODULATE);
		// set transparency
		glColor4f(texcolor, texcolor, texcolor, withalpha ? texalpha : 1.0);

    }
    // draw
    glCallList(MainRenderWidget::quad_texured);
}



