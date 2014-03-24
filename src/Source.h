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
 *   Copyright 2009, 2012 Bruno Herbelin
 *
 */

#ifndef SOURCE_H_
#define SOURCE_H_

#include <set>
#include <QColor>
#include <QMap>
#include <QDataStream>
#include <QRectF>

#include "common.h"
#include "defines.h"

#ifdef FFGL
#include "FFGLPluginSourceStack.h"
#endif

class SourceConstructorException : public AllocationException {
public:
    virtual QString message() { return "Could not allocate source"; }
    void raise() const { throw *this; }
    Exception *clone() const { return new SourceConstructorException(*this); }
};

class QtProperty;
class QGLFramebufferObject;

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
        SVG_SOURCE,
        SHM_SOURCE,
        FFGL_SOURCE
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
    // bind the texture
    void bind() const;
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
    /*
     * unique ID of this source
     *
     */
    inline GLuint getId() const {
        return id;
    }
    /*
     * Source Name
     *
     */
    void setName(QString n);
    inline QString getName() const {
        return name;
    }

    // returns the list of clones of this source (used to delete them)
    inline SourceList *getClones() const {
        return clones;
    }
    inline bool isCloned() const {
        return clones->size() > 0;
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
    inline QRectF getTextureCoordinates() const {
        return textureCoordinates;
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
        v += 360.0;
        rotangle = ABS( v - (double)( (int) v / 360 ) * 360.0 );
    }
    void moveTo(GLdouble posx, GLdouble posy);

    inline void setTextureCoordinates(QRectF textureCoords) {
        textureCoordinates = textureCoords;
    }
    inline void resetTextureCoordinates() {
        textureCoordinates.setCoords(0.0, 0.0, 1.0, 1.0);
    }

    void setScale(GLdouble sx, GLdouble sy);
    void scaleBy(GLfloat fx, GLfloat fy);
    void clampScale();

    inline void setVerticalFlip(bool on) { flipVertical = on; }
    inline bool isVerticalFlip() const { return flipVertical; }

    inline void setFixedAspectRatio(bool on) { fixedAspectRatio = on; }
    inline bool isFixedAspectRatio() const { return fixedAspectRatio; }

    typedef enum { SCALE_CROP= 0, SCALE_FIT, SCALE_DEFORM, SCALE_PIXEL} scalingMode;
    void resetScale(scalingMode sm = SCALE_CROP);

    inline bool isCulled() const {
        return culled;
    }
    void testGeometryCulling();

    /**
     * standby
     */
    virtual void setStandby(bool on);
    inline bool isStandby() const {
        return standby;
    }

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

    void setMask(int maskType, GLuint texture = 0);
    int getMask() const {
        return (int) mask_type;
    }

    /**
     * Coloring, image processing
     */
    // set canvas color
    inline void setColor(QColor c){
        texcolor = c;
    }
    // Adjust brightness factor
    inline virtual void setBrightness(int b) {
         brightness  = GLfloat(b) / 100.f;
    }
    inline virtual int getBrightness() const {
        return (int)(brightness * 100.f);
    }
    // Adjust contrast factor
    inline virtual void setContrast(int c) {
         contrast  = GLfloat(c + 100) / 100.f;
    }
    inline virtual int getContrast() const {
        return (int)(contrast * 100.f) -100;
    }
    // Adjust saturation factor
    inline virtual void setSaturation(int s){
        saturation  = GLfloat(s + 100) / 100.f;
    }
    inline virtual int getSaturation() const {
        return (int)(saturation * 100.f) -100;
    }

    // Adjust hue shift factor
    inline void setHueShift(int h){
        hueShift = qBound(0.f, GLfloat(h) / 360.f, 1.f);
    }
    inline int getHueShift() const {
        return (int)(hueShift * 360.f);
    }
    // Adjust Luminance Threshold
    inline void setLuminanceThreshold(int l){
        luminanceThreshold = qBound(0, l, 100);
    }
    inline int getLuminanceThreshold() const {
        return luminanceThreshold;
    }
    // Adjust number of colors
    inline void setNumberOfColors(int n){
        numberOfColors = qBound(0, n, 256);
    }
    inline int getNumberOfColors() const {
        return numberOfColors;
    }
    // chroma keying
    inline void setChromaKey(bool on) {
        useChromaKey = on;
    }
    inline bool getChromaKey() const {
        return useChromaKey;
    }
    inline void setChromaKeyColor(QColor c) {
        chromaKeyColor = c;
    }
    inline QColor getChromaKeyColor() const {
        return chromaKeyColor;
    }
    inline void setChromaKeyTolerance(int t) {
        chromaKeyTolerance = qBound(0.f, GLfloat(t) / 100.f, 1.f);;
    }
    inline int getChromaKeyTolerance() const {
        return (int)(chromaKeyTolerance * 100.f);
    }

    // Adjust gamma and levels factors
    inline void setGamma(float g, float minI, float maxI, float minO, float maxO){
        gamma = g;
        gammaMinIn = qBound(0.f, minI, 1.f);
        gammaMaxIn = qBound(0.f, maxI, 1.f);
        gammaMinOut = qBound(0.f, minO, 1.f);
        gammaMaxOut = qBound(0.f, maxO, 1.f);
    }
    inline float getGamma() const {
        return gamma;
    }
    inline float getGammaMinInput() const {
        return gammaMinIn;
    }
    inline float getGammaMaxInput() const {
        return gammaMaxIn;
    }
    inline float getGammaMinOuput() const {
        return gammaMinOut;
    }
    inline float getGammaMaxOutput() const {
        return gammaMaxOut;
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
        INVERT_NONE = 0,
        INVERT_COLOR,
        INVERT_LUMINANCE
    } invertModeType;
    inline void setInvertMode(invertModeType i) {
        invertMode = qBound(INVERT_NONE, i, INVERT_LUMINANCE);
    }
    inline invertModeType getInvertMode() const {
        return invertMode;
    }

    // select a filter
    typedef enum {
        FILTER_NONE = 0,
        FILTER_BLUR_GAUSSIAN,
        FILTER_BLUR_MEAN,
        FILTER_SHARPEN,
        FILTER_SHARPEN_MORE,
        FILTER_EDGE_GAUSSIAN,
        FILTER_EDGE_LAPLACE,
        FILTER_EDGE_LAPLACE_2,
        FILTER_EMBOSS,
        FILTER_EMBOSS_EDGE,
        FILTER_EROSION_3X3,
        FILTER_EROSION_5X5,
        FILTER_EROSION_7X7,
        FILTER_DILATION_3X3,
        FILTER_DILATION_5X5,
        FILTER_DILATION_7X7,
        FILTER_CUSTOM_GLSL
    } filterType;

    inline void setFilter(filterType c) {
        filter = qBound(FILTER_NONE, c, FILTER_CUSTOM_GLSL);
    }

    inline filterType getFilter() const {
        return filter;
    }

    static QStringList getFilterNames();

    void importProperties(const Source *s, bool withGeometry = true);

    virtual int getFrameWidth() const { return 1; }
    virtual int getFrameHeight() const { return 1; }
    virtual double getFrameRate() const { return 0.0; }

    // stick the source
    inline void setModifiable(bool on) {
        modifiable = on;
    }
    inline bool isModifiable() const {
        return modifiable;
    }

#ifdef FFGL
    // freeframe gl plugin
    FFGLPluginSource *addFreeframeGLPlugin(QString filename = QString::null);
    FFGLPluginSourceStack *getFreeframeGLPluginStack();
    bool hasFreeframeGLPlugin();
    void clearFreeframeGLPlugin();
#endif

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

    // identity
    GLuint id;
    QString name;

    // flags for updating (or not)
    bool culled, standby, wasplaying, frameChanged;
    // properties and clone list
    bool modifiable, fixedAspectRatio;
    SourceList *clones;

    // GL Stuff
    GLuint textureIndex, maskTextureIndex;
    GLdouble x, y, z;
    GLdouble scalex, scaley;
    GLdouble alphax, alphay;
    GLdouble centerx, centery, rotangle;
    GLdouble aspectratio;
    GLfloat texalpha;
    QColor texcolor;
    GLenum source_blend, destination_blend;
    GLenum blend_eq;
    QRectF textureCoordinates;

    // some textures are inverted
    bool flipVertical;

    // if should be set to GL_NEAREST
    bool pixelated;
    // which filter to apply?
    filterType filter;
    invertModeType invertMode;
    // which mask to use ?
    int mask_type;
    // Brightness, contrast and saturation
    GLfloat brightness, contrast, saturation;
    // gamma and its levels
    GLfloat gamma, gammaMinIn, gammaMaxIn, gammaMinOut, gammaMaxOut;
    // color manipulation
    GLfloat hueShift, chromaKeyTolerance;
    int luminanceThreshold, numberOfColors;
    QColor chromaKeyColor;
    bool useChromaKey;

#ifdef FFGL
    // freeframe plugin
    FFGLPluginSourceStack _ffgl_plugins;
#endif
    // statics
    static GLuint lastid;

};

// read and write of properties into data stream
// (used for default source save and restore in preferences)
QDataStream &operator<<(QDataStream &, const Source *);
QDataStream &operator>>(QDataStream &, Source *);

#endif /* SOURCE_H_ */
