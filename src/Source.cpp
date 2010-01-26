/*
 * Source.cpp
 *
 *  Created on: Jun 29, 2009
 *      Author: bh
 */

#include "Source.h"
#include "glRenderWidget.h"

GLuint Source::lastid = 1;
GLuint Source::squareDisplayList = 0;
GLuint Source::halfDisplayList = 0;
GLuint Source::selectDisplayList = 0;
GLuint Source::lineDisplayList[2] =  { 0, 0 };

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
	attachmentPoint = GL_COLOR_ATTACHMENT0_EXT;

	// create display list if never created
	if (!squareDisplayList)
		squareDisplayList = buildQuadList();
	if (!halfDisplayList)
		halfDisplayList = buildHalfList();
	if (!selectDisplayList)
		selectDisplayList = buildSelectList();

	if (lineDisplayList[0] == 0){
		lineDisplayList[0] = buildLineList();
		lineDisplayList[1] = lineDisplayList[0] + 1;
	}

	// set attributes and children
	dom.setAttribute("id", id);
	QDomElement coordinates;
	coordinates.setAttribute("x", x);
	coordinates.setAttribute("y", y);
	coordinates.setAttribute("z", z);
	dom.appendChild(coordinates);

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


void Source::draw(bool withalpha, bool withborder, GLenum mode) const {
    // set id in select mode, avoid texturing if not rendering.
    if (mode == GL_SELECT)
        glLoadName(id);
    else {
        if (withborder) {
            if (active)
                glCallList(lineDisplayList[1]);
            else
                glCallList(lineDisplayList[0]);
        }
        glBindTexture(GL_TEXTURE_2D, textureIndex);

    }

    // set transparency
    glColor4f(texcolor, texcolor, texcolor, withalpha ? texalpha : 1.0);
    // draw
    glCallList(squareDisplayList);
}

void Source::drawHalf() const {
	glCallList(halfDisplayList);
}


void Source::drawSelect() const {
	glCallList(selectDisplayList);
}

/**
 * Build a display list of a textured QUAD and returns its id
 **/
GLuint Source::buildHalfList() {

    GLuint id = glGenLists(1);
    glNewList(id, GL_COMPILE);


    glBegin(GL_TRIANGLES); // begin drawing a triangle

    glColor4f(1.0, 1.0, 1.0, 1.0);
    // Front Face (note that the texture's corners have to match the quad's corners)
    glNormal3f(0.0f, 0.0f, 1.0f); // front face points out of the screen on z.

    glTexCoord2f(0.0f, 1.0f);
    glVertex3d(-1.0, -1.0, 0.0); // Bottom Left
    glTexCoord2f(1.0f, 1.0f);
    glVertex3d(1.0, -1.0, 0.0); // Bottom Right
    glTexCoord2f(1.0f, 0.0f);
    glVertex3d(1.0, 1.0, 0.0); // Top Right

    glEnd();

    glEndList();
    return id;
}

/**
 * Build a display lists for the line borders and returns its id
 **/
GLuint Source::buildSelectList() {

    GLuint base = glGenLists(1);

    // selected
    glNewList(base, GL_COMPILE);

    glPushAttrib(GL_ENABLE_BIT | GL_CURRENT_BIT | GL_COLOR_BUFFER_BIT);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDisable(GL_TEXTURE_2D);

    glLineWidth(3.0);
    glColor4f(0.2, 0.80, 0.2, 1.0);

    glBegin(GL_LINE_LOOP); // begin drawing a square
    glVertex3d(-1.1 , -1.1 , 0.0); // Bottom Left
    glVertex3d(1.1 , -1.1 , 0.0); // Bottom Right
    glVertex3d(1.1 , 1.1 , 0.0); // Top Right
    glVertex3d(-1.1 , 1.1 , 0.0); // Top Left
    glEnd();

    glPopAttrib();

    glEndList();

    return base;
}


/**
 * Build a display list of a textured QUAD and returns its id
 **/
GLuint Source::buildQuadList() {

    GLuint id = glGenLists(1);
    glNewList(id, GL_COMPILE);

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

    glEndList();
    return id;
}

/**
 * Build 2 display lists for the line borders and shadows
 **/
GLuint Source::buildLineList() {

    GLuint texid = glcontext->bindTexture(QPixmap(QString::fromUtf8(":/glmixer/textures/shadow_corner.png")), GL_TEXTURE_2D);

    GLuint base = glGenLists(2);
    glListBase(base);

    // default
    glNewList(base, GL_COMPILE);

    glPushAttrib(GL_ENABLE_BIT | GL_CURRENT_BIT | GL_COLOR_BUFFER_BIT);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, texid); // 2d texture (x and y size)

    glPushMatrix();
    glTranslatef(0.05, -0.1, 0.1);
    glScalef(1.3, 1.3, 1.0);
    glBegin(GL_QUADS); // begin drawing a square
    glTexCoord2f(0.0f, 0.0f);
    glVertex3f(-1.f, -1.f, 0.0f); // Bottom Left
    glTexCoord2f(1.0f, 0.0f);
    glVertex3f(1.f, -1.f, 0.0f); // Bottom Right
    glTexCoord2f(1.0f, 1.0f);
    glVertex3f(1.f, 1.f, 0.0f); // Top Right
    glTexCoord2f(0.0f, 1.0f);
    glVertex3f(-1.f, 1.f, 0.0f); // Top Left
    glEnd();
    glPopMatrix();

    glDisable(GL_TEXTURE_2D);
    glLineWidth(1.0);
    glColor4f(0.9, 0.9, 0.0, 0.7);

    glBegin(GL_LINE_LOOP); // begin drawing a square
    glVertex3f(-1.05f, -1.05f, 0.0f); // Bottom Left
    glVertex3f(1.05f, -1.05f, 0.0f); // Bottom Right
    glVertex3f(1.05f, 1.05f, 0.0f); // Top Right
    glVertex3f(-1.05f, 1.05f, 0.0f); // Top Left
    glEnd();

    glPopAttrib();
    glEndList();

    // over
    glNewList(base + 1, GL_COMPILE);

    glPushAttrib(GL_ENABLE_BIT | GL_CURRENT_BIT | GL_COLOR_BUFFER_BIT);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, texid); // 2d texture (x and y size)

    glPushMatrix();
    glTranslatef(0.15, -0.3, 0.1);
    glScalef(1.2, 1.2, 1.0);
    glBegin(GL_QUADS); // begin drawing a square
    glTexCoord2f(0.0f, 0.0f);
    glVertex3f(-1.f, -1.f, 0.0f); // Bottom Left
    glTexCoord2f(1.0f, 0.0f);
    glVertex3f(1.f, -1.f, 0.0f); // Bottom Right
    glTexCoord2f(1.0f, 1.0f);
    glVertex3f(1.f, 1.f, 0.0f); // Top Right
    glTexCoord2f(0.0f, 1.0f);
    glVertex3f(-1.f, 1.f, 0.0f); // Top Left
    glEnd();
    glPopMatrix();

    glDisable(GL_TEXTURE_2D);
    glLineWidth(3.0);
    glColor4f(0.9, 0.9, 0.0, 0.7);

    glBegin(GL_LINE_LOOP); // begin drawing a square
    glVertex3f(-1.05f, -1.05f, 0.0f); // Bottom Left
    glVertex3f(1.05f, -1.05f, 0.0f); // Bottom Right
    glVertex3f(1.05f, 1.05f, 0.0f); // Top Right
    glVertex3f(-1.05f, 1.05f, 0.0f); // Top Left
    glEnd();

    glPopAttrib();

    glEndList();

    return base;
}


