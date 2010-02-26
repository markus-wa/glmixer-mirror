/*
 * Source.cpp
 *
 *  Created on: Jun 29, 2009
 *      Author: bh
 */

 
#include "Source.h"
#include "ViewRenderWidget.h"
#include "OutputRenderWindow.h"

GLuint Source::lastid = 1;

Source::Source(GLuint texture, double depth) :
		textureIndex(texture), x(0.0), y(0.0), z(depth), scalex(SOURCE_UNIT), scaley(SOURCE_UNIT), alphax(0.0), alphay(0.0),
			aspectratio(1.0), texalpha(1.0) {

	texcolor = Qt::white;
//	blendcolor = Qt::white;
	source_blend = GL_SRC_ALPHA;
	destination_blend =  GL_DST_ALPHA;
	blend_eq = GL_FUNC_ADD;

	// give it a unique identifying name
	id = lastid++;

	// TODO set attributes and children
	dom.setAttribute("id", id);
	QDomElement coordinates;
	coordinates.setAttribute("x", x);
	coordinates.setAttribute("y", y);
	coordinates.setAttribute("z", z);
//	dom.appendChild(coordinates);

}


Source::~Source() {

	glDeleteTextures(1, &textureIndex);
}


Source::Source(Source *clone, double d) {

    // clone everything
    id = clone->id;
    x = clone->x;
    y = clone->y;
    scalex = clone->scalex;
    scaley = clone->scaley;
    alphax = clone->alphax;
    alphay = clone->alphay;
    aspectratio = clone->aspectratio;
    texalpha = clone->texalpha;
    texcolor = clone->texcolor;
    active = clone->active;
    textureIndex = clone->textureIndex;
	source_blend  = clone->source_blend;
	destination_blend = clone->destination_blend;
	blend_eq = clone->blend_eq;

    // new depth (if in correct value range)
    if (d > MIN_DEPTH_LAYER && d < MAX_DEPTH_LAYER)
        z = d;
    else
        z = clone->z;

}

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
	// adjust alpha according to distance to center
	if (d < 1.0)
		texalpha = 1.0 - d;
	else
		texalpha = 0.0;
}


void Source::resetScale() {
	scalex = SOURCE_UNIT;
	scaley = SOURCE_UNIT;

	float renderingAspectRatio = OutputRenderWindow::getInstance()->getAspectRatio();
	if (aspectratio < renderingAspectRatio)
		scalex *= aspectratio / renderingAspectRatio;
	else
		scaley *= renderingAspectRatio / aspectratio;
}

void Source::draw(bool withalpha, GLenum mode) const {
    // set id in select mode, avoid texturing if not rendering.
    if (mode == GL_SELECT)
        glLoadName(id);
    else {
		// set transparency and color
		glColor4f(texcolor.redF(), texcolor.greenF(), texcolor.blueF(), withalpha ? texalpha : 1.0);
    }
    // draw
    glCallList(ViewRenderWidget::quad_texured);
}

void Source::blend() const {
//	glBlendColor(blendcolor.redF(), blendcolor.greenF(), blendcolor.blueF(), blendcolor.alphaF());
	glBlendEquation(blend_eq);
	glBlendFunc(source_blend, destination_blend);
}


