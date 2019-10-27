/*
 *  ProtoSource.h
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
 *   Copyright 2009, 2016 Bruno Herbelin
 *
 */

#ifndef PROTOSOURCE_H
#define PROTOSOURCE_H

#include <limits>

#include <QObject>
#include <QColor>
#include <QRect>
#include <QDomDocument>
#include <QVariant>
#include <QPair>


typedef QPair<QVariant, QVariant> QVariantPair;
#define S_ARG(before, after) QVariantPair(QVariant(before), QVariant(after))

/*
 * Arguments stored in history have to keep any type of value
 * A pointer to the member variable is given inside the QGenericArgument
 * which is used when the method is invoked.
 *
*/
class SourceArgument
{
    int intValue;
    uint uintValue;
    double doubleValue;
    bool boolValue;
    QRectF rectValue;
    QString stringValue;
    QColor colorValue;

    QVariant::Type type;

public:
    SourceArgument(QVariant v = QVariant());
    QVariant variant() const;
    QString string() const;
    QString typeName() const { return QString(QVariant::typeToName(type)); }
    QGenericArgument argument() const;

    bool operator == ( const SourceArgument & other ) const;
    bool operator != ( const SourceArgument & other ) const;
    SourceArgument & operator = (const SourceArgument & other );
};

QDebug operator << ( QDebug out, const SourceArgument & a );


/**
 * Common ancestor for all Sources.
 *
 * A ProtoSource contains all the attributes of a source.
 * It allows manipulating what is changed by the user.
 *
 * A ProtoSource cannot be rendered.
 */
class ProtoSource : public QObject
{
    Q_OBJECT

public:
    ProtoSource(QObject *parent = 0);

    inline QString getName() const {
        return name;
    }
    inline double getX() const {
        return x;
    }
    inline double getY() const {
        return y;
    }
    inline double getDepth() const {
        return z;
    }
    inline double getScaleX() const {
        return scalex;
    }
    inline double getScaleY() const {
        return scaley;
    }
    inline double getRotationCenterX() const {
        return centerx;
    }
    inline double getRotationCenterY() const {
        return centery;
    }
    inline double getRotationAngle() const {
        return rotangle;
    }
    inline QRectF getTextureCoordinates() const {
        return textureCoordinates;
    }
    inline bool isFixedAspectRatio() const {
        return fixedAspectRatio;
    }
    inline int getMask() const {
        return mask_type;
    }
    inline double getAlphaX() const {
        return alphax;
    }
    inline double getAlphaY() const {
        return alphay;
    }
    inline double getAlpha() const {
        return texalpha;
    }
    inline QColor getColor() const {
        return texcolor;
    }
    inline uint getBlendEquation() const {
        return blend_eq;
    }
    inline uint getBlendFuncSource() const {
        return source_blend;
    }
    inline uint getBlendFuncDestination() const {
        return destination_blend;
    }
    inline int getBrightness() const {
        return (int)(brightness * 100.0);
    }
    inline int getContrast() const {
        return (int)(contrast * 100.0) -100;
    }
    inline int getSaturation() const {
        return (int)(saturation * 100.0) -100;
    }
    inline int getHueShift() const {
        return (int)(hueShift * 360.0);
    }
    inline int getThreshold() const {
        return luminanceThreshold;
    }
    inline int getLumakey() const {
        return lumakeyThreshold;
    }
    inline int getPosterized() const {
        return numberOfColors;
    }
    inline bool getChromaKey() const {
        return useChromaKey;
    }
    inline QColor getChromaKeyColor() const {
        return chromaKeyColor;
    }
    inline int getChromaKeyTolerance() const {
        return (int)(chromaKeyTolerance * 100.0);
    }
    inline double getGamma() const {
        return gamma;
    }
    inline double getGammaRed() const {
        return gammaRed;
    }
    inline double getGammaGreen() const {
        return gammaGreen;
    }
    inline double getGammaBlue() const {
        return gammaBlue;
    }
    inline double getGammaMinInput() const {
        return gammaMinIn;
    }
    inline double getGammaMaxInput() const {
        return gammaMaxIn;
    }
    inline double getGammaMinOuput() const {
        return gammaMinOut;
    }
    inline double getGammaMaxOutput() const {
        return gammaMaxOut;
    }
    inline bool isPixelated() const {
        return pixelated;
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

    inline filterType getFilter() const {
        return filter;
    }

    static QStringList getFilterNames();

    typedef enum {
        INVERT_NONE = 0,
        INVERT_COLOR,
        INVERT_LUMINANCE
    } invertModeType;

    inline invertModeType getInvertMode() const {
        return invertMode;
    }


    // import all the properties of a source
    void importProperties(const ProtoSource *s, bool withGeometry = true);

    // get XML config
    QDomElement getConfiguration(QDomDocument &doc);
    // set XML config
    bool setConfiguration(QDomElement xmlconfig);

signals:

    void methodCalled(QString signature,
                      QVariantPair arg0 = QVariantPair(),
                      QVariantPair arg1 = QVariantPair(),
                      QVariantPair arg2 = QVariantPair(),
                      QVariantPair arg3 = QVariantPair(),
                      QVariantPair arg4 = QVariantPair(),
                      QVariantPair arg5 = QVariantPair(),
                      QVariantPair arg6 = QVariantPair() );

public:

    Q_INVOKABLE void _setName(QString n);
    Q_INVOKABLE void _setGeometry(double px, double py, double sx, double sy, double rx, double ry, double a);
    Q_INVOKABLE void _setFixedAspectRatio(bool on);
    Q_INVOKABLE void _setTextureCoordinates(QRectF textureCoords);
    Q_INVOKABLE void _setAlphaCoordinates(double ax, double ay);
    Q_INVOKABLE void _setAlpha(double a);
    Q_INVOKABLE void _setMask(int maskType);
    Q_INVOKABLE void _setColor(QColor c);
    Q_INVOKABLE void _setBrightness(int b = 0);
    Q_INVOKABLE void _setContrast(int c = 0);
    Q_INVOKABLE void _setSaturation(int s = 0);
    Q_INVOKABLE void _setHueShift(int h = 0);
    Q_INVOKABLE void _setThreshold(int l = 0);
    Q_INVOKABLE void _setLumakey(int l = 0);
    Q_INVOKABLE void _setPosterized(int n = 255);
    Q_INVOKABLE void _setChromaKey(bool on = false);
    Q_INVOKABLE void _setChromaKeyColor(QColor c);
    Q_INVOKABLE void _setChromaKeyTolerance(int t = 10);
    Q_INVOKABLE void _setGammaColor(double value = 1.0, double red = 1.0, double green = 1.0, double blue = 1.0);
    Q_INVOKABLE void _setGammaLevels(double minI = 0.0, double maxI = 1.0, double minO = 0.0, double maxO = 1.0);
    Q_INVOKABLE void _setPixelated(bool on = false);
    Q_INVOKABLE void _setBlending(uint sfactor, uint dfactor, uint eq);
    Q_INVOKABLE void _setInvertMode(invertModeType i = INVERT_NONE);
    Q_INVOKABLE void _setFilter(filterType c);

    // for type compatibility
    Q_INVOKABLE void _setPosition(double px, double py);
    Q_INVOKABLE void _setPositionX(double px);
    Q_INVOKABLE void _setPositionY(double py);
    Q_INVOKABLE void _setScale(double sx, double sy);
    Q_INVOKABLE void _setScaleX(double sx);
    Q_INVOKABLE void _setScaleY(double sy);
    Q_INVOKABLE void _setRotation(double a);
    Q_INVOKABLE void _setInvertMode(int i);
    Q_INVOKABLE void _setInvertMode(double i);
    Q_INVOKABLE void _setFilter(int c);
    Q_INVOKABLE void _setColor(int r, int g, int b);
    Q_INVOKABLE void _setChromaKeyColor(int r, int g, int b);
    Q_INVOKABLE void _setAlphaCoordinateX(double ax);
    Q_INVOKABLE void _setAlphaCoordinateY(double ay);
    Q_INVOKABLE void _setBrightness(double b);
    Q_INVOKABLE void _setContrast(double c);
    Q_INVOKABLE void _setSaturation(double s);
    Q_INVOKABLE void _setHueShift(double h);
    Q_INVOKABLE void _setThreshold(double t);
    Q_INVOKABLE void _setLumakey(double t);
    Q_INVOKABLE void _setInvertColor(bool i);
    Q_INVOKABLE void _setInvertLuminance(bool i);
    Q_INVOKABLE void _setInvertColor(double i);
    Q_INVOKABLE void _setInvertLuminance(double i);
    Q_INVOKABLE void _setGammaValue(double value);
    Q_INVOKABLE void _setGammaRed(double value);
    Q_INVOKABLE void _setGammaGreen(double value);
    Q_INVOKABLE void _setGammaBlue(double value);


    // for OSC commands
    Q_INVOKABLE void _setIncrementAlpha(double a);
    Q_INVOKABLE void _setIncrementAlphaCoordinateX(double ax);
    Q_INVOKABLE void _setIncrementAlphaCoordinateY(double ay);
    Q_INVOKABLE void _setIncrementPositionX(double px);
    Q_INVOKABLE void _setIncrementPositionY(double py);
    Q_INVOKABLE void _setIncrementRotation(double a);

protected:

    QString name;
    bool fixedAspectRatio;
    double x, y, z;
    double scalex, scaley;
    double alphax, alphay, alphaangle;
    double centerx, centery, rotangle;
    double texalpha;
    QColor texcolor;
    uint source_blend, destination_blend;
    uint blend_eq;
    QRectF textureCoordinates;
    // if should be set to GL_NEAREST
    bool pixelated;
    // which filter to apply?
    filterType filter;
    invertModeType invertMode;
    // which mask to use ?
    int mask_type;
    // Brightness, contrast and saturation
    double brightness, contrast, saturation;
    // gamma and its levels
    double gamma, gammaRed, gammaGreen, gammaBlue;
    double gammaMinIn, gammaMaxIn, gammaMinOut, gammaMaxOut;
    // color manipulation
    double hueShift, chromaKeyTolerance;
    int luminanceThreshold, lumakeyThreshold, numberOfColors;
    QColor chromaKeyColor;
    bool useChromaKey;
};


// read and write of properties into data stream
// (used for default source save and restore in preferences)
QDataStream &operator<<(QDataStream &, const ProtoSource *);
QDataStream &operator>>(QDataStream &, ProtoSource *);


#endif // PROTOSOURCE_H
