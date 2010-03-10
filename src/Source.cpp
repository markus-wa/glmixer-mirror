/*
 * Source.cpp
 *
 *  Created on: Jun 29, 2009
 *      Author: bh
 */

#include "SourceSet.h"
#include "ViewRenderWidget.h"
#include "OutputRenderWindow.h"

GLuint Source::lastid = 1;

Source::Source(GLuint texture, double depth) :
		active(false), culled(false), textureIndex(texture), x(0.0), y(0.0), z(depth), scalex(SOURCE_UNIT), scaley(SOURCE_UNIT), alphax(0.0), alphay(0.0),
			aspectratio(1.0), texalpha(1.0) {

	z = CLAMP(z, MIN_DEPTH_LAYER, MAX_DEPTH_LAYER);

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

	clones = new SourceList;
}


Source::~Source() {

	delete clones;
}

// TODO ; do we need a copy constructor ?
//Source::Source(Source *duplicate, double d) {
//
//}

void Source::testCulling(){

	// if all coordinates of center are between viewport limits, it is obviously visible
	if ( x > -SOURCE_UNIT && x < SOURCE_UNIT && y > -SOURCE_UNIT && y < SOURCE_UNIT )
		culled = false;
	else {
		// not obviously visible
		// but it might still be parly visible if the distance from the center to the borders is less than the width
		if ( ( x + ABS(scalex) < -SOURCE_UNIT ) || ( x - ABS(scalex) > SOURCE_UNIT ) )
			culled = true;
		else if ( ( y + ABS(scaley) < -SOURCE_UNIT ) || ( y - ABS(scaley) > SOURCE_UNIT ) )
			culled = true;
		else
			culled = false;
	}
}

void Source::scaleBy(float fx, float fy) {
	scalex *= fx;
	scaley *= fy;
}

void Source::clampScale(){

	scalex = (scalex > 0 ? 1.0 : -1.0) *  CLAMP( ABS(scalex), MIN_SCALE, MAX_SCALE);
	scaley = (scaley > 0 ? 1.0 : -1.0) *  CLAMP( ABS(scaley), MIN_SCALE, MAX_SCALE);
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


