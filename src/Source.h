/*
 * Source.h
 *
 *  Created on: Jun 29, 2009
 *      Author: bh
 */

#ifndef SOURCE_H_
#define SOURCE_H_

#include <set>
#include <QColor>
#include <QtCore/QMap>

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

protected:
	/*
	 *
	 */
	Source(GLuint texture, double depth);

public:
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
		CAPTURE_SOURCE
	} RTTI;
	static RTTI type;
	virtual RTTI rtti() const {
		return type;
	}

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

	typedef enum {
		NO_MASK,
		ROUNDCORNER_MASK,
		CIRCLE_MASK,
		GRADIENT_CIRCLE_MASK,
		GRADIENT_SQUARE_MASK,
		CUSTOM_MASK
	} maskType;
	void setMask(maskType t, GLuint texture = 0);
	int getMask() {
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

protected:

	// identity and properties
	GLuint id;
	QString name;
	bool active, culled, frameChanged;
	SourceList *clones;

	// GL Stuff
	GLuint textureIndex, maskTextureIndex, iconIndex;
	GLdouble x, y, z;
	GLdouble scalex, scaley;
	GLdouble alphax, alphay;
	GLdouble centerx, centery, rotangle, rothandle;
	GLdouble aspectratio;
	GLfloat texalpha;
	QColor texcolor;
	GLenum source_blend, destination_blend;
	GLenum blend_eq;

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

#endif /* SOURCE_H_ */
