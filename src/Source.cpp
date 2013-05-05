/*
 * Source.cpp
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

#include "SourceSet.h"
#include "ViewRenderWidget.h"
#include "OutputRenderWindow.h"
#include "RenderingManager.h"

#include <QtProperty>
#include <QtVariantPropertyManager>

GLuint Source::lastid = 1;
Source::RTTI Source::type = Source::SIMPLE_SOURCE;
bool Source::playable = false;

// Infective source just to get default parameters
Source::Source() :
			culled(false), standby(false), wasplaying(true), frameChanged(false), modifiable(true), fixedAspectRatio(false),
			clones(NULL), textureIndex(0), maskTextureIndex(0), x(0.0), y(0.0), z(MAX_DEPTH_LAYER),
			scalex(SOURCE_UNIT), scaley(SOURCE_UNIT), alphax(0.0), alphay(0.0),
			centerx(0.0), centery(0.0), rotangle(0.0), aspectratio(1.0), texalpha(1.0), flipVertical(false),
            pixelated(false), filter(FILTER_NONE), invertMode(INVERT_NONE), mask_type(0),
			brightness(0.f), contrast(1.f),	saturation(1.f),
			gamma(1.f), gammaMinIn(0.f), gammaMaxIn(1.f), gammaMinOut(0.f), gammaMaxOut(1.f),
			hueShift(0.f), chromaKeyTolerance(0.1f), luminanceThreshold(0), numberOfColors (0),
			useChromaKey(false)
{
	id = 0;

	texcolor = Qt::white;
	chromaKeyColor = Qt::green;
	source_blend = GL_SRC_ALPHA;
	destination_blend = GL_ONE;
	blend_eq = GL_FUNC_ADD;

	textureCoordinates.setCoords(0.0, 0.0, 1.0, 1.0);

	// default name
	name = QString("Source");
}

// the 'REAL' source constructor.
Source::Source(GLuint texture, double depth)
{
	*this = Source();

	// give it a unique identifier
	id = Source::lastid++;

	clones = new SourceList;
	CHECK_PTR_EXCEPTION(clones)

	textureIndex = texture;
	z = CLAMP(depth, MIN_DEPTH_LAYER, MAX_DEPTH_LAYER);
}

Source::~Source() {

//	if (clones)
//		delete clones;  // TODO: why not clean clone list ?
}

void Source::setName(QString n) {
	name = n;
}

void Source::testGeometryCulling() {

	int w = SOURCE_UNIT;
	int h = SOURCE_UNIT;

	if (OutputRenderWindow::getInstance()->getAspectRatio() > 1)
		w *= OutputRenderWindow::getInstance()->getAspectRatio();
	else
		h *= OutputRenderWindow::getInstance()->getAspectRatio();

	// if all coordinates of center are between viewport limits, it is obviously visible
	if (x > -w && x < w && y > -h && y < h)
		culled = false;
	else {
		// not obviously visible
		// but it might still be parly visible if the distance from the center to the borders is less than the width
		int d = sqrt( scalex*scalex + scaley*scaley);
		if ((x + d < -w) || (x - d > w))
			culled = true;
		else if ((y + d < -h) || (y - d > h))
			culled = true;
			else
				culled = false;
	}

}

void Source::setDepth(GLdouble v) {
	z = CLAMP(v, MIN_DEPTH_LAYER, MAX_DEPTH_LAYER);
}

void Source::moveTo(GLdouble posx, GLdouble posy) {
	x = posx;
	y = posy;

	// test for culling
	testGeometryCulling();
}

void Source::setScale(GLdouble sx, GLdouble sy) {
	scalex = sx;
	scaley = sy;

	// test for culling
	testGeometryCulling();
}

void Source::scaleBy(float fx, float fy) {
	scalex *= fx;
	scaley *= fy;

	// test for culling
	testGeometryCulling();
}

void Source::clampScale() {

	scalex = (scalex > 0 ? 1.0 : -1.0) * CLAMP( ABS(scalex), MIN_SCALE, MAX_SCALE);
	scaley = (scaley > 0 ? 1.0 : -1.0) * CLAMP( ABS(scaley), MIN_SCALE, MAX_SCALE);

	// test for culling
	testGeometryCulling();
}

void Source::setAlphaCoordinates(double x, double y) {

	// set new alpha coordinates
	alphax = x;
	alphay = y;

	// TODO : configure the mixing to be linear or quadratic , or with a custom curve ?
	// Compute distance to the center
	GLdouble d = ((x * x) + (y * y)) / (SOURCE_UNIT * SOURCE_UNIT * CIRCLE_SIZE * CIRCLE_SIZE); // QUADRATIC

	// adjust alpha according to distance to center
	if (d < 1.0)
		texalpha = 1.0 - d;
	else
		texalpha = 0.0;

}

void Source::setStandby(bool on) {

	bool wasstandby = standby;
	// set the source to stanby if there is no clone relying on its update
	standby = on && !isCloned();

	if (isPlayable() && wasstandby != standby)
	{
		if (standby)
		{
			wasplaying = isPlaying();
			play(false);
		}
		else
			play(wasplaying);
	}

}

void Source::setAlpha(GLfloat a) {

	texalpha = CLAMP(a, 0.0, 1.0);

	// compute new alpha coordinates to match this alpha
	GLdouble dx = 0, dy = 0;

	// special case when source at the center
	if (ABS(alphax) < EPSILON && ABS(alphay) < EPSILON)
		dy = 1.0;
	else { // general case ; compute direction of the alpha coordinates
		dx = alphax / sqrt(alphax * alphax + alphay * alphay);
		dy = alphay / sqrt(alphax * alphax + alphay * alphay);
	}

	GLdouble da = sqrt((1.0 - texalpha) * (SOURCE_UNIT * SOURCE_UNIT * CIRCLE_SIZE * CIRCLE_SIZE));

	// set new alpha coordinates
	alphax = dx * da;
	alphay = dy * da;

}

void Source::resetScale(scalingMode sm) {

	scalex = SOURCE_UNIT;
	scaley = SOURCE_UNIT;
	float renderingAspectRatio = OutputRenderWindow::getInstance()->getAspectRatio();

	switch (sm) {
	case Source::SCALE_PIXEL:
		{
			double U = 1.0;
			if (RenderingManager::getInstance()->getFrameBufferAspectRatio() > 1.0 )
				U = (double) RenderingManager::getInstance()->getFrameBufferHeight();
			else
				U = (double) RenderingManager::getInstance()->getFrameBufferWidth();
			scalex *=  (double) getFrameWidth() / U;
			scaley *=  (double) getFrameHeight() / U;
		}
		break;
	case Source::SCALE_FIT:
		// keep original aspect ratio
		scalex  *= aspectratio;
		// if it is too large, scale it
		if (aspectratio > renderingAspectRatio) {
			scalex *= renderingAspectRatio / aspectratio;
			scaley *= renderingAspectRatio / aspectratio;
		}
		break;
	case Source::SCALE_DEFORM:
		// alter aspect ratio to match the rendering area
		scalex  *= renderingAspectRatio;
		break;
	default:
	case Source::SCALE_CROP:
		// keep original aspect ratio
		scalex  *= aspectratio;
		// if it is not large enough, scale it
		if (aspectratio < renderingAspectRatio) {
			scalex *= renderingAspectRatio / aspectratio;
			scaley *= renderingAspectRatio / aspectratio;
		}
		break;
	}

	scaley *= flipVertical ? -1.0 : 1.0;

	// test for culling
	testGeometryCulling();
}



void Source::draw(bool withalpha, GLenum mode) const
{
	// set id in select mode, avoid texturing if not rendering.
	if (mode == GL_SELECT) {
		glLoadName(id);
		glCallList(ViewRenderWidget::quad_texured);
	}
	else {
		// set transparency and color
		if (!standby) {
			glColor4f(texcolor.redF(), texcolor.greenF(), texcolor.blueF(),
					withalpha ? texalpha : 1.0);

			// texture coordinate changes
			ViewRenderWidget::texc[0] = textureCoordinates.left();
			ViewRenderWidget::texc[1] = textureCoordinates.top();
			ViewRenderWidget::texc[2] = textureCoordinates.right();
			ViewRenderWidget::texc[3] = textureCoordinates.top();
			ViewRenderWidget::texc[4] = textureCoordinates.right();
			ViewRenderWidget::texc[5] = textureCoordinates.bottom();
			ViewRenderWidget::texc[6] = textureCoordinates.left();
			ViewRenderWidget::texc[7] = textureCoordinates.bottom();
		}
		else
			glColor4f(0.0, 0.0, 0.0, 1.0);

	    glDrawArrays(GL_QUADS, 0, 4);
	}
}


void Source::update() {

 	glBindTexture(GL_TEXTURE_2D, textureIndex);

	if (pixelated) {
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	} else {
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	}
}


void Source::beginEffectsSection() const {

	ViewRenderWidget::program->setUniformValue("gamma", gamma);
	//             gamma levels : minInput, maxInput, minOutput, maxOutput:
	ViewRenderWidget::program->setUniformValue("levels", gammaMinIn, gammaMaxIn, gammaMinOut, gammaMaxOut);

	ViewRenderWidget::program->setUniformValue("contrast", contrast);
	ViewRenderWidget::program->setUniformValue("brightness", brightness);
	ViewRenderWidget::program->setUniformValue("saturation", saturation);
	ViewRenderWidget::program->setUniformValue("hueshift", hueShift);

	ViewRenderWidget::program->setUniformValue("invertMode", (GLint) invertMode);
	ViewRenderWidget::program->setUniformValue("nbColors", (GLint) numberOfColors);

	if (luminanceThreshold > 0 )
		ViewRenderWidget::program->setUniformValue("threshold", (GLfloat) luminanceThreshold / 100.f);
	else
		ViewRenderWidget::program->setUniformValue("threshold", -1.f);

	if (useChromaKey) {
		ViewRenderWidget::program->setUniformValue("chromakey", chromaKeyColor.hueF(), chromaKeyColor.saturationF(), chromaKeyColor.lightnessF(), 1.f );
		ViewRenderWidget::program->setUniformValue("chromadelta", chromaKeyTolerance);
	} else
		ViewRenderWidget::program->setUniformValue("chromakey", 0.f, 0.f, 0.f, 0.f );

	if (ViewRenderWidget::disableFiltering)
		return;

	// else enabled filtering
	ViewRenderWidget::program->setUniformValue("filter", (GLint) filter);
    ViewRenderWidget::program->setUniformValue("filter_step", 1.f / (float) getFrameWidth(), 1.f / (float) getFrameHeight());
    ViewRenderWidget::program->setUniformValue("filter_kernel", ViewRenderWidget::filter_kernel[filter]);

}

void Source::endEffectsSection() const {
//
//	if (pixelated) {
//		glActiveTexture(GL_TEXTURE0);
//    	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
//    	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
//	}

	// disable the mask
	glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, ViewRenderWidget::mask_textures[0]);
	glActiveTexture(GL_TEXTURE0);

//	// reset the blending
//	glBlendEquation(GL_FUNC_ADD);
//	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

}

void Source::blend() const {

//	glBlendEquation(blend_eq);
	glBlendEquationSeparate(blend_eq, GL_MAX);

//	glBlendFunc(source_blend, destination_blend);
	glBlendFuncSeparate(source_blend, destination_blend, GL_ONE_MINUS_SRC_ALPHA, GL_ZERO);


	// activate texture 1 ; double texturing of the mask
	glActiveTexture(GL_TEXTURE1);
	// select and enable the texture corresponding to the mask
	glBindTexture(GL_TEXTURE_2D, maskTextureIndex);

	// back to texture 0 for the following // not needed
//		glActiveTexture(GL_TEXTURE0);


}

void Source::setMask(int maskType, GLuint texture) {

    mask_type = maskType;

    if (texture != 0)
        maskTextureIndex = texture;
    else
		maskTextureIndex = ViewRenderWidget::mask_textures[mask_type];

}


QDataStream &operator<<(QDataStream &stream, const Source *source){

    if (!source) {
    	stream << (qint32) 0; // null source marker
        return stream;
    } else {
        stream << (qint32) 1;
        // continue ...
    }

	stream  << source->getX()
	    	<< source->getY()
			<< source->getCenterX()
			<< source->getCenterY()
			<< source->getRotationAngle()
//			<< source->getScaleX()
//			<< source->getScaleY()
			<< source->getAlpha()
			<< (unsigned int) source->getBlendFuncDestination()
			<< (unsigned int) source->getBlendEquation()
			<< source->getTextureCoordinates()
			<< source->getColor()
			<< source->isPixelated()
			<< (unsigned int) source->getFilter()
			<< (unsigned int) source->getInvertMode()
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

QDataStream &operator>>(QDataStream &stream, Source *source){

    qint32 nullMarker;
    stream >> nullMarker;
    if (!nullMarker || !source) {
        return stream; // null source
    }

	// Read and setup the source properties
	QString stringValue;
	unsigned int uintValue;
	int intValue;
	double doubleValue;
	float floatValue, f2, f3, f4, f5;
	QColor colorValue;
	bool boolValue;
	QRectF rectValue;

	stream >> doubleValue; 	source->setX(doubleValue);
	stream >> doubleValue; 	source->setY(doubleValue);
	stream >> doubleValue; 	source->setCenterX(doubleValue);
	stream >> doubleValue; 	source->setCenterY(doubleValue);
	stream >> doubleValue; 	source->setRotationAngle(doubleValue);
//	stream >> doubleValue; 	source->setScaleX(doubleValue);
//	stream >> doubleValue; 	source->setScaleY(doubleValue);
	stream >> floatValue; 	source->setAlpha(floatValue);
	stream >> uintValue;	source->setBlendFunc(GL_SRC_ALPHA, (GLenum) uintValue);
	stream >> uintValue;	source->setBlendEquation(uintValue);
	stream >> rectValue;	source->setTextureCoordinates(rectValue);
	stream >> colorValue;	source->setColor(colorValue);
	stream >> boolValue;	source->setPixelated(boolValue);
	stream >> uintValue;	source->setFilter( (Source::filterType) uintValue);
	stream >> uintValue;	source->setInvertMode( (Source::invertModeType) uintValue);
    stream >> intValue;		source->setMask(intValue);
	stream >> intValue;		source->setBrightness(intValue);
	stream >> intValue;		source->setContrast(intValue);
	stream >> intValue;		source->setSaturation(intValue);
	stream >> floatValue >> f2 >> f3 >> f4 >> f5; 	source->setGamma(floatValue, f2, f3, f4, f5);
	stream >> intValue;		source->setHueShift(intValue);
	stream >> intValue;		source->setLuminanceThreshold(intValue);
	stream >> intValue;		source->setNumberOfColors(intValue);
	stream >> boolValue;	source->setChromaKey(boolValue);
	stream >> colorValue;	source->setChromaKeyColor(colorValue);
	stream >> intValue;		source->setChromaKeyTolerance(intValue);
	stream >> boolValue;	source->setModifiable(boolValue);
	stream >> boolValue;	source->setFixedAspectRatio(boolValue);
    stream >> stringValue;  source->setName(stringValue);

	return stream;
}


void Source::importProperties(const Source source, bool withGeometry){

	destination_blend = source.destination_blend;
	blend_eq =  source.blend_eq;
	pixelated = source.pixelated;
	texcolor = source.texcolor;
	brightness = source.brightness;
	contrast = source.contrast;
	saturation = source.saturation;
	hueShift = source.hueShift;
	filter = source.filter;
	invertMode = source.invertMode;
	setMask( source.mask_type );

	gamma = source.gamma;
	gammaMinIn = source.gammaMinIn;
	gammaMaxIn = source.gammaMaxIn;
	gammaMinOut = source.gammaMinOut;
	gammaMaxOut = source.gammaMaxOut;
	luminanceThreshold = source.luminanceThreshold;
	numberOfColors = source.numberOfColors;
	chromaKeyColor = source.chromaKeyColor;
	useChromaKey = source.useChromaKey;
	chromaKeyTolerance = source.chromaKeyTolerance;

	if (withGeometry) {
		x = source.x;
		y = source.y;
		centerx = source.centerx;
		centery = source.centery;
		rotangle = source.rotangle;
	//	scalex = source.scalex;
	//	scaley = source.scaley;
		modifiable = source.modifiable;
		fixedAspectRatio = source.fixedAspectRatio;

		setAlpha(source.texalpha);
		textureCoordinates = source.textureCoordinates;
		flipVertical = source.flipVertical;
	}
}


QString Source::getFilterName(filterType c) {

	static QStringList enumNames;
	enumNames << "None" << "Gaussian blur" << "Median blur" << "Sharpen" << "Sharpen more"<< "Smooth edge detect"
			  << "Medium edge detect"<< "Hard edge detect"<<"Emboss"<<"Edge emboss"
			  << "Erosion 3x3"<< "Erosion 5x5"<< "Erosion 7x7"
			  << "Dilation 3x3"<< "Dilation 5x5"<< "Dilation 7x7";

	return enumNames[int(c)];

}
