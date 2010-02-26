/*
 * Source.h
 *
 *  Created on: Jun 29, 2009
 *      Author: bh
 */

#ifndef SOURCE_H_
#define SOURCE_H_

#include <QDomElement>
#include <QColor>

#include "common.h"

class Source {

public:

	Source(GLuint texture, double depth);
	//    Source(Source *clone, double newdepth = MIN_DEPTH_LAYER);
	virtual ~Source();

	virtual void update() {
    	glBindTexture(GL_TEXTURE_2D, textureIndex);
    }

	bool operator==(Source s2){
		return ( id == s2.id );
	}

    void draw(bool withalpha = true, GLenum mode = GL_RENDER) const;
	void blend() const;

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
	inline QColor getColor() const {
		return texcolor;
	}
//	inline QColor getBlendColor() const {
//		return blendcolor;
//	}
	inline GLenum getBlendEquation() const {
		return blend_eq;
	}
	inline GLenum getBlendFuncSource() const {
		return source_blend;
	}
	inline GLenum getBlendFuncDestination() const {
		return destination_blend;
	}

	// sets
	inline void setAspectRatio(GLdouble ar) {
		aspectratio = ar;
	}
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
	inline void setColor(QColor c) {
		texcolor = c;
	}
//	inline void setBlendColor(QColor c) {
//		blendcolor = c;
//	}
	inline void setBlendFuncAndEquation(GLenum sfactor, GLenum dfactor, GLenum eq) {
		source_blend = sfactor;
		destination_blend = dfactor;
		blend_eq = eq;
	}
	void scaleBy(GLfloat fx, GLfloat fy);
	void setAlphaCoordinates(GLdouble x, GLdouble y, GLdouble max);
	void resetScale();


protected:

	// identity and properties
	GLuint id;
	QDomElement dom;
	bool active;

	// GL Stuff
	GLuint textureIndex;
	GLdouble x, y, z;
	GLdouble scalex, scaley;
	GLdouble alphax, alphay;
	GLdouble aspectratio;
	GLfloat texalpha;
	QColor texcolor;
//	QColor blendcolor;
	GLenum source_blend, destination_blend;
	GLenum blend_eq;

	// statics
	static GLuint lastid;

};

#endif /* SOURCE_H_ */
