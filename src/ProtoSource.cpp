#include "ProtoSource.moc"
#include "common.h"

#include <QtGui>

QStringList ProtoSource::getFilterNames() {

    static QStringList enumNames = QStringList() << "None" << "Gaussian blur" << "Median blur"
              << "Sharpen" << "Sharpen more"<< "Smooth edge detect"
              << "Medium edge detect"<< "Hard edge detect"<<"Emboss"<<"Edge emboss"
              << "Erosion 3x3"<< "Erosion 5x5"<< "Erosion 7x7"
              << "Dilation 3x3"<< "Dilation 5x5"<< "Dilation 7x7" << "Custom";

    return enumNames;

}

ProtoSource::ProtoSource(QObject *parent) : QObject(parent),
    modifiable(true), fixedAspectRatio(false), x(0.0), y(0.0), z(MAX_DEPTH_LAYER),
    scalex(SOURCE_UNIT), scaley(SOURCE_UNIT), alphax(0.0), alphay(0.0),
    centerx(0.0), centery(0.0), rotangle(0.0), texalpha(1.0), pixelated(false),
    filter(FILTER_NONE), invertMode(INVERT_NONE), mask_type(0),
    brightness(0.f), contrast(1.f),	saturation(1.f),
    gamma(1.f), gammaMinIn(0.f), gammaMaxIn(1.f), gammaMinOut(0.f), gammaMaxOut(1.f),
    hueShift(0.f), chromaKeyTolerance(0.1f), luminanceThreshold(0), numberOfColors (0),
    useChromaKey(false)
{
    // default name
    name = QString("Source");

    texcolor = Qt::white;
    chromaKeyColor = Qt::green;
    source_blend = GL_SRC_ALPHA;
    destination_blend = GL_ONE;
    blend_eq = GL_FUNC_ADD;

    // default texture coordinates
    textureCoordinates.setCoords(0.0, 0.0, 1.0, 1.0);

}


void ProtoSource::_setName(QString n) {
    name = n;
    setObjectName(n);
}

void ProtoSource::_setX(double v) {
    x = v;
}

void ProtoSource::_setY(double v) {
    y = v;
}

void ProtoSource::_setPosition(double posx, double posy) {
    x = posx;
    y = posy;
//    qDebug() << "ProtoSource::_setPosition " << x << y;
}

void ProtoSource::_setRotationCenterX(double v) {
    centerx = v;
}

void ProtoSource::_setRotationCenterY(double v) {
    centery = v;
}

void ProtoSource::_setRotationAngle(double v) {
    v += 360.0;
    rotangle = ABS( v - (double)( (int) v / 360 ) * 360.0 );
}

void ProtoSource::_setScaleX(double v) {
    scalex = v;
}

void ProtoSource::_setScaleY(double v) {
    scaley = v;
}

void ProtoSource::_setScale(double sx, double sy) {
    scalex = sx;
    scaley = sy;
}

void ProtoSource::_setFixedAspectRatio(bool on) {
    fixedAspectRatio = on;
}

void ProtoSource::_setTextureCoordinates(QRectF textureCoords) {
    textureCoordinates = textureCoords;
}

void ProtoSource::_setAlphaCoordinates(double x, double y) {

    // set new alpha coordinates
    alphax = x;
    alphay = y;

    // Compute distance to the center
    // QUADRATIC
    double d = ((x * x) + (y * y)) / (SOURCE_UNIT * SOURCE_UNIT * CIRCLE_SIZE * CIRCLE_SIZE);

    // adjust alpha according to distance to center
    if (d < 1.0)
        texalpha = 1.0 - d;
    else
        texalpha = 0.0;
}

void ProtoSource::_setAlpha(double a) {

    texalpha = CLAMP(a, 0.0, 1.0);

    // compute new alpha coordinates to match this alpha
    double dx = 0, dy = 0;

    // special case when source at the center
    if (ABS(alphax) < EPSILON && ABS(alphay) < EPSILON)
        dy = 1.0;
    else { // general case ; compute direction of the alpha coordinates
        dx = alphax / sqrt(alphax * alphax + alphay * alphay);
        dy = alphay / sqrt(alphax * alphax + alphay * alphay);
    }

    double da = sqrt((1.0 - texalpha) * (SOURCE_UNIT * SOURCE_UNIT * CIRCLE_SIZE * CIRCLE_SIZE));

    // set new alpha coordinates
    alphax = dx * da;
    alphay = dy * da;

}

void ProtoSource::_setMask(int maskType) {
    mask_type = maskType;
}


void ProtoSource::_setColor(QColor c){
    texcolor = c;
}

void ProtoSource::_setBrightness(int b) {
     brightness  = double(b) / 100.0;
}

void ProtoSource::_setContrast(int c) {
     contrast  = double(c + 100) / 100.0;
}

void ProtoSource::_setSaturation(int s){
    saturation  = double(s + 100) / 100.0;
}

void ProtoSource::_setHueShift(int h){
    hueShift = CLAMP(double(h) / 360.0, 0.0, 1.0);
}

void ProtoSource::_setLuminanceThreshold(int l){
    luminanceThreshold = CLAMP(l, 0, 100);

}

void ProtoSource::_setNumberOfColors(int n){
    numberOfColors = CLAMP(n, 0, 256);
}

void ProtoSource::_setChromaKey(bool on) {
    useChromaKey = on;
}

void ProtoSource::_setChromaKeyColor(QColor c) {
    chromaKeyColor = c;
}

void ProtoSource::_setChromaKeyTolerance(int t) {
    chromaKeyTolerance = CLAMP( double(t) / 100.0, 0.0, 1.0);
}

void ProtoSource::_setGamma(double g, double minI, double maxI, double minO, double maxO){
    gamma = CLAMP(g, 0.001, 50.0);
    gammaMinIn = CLAMP(minI, 0.0, 1.0);
    gammaMaxIn = CLAMP(maxI, 0.0, 1.0);
    gammaMinOut = CLAMP(minO, 0.0, 1.0);
    gammaMaxOut = CLAMP(maxO, 0.0, 1.0);

//    qDebug() << "_setGamma " << gamma ;
}

void ProtoSource::_setPixelated(bool on) {
    pixelated = on;
}

void ProtoSource::_setModifiable(bool on) {
    modifiable = on;
}

void ProtoSource::_setBlendFunc(uint sfactor, uint dfactor) {
    source_blend = sfactor;
    destination_blend = dfactor;
}

void ProtoSource::_setBlendEquation(uint eq) {
    blend_eq = eq;
}

void ProtoSource::_setInvertMode(invertModeType i) {
    invertMode = CLAMP( i, INVERT_NONE, INVERT_LUMINANCE);
}

void ProtoSource::_setFilter(filterType c) {
    filter = CLAMP( c, FILTER_NONE, FILTER_CUSTOM_GLSL);
}

void ProtoSource::importProperties(const ProtoSource *source, bool withGeometry){

    destination_blend = source->destination_blend;
    blend_eq =  source->blend_eq;
    pixelated = source->pixelated;
    texcolor = source->texcolor;
    brightness = source->brightness;
    contrast = source->contrast;
    saturation = source->saturation;
    hueShift = source->hueShift;
    filter = source->filter;
    invertMode = source->invertMode;
    mask_type = source->mask_type;

    gamma = source->gamma;
    gammaMinIn = source->gammaMinIn;
    gammaMaxIn = source->gammaMaxIn;
    gammaMinOut = source->gammaMinOut;
    gammaMaxOut = source->gammaMaxOut;
    luminanceThreshold = source->luminanceThreshold;
    numberOfColors = source->numberOfColors;
    chromaKeyColor = source->chromaKeyColor;
    useChromaKey = source->useChromaKey;
    chromaKeyTolerance = source->chromaKeyTolerance;

    if (withGeometry) {
        x = source->x;
        y = source->y;
        centerx = source->centerx;
        centery = source->centery;
        rotangle = source->rotangle;
        scalex = source->scalex;
        scaley = source->scaley;
        modifiable = source->modifiable;
        fixedAspectRatio = source->fixedAspectRatio;

        _setAlpha(source->texalpha);
        textureCoordinates = source->textureCoordinates;
    }

}

QDataStream &operator<<(QDataStream &stream, const ProtoSource *source){

    if (!source) {
        stream << (qint32) 0; // null source marker
        return stream;
    } else {
        stream << (qint32) 1;
        // continue ...
    }

    stream  << source->getX()
            << source->getY()
            << source->getRotationCenterX()
            << source->getRotationCenterY()
            << source->getRotationAngle()
//			<< source->getScaleX()
//			<< source->getScaleY()
            << source->getAlpha()
            << (uint) source->getBlendFuncDestination()
            << (uint) source->getBlendEquation()
            << source->getTextureCoordinates()
            << source->getColor()
            << source->isPixelated()
            << (uint) source->getFilter()
            << (uint) source->getInvertMode()
            << source->getMask()
            << source->getBrightness()
            << source->getContrast()
            << source->getSaturation()
            << source->getGamma()
            << source->getGammaMinInput()
            << source->getGammaMaxInput()
            << source->getGammaMinOuput()
            << source->getGammaMaxOutput()
            << source->getHueShift()
            << source->getLuminanceThreshold()
            << source->getNumberOfColors()
            << source->getChromaKey()
            << source->getChromaKeyColor()
            << source->getChromaKeyTolerance()
            << source->isModifiable()
            << source->isFixedAspectRatio()
            << source->getName();

    return stream;
}

QDataStream &operator>>(QDataStream &stream, ProtoSource *source){

    qint32 nullMarker;
    stream >> nullMarker;
    if (!nullMarker || !source) {
        return stream; // null source
    }

    // Read and setup the source properties
    QString stringValue;
    uint uintValue;
    int intValue;
    double doubleValue;
    double floatValue, f2, f3, f4, f5;
    QColor colorValue;
    bool boolValue;
    QRectF rectValue;

    stream >> doubleValue; 	source->_setX(doubleValue);
    stream >> doubleValue; 	source->_setY(doubleValue);
    stream >> doubleValue; 	source->_setRotationCenterX(doubleValue);
    stream >> doubleValue; 	source->_setRotationCenterY(doubleValue);
    stream >> doubleValue; 	source->_setRotationAngle(doubleValue);
//	stream >> doubleValue; 	source->setScaleX(doubleValue);
//	stream >> doubleValue; 	source->setScaleY(doubleValue);
    stream >> floatValue; 	source->_setAlpha(floatValue);
    stream >> uintValue;	source->_setBlendFunc(GL_SRC_ALPHA, uintValue);
    stream >> uintValue;	source->_setBlendEquation(uintValue);
    stream >> rectValue;	source->_setTextureCoordinates(rectValue);
    stream >> colorValue;	source->_setColor(colorValue);
    stream >> boolValue;	source->_setPixelated(boolValue);
    stream >> uintValue;	source->_setFilter( (ProtoSource::filterType) uintValue);
    stream >> uintValue;	source->_setInvertMode( (ProtoSource::invertModeType) uintValue);
    stream >> intValue;		source->_setMask(intValue);
    stream >> intValue;		source->_setBrightness(intValue);
    stream >> intValue;		source->_setContrast(intValue);
    stream >> intValue;		source->_setSaturation(intValue);
    stream >> floatValue >> f2 >> f3 >> f4 >> f5; 	source->_setGamma(floatValue, f2, f3, f4, f5);
    stream >> intValue;		source->_setHueShift(intValue);
    stream >> intValue;		source->_setLuminanceThreshold(intValue);
    stream >> intValue;		source->_setNumberOfColors(intValue);
    stream >> boolValue;	source->_setChromaKey(boolValue);
    stream >> colorValue;	source->_setChromaKeyColor(colorValue);
    stream >> intValue;		source->_setChromaKeyTolerance(intValue);
    stream >> boolValue;	source->_setModifiable(boolValue);
    stream >> boolValue;	source->_setFixedAspectRatio(boolValue);
    stream >> stringValue;  source->_setName(stringValue);

    return stream;
}

