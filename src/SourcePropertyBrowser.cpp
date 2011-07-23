/*
 * SourcePropertyBrowser.cpp
 *
 *  Created on: Mar 14, 2010
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

#include <SourcePropertyBrowser.moc>

#include <QVBoxLayout>

#include <QPair>
#include <QtTreePropertyBrowser>
#include <QtButtonPropertyBrowser>
#include <QtGroupBoxPropertyBrowser>
#include <QtDoublePropertyManager>
#include <QtIntPropertyManager>
#include <QtStringPropertyManager>
#include <QtColorPropertyManager>
#include <QtRectFPropertyManager>
#include <QtPointFPropertyManager>
#include <QtSizePropertyManager>
#include <QtEnumPropertyManager>
#include <QtBoolPropertyManager>
#include <QtTimePropertyManager>
#include <QtDoubleSpinBoxFactory>
#include <QtCheckBoxFactory>
#include <QtSpinBoxFactory>
#include <QtSliderFactory>
#include <QtLineEditFactory>
#include <QtEnumEditorFactory>
#include <QtCheckBoxFactory>
#include <QtTimeEditFactory>
#include <QtColorEditorFactory>
#include <QFileInfo>

#include "RenderingManager.h"
#include "ViewRenderWidget.h"
#include "RenderingSource.h"
#include "AlgorithmSource.h"
#include "CaptureSource.h"
#include "CloneSource.h"
#include "VideoSource.h"
#ifdef OPEN_CV
#include "OpencvSource.h"
#endif


QMap<GLenum, int> glblendToEnum;
QMap<int, GLenum> enumToGlblend;
QMap<GLenum, int> glequationToEnum;
QMap<int, GLenum> enumToGlequation;
QMap<int, QPair<int, int> > presetBlending;

QString getSizeString(float num);

SourcePropertyBrowser::SourcePropertyBrowser(QWidget *parent) : QWidget (parent), root(0), currentItem(0) {

	layout = new QVBoxLayout(this);
	layout->setObjectName(QString::fromUtf8("verticalLayout"));

	// property Group Box
	propertyGroupEditor = new QtGroupBoxPropertyBrowser(this);
	propertyGroupEditor->setObjectName(QString::fromUtf8("Property Groups"));
	propertyGroupEditor->setContextMenuPolicy(Qt::CustomContextMenu);
	connect(propertyGroupEditor, SIGNAL(customContextMenuRequested(const QPoint &)), this, SLOT(ctxMenuGroup(const QPoint &)));

	propertyGroupArea = new QScrollArea(this);
	propertyGroupArea->setWidgetResizable(true);
	propertyGroupArea->setWidget(propertyGroupEditor);
	propertyGroupArea->setVisible(false);

	// property TREE
    propertyTreeEditor = new QtTreePropertyBrowser(this);
    propertyTreeEditor->setObjectName(QString::fromUtf8("Property Tree"));
    propertyTreeEditor->setContextMenuPolicy(Qt::CustomContextMenu);
    propertyTreeEditor->setResizeMode(QtTreePropertyBrowser::Interactive);
	connect(propertyTreeEditor, SIGNAL(customContextMenuRequested(const QPoint &)), this, SLOT(ctxMenuTree(const QPoint &)));

	// TODO ; read default from application config
	propertyTreeEditor->setVisible(true);
    layout->addWidget(propertyTreeEditor);

    // create the property managers for every possible types
    groupManager = new QtGroupPropertyManager(this);
    doubleManager = new QtDoublePropertyManager(this);
    intManager = new QtIntPropertyManager(this);
    stringManager = new QtStringPropertyManager(this);
    colorManager = new QtColorPropertyManager(this);
    pointManager = new QtPointFPropertyManager(this);
    enumManager = new QtEnumPropertyManager(this);
    boolManager = new QtBoolPropertyManager(this);
    rectManager = new QtRectFPropertyManager(this);
    // special managers which are not associated with a factory (i.e non editable)
    infoManager = new QtStringPropertyManager(this);
    sizeManager = new QtSizePropertyManager(this);

    // use the managers to create the property tree
	createPropertyTree();

    // specify the factory for each of the property managers
    QtDoubleSpinBoxFactory *doubleSpinBoxFactory = new QtDoubleSpinBoxFactory(this);    // for double
    QtCheckBoxFactory *checkBoxFactory = new QtCheckBoxFactory(this);					// for bool
    QtSpinBoxFactory *spinBoxFactory = new QtSpinBoxFactory(this);                      // for int
    QtSliderFactory *sliderFactory = new QtSliderFactory(this);							// for int
    QtLineEditFactory *lineEditFactory = new QtLineEditFactory(this);					// for text
    QtEnumEditorFactory *comboBoxFactory = new QtEnumEditorFactory(this);				// for enum
    QtColorEditorFactory *colorFactory = new QtColorEditorFactory(this);				// for color

    propertyTreeEditor->setFactoryForManager(doubleManager, doubleSpinBoxFactory);
    propertyTreeEditor->setFactoryForManager(intManager, spinBoxFactory);
    propertyTreeEditor->setFactoryForManager(stringManager, lineEditFactory);
    propertyTreeEditor->setFactoryForManager(colorManager, colorFactory);
    propertyTreeEditor->setFactoryForManager(pointManager->subDoublePropertyManager(), doubleSpinBoxFactory);
    propertyTreeEditor->setFactoryForManager(enumManager, comboBoxFactory);
    propertyTreeEditor->setFactoryForManager(boolManager, checkBoxFactory);
    propertyTreeEditor->setFactoryForManager(rectManager->subDoublePropertyManager(), doubleSpinBoxFactory);

    propertyGroupEditor->setFactoryForManager(doubleManager, doubleSpinBoxFactory);
    propertyGroupEditor->setFactoryForManager(intManager, sliderFactory);
    propertyGroupEditor->setFactoryForManager(stringManager, lineEditFactory);
    propertyGroupEditor->setFactoryForManager(colorManager, colorFactory);
    propertyGroupEditor->setFactoryForManager(pointManager->subDoublePropertyManager(), doubleSpinBoxFactory);
    propertyGroupEditor->setFactoryForManager(enumManager, comboBoxFactory);
    propertyGroupEditor->setFactoryForManager(boolManager, checkBoxFactory);
    propertyGroupEditor->setFactoryForManager(rectManager->subDoublePropertyManager(), doubleSpinBoxFactory);

}

QString aspectRatioToString(double ar)
{
	if ( ABS(ar - 1.0 ) < EPSILON )
		return QString("1:1");
	else if ( ABS(ar - (5.0 / 4.0) ) < EPSILON )
		return QString("5:4");
	else if ( ABS(ar - (4.0 / 3.0) ) < EPSILON )
		return QString("4:3");
	else if ( ABS(ar - (16.0 / 9.0) ) < EPSILON )
		return QString("16:9");
	else  if ( ABS(ar - (3.0 / 2.0) ) < EPSILON )
		return QString("3:2");
	else if ( ABS(ar - (16.0 / 10.0) ) < EPSILON )
		return QString("16:10");
	else
		return QString::number(ar);

}


void SourcePropertyBrowser::createPropertyTree(){

	// we will need these for the correspondance between comboBox and GLenums:
	glblendToEnum[GL_ZERO] = 0; enumToGlblend[0] = GL_ZERO;
	glblendToEnum[GL_ONE] = 1; enumToGlblend[1] = GL_ONE;
	glblendToEnum[GL_SRC_COLOR] = 2; enumToGlblend[2] = GL_SRC_COLOR;
	glblendToEnum[GL_ONE_MINUS_SRC_COLOR] = 3; enumToGlblend[3] = GL_ONE_MINUS_SRC_COLOR;
	glblendToEnum[GL_DST_COLOR] = 4; enumToGlblend[4] = GL_DST_COLOR;
	glblendToEnum[GL_ONE_MINUS_DST_COLOR] = 5; enumToGlblend[5] = GL_ONE_MINUS_DST_COLOR;
	glblendToEnum[GL_SRC_ALPHA] = 6; enumToGlblend[6] = GL_SRC_ALPHA;
	glblendToEnum[GL_ONE_MINUS_SRC_ALPHA] = 7; enumToGlblend[7] = GL_ONE_MINUS_SRC_ALPHA;
	glblendToEnum[GL_DST_ALPHA] = 8; enumToGlblend[8] = GL_DST_ALPHA;
	glblendToEnum[GL_ONE_MINUS_DST_ALPHA] = 9; enumToGlblend[9] = GL_ONE_MINUS_DST_ALPHA;
	glequationToEnum[GL_FUNC_ADD] = 0; enumToGlequation[0] = GL_FUNC_ADD;
	glequationToEnum[GL_FUNC_SUBTRACT] = 1; enumToGlequation[1] = GL_FUNC_SUBTRACT;
	glequationToEnum[GL_FUNC_REVERSE_SUBTRACT] = 2; enumToGlequation[2] = GL_FUNC_REVERSE_SUBTRACT;
	glequationToEnum[GL_MIN] = 3; enumToGlequation[3] = GL_MIN;
	glequationToEnum[GL_MAX] = 4; enumToGlequation[4] = GL_MAX;
	// the comboBox presents of combinations
	presetBlending[1] = qMakePair( 1, 0 );
	presetBlending[2] = qMakePair( 1, 2 );
	presetBlending[3] = qMakePair( 8, 0 );
	presetBlending[4] = qMakePair( 8, 2 );
	presetBlending[5] = qMakePair( 7, 0 );

	QtProperty *property;

	// the top property holding all the source sub-properties
	root = groupManager->addProperty( QLatin1String("Source root") );

	// Name
	property = stringManager->addProperty( QLatin1String("Name") );
	idToProperty[property->propertyName()] = property;
	property->setToolTip("A name to identify the source");
	root->addSubProperty(property);

	// modifyable on/off
	QtProperty *modifyroperty = boolManager->addProperty("Modifiable");
	modifyroperty->setToolTip("Can you modify this source?");
	idToProperty[modifyroperty->propertyName()] = modifyroperty;
	root->addSubProperty(modifyroperty);

		// Position
		property = pointManager->addProperty("Position");
		idToProperty[property->propertyName()] = property;
		property->setToolTip("X and Y coordinates of the center");
		pointManager->subDoublePropertyManager()->setSingleStep(property->subProperties().first(), 0.1);
		pointManager->subDoublePropertyManager()->setSingleStep(property->subProperties().last(), 0.1);
		modifyroperty->addSubProperty(property);
		// Scale
		property = pointManager->addProperty("Scale");
		idToProperty[property->propertyName()] = property;
		property->setToolTip("Scaling factors on X and Y");
		pointManager->subDoublePropertyManager()->setSingleStep(property->subProperties()[0], 0.1);
		pointManager->subDoublePropertyManager()->setSingleStep(property->subProperties()[1], 0.1);
		modifyroperty->addSubProperty(property);
		// Rotation angle
		property = doubleManager->addProperty("Angle");
		property->setToolTip("Angle of rotation in degrees (counter clock wise)");
		idToProperty[property->propertyName()] = property;
		doubleManager->setRange(property, 0, 360);
		doubleManager->setSingleStep(property, 10.0);
		modifyroperty->addSubProperty(property);
		// Rotation center
	//	property = pointManager->addProperty("Rotation center");
	//	property->setToolTip("X and Y coordinates of the rotation center (relative to the center)");
	//	idToProperty[property->propertyName()] = property;
	//	pointManager->subDoublePropertyManager()->setSingleStep(property->subProperties().first(), 0.1);
	//	pointManager->subDoublePropertyManager()->setSingleStep(property->subProperties().last(), 0.1);
	//	root->addSubProperty(property);
		// Texture coordinates
		property = rectManager->addProperty("Crop");
		idToProperty[property->propertyName()] = property;
		property->setToolTip("Texture coordinates");
		rectManager->subDoublePropertyManager()->setSingleStep(property->subProperties()[0], 0.1);
		rectManager->subDoublePropertyManager()->setSingleStep(property->subProperties()[1], 0.1);
		modifyroperty->addSubProperty(property);
		// Depth
		property = doubleManager->addProperty("Depth");
		property->setToolTip("Depth of the layer");
		idToProperty[property->propertyName()] = property;
		doubleManager->setRange(property, MIN_DEPTH_LAYER, MAX_DEPTH_LAYER);
		modifyroperty->addSubProperty(property);
		// Alpha
		property = doubleManager->addProperty("Alpha");
		property->setToolTip("Opacity (0 = transparent)");
		idToProperty[property->propertyName()] = property;
		doubleManager->setRange(property, 0.0, 1.0);
		doubleManager->setSingleStep(property, 0.01);
		doubleManager->setDecimals(property, 4);
		modifyroperty->addSubProperty(property);

	// enum list of Destination blending func
	QtProperty *blendingItem = enumManager->addProperty("Blending");
	idToProperty[blendingItem->propertyName()] = blendingItem;
	blendingItem->setToolTip("How the colors are mixed with the sources in lower layers.");
	QStringList enumNames;
	enumNames << "Custom" << "Color mix" << "Inverse color mix" << "Layer color mix" << "Layer inverse color mix" << "Layer opacity";
	enumManager->setEnumNames(blendingItem, enumNames);
	// Custom Blending
	// enum list of Destination blending func
	property = enumManager->addProperty("Destination");
	idToProperty[property->propertyName()] = property;
	property->setToolTip("OpenGL blending function");
	enumNames.clear();
	enumNames << "Zero" << "One" << "Color" << "Invert color" << "Background color" << "Invert background color" << "Alpha" << "Invert alpha" << "Background Alpha" << "Invert background Alpha";
	enumManager->setEnumNames(property, enumNames);
	blendingItem->addSubProperty(property);
	// enum list of blending Equations
	property = enumManager->addProperty("Equation");
	idToProperty[property->propertyName()] = property;
	property->setToolTip("OpenGL blending equation");
	enumNames.clear();
	enumNames << "Add" << "Subtract" << "Reverse" << "Min" << "Max";
	enumManager->setEnumNames(property, enumNames);
	blendingItem->addSubProperty(property);
	// Confirm and add the blending item
	root->addSubProperty(blendingItem);
	// enum list of blending masks
	property = enumManager->addProperty("Mask");
	idToProperty[property->propertyName()] = property;
	property->setToolTip("Layer mask (where black is opaque)");
	enumNames.clear();
	// TODO implement selection of custom file mask
	enumNames << "None" <<"Rounded corners" <<  "Circle" << "Circular gradient" << "Square gradient" << "Left to right" << "Right to left" << "Top down" << "Bottom up"<< "Horizontal bar" << "Vertical bar" <<"Antialiasing";
	enumManager->setEnumNames(property, enumNames);
    QMap<int, QIcon> enumIcons;
    enumIcons[0] = QIcon();
    enumIcons[1] = QIcon(":/glmixer/textures/mask_roundcorner.png");
    enumIcons[2] = QIcon(":/glmixer/textures/mask_circle.png");
    enumIcons[3] = QIcon(":/glmixer/textures/mask_linear_circle.png");
    enumIcons[4] = QIcon(":/glmixer/textures/mask_linear_square.png");
    enumIcons[5] = QIcon(":/glmixer/textures/mask_linear_left.png");
    enumIcons[6] = QIcon(":/glmixer/textures/mask_linear_right.png");
    enumIcons[7] = QIcon(":/glmixer/textures/mask_linear_top.png");
    enumIcons[8] = QIcon(":/glmixer/textures/mask_linear_bottom.png");
    enumIcons[9] = QIcon(":/glmixer/textures/mask_linear_horizontal.png");
    enumIcons[10] = QIcon(":/glmixer/textures/mask_linear_vertical.png");
    enumIcons[11] = QIcon(":/glmixer/textures/mask_antialiasing.png");
//    enumIcons[9] = QIcon(":/glmixer/icons/fileopen.png");
    enumManager->setEnumIcons(property, enumIcons);
	root->addSubProperty(property);
	// Color
	property = colorManager->addProperty("Color");
	idToProperty[property->propertyName()] = property;
	property->setToolTip("Base tint of the source");
	root->addSubProperty(property);

	// Pixelated on/off
	property = boolManager->addProperty("Pixelated");
	property->setToolTip("Do not smooth pixels");
	idToProperty[property->propertyName()] = property;
	root->addSubProperty(property);

	// Filtered on/off
	QtProperty *filter = boolManager->addProperty("Filtered");
	filter->setToolTip("Use GLSL filters");
	idToProperty[filter->propertyName()] = filter;
	root->addSubProperty(filter);
	{
		// Brightness
		property = intManager->addProperty( QLatin1String("Brightness") );
		property->setToolTip("Brightness (from black to white)");
		idToProperty[property->propertyName()] = property;
		intManager->setRange(property, -100, 100);
		intManager->setSingleStep(property, 10);
		filter->addSubProperty(property);
		// Contrast
		property = intManager->addProperty( QLatin1String("Contrast") );
		property->setToolTip("Contrast (from uniform color to high deviation)");
		idToProperty[property->propertyName()] = property;
		intManager->setRange(property, -100, 100);
		intManager->setSingleStep(property, 10);
		filter->addSubProperty(property);
		// Saturation
		property = intManager->addProperty( QLatin1String("Saturation") );
		property->setToolTip("Saturation (from greyscale to enhanced colors)");
		idToProperty[property->propertyName()] = property;
		intManager->setRange(property, -100, 100);
		intManager->setSingleStep(property, 10);
		filter->addSubProperty(property);
		// hue
		property = intManager->addProperty( QLatin1String("Hue shift") );
		property->setToolTip("Hue shift (circular shift of color Hue)");
		idToProperty[property->propertyName()] = property;
		intManager->setRange(property, 0, 360);
		intManager->setSingleStep(property, 36);
		filter->addSubProperty(property);
		// threshold
		property = intManager->addProperty( QLatin1String("Threshold") );
		property->setToolTip("Luminance threshold (convert to black & white, keeping colors above the threshold, 0 to keep original)");
		idToProperty[property->propertyName()] = property;
		intManager->setRange(property, 0, 100);
		intManager->setSingleStep(property, 10);
		filter->addSubProperty(property);
		// nb colors
		property = intManager->addProperty( QLatin1String("Posterize") );
		property->setToolTip("Posterize (reduce number of colors, 0 to keep original)");
		idToProperty[property->propertyName()] = property;
		intManager->setRange(property, 0, 256);
		intManager->setSingleStep(property, 1);
		filter->addSubProperty(property);
		// enum list of inversion types
		property = enumManager->addProperty("Color inversion");
		idToProperty[property->propertyName()] = property;
		property->setToolTip("Invert colors or luminance");
		enumNames.clear();
		enumNames << "None" << "RGB invert" << "Luminance invert";
		enumManager->setEnumNames(property, enumNames);
		filter->addSubProperty(property);
		// enum list of filters
		property = enumManager->addProperty("Filter");
		idToProperty[property->propertyName()] = property;
		property->setToolTip("Imaging filters (convolutions & morphological operators)");
		enumNames.clear();
		enumNames << "None" << "Gaussian blur" << "Median blur" << "Sharpen" << "Sharpen more"<< "Smooth edge detect"
				  << "Medium edge detect"<< "Hard edge detect"<<"Emboss"<<"Edge emboss"
				  << "Erosion 3x3"<< "Erosion 5x5"<< "Erosion 7x7"
				  << "Dilation 3x3"<< "Dilation 5x5"<< "Dilation 7x7";
		enumManager->setEnumNames(property, enumNames);
		filter->addSubProperty(property);
		// Chroma key on/off
		QtProperty *chroma = boolManager->addProperty("Chroma key");
		chroma->setToolTip("Enables chroma-keying (removes a key color).");
		idToProperty[chroma->propertyName()] = chroma;
		filter->addSubProperty(chroma);
		// chroma key Color
		property = colorManager->addProperty("Key Color");
		idToProperty[property->propertyName()] = property;
		property->setToolTip("Color used for the chroma-keying.");
		chroma->addSubProperty(property);
		// threshold
		property = intManager->addProperty( QLatin1String("Key Tolerance") );
		property->setToolTip("Percentage of tolerance around the key color");
		idToProperty[property->propertyName()] = property;
		intManager->setRange(property, 0, 100);
		intManager->setSingleStep(property, 10);
		chroma->addSubProperty(property);
	}

	// Frames size
	QtProperty *fs = sizeManager->addProperty( QLatin1String("Frames size") );
	fs->setToolTip("Width & height of frames");
	fs->setItalics(true);
	idToProperty[fs->propertyName()] = fs;

	// AspectRatio
	QtProperty *ar = infoManager->addProperty("Aspect ratio");
	ar->setToolTip("Ratio of pixel dimensions of acquired frames");
	ar->setItalics(true);
	idToProperty[ar->propertyName()] = ar;

	// Frame rate
	QtProperty *fr = infoManager->addProperty( QLatin1String("Frame rate") );
	fr->setItalics(true);
	idToProperty[fr->propertyName()] = fr;

	rttiToProperty[Source::VIDEO_SOURCE] = groupManager->addProperty( QLatin1String("Media file"));

		// File Name
		property = infoManager->addProperty( QLatin1String("File name") );
		property->setItalics(true);
		idToProperty[property->propertyName()] = property;
		rttiToProperty[Source::VIDEO_SOURCE]->addSubProperty(property);
		// File size
		property = infoManager->addProperty( QLatin1String("File size") );
		property->setItalics(true);
		idToProperty[property->propertyName()] = property;
		rttiToProperty[Source::VIDEO_SOURCE]->addSubProperty(property);
		// Codec
		property = infoManager->addProperty( QLatin1String("Codec") );
		property->setItalics(true);
		idToProperty[property->propertyName()] = property;
		rttiToProperty[Source::VIDEO_SOURCE]->addSubProperty(property);
		// Pixel Format
		property = infoManager->addProperty( QLatin1String("Pixel format") );
		property->setItalics(true);
		idToProperty[property->propertyName()] = property;
		rttiToProperty[Source::VIDEO_SOURCE]->addSubProperty(property);
		// interlacing
		property = infoManager->addProperty( QLatin1String("Interlaced") );
		property->setItalics(true);
		idToProperty[property->propertyName()] = property;
		rttiToProperty[Source::VIDEO_SOURCE]->addSubProperty(property);

		// Frames size & aspect ratio
		rttiToProperty[Source::VIDEO_SOURCE]->addSubProperty(fs);
		rttiToProperty[Source::VIDEO_SOURCE]->addSubProperty(ar);

		// Frames size special case when power of two dimensions are generated
		property = sizeManager->addProperty( QLatin1String("Original size") );
		property->setItalics(true);
		idToProperty[property->propertyName()] = property;
		// Frame rate
		rttiToProperty[Source::VIDEO_SOURCE]->addSubProperty(fr);
		// Duration
		property = infoManager->addProperty( QLatin1String("Duration") );
		property->setItalics(true);
		idToProperty[property->propertyName()] = property;
		rttiToProperty[Source::VIDEO_SOURCE]->addSubProperty(property);
		// mark IN
		property = infoManager->addProperty( QLatin1String("Mark in") );
		property->setItalics(true);
		idToProperty[property->propertyName()] = property;
		rttiToProperty[Source::VIDEO_SOURCE]->addSubProperty(property);
		// mark OUT
		property = infoManager->addProperty( QLatin1String("Mark out") );
		property->setItalics(true);
		idToProperty[property->propertyName()] = property;
		rttiToProperty[Source::VIDEO_SOURCE]->addSubProperty(property);
		// Ignore alpha channel
		property = boolManager->addProperty("Ignore alpha");
		property->setToolTip("Do not use the alpha channel of the images (black instead).");
		idToProperty[property->propertyName()] = property;
		rttiToProperty[Source::VIDEO_SOURCE]->addSubProperty(property);


#ifdef OPEN_CV
	rttiToProperty[Source::CAMERA_SOURCE] = groupManager->addProperty( QLatin1String("Camera"));

		// Identifier
		property = infoManager->addProperty( QLatin1String("Identifier") );
		property->setItalics(true);
		idToProperty[property->propertyName()] = property;
		rttiToProperty[Source::CAMERA_SOURCE]->addSubProperty(property);
		// Frame rate
		rttiToProperty[Source::CAMERA_SOURCE]->addSubProperty(fr);

		// Frames size & aspect ratio
		rttiToProperty[Source::CAMERA_SOURCE]->addSubProperty(fs);
		rttiToProperty[Source::CAMERA_SOURCE]->addSubProperty(ar);
#endif

	rttiToProperty[Source::RENDERING_SOURCE] = groupManager->addProperty( QLatin1String("Render loop-back"));

		// Identifier
		property = infoManager->addProperty( QLatin1String("Rendering mechanism") );
		property->setItalics(true);
		idToProperty[property->propertyName()] = property;
		rttiToProperty[Source::RENDERING_SOURCE]->addSubProperty(property);
		// Frame rate
		rttiToProperty[Source::RENDERING_SOURCE]->addSubProperty(fr);

		// Frames size & aspect ratio
		rttiToProperty[Source::RENDERING_SOURCE]->addSubProperty(fs);
		rttiToProperty[Source::RENDERING_SOURCE]->addSubProperty(ar);

	rttiToProperty[Source::ALGORITHM_SOURCE] = groupManager->addProperty( QLatin1String("Algorithm"));

		// Identifier
		property = infoManager->addProperty( QLatin1String("Algorithm") );
		property->setItalics(true);
		idToProperty[property->propertyName()] = property;
		rttiToProperty[Source::ALGORITHM_SOURCE]->addSubProperty(property);
		// Frame rate
		rttiToProperty[Source::ALGORITHM_SOURCE]->addSubProperty(fr);
		// Frames size & aspect ratio
		rttiToProperty[Source::ALGORITHM_SOURCE]->addSubProperty(fs);
		rttiToProperty[Source::ALGORITHM_SOURCE]->addSubProperty(ar);
		// Variability
		property = intManager->addProperty( QLatin1String("Variability") );
		idToProperty[property->propertyName()] = property;
		intManager->setRange(property, 0, 100);
		rttiToProperty[Source::ALGORITHM_SOURCE]->addSubProperty(property);
		// Periodicity
		property = intManager->addProperty( QLatin1String("Update frequency") );
		idToProperty[property->propertyName()] = property;
		intManager->setRange(property, 1, 60);
		rttiToProperty[Source::ALGORITHM_SOURCE]->addSubProperty(property);

	rttiToProperty[Source::CLONE_SOURCE] = groupManager->addProperty( QLatin1String("Clone"));

		// Identifier
		property = infoManager->addProperty( QLatin1String("Clone of") );
		property->setItalics(true);
		idToProperty[property->propertyName()] = property;
		rttiToProperty[Source::CLONE_SOURCE]->addSubProperty(property);

		// Frames size & aspect ratio
		rttiToProperty[Source::CLONE_SOURCE]->addSubProperty(fs);
		rttiToProperty[Source::CLONE_SOURCE]->addSubProperty(ar);

//	rttiToProperty[Source::CAPTURE_SOURCE] = groupManager->addProperty( QLatin1String("Rendering capture properties"));

}


void SourcePropertyBrowser::setPropertyEnabled(QString propertyName, bool enabled){

	if (idToProperty.contains(propertyName))
		idToProperty[propertyName]->setEnabled(enabled);

}

void SourcePropertyBrowser::updatePropertyTree(Source *s){

    // connect the managers to the corresponding value change
    disconnect(doubleManager, SIGNAL(valueChanged(QtProperty *, double)),
                this, SLOT(valueChanged(QtProperty *, double)));
    disconnect(stringManager, SIGNAL(valueChanged(QtProperty *, const QString &)),
                this, SLOT(valueChanged(QtProperty *, const QString &)));
    disconnect(colorManager, SIGNAL(valueChanged(QtProperty *, const QColor &)),
                this, SLOT(valueChanged(QtProperty *, const QColor &)));
    disconnect(pointManager, SIGNAL(valueChanged(QtProperty *, const QPointF &)),
                this, SLOT(valueChanged(QtProperty *, const QPointF &)));
    disconnect(enumManager, SIGNAL(valueChanged(QtProperty *, int)),
                this, SLOT(enumChanged(QtProperty *, int)));
    disconnect(intManager, SIGNAL(valueChanged(QtProperty *, int)),
                this, SLOT(valueChanged(QtProperty *, int)));
    disconnect(boolManager, SIGNAL(valueChanged(QtProperty *, bool)),
                this, SLOT(valueChanged(QtProperty *, bool)));
    disconnect(rectManager, SIGNAL(valueChanged(QtProperty *, const QRectF &)),
                this, SLOT(valueChanged(QtProperty *, const QRectF &)));

	if (s) {
		stringManager->setValue(idToProperty["Name"], s->getName() );
		boolManager->setValue(idToProperty["Modifiable"], s->isModifiable() );
		idToProperty["Position"]->setEnabled(s->isModifiable());
		pointManager->setValue(idToProperty["Position"], QPointF( s->getX() / SOURCE_UNIT, s->getY() / SOURCE_UNIT));
//		idToProperty["Rotation center"]->setEnabled(s->isModifiable());
//		pointManager->setValue(idToProperty["Rotation center"], QPointF( s->getCenterX() / SOURCE_UNIT, s->getCenterY() / SOURCE_UNIT));
		idToProperty["Angle"]->setEnabled(s->isModifiable());
		doubleManager->setValue(idToProperty["Angle"], s->getRotationAngle() );
		idToProperty["Scale"]->setEnabled(s->isModifiable());
		pointManager->setValue(idToProperty["Scale"], QPointF( s->getScaleX() / SOURCE_UNIT, s->getScaleY() / SOURCE_UNIT));
		idToProperty["Crop"]->setEnabled(s->isModifiable());
		rectManager->setValue(idToProperty["Crop"], s->getTextureCoordinates());
		idToProperty["Depth"]->setEnabled(s->isModifiable());
		doubleManager->setValue(idToProperty["Depth"], s->getDepth() );
		idToProperty["Alpha"]->setEnabled(s->isModifiable());
		doubleManager->setValue(idToProperty["Alpha"], s->getAlpha() );

		int preset = presetBlending.key( qMakePair( glblendToEnum[s->getBlendFuncDestination()], glequationToEnum[s->getBlendEquation()] ) );
		enumManager->setValue(idToProperty["Blending"], preset );
		enumManager->setValue(idToProperty["Destination"], glblendToEnum[ s->getBlendFuncDestination() ]);
		enumManager->setValue(idToProperty["Equation"], glequationToEnum[ s->getBlendEquation() ]);
		idToProperty["Destination"]->setEnabled(preset == 0);
		idToProperty["Equation"]->setEnabled(preset == 0);

		enumManager->setValue(idToProperty["Mask"], s->getMask());
		colorManager->setValue(idToProperty["Color"], QColor( s->getColor()));
		boolManager->setValue(idToProperty["Pixelated"], s->isPixelated());
		infoManager->setValue(idToProperty["Aspect ratio"], aspectRatioToString(s->getAspectRatio()) );


		idToProperty["Filtered"]->setEnabled(ViewRenderWidget::filteringEnabled());
		if (ViewRenderWidget::filteringEnabled()) {

			boolManager->setValue(idToProperty["Filtered"], s->isFiltered());
			intManager->setValue(idToProperty["Brightness"], s->getBrightness() );
			intManager->setValue(idToProperty["Contrast"], s->getContrast() );
			intManager->setValue(idToProperty["Saturation"], s->getSaturation() );
			intManager->setValue(idToProperty["Hue shift"], s->getHueShift() );
			intManager->setValue(idToProperty["Threshold"], s->getLuminanceThreshold() );
			intManager->setValue(idToProperty["Posterize"], s->getNumberOfColors() );
			enumManager->setValue(idToProperty["Color inversion"], (int) s->getInvertMode());
			enumManager->setValue(idToProperty["Filter"], (int) s->getFilter());
			boolManager->setValue(idToProperty["Chroma key"], s->getChromaKey());
			colorManager->setValue(idToProperty["Key Color"], QColor( s->getChromaKeyColor() ) );
			intManager->setValue(idToProperty["Key Tolerance"], s->getChromaKeyTolerance() );

			// enable / disable properties depending on their dependencies
			idToProperty["Brightness"]->setEnabled(s->isFiltered());
			idToProperty["Contrast"]->setEnabled(s->isFiltered());
			idToProperty["Threshold"]->setEnabled(s->isFiltered());
			idToProperty["Color inversion"]->setEnabled(s->isFiltered());
			idToProperty["Filter"]->setEnabled(s->isFiltered());
			idToProperty["Chroma key"]->setEnabled(s->isFiltered());

			if (s->isFiltered()) {
				idToProperty["Key Color"]->setEnabled(s->getChromaKey());
				idToProperty["Key Tolerance"]->setEnabled(s->getChromaKey());
				idToProperty["Saturation"]->setEnabled(s->getLuminanceThreshold() < 1);
				idToProperty["Hue shift"]->setEnabled(s->getLuminanceThreshold() < 1);
				idToProperty["Posterize"]->setEnabled(s->getLuminanceThreshold() < 1);
			} else {
				idToProperty["Posterize"]->setEnabled(false);
				idToProperty["Hue shift"]->setEnabled(false);
				idToProperty["Saturation"]->setEnabled(false);
				idToProperty["Key Color"]->setEnabled(false);
				idToProperty["Key Tolerance"]->setEnabled(false);
			}
		}

		if (s->rtti() == Source::VIDEO_SOURCE) {
			infoManager->setValue(idToProperty["Type"], QLatin1String("Media file") );

			VideoSource *vs = dynamic_cast<VideoSource *>(s);
			VideoFile *vf = vs->getVideoFile();
			infoManager->setValue(idToProperty["File name"], QFileInfo(vf->getFileName()).fileName() );
			idToProperty["File name"]->setToolTip(vf->getFileName());
			infoManager->setValue(idToProperty["File size"], getSizeString( QFileInfo(vf->getFileName()).size() ) );
			infoManager->setValue(idToProperty["Codec"], vf->getCodecName() );
			infoManager->setValue(idToProperty["Pixel format"], vf->getPixelFormatName() );
			boolManager->setValue(idToProperty["Ignore alpha"], vf->ignoresAlphaChannel());
			idToProperty["Ignore alpha"]->setEnabled(vf->pixelFormatHasAlphaChannel());
			sizeManager->setValue(idToProperty["Frames size"], QSize(vf->getFrameWidth(),vf->getFrameHeight()) );
			// Frames size special case when power of two dimensions are generated
			if (vf->getStreamFrameWidth() != vf->getFrameWidth() || vf->getStreamFrameHeight() != vf->getFrameHeight()) {
				sizeManager->setValue(idToProperty["Original size"], QSize(vf->getStreamFrameWidth(),vf->getStreamFrameHeight()) );
				if ( !rttiToProperty[Source::VIDEO_SOURCE]->subProperties().contains(idToProperty["Original size"]))
					rttiToProperty[Source::VIDEO_SOURCE]->insertSubProperty(idToProperty["Original size"], idToProperty["Ignore alpha"]);
			} else {
				if ( rttiToProperty[Source::VIDEO_SOURCE]->subProperties().contains(idToProperty["Original size"]))
					rttiToProperty[Source::VIDEO_SOURCE]->removeSubProperty(idToProperty["Original size"]);
			}
			infoManager->setValue(idToProperty["Frame rate"], QString::number( vf->getFrameRate() ) + QString(" fps") );
			infoManager->setValue(idToProperty["Duration"], vf->getTimeFromFrame(vf->getEnd()) );
			infoManager->setValue(idToProperty["Mark in"],  vf->getTimeFromFrame(vf->getMarkIn()) );
			infoManager->setValue(idToProperty["Mark out"], vf->getTimeFromFrame(vf->getMarkOut()) );
			infoManager->setValue(idToProperty["Interlaced"], vf->isInterlaced() ? tr("Yes") : tr("No") );

		} else
#ifdef OPEN_CV
		if (s->rtti() == Source::CAMERA_SOURCE) {

			infoManager->setValue(idToProperty["Type"], QLatin1String("Camera device") );
			OpencvSource *cvs = dynamic_cast<OpencvSource *>(s);
			infoManager->setValue(idToProperty["Identifier"], QString("OpenCV Camera %1").arg(cvs->getOpencvCameraIndex()) );
			sizeManager->setValue(idToProperty["Frames size"], QSize(cvs->getFrameWidth(), cvs->getFrameHeight()) );
			infoManager->setValue(idToProperty["Frame rate"], QString::number( cvs->getFrameRate() ) + QString(" fps") );
		} else
#endif
		if (s->rtti() == Source::RENDERING_SOURCE) {

			infoManager->setValue(idToProperty["Type"], QLatin1String("Rendering loop-back") );
			if (RenderingManager::getInstance()->getUseFboBlitExtension())
				infoManager->setValue(idToProperty["Rendering mechanism"], "Blit to frame buffer object" );
			else
				infoManager->setValue(idToProperty["Rendering mechanism"], "Draw in frame buffer object" );
			sizeManager->setValue(idToProperty["Frames size"], QSize(RenderingManager::getInstance()->getFrameBufferWidth(), RenderingManager::getInstance()->getFrameBufferHeight()) );
			infoManager->setValue(idToProperty["Frame rate"], QString::number( RenderingManager::getRenderingWidget()->getFramerate() / float(RenderingManager::getInstance()->getPreviousFrameDelay()) ) + QString(" fps") );
		} else
		if (s->rtti() == Source::ALGORITHM_SOURCE) {

			infoManager->setValue(idToProperty["Type"], QLatin1String("Algorithm") );
			AlgorithmSource *as = dynamic_cast<AlgorithmSource *>(s);
			infoManager->setValue(idToProperty["Algorithm"], AlgorithmSource::getAlgorithmDescription(as->getAlgorithmType()) );
			sizeManager->setValue(idToProperty["Frames size"], QSize(as->getFrameWidth(), as->getFrameHeight()) );
			infoManager->setValue(idToProperty["Frame rate"], QString::number(as->getFrameRate() ) + QString(" fps"));

			intManager->setValue(idToProperty["Variability"], (int) ( as->getVariability() * 100.0 ) );
			intManager->setValue(idToProperty["Update frequency"], (int) ( 1000000.0 / double(as->getPeriodicity()) ) );

		} else
		if (s->rtti() == Source::CLONE_SOURCE) {

			infoManager->setValue(idToProperty["Type"], QLatin1String("Clone") );
			CloneSource *cs = dynamic_cast<CloneSource *>(s);
			infoManager->setValue(idToProperty["Clone of"], cs->getOriginalName() );
			sizeManager->setValue(idToProperty["Frames size"], QSize(cs->getFrameWidth(), cs->getFrameHeight()) );
		} else
		if (s->rtti() == Source::CAPTURE_SOURCE) {
			infoManager->setValue(idToProperty["Type"], QLatin1String("Captured image") );
			CaptureSource *cs = dynamic_cast<CaptureSource *>(s);
			sizeManager->setValue(idToProperty["Frames size"], QSize(cs->getFrameWidth(), cs->getFrameHeight()) );
		}

	    // reconnect the managers to the corresponding value change
	    connect(doubleManager, SIGNAL(valueChanged(QtProperty *, double)),
	                this, SLOT(valueChanged(QtProperty *, double)));
	    connect(stringManager, SIGNAL(valueChanged(QtProperty *, const QString &)),
	                this, SLOT(valueChanged(QtProperty *, const QString &)));
	    connect(colorManager, SIGNAL(valueChanged(QtProperty *, const QColor &)),
	                this, SLOT(valueChanged(QtProperty *, const QColor &)));
	    connect(pointManager, SIGNAL(valueChanged(QtProperty *, const QPointF &)),
	                this, SLOT(valueChanged(QtProperty *, const QPointF &)));
	    connect(enumManager, SIGNAL(valueChanged(QtProperty *, int)),
	                this, SLOT(enumChanged(QtProperty *, int)));
	    connect(intManager, SIGNAL(valueChanged(QtProperty *, int)),
	                this, SLOT(valueChanged(QtProperty *, int)));
	    connect(boolManager, SIGNAL(valueChanged(QtProperty *, bool)),
	                this, SLOT(valueChanged(QtProperty *, bool)));
	    connect(rectManager, SIGNAL(valueChanged(QtProperty *, const QRectF &)),
	                this, SLOT(valueChanged(QtProperty *, const QRectF &)));
	}
}

void SourcePropertyBrowser::showProperties(SourceSet::iterator sourceIt){
	// this slot is called only when a different source is clicked (or when none is clicked)

	// remember expanding state
    updateExpandState();

	// clear the GUI
    propertyTreeEditor->clear();
    propertyGroupEditor->clear();

	if ( RenderingManager::getInstance()->isValid(sourceIt) )
		showProperties(*sourceIt);
	else
		showProperties(0);

}

void SourcePropertyBrowser::showProperties(Source *source) {

	currentItem = source;
	updatePropertyTree( source );

	if (source) {

		// show all the Properties into the browser:
		QListIterator<QtProperty *> it(root->subProperties());

		// first property ; the name
		addProperty(it.next());

		// add the sub tree of the properties related to this source type
		if ( rttiToProperty.contains(source->rtti()) )
			addProperty( rttiToProperty[ source->rtti() ] );

		// the rest of the properties
		while (it.hasNext()) {
			addProperty(it.next());
		}

	}
}


void SourcePropertyBrowser::addProperty(QtProperty *property)
{
	// add to the group view
    propertyGroupEditor->addProperty(property);

    // add to the tree view
    QtBrowserItem *item = propertyTreeEditor->addProperty(property);
    if (idToExpanded.contains(property->propertyName()))
    	propertyTreeEditor->setExpanded(item, idToExpanded[property->propertyName()]);
    else
    	propertyTreeEditor->setExpanded(item, false);
}


void SourcePropertyBrowser::updateExpandState()
{
    QList<QtBrowserItem *> list = propertyTreeEditor->topLevelItems();
    QListIterator<QtBrowserItem *> it(list);
    while (it.hasNext()) {
        QtBrowserItem *item = it.next();
        idToExpanded[item->property()->propertyName()] = propertyTreeEditor->isExpanded(item);
    }
}


void SourcePropertyBrowser::setGlobalExpandState(bool expanded)
{
    QList<QtBrowserItem *> list = propertyTreeEditor->topLevelItems();
    QListIterator<QtBrowserItem *> it(list);
    while (it.hasNext()) {
        QtBrowserItem *item = it.next();
        idToExpanded[item->property()->propertyName()] = expanded;
        propertyTreeEditor->setExpanded(item, expanded);
    }
}



void SourcePropertyBrowser::valueChanged(QtProperty *property, const QString &value){

	if (!currentItem)
			return;

	if ( property == idToProperty["Name"] ) {
		currentItem->setName(value);
	}
}


void SourcePropertyBrowser::valueChanged(QtProperty *property, const QPointF &value){

	if (!currentItem)
			return;

	if ( property == idToProperty["Position"] ) {
		currentItem->setX( value.x() * SOURCE_UNIT);
		currentItem->setY( value.y() * SOURCE_UNIT);
	}
	else if ( property == idToProperty["Rotation center"] ) {
		currentItem->setCenterX( value.x() * SOURCE_UNIT );
		currentItem->setCenterY( value.y() * SOURCE_UNIT);
	}
	else if ( property == idToProperty["Scale"] ) {
		currentItem->setScaleX( value.x() * SOURCE_UNIT );
		currentItem->setScaleY( value.y() * SOURCE_UNIT);
	}
}

void SourcePropertyBrowser::valueChanged(QtProperty *property, const QRectF &value){

	if (!currentItem)
			return;

	if ( property == idToProperty["Crop"] ) {
		currentItem->setTextureCoordinates(value);
	}
}



void SourcePropertyBrowser::valueChanged(QtProperty *property, const QColor &value){

	if (!currentItem)
			return;

	if ( property == idToProperty["Color"] ) {
		currentItem->setColor(value);
	}
	else if ( property == idToProperty["Key Color"] ) {
			currentItem->setChromaKeyColor(value);
	}
}


void SourcePropertyBrowser::valueChanged(QtProperty *property, double value){

	if (!currentItem)
			return;

	if ( property == idToProperty["Depth"] ) {
		if ( RenderingManager::getInstance()->notAtEnd(RenderingManager::getInstance()->getCurrentSource()) ) {
			// ask the rendering manager to change the depth of the source
			SourceSet::iterator c = RenderingManager::getInstance()->changeDepth(RenderingManager::getInstance()->getCurrentSource(), value);
			// we need to set current again (the list changed)
			RenderingManager::getInstance()->setCurrentSource(c);

			// forces the update of the value, without calling valueChanded again.
			disconnect(doubleManager, SIGNAL(valueChanged(QtProperty *, double)), this, SLOT(valueChanged(QtProperty *, double)));
			doubleManager->setValue(idToProperty["Depth"], (*c)->getDepth() );
			connect(doubleManager, SIGNAL(valueChanged(QtProperty *, double)), this, SLOT(valueChanged(QtProperty *, double)));
		}
	}
	else if ( property == idToProperty["Angle"] ) {
		currentItem->setRotationAngle(value);
	}
	else if ( property == idToProperty["Alpha"] ) {
		currentItem->setAlpha(value);
	}
}

void SourcePropertyBrowser::valueChanged(QtProperty *property,  bool value){

	if (!currentItem)
		return;

	if ( property == idToProperty["Modifiable"] ) {
		currentItem->setModifiable(value);
		updatePropertyTree(currentItem);
	}
	else if ( property == idToProperty["Pixelated"] ) {
		currentItem->setPixelated(value);
	}
	else if ( property == idToProperty["Ignore alpha"] ) {
		if (currentItem->rtti() == Source::VIDEO_SOURCE) {
			VideoSource *vs = dynamic_cast<VideoSource *>(currentItem);
			if (vs != 0) {
				VideoFile *vf = vs->getVideoFile();
				vf->stop();
				vf->close();
				vf->open(vf->getFileName(), vf->getMarkIn(), vf->getMarkOut(), value);
			}
		}
	}
	else if ( property == idToProperty["Filtered"] ) {
		currentItem->setFiltered(value);
		updatePropertyTree(currentItem);
	}
	else if ( property == idToProperty["Chroma key"] ) {
		currentItem->setChromaKey(value);
		idToProperty["Key Color"]->setEnabled(value);
		idToProperty["Key Tolerance"]->setEnabled(value);
	}
}

void SourcePropertyBrowser::valueChanged(QtProperty *property,  int value){

	if (!currentItem)
			return;

	if ( property == idToProperty["Brightness"] ) {
		currentItem->setBrightness(value);
	}
	else if ( property == idToProperty["Contrast"] ) {
		currentItem->setContrast(value);
	}
	else if ( property == idToProperty["Saturation"] ) {
		currentItem->setSaturation(value);
	}
	else if ( property == idToProperty["Hue shift"] ) {
		currentItem->setHueShift(value);
	}
	else if ( property == idToProperty["Threshold"] ) {
		currentItem->setLuminanceThreshold(value);
		if (value > 0) {
			idToProperty["Saturation"]->setEnabled(false);
			idToProperty["Hue shift"]->setEnabled(false);
			idToProperty["Posterize"]->setEnabled(false);
		} else {
			idToProperty["Saturation"]->setEnabled(true);
			idToProperty["Hue shift"]->setEnabled(true);
			idToProperty["Posterize"]->setEnabled(true);
		}
	}
	else if ( property == idToProperty["Posterize"] ) {
		currentItem->setNumberOfColors(value);
	}
	else if ( property == idToProperty["Key Tolerance"] ) {
		currentItem->setChromaKeyTolerance(value);
	}
	else if ( property == idToProperty["Variability"] ) {
		if (currentItem->rtti() == Source::ALGORITHM_SOURCE) {
			AlgorithmSource *as = dynamic_cast<AlgorithmSource *>(currentItem);
			as->setVariability( double(value) / 100.0);
		}
	}
	else if ( property == idToProperty["Update frequency"] ) {
		if (currentItem->rtti() == Source::ALGORITHM_SOURCE) {
			AlgorithmSource *as = dynamic_cast<AlgorithmSource *>(currentItem);
			as->setPeriodicity( (unsigned long) ( 1000000.0 / double(value) ) );
		}
	}
}

void SourcePropertyBrowser::enumChanged(QtProperty *property,  int value){

	if (!currentItem)
			return;

	if ( property == idToProperty["Blending"] ) {

		if ( value != 0) {
			currentItem->setBlendFunc(GL_SRC_ALPHA, enumToGlblend[ presetBlending[value].first ] );
			currentItem->setBlendEquation( enumToGlequation[ presetBlending[value].second ] );
			enumManager->setValue(idToProperty["Destination"], glblendToEnum[ currentItem->getBlendFuncDestination() ]);
			enumManager->setValue(idToProperty["Equation"], glequationToEnum[ currentItem->getBlendEquation() ]);
		}
		idToProperty["Destination"]->setEnabled(value == 0);
		idToProperty["Equation"]->setEnabled(value == 0);

	}
	else if ( property == idToProperty["Destination"] ) {

		currentItem->setBlendFunc(GL_SRC_ALPHA, enumToGlblend[value]);
	}
	else if ( property == idToProperty["Equation"] ) {

		currentItem->setBlendEquation(enumToGlequation[value]);
	}
	else if ( property == idToProperty["Mask"] ) {

// TODO : implement custom file mask
//			if ( (Source::maskType) value == Source::CUSTOM_MASK ) {
//				static QDir d = QDir::home();
//				qDebug("CUSTOM MASK");
//				QString fileName = QFileDialog::getOpenFileName(0, tr("Open File"), d.absolutePath(),
//															   tr("Image (*.png *.tif *.tiff *.gif *.tga)"));
//				if (!fileName.isEmpty()) {
//					d.setPath(fileName);
//				}
//			} else
			currentItem->setMask( (Source::maskType) value );

	}
	else if ( property == idToProperty["Filter"] ) {

		currentItem->setFilter( (Source::filterType) value );
		// indicate that this change affects performance
		property->setModified(value != 0);
	}
	else if ( property == idToProperty["Color inversion"] ) {

		currentItem->setInvertMode( (Source::invertModeType) value );

	}
}


void SourcePropertyBrowser::updateMixingProperties(){

	if (!currentItem)
		return;

	disconnect(doubleManager, SIGNAL(valueChanged(QtProperty *, double)), this, SLOT(valueChanged(QtProperty *, double)));
	doubleManager->setValue(idToProperty["Alpha"], currentItem->getAlpha() );
	connect(doubleManager, SIGNAL(valueChanged(QtProperty *, double)), this, SLOT(valueChanged(QtProperty *, double)));
}


void SourcePropertyBrowser::updateGeometryProperties(){

	if (!currentItem)
			return;

	disconnect(pointManager, SIGNAL(valueChanged(QtProperty *, const QPointF &)), this, SLOT(valueChanged(QtProperty *, const QPointF &)));
	pointManager->setValue(idToProperty["Position"], QPointF( currentItem->getX() / SOURCE_UNIT, currentItem->getY() / SOURCE_UNIT));
	pointManager->setValue(idToProperty["Scale"], QPointF( currentItem->getScaleX() / SOURCE_UNIT, currentItem->getScaleY() / SOURCE_UNIT));
	doubleManager->setValue(idToProperty["Angle"], currentItem->getRotationAngle());
	connect(pointManager, SIGNAL(valueChanged(QtProperty *, const QPointF &)), this, SLOT(valueChanged(QtProperty *, const QPointF &)));

	disconnect(rectManager, SIGNAL(valueChanged(QtProperty *, const QRectF &)), this, SLOT(valueChanged(QtProperty *, const QRectF &)));
	rectManager->setValue(idToProperty["Crop"], currentItem->getTextureCoordinates() );
	connect(rectManager, SIGNAL(valueChanged(QtProperty *, const QRectF &)), this, SLOT(valueChanged(QtProperty *, const QRectF &)));

}

void SourcePropertyBrowser::updateLayerProperties(){

	if (!currentItem)
			return;

	disconnect(doubleManager, SIGNAL(valueChanged(QtProperty *, double)), this, SLOT(valueChanged(QtProperty *, double)));
	doubleManager->setValue(idToProperty["Depth"], currentItem->getDepth() );
	connect(doubleManager, SIGNAL(valueChanged(QtProperty *, double)), this, SLOT(valueChanged(QtProperty *, double)));
}

void SourcePropertyBrowser::updateMarksProperties(bool showFrames){

	if (!currentItem)
			return;

	if (currentItem->rtti() == Source::VIDEO_SOURCE) {
		VideoSource *vs = dynamic_cast<VideoSource *>(currentItem);
		if (vs != 0) {
			VideoFile *vf = vs->getVideoFile();
			if (showFrames) {
				infoManager->setValue(idToProperty["Duration"], vf->getExactFrameFromFrame(vf->getEnd()) );
				infoManager->setValue(idToProperty["Mark in"],  vf->getExactFrameFromFrame(vf->getMarkIn()) );
				infoManager->setValue(idToProperty["Mark out"], vf->getExactFrameFromFrame(vf->getMarkOut()) );
			} else {
				infoManager->setValue(idToProperty["Duration"], vf->getTimeFromFrame(vf->getEnd()) );
				infoManager->setValue(idToProperty["Mark in"],  vf->getTimeFromFrame(vf->getMarkIn()) );
				infoManager->setValue(idToProperty["Mark out"], vf->getTimeFromFrame(vf->getMarkOut()) );
			}
		}
	}
}


void SourcePropertyBrowser::ctxMenuGroup(const QPoint &pos){

    static QMenu *menu = 0;
    if (!menu) {
    	menu = new QMenu;
    	menu->addAction(tr("Switch to Tree view"), this, SLOT(switchToTreeView()));
        menu->addSeparator();
        menu->addAction(tr("Reset all properties"), RenderingManager::getInstance(), SLOT(resetCurrentSource()));
    }
    menu->exec(mapToGlobal(pos));
}


void SourcePropertyBrowser::ctxMenuTree(const QPoint &pos){

    static QMenu *menu = 0;
    if (!menu) {
    	menu = new QMenu;
        menu->addAction(tr("Switch to Groups view"), this, SLOT(switchToGroupView()));
        menu->addAction(tr("Expand tree"), this, SLOT(expandAll()));
        menu->addAction(tr("Collapse tree"), this, SLOT(collapseAll()));
        menu->addSeparator();
        menu->addAction(tr("Reset all properties"), RenderingManager::getInstance(), SLOT(resetCurrentSource()));
        // TODO ; context entry to reset the current property
//        menu->addAction(tr("Reset this property"), RenderingManager::getInstance(), SLOT(resetCurrentSource()));
        // use QtAbstractPropertyBrowser :  propertyTreeEditor->currentItem()->property();
    }

    menu->exec(mapToGlobal(pos));
}

void SourcePropertyBrowser::switchToTreeView(){

	propertyGroupArea->setVisible(false);
	layout->removeWidget(propertyGroupArea);

    layout->addWidget(propertyTreeEditor);
    propertyTreeEditor->setVisible(true);
}

void SourcePropertyBrowser::switchToGroupView(){

	propertyTreeEditor->setVisible(false);
	layout->removeWidget(propertyTreeEditor);

    layout->addWidget(propertyGroupArea);
    propertyGroupArea->setVisible(true);
}

QString getSizeString(float num)
{
    QStringList list;
    list << "KB" << "MB" << "GB" << "TB";

    QStringListIterator i(list);
    QString unit("bytes");

    while(num >= 1024.0 && i.hasNext())
     {
        unit = i.next();
        num /= 1024.0;
    }
    return QString().setNum(num,'f',2)+" "+unit;
}

