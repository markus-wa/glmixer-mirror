/*
 * Source.h
 *
 *  Created on: Jun 29, 2009
 *      Author: bh
 *
 *  This file is part of GLMixer.
 *
 *   GLMixer is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation, either version 3 of the License, or
 *   (at your option) any later version.
 *
 *   GLMixer is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with GLMixer.  If not, see <http://www.gnu.org/licenses/>.
 *
 *   Copyright 2009, 2010 Bruno Herbelin
 *
 */

#ifndef SOURCE_H_
#define SOURCE_H_

#include <set>
#include <QColor>
#include <QMap>
#include <QDataStream>

#include "common.h"

class QtProperty;

class Source;
typedef std::set<Source *> SourceList;

/**
 * Base class for every source mixed in GLMixer.
 *
 * A source is holding a texture index, all the geometric and mixing attributes, and the corresponding drawing methods.
 * Every sources shall be instanciated by the rendering manager; this is because the creation and manipulation of sources
 * requires an active opengl context; this is the task of the rendering manager to call source methods after having made
 * an opengl context current.
 *
 *
 */
class Source {

	friend class RenderingManager;

public:
	Source();
	virtual ~Source();

	bool operator==(Source s2) {
		return (id == s2.id);
	}

	// Run-Time Type Information
	typedef enum {
		SIMPLE_SOURCE = 0,
		CLONE_SOURCE,
		VIDEO_SOURCE,
		CAMERA_SOURCE,
		ALGORITHM_SOURCE,
		RENDERING_SOURCE,
		CAPTURE_SOURCE,
		MIX_SOURCE
	} RTTI;
	virtual RTTI rtti() const { return type; }
	virtual bool isPlayable() const { return playable; }
	virtual bool isPlaying() const { return false; }
	virtual void play(bool on) {}

	/**
	 *  Rendering
	 */
	// to be called in the OpenGL loop to bind the source texture before drawing
	// In subclasses of Source, the texture content is also updated
	virtual void update();
	// Request update explicitly (e.g. after changing a filter)
	inline void requestUpdate() {
		frameChanged = true;
	}
	// apply the blending (including mask)
	// to be called in the OpenGL loop before drawing if the source shall be blended
	void blend() const;
	// begin and end the section which applies the various effects (convolution, color tables, etc).
	void beginEffectsSection() const;
	void endEffectsSection() const;

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
	inline SourceList *getClones() const {
		return clones;
	}

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
	inline GLdouble getCenterX() const {
		return centerx;
	}
	inline GLdouble getCenterY() const {
		return centery;
	}
	inline GLdouble getRotationAngle() const {
		return rotangle;
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
	inline void setCenterX(GLdouble v) {
		centerx = v;
	}
	inline void setCenterY(GLdouble v) {
		centery = v;
	}
	inline void setRotationAngle(GLdouble v) {
		rotangle = v;
	}
	void moveTo(GLdouble posx, GLdouble posy);
	void setScale(GLdouble sx, GLdouble sy);
	void scaleBy(GLfloat fx, GLfloat fy);
	void clampScale();

	typedef enum { SCALE_CROP= 0, SCALE_FIT, SCALE_DEFORM, SCALE_PIXEL} scalingMode;
	void resetScale(scalingMode sm = SCALE_CROP);

	inline bool isCulled() const {
		return culled;
	}
	void testCulling();

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

	typedef enum {
		NO_MASK,
		ROUNDCORNER_MASK,
		CIRCLE_MASK,
		GRADIENT_CIRCLE_MASK,
		GRADIENT_SQUARE_MASK,
		CUSTOM_MASK
	} maskType;
	void setMask(maskType t, GLuint texture = 0);
	int getMask() const {
		return (int) mask_type;
	}

	/**
	 * Coloring, image processing
	 */
	// set canvas color
	void setColor(QColor c);
	// Adjust brightness factor
	inline virtual void setBrightness(int b) {
		brightness = b;
	}
	inline virtual int getBrightness() const {
		return brightness;
	}
	// Adjust contrast factor
	inline virtual void setContrast(int c) {
		contrast = c;
	}
	inline virtual int getContrast() const {
		return contrast;
	}
	// Adjust saturation factor
	virtual void setSaturation(int s);
	inline virtual int getSaturation() const {
		return saturation;
	}
	// display pixelated ?
	inline void setPixelated(bool on) {
		pixelated = on;
	}
	inline bool isPixelated() const {
		return pixelated;
	}

	// select a color table
	typedef enum {
		NO_COLORTABLE,
		COLOR_16_COLORTABLE,
		COLOR_8_COLORTABLE,
		COLOR_4_COLORTABLE,
		COLOR_2_COLORTABLE,
		INVERT_COLORTABLE
	} colorTableType;
	inline void setColorTable(colorTableType c) {
		colorTable = c;
	}
	inline colorTableType getColorTable() const {
		return colorTable;
	}

	// select a filter
	typedef enum {
		NO_CONVOLUTION,
		BLUR_CONVOLUTION,
		SHARPEN_CONVOLUTION,
		EMBOSS_CONVOLUTION,
		EDGE_CONVOLUTION
	} convolutionType;
	inline void setConvolution(convolutionType c) {
		convolution = c;
	}
	inline convolutionType getConvolution() const {
		return convolution;
	}

	void copyPropertiesFrom(const Source *s);

	virtual int getFrameWidth() const { return 0; }
	virtual int getFrameHeight() const { return 0; }

protected:
	/*
	 * Constructor ; only Rendering Manager is allowed
	 */
	Source(GLuint texture, double depth);
	/*
	 * also depth should only be modified by Rendering Manager
	 *
	 */
	void setDepth(GLdouble v);

	// RTTI
	static RTTI type;
	static bool playable;

	// identity and properties
	GLuint id;
	QString name;
	bool active, culled, frameChanged, cropped;
	SourceList *clones;

	// GL Stuff
	GLuint textureIndex, maskTextureIndex, iconIndex;
	GLdouble x, y, z;
	GLdouble scalex, scaley;
	GLdouble alphax, alphay;
	GLdouble centerx, centery, rotangle;
	GLdouble aspectratio;
	GLfloat texalpha;
	QColor texcolor;
	GLenum source_blend, destination_blend;
	GLenum blend_eq;
	GLfloat crop_start_x, crop_start_y, crop_end_x, crop_end_y;

	// if should be set to GL_NEAREST
	bool pixelated;
	// which convolution filter to apply?
	convolutionType convolution;
	// which color table to apply?
	colorTableType colorTable;
	// which mask to use ?
	maskType mask_type;
	// Brightness, contrast and saturation
	int brightness, contrast, saturation;
	GLfloat saturationMatrix[16];

	// statics
	static GLuint lastid;
	static bool imaging_extension;
};


QDataStream &operator<<(QDataStream &, const Source *);
QDataStream &operator>>(QDataStream &, Source *);

#endif /* SOURCE_H_ */
