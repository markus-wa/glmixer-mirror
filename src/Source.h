/*
 * Source.h
 *
 *  Created on: Jun 29, 2009
 *      Author: bh
 */

#ifndef SOURCE_H_
#define SOURCE_H_

#include <vector>
#include <QDomElement>
#include <QColor>
#include <QtCore/QMap>

#include "common.h"

class QtProperty;

class Source;
typedef std::vector<Source *> SourceList;

class Source {

public:

	Source(GLuint texture, double depth);
	virtual ~Source();
	bool operator==(Source s2){
		return ( id == s2.id );
	}

	typedef enum { SIMPLE_SOURCE = 0, CLONE_SOURCE, VIDEO_SOURCE, CAMERA_SOURCE, ALGORITHM_SOURCE, RENDERING_SOURCE } RTTI;
	static RTTI type;
	virtual RTTI rtti() const { return type; }

	/**
	 *  Rendering
	 */

	virtual void update() {
    	glBindTexture(GL_TEXTURE_2D, textureIndex);
    }
	void blend() const;
    void draw(bool withalpha = true, GLenum mode = GL_RENDER) const;
	// OpenGL access
	inline GLuint getTextureIndex() {
		return textureIndex;
	}

	/**
	 * Manipulation
	 */
	inline GLuint getId() const {
		return id;
	}
	inline bool isActive() const {
		return active;
	}
	inline void activate(bool flag) {
		active = flag;
	}
	inline QtProperty *getProperty() const {
		return property;
	}
	void setProperty(QtProperty *p);
	inline SourceList *getClones() const { return clones; }


	/**
	 *  Geometry and deformation
	 */
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
	inline GLdouble getDepth() const {
		return z;
	}
	inline GLdouble getScaleX() const {
		return scalex;
	}
	inline GLdouble getScaleY() const {
		return scaley;
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
	inline void setDepth(GLdouble v) {
		z = CLAMP(v, MIN_DEPTH_LAYER, MAX_DEPTH_LAYER);
	}
	inline void setScaleX(GLdouble v) {
		scalex = v;
	}
	inline void setScaleY(GLdouble v) {
		scaley = v;
	}
	void moveTo(GLdouble posx, GLdouble posy);
	void setScale(GLdouble sx, GLdouble sy);
	void scaleBy(GLfloat fx, GLfloat fy);
	void clampScale();
	void resetScale();


	inline bool isCulled() const {
		return culled;
	}
	void testCulling();


	/**
	 * Blending
	 */
	void setAlphaCoordinates(GLdouble x, GLdouble y, GLdouble max);

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
	inline GLenum getBlendEquation() const {
		return blend_eq;
	}
	inline GLenum getBlendFuncSource() const {
		return source_blend;
	}
	inline GLenum getBlendFuncDestination() const {
		return destination_blend;
	}
	inline void setBlendFuncAndEquation(GLenum sfactor, GLenum dfactor, GLenum eq) {
		source_blend = sfactor;
		destination_blend = dfactor;
		blend_eq = eq;
	}

	/**
	 * Coloring, image processing
	 */

	void setColor(QColor c);

	typedef enum { SHARPEN, BLUR, EDGE, EMBOSS } convolutionType;
	convolutionType convolution;

protected:

	// identity and properties
	GLuint id;
//	QDomElement dom;
	bool active, culled;
	SourceList *clones;

	// GL Stuff
	GLuint textureIndex, iconIndex;
	GLdouble x, y, z;
	GLdouble scalex, scaley;
	GLdouble alphax, alphay;
	GLdouble aspectratio;
	GLfloat texalpha;
	QColor texcolor;
	GLenum source_blend, destination_blend;
	GLenum blend_eq;

	// if should be set to GL_NEAREST
	bool pixelated;
	// apply the Luminance matrix
	bool greyscale;
	// if should inversion matrix on color table
	bool invertcolors;
	// if a color or processing has been changed, it needs to be updated
	bool needs_update;

	// id counter
	static GLuint lastid;

	// the root of the property tree used to display and alter the source informations
	QtProperty *property;
    QMap<QString, QtProperty *> idToProperty;

};

#endif /* SOURCE_H_ */
