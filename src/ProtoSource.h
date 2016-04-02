#ifndef PROTOSOURCE_H
#define PROTOSOURCE_H

#include <QObject>
#include <QColor>
#include <QRect>

#include "defines.h"

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
    inline int getLuminanceThreshold() const {
        return luminanceThreshold;
    }
    inline int getNumberOfColors() const {
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
    inline bool isModifiable() const {
        return modifiable;
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

signals:

    void methodCalled(QString signature,
                      QVariantPair arg0 = QVariantPair(),
                      QVariantPair arg1 = QVariantPair(),
                      QVariantPair arg2 = QVariantPair(),
                      QVariantPair arg3 = QVariantPair(),
                      QVariantPair arg4 = QVariantPair() );

public:

    Q_INVOKABLE void _setName(QString n);
    Q_INVOKABLE void _setX(double v);
    Q_INVOKABLE void _setY(double v);
    Q_INVOKABLE void _setPosition(double, double);
    Q_INVOKABLE void _setRotationCenterX(double v);
    Q_INVOKABLE void _setRotationCenterY(double v);
    Q_INVOKABLE void _setRotationAngle(double v);
    Q_INVOKABLE void _setScaleX(double v);
    Q_INVOKABLE void _setScaleY(double v);
    Q_INVOKABLE void _setScale(double sx, double sy);
    Q_INVOKABLE void _setFixedAspectRatio(bool on);
    Q_INVOKABLE void _setTextureCoordinates(QRectF textureCoords);
    Q_INVOKABLE void _setAlphaCoordinates(double x, double y);
    Q_INVOKABLE void _setAlpha(double a);
    Q_INVOKABLE void _setMask(int maskType);
    Q_INVOKABLE void _setColor(QColor c);
    Q_INVOKABLE void _setBrightness(int b);
    Q_INVOKABLE void _setContrast(int c);
    Q_INVOKABLE void _setSaturation(int s);
    Q_INVOKABLE void _setHueShift(int h);
    Q_INVOKABLE void _setLuminanceThreshold(int l);
    Q_INVOKABLE void _setNumberOfColors(int n);
    Q_INVOKABLE void _setChromaKey(bool on);
    Q_INVOKABLE void _setChromaKeyColor(QColor c);
    Q_INVOKABLE void _setChromaKeyTolerance(int t);
    Q_INVOKABLE void _setGamma(double g, double minI, double maxI, double minO, double maxO);
    Q_INVOKABLE void _setPixelated(bool on);
    Q_INVOKABLE void _setModifiable(bool on);
    Q_INVOKABLE void _setBlendFunc(uint sfactor, uint dfactor);
    Q_INVOKABLE void _setBlendEquation(uint eq);
    Q_INVOKABLE void _setInvertMode(invertModeType i);
    Q_INVOKABLE void _setFilter(filterType c);

protected:

    QString name;
    bool modifiable, fixedAspectRatio;
    double x, y, z;
    double scalex, scaley;
    double alphax, alphay;
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
    double gamma, gammaMinIn, gammaMaxIn, gammaMinOut, gammaMaxOut;
    // color manipulation
    double hueShift, chromaKeyTolerance;
    int luminanceThreshold, numberOfColors;
    QColor chromaKeyColor;
    bool useChromaKey;
};


// read and write of properties into data stream
// (used for default source save and restore in preferences)
QDataStream &operator<<(QDataStream &, const ProtoSource *);
QDataStream &operator>>(QDataStream &, ProtoSource *);


#endif // PROTOSOURCE_H
