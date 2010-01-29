/*
 * Source.h
 *
 *  Created on: Jun 29, 2009
 *      Author: bh
 */

#ifndef SOURCE_H_
#define SOURCE_H_

#include <QtOpenGL>
#include <QDomElement>

#include "common.h"

#define SELECTBUFSIZE 64
#define MIN_DEPTH_LAYER 0.0
#define MAX_DEPTH_LAYER -15.0
#define MIN_SCALE 0.31
#define MAX_SCALE 5.0
#define SOURCE_UNIT 1000.0

class Source {

public:

	Source(QGLWidget *context);
	//    Source(Source *clone, double newdepth = MIN_DEPTH_LAYER);
	virtual ~Source();

	virtual void update() = 0;


    void draw(bool withalpha = true, bool withborder = false, GLenum mode = GL_RENDER) const;
    void drawHalf() const;
    void drawSelect() const;

	// manipulation
	inline GLuint getId() const {
		return id;
	}
	inline GLdouble getDepth() const {
		return z;
	}
	inline bool isActive() const {
		return active;
	}
	inline void activate(bool flag) {
		active = flag;
	}

	// OpenGL access
	inline GLuint getFboAttachmentPoint() {
		return attachmentPoint;
	}

	inline GLuint getTextureIndex() {
		return textureIndex;
	}

	//  Geometry
	// gets
	inline GLdouble getAspectRatio() const {
		return aspectratio;
	}
	inline GLdouble getX() const {
		return x;
	}
	inline GLdouble getY() const {
		return y;
	}
	inline GLdouble getScaleX() const {
		return scalex;
	}
	inline GLdouble getScaleY() const {
		return scaley;
	}
	inline GLdouble getAlphaX() const {
		return alphax;
	}
	inline GLdouble getAlphaY() const {
		return alphay;
	}
	inline GLfloat getAlpha() const {
		return texalpha;
	}

	// sets
	inline void setX(GLdouble v) {
		x = v;
	}
	inline void setY(GLdouble v) {
		y = v;
	}
	inline void setScaleX(GLdouble v) {
		scalex = v;
	}
	inline void setScaleY(GLdouble v) {
		scaley = v;
	}
	inline void moveTo(GLdouble posx, GLdouble posy) {
		x = posx;
		y = posy;
	}
	inline void setScale(GLdouble sx, GLdouble sy) {
		scalex = sx;
		scaley = sy;
	}
	void scaleBy(GLfloat fx, GLfloat fy);
	void setAlphaCoordinates(GLdouble x, GLdouble y, GLdouble max);


protected:

	// identity and properties
	GLuint id;
	QDomElement dom;

	// GL Stuff
	QGLWidget *glcontext;
	GLuint textureIndex;
	GLenum attachmentPoint;
	GLdouble x, y, z;
	GLdouble scalex, scaley;
	GLdouble alphax, alphay;
	GLdouble aspectratio;
	GLfloat texalpha;
	GLfloat texcolor;

	// state
	bool active;

	// statics
	static GLuint lastid;
	static GLuint squareDisplayList, halfDisplayList, selectDisplayList;
	static GLuint lineDisplayList[2];

	// utility
    GLuint buildHalfList();
    GLuint buildSelectList();
    GLuint buildLineList();
    GLuint buildQuadList();
};

#endif /* SOURCE_H_ */
