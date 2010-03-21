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

	friend class RenderingManager;

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
	// to be called in the OpenGL loop to bind the source texture before drawing
	// In subclasses of Source, the texture content is also updated
	virtual void update() {
    	glBindTexture(GL_TEXTURE_2D, textureIndex);
    }
	// to be called in the OpenGL loop before drawing if the source shall be blended
	void startBlendingSection() const;
	void endBlendingSection() const;

	// to be called in the OpenGL loop to draw this source
    void draw(bool withalpha = true, GLenum mode = GL_RENDER) const;
	// OpenGL access to the texture index
	inline GLuint getTextureIndex() {
		return textureIndex;
	}
	// return true if this source is activated (shown as the current with a border)
	inline bool isActive() const {
		return active;
	}
	// sets if this source is active or not
	inline void activate(bool flag) {
		active = flag;
	}

	/**
	 * Manipulation
	 */
	// return the unique ID of this source
	inline GLuint getId() const {
		return id;
	}
	inline QString getName() const {
		return name;
	}
	void setName(QString n);

	// returns the list of clones of this source (used to delete them)
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

protected:
	// special case for depth; should only be modified by Rendering Manager
	void setDepth(GLdouble v);

public:

	/**
	 * Blending
	 */
	void setAlphaCoordinates(GLdouble x, GLdouble y);
	void setAlpha(GLfloat a);
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
	inline void setBlendFunc(GLenum sfactor, GLenum dfactor) {
		source_blend = sfactor;
		destination_blend = dfactor;
	}
	inline void setBlendEquation(GLenum eq) {
		blend_eq = eq;
	}
	/**
	 * Coloring, image processing
	 */

	void setColor(QColor c);

	inline void setGreyscale(bool on) { greyscale = on;}
	inline bool isGreyscale() const { return greyscale; }
	inline void setInvertcolors(bool on) { invertcolors = on;}
	inline bool isInvertcolors() const { return invertcolors; }

	typedef enum { NONE, SHARPEN, BLUR, EDGE, EMBOSS } convolutionType;
	inline void setConvolution( convolutionType c) { convolution = c; }
	inline convolutionType getConvolution() const { return convolution; }

protected:

	// identity and properties
	GLuint id;
	QString name;
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
	// which convolution filter to apply?
	convolutionType convolution;
	// if a color or processing has been changed, it needs to be updated
	bool needs_update;

	// id counter
	static GLuint lastid;

//	// the root of the property tree used to display and alter the source informations
//	QtProperty *property;
//    QMap<QString, QtProperty *> idToProperty;

};

#endif /* SOURCE_H_ */
