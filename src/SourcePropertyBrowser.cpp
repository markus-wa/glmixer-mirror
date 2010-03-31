/*
 * SourcePropertyBrowser.cpp
 *
 *  Created on: Mar 14, 2010
 *      Author: bh
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

#include "RenderingManager.h"
#include "ViewRenderWidget.h"
#include "RenderingSource.h"
#include "AlgorithmSource.h"
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


SourcePropertyBrowser::SourcePropertyBrowser(QWidget *parent) : QWidget (parent), root(0) {

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
    timeManager = new QtTimePropertyManager(this);
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
    QtTimeEditFactory *timeFactory = new QtTimeEditFactory(this);						// for time
    QtColorEditorFactory *colorFactory = new QtColorEditorFactory(this);				// for color

    propertyTreeEditor->setFactoryForManager(doubleManager, doubleSpinBoxFactory);
    propertyTreeEditor->setFactoryForManager(intManager, spinBoxFactory);
    propertyTreeEditor->setFactoryForManager(stringManager, lineEditFactory);
    propertyTreeEditor->setFactoryForManager(colorManager, colorFactory);
    propertyTreeEditor->setFactoryForManager(pointManager->subDoublePropertyManager(), doubleSpinBoxFactory);
    propertyTreeEditor->setFactoryForManager(enumManager, comboBoxFactory);
    propertyTreeEditor->setFactoryForManager(boolManager, checkBoxFactory);
    propertyTreeEditor->setFactoryForManager(timeManager, timeFactory);

    propertyGroupEditor->setFactoryForManager(doubleManager, doubleSpinBoxFactory);
    propertyGroupEditor->setFactoryForManager(intManager, sliderFactory);
    propertyGroupEditor->setFactoryForManager(stringManager, lineEditFactory);
    propertyGroupEditor->setFactoryForManager(colorManager, colorFactory);
    propertyGroupEditor->setFactoryForManager(pointManager->subDoublePropertyManager(), doubleSpinBoxFactory);
    propertyGroupEditor->setFactoryForManager(enumManager, comboBoxFactory);
    propertyGroupEditor->setFactoryForManager(boolManager, checkBoxFactory);
    propertyGroupEditor->setFactoryForManager(timeManager, timeFactory);

}

SourcePropertyBrowser::~SourcePropertyBrowser() {
	// TODO delete properties of root and of rtti map
}



void SourcePropertyBrowser::createPropertyTree(){

	// we will need these:
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
	root->addSubProperty(property);
	// Type (not editable)
	property = infoManager->addProperty( QLatin1String("Type") );
	property->setItalics(true);
	idToProperty[property->propertyName()] = property;
	root->addSubProperty(property);
	// Position
	property = pointManager->addProperty("Position");
	idToProperty[property->propertyName()] = property;
	pointManager->subDoublePropertyManager()->setSingleStep(property->subProperties().first(), 0.1);
	pointManager->subDoublePropertyManager()->setSingleStep(property->subProperties().last(), 0.1);
	root->addSubProperty(property);
	// Scale
	property = pointManager->addProperty("Scale");
	idToProperty[property->propertyName()] = property;
	pointManager->subDoublePropertyManager()->setSingleStep(property->subProperties().first(), 0.1);
	pointManager->subDoublePropertyManager()->setSingleStep(property->subProperties().last(), 0.1);
	root->addSubProperty(property);
	// Depth
	property = doubleManager->addProperty("Depth");
	idToProperty[property->propertyName()] = property;
	doubleManager->setRange(property, MIN_DEPTH_LAYER, MAX_DEPTH_LAYER);
	root->addSubProperty(property);
	// Alpha
	property = doubleManager->addProperty("Alpha");
	idToProperty[property->propertyName()] = property;
	doubleManager->setRange(property, 0.0, 1.0);
	doubleManager->setSingleStep(property, 0.05);
	root->addSubProperty(property);
	// enum list of Destination blending func
	QtProperty *blendingItem = enumManager->addProperty("Blending");
	idToProperty[blendingItem->propertyName()] = blendingItem;
	QStringList enumNames;
	enumNames << "Custom" << "Color mix" << "Inverse color mix" << "Layer color mix" << "Layer inverse color mix" << "Layer opacity";
	enumManager->setEnumNames(blendingItem, enumNames);
	// Custom Blending
	// enum list of Destination blending func
	property = enumManager->addProperty("Destination");
	idToProperty[property->propertyName()] = property;
	enumNames.clear();
	enumNames << "Zero" << "One" << "Color" << "Invert color" << "Background color" << "Invert background color" << "Alpha" << "Invert alpha" << "Background Alpha" << "Invert background Alpha";
	enumManager->setEnumNames(property, enumNames);
	blendingItem->addSubProperty(property);
	// enum list of blending Equations
	property = enumManager->addProperty("Equation");
	idToProperty[property->propertyName()] = property;
	enumNames.clear();
	enumNames << "Add" << "Subtract" << "Reverse" << "Min" << "Max";
	enumManager->setEnumNames(property, enumNames);
	blendingItem->addSubProperty(property);
	// Confirm and add the blending item
	root->addSubProperty(blendingItem);
	// enum list of blending masks
	property = enumManager->addProperty("Mask");
	idToProperty[property->propertyName()] = property;
	enumNames.clear();
	enumNames << "None" <<"Rounded corners" <<  "Circle" << "Circular gradient" << "Square gradient" << "Custom file";
	enumManager->setEnumNames(property, enumNames);
    QMap<int, QIcon> enumIcons;
    enumIcons[0] = QIcon();
    enumIcons[1] = QIcon(":/glmixer/textures/mask_roundcorner.png");
    enumIcons[2] = QIcon(":/glmixer/textures/mask_circle.png");
    enumIcons[3] = QIcon(":/glmixer/textures/mask_linear_circle.png");
    enumIcons[4] = QIcon(":/glmixer/textures/mask_linear_square.png");
    enumIcons[5] = QIcon(":/glmixer/icons/fileopen.png");
    enumManager->setEnumIcons(property, enumIcons);

	root->addSubProperty(property);
	// Color
	property = colorManager->addProperty("Color");
	idToProperty[property->propertyName()] = property;
	root->addSubProperty(property);
	// Greyscale on/off
	property = boolManager->addProperty("Greyscale");
	idToProperty[property->propertyName()] = property;
	root->addSubProperty(property);
	// Invert color on/off
	property = boolManager->addProperty("Color Invert");
	idToProperty[property->propertyName()] = property;
	root->addSubProperty(property);
	// enum list of filters
	property = enumManager->addProperty("Filter");
	idToProperty[property->propertyName()] = property;
	enumNames.clear();
	enumNames << "None" << "Blur" << "Sharpen" << "Edge detect" << "Emboss";
	enumManager->setEnumNames(property, enumNames);
	root->addSubProperty(property);
	// Brightness
	property = intManager->addProperty( QLatin1String("Brightness") );
	idToProperty[property->propertyName()] = property;
	intManager->setRange(property, -100, 100);
	root->addSubProperty(property);
	// Contrast
	property = intManager->addProperty( QLatin1String("Contrast") );
	idToProperty[property->propertyName()] = property;
	intManager->setRange(property, -100, 100);
	root->addSubProperty(property);
	// Saturation
	property = intManager->addProperty( QLatin1String("Saturation") );
	idToProperty[property->propertyName()] = property;
	intManager->setRange(property, -100, 100);
	root->addSubProperty(property);
	// AspectRatio
	property = infoManager->addProperty("Aspect ratio");
	property->setItalics(true);
	idToProperty[property->propertyName()] = property;
	root->addSubProperty(property);


	rttiToProperty[Source::VIDEO_SOURCE] = groupManager->addProperty( QLatin1String("Media file properties"));

		// File Name
		property = infoManager->addProperty( QLatin1String("File name") );
		property->setItalics(true);
		idToProperty[property->propertyName()] = property;
		rttiToProperty[Source::VIDEO_SOURCE]->addSubProperty(property);
		// Codec
		property = infoManager->addProperty( QLatin1String("Codec") );
		property->setItalics(true);
		idToProperty[property->propertyName()] = property;
		rttiToProperty[Source::VIDEO_SOURCE]->addSubProperty(property);

		// Frames size
		QtProperty *fs = sizeManager->addProperty( QLatin1String("Frames size") );
		fs->setItalics(true);
		idToProperty[fs->propertyName()] = fs;
		rttiToProperty[Source::VIDEO_SOURCE]->addSubProperty(fs);

		// Frames size special case when power of two dimensions are generated
		property = sizeManager->addProperty( QLatin1String("Converted size") );
		property->setItalics(true);
		idToProperty[property->propertyName()] = property;
//		rttiToProperty[Source::VIDEO_SOURCE]->addSubProperty(property);

		// Frame rate
		QtProperty *fr = infoManager->addProperty( QLatin1String("Frame rate") );
		fr->setItalics(true);
		idToProperty[fr->propertyName()] = fr;
		rttiToProperty[Source::VIDEO_SOURCE]->addSubProperty(fr);
		// Duration
		property = infoManager->addProperty( QLatin1String("Duration") );
		property->setItalics(true);
		idToProperty[property->propertyName()] = property;
		rttiToProperty[Source::VIDEO_SOURCE]->addSubProperty(property);

//		// pre filtering options (brightness, contrast, etc.)
//		QtProperty *prefilterItem = groupManager->addProperty( QLatin1String("Pre-filtering"));
//
//		// Brightness
//		property = intManager->addProperty( QLatin1String("Brightness") );
//		idToProperty[QString("pre-") + property->propertyName()] = property;
//		intManager->setRange(property, -100, 100);
//		prefilterItem->addSubProperty(property);
//		// Contrast
//		property = intManager->addProperty( QLatin1String("Contrast") );
//		idToProperty[QString("pre-") + property->propertyName()] = property;
//		intManager->setRange(property, -100, 100);
//		prefilterItem->addSubProperty(property);
//		// Saturation
//		property = intManager->addProperty( QLatin1String("Saturation") );
//		idToProperty[QString("pre-") + property->propertyName()] = property;
//		intManager->setRange(property, -100, 100);
//		prefilterItem->addSubProperty(property);
//
//		rttiToProperty[Source::VIDEO_SOURCE]->addSubProperty(prefilterItem);

#ifdef OPEN_CV
	rttiToProperty[Source::CAMERA_SOURCE] = groupManager->addProperty( QLatin1String("Camera properties"));

		// Identifier
		property = infoManager->addProperty( QLatin1String("Identifier") );
		property->setItalics(true);
		idToProperty[property->propertyName()] = property;
		rttiToProperty[Source::CAMERA_SOURCE]->addSubProperty(property);
		// Frames size
		rttiToProperty[Source::CAMERA_SOURCE]->addSubProperty(fs);
		// Frame rate
		rttiToProperty[Source::CAMERA_SOURCE]->addSubProperty(fr);
#endif

	rttiToProperty[Source::RENDERING_SOURCE] = groupManager->addProperty( QLatin1String("Render loop-back properties"));

		// Identifier
		property = infoManager->addProperty( QLatin1String("Rendering mechanism") );
		property->setItalics(true);
		idToProperty[property->propertyName()] = property;
		rttiToProperty[Source::RENDERING_SOURCE]->addSubProperty(property);
		// Frames size
		rttiToProperty[Source::RENDERING_SOURCE]->addSubProperty(fs);
		// Frame rate
		rttiToProperty[Source::RENDERING_SOURCE]->addSubProperty(fr);


	rttiToProperty[Source::ALGORITHM_SOURCE] = groupManager->addProperty( QLatin1String("Algorithm properties"));

		// Identifier
		property = infoManager->addProperty( QLatin1String("Algorithm") );
		property->setItalics(true);
		idToProperty[property->propertyName()] = property;
		rttiToProperty[Source::ALGORITHM_SOURCE]->addSubProperty(property);
		// Frames size
		rttiToProperty[Source::ALGORITHM_SOURCE]->addSubProperty(fs);
		// Frame rate
		rttiToProperty[Source::ALGORITHM_SOURCE]->addSubProperty(fr);

	rttiToProperty[Source::CLONE_SOURCE] = groupManager->addProperty( QLatin1String("Clone properties"));

		// Identifier
		property = infoManager->addProperty( QLatin1String("Clone of") );
		property->setItalics(true);
		idToProperty[property->propertyName()] = property;
		rttiToProperty[Source::CLONE_SOURCE]->addSubProperty(property);

	rttiToProperty[Source::SIMPLE_SOURCE] = groupManager->addProperty( QLatin1String("Rendering capture properties"));

		// Frames size
		rttiToProperty[Source::SIMPLE_SOURCE]->addSubProperty(fs);


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

	if (s) {
		stringManager->setValue(idToProperty["Name"], s->getName() );
		pointManager->setValue(idToProperty["Position"], QPointF( s->getX() / SOURCE_UNIT, s->getY() / SOURCE_UNIT));
		pointManager->setValue(idToProperty["Scale"], QPointF( s->getScaleX() / SOURCE_UNIT, s->getScaleY() / SOURCE_UNIT));
		doubleManager->setValue(idToProperty["Depth"], s->getDepth() );
		doubleManager->setValue(idToProperty["Alpha"], s->getAlpha() );
		enumManager->setValue(idToProperty["Blending"], presetBlending.key( qMakePair( glblendToEnum[s->getBlendFuncDestination()], glequationToEnum[s->getBlendEquation()] ) ) );
		enumManager->setValue(idToProperty["Destination"], glblendToEnum[ s->getBlendFuncDestination() ]);
		enumManager->setValue(idToProperty["Equation"], glequationToEnum[ s->getBlendEquation() ]);
		enumManager->setValue(idToProperty["Mask"], s->getMask());
		colorManager->setValue(idToProperty["Color"], QColor( s->getColor()));
		boolManager->setValue(idToProperty["Greyscale"], s->isGreyscale());
		boolManager->setValue(idToProperty["Color Invert"], s->isInvertcolors());
		enumManager->setValue(idToProperty["Filter"], (int) s->getConvolution());
		infoManager->setValue(idToProperty["Aspect ratio"], QString::number(s->getAspectRatio()) );


		if (s->rtti() == Source::VIDEO_SOURCE) {
			infoManager->setValue(idToProperty["Type"], QLatin1String("Media file") );

			VideoSource *vs = dynamic_cast<VideoSource *>(s);
			VideoFile *vf = vs->getVideoFile();
			infoManager->setValue(idToProperty["File name"], QLatin1String(vf->getFileName()) );
			infoManager->setValue(idToProperty["Codec"], vf->getCodecName() );
			sizeManager->setValue(idToProperty["Frames size"], QSize(vf->getStreamFrameWidth(),vf->getStreamFrameHeight()) );
			// Frames size special case when power of two dimensions are generated
			sizeManager->setValue(idToProperty["Converted size"], QSize(vf->getFrameWidth(),vf->getFrameHeight()) );
			if (vf->getStreamFrameWidth() != vf->getFrameWidth() || vf->getStreamFrameHeight() != vf->getFrameHeight()) {
				if ( !rttiToProperty[Source::VIDEO_SOURCE]->subProperties().contains(idToProperty["Converted size"]))
					rttiToProperty[Source::VIDEO_SOURCE]->insertSubProperty(idToProperty["Converted size"], idToProperty["Frames size"]);
			} else {
				if ( rttiToProperty[Source::VIDEO_SOURCE]->subProperties().contains(idToProperty["Converted size"]))
					rttiToProperty[Source::VIDEO_SOURCE]->removeSubProperty(idToProperty["Converted size"]);
			}
			infoManager->setValue(idToProperty["Frame rate"], QString::number( vf->getFrameRate() ) + QString(" fps") );
			infoManager->setValue(idToProperty["Duration"], QString::number( vf->getDuration() ) + QString(" s") );

			// Enable all filters for video file and read the current values
			idToProperty["Brightness"]->setEnabled(true);
			intManager->setValue(idToProperty["Brightness"] , vf->getBrightness() );
			idToProperty["Contrast"]->setEnabled(true);
			intManager->setValue(idToProperty["Contrast"], vf->getContrast() );
			idToProperty["Saturation"]->setEnabled(true);
			intManager->setValue(idToProperty["Saturation"], vf->getSaturation() );
		} else {
			// Enable only the generic source filtering
			idToProperty["Brightness"]->setEnabled(true);
			intManager->setValue(idToProperty["Brightness"], s->getBrightness() );
			idToProperty["Contrast"]->setEnabled(true);
			// TODO : contrast generic solution ?
			intManager->setValue(idToProperty["Contrast"], 0 );
			idToProperty["Saturation"]->setEnabled(false);
			intManager->setValue(idToProperty["Saturation"], 0 );

#ifdef OPEN_CV
			if (s->rtti() == Source::CAMERA_SOURCE) {

				infoManager->setValue(idToProperty["Type"], QLatin1String("Camera device") );
				OpencvSource *cvs = dynamic_cast<OpencvSource *>(s);
				infoManager->setValue(idToProperty["Identifier"], QString("OpenCV Camera %1").arg(cvs->getOpencvCameraIndex()) );
				sizeManager->setValue(idToProperty["Frames size"], QSize(cvs->getFrameWidth(), cvs->getFrameHeight()) );
				infoManager->setValue(idToProperty["Frame rate"], QString::number( cvs->getFrameRate() ) + QString(" fps") );
			}
#endif
			if (s->rtti() == Source::RENDERING_SOURCE) {

				idToProperty["Brightness"]->setEnabled(false);

				infoManager->setValue(idToProperty["Type"], QLatin1String("Rendering loop-back") );
				if (glSupportsExtension("GL_EXT_framebuffer_blit"))
					infoManager->setValue(idToProperty["Rendering mechanism"], "Blit to frame buffer object" );
				else
					infoManager->setValue(idToProperty["Rendering mechanism"], "Draw to frame buffer object" );
				sizeManager->setValue(idToProperty["Frames size"], QSize(RenderingManager::getInstance()->getFrameBufferWidth(), RenderingManager::getInstance()->getFrameBufferHeight()) );
				infoManager->setValue(idToProperty["Frame rate"], QString::number( RenderingManager::getRenderingWidget()->getFPS() / float(RenderingManager::getInstance()->getPreviousFrameDelay()) ) + QString(" fps") );
			}
			else if (s->rtti() == Source::ALGORITHM_SOURCE) {

				infoManager->setValue(idToProperty["Type"], QLatin1String("Algorithm") );
				AlgorithmSource *as = dynamic_cast<AlgorithmSource *>(s);
				infoManager->setValue(idToProperty["Algorithm"], AlgorithmSource::getAlgorithmDescription(as->getAlgorithmType()) );
				sizeManager->setValue(idToProperty["Frames size"], QSize(as->getFrameWidth(), as->getFrameHeight()) );
				infoManager->setValue(idToProperty["Frame rate"], QString::number(as->getFrameRate() ) + QString(" fps"));
			}
			else if (s->rtti() == Source::CLONE_SOURCE) {

				idToProperty["Brightness"]->setEnabled(false);

				infoManager->setValue(idToProperty["Type"], QLatin1String("Clone") );
				CloneSource *cs = dynamic_cast<CloneSource *>(s);
				infoManager->setValue(idToProperty["Clone of"], cs->getOriginalName() );
			}
			else {
				infoManager->setValue(idToProperty["Type"], QLatin1String("Captured image") );
				sizeManager->setValue(idToProperty["Frames size"], QSize(RenderingManager::getInstance()->getFrameBufferWidth(), RenderingManager::getInstance()->getFrameBufferHeight()) );
			}
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
	}
}

void SourcePropertyBrowser::showProperties(SourceSet::iterator csi){

	// this slot is called only when a different source is clicked (or when none is clicked)

	// remember expanding state
    updateExpandState();

	// clear the GUI
    propertyTreeEditor->clear();
    propertyGroupEditor->clear();

	if ( RenderingManager::getInstance()->isValid(csi) ) {

		// ok, we got a valid source
		updatePropertyTree( *csi );

		// show all the Properties into the browser:
		QListIterator<QtProperty *> it(root->subProperties());
		while (it.hasNext()) {
			addProperty(it.next());
		}

		// add the sub tree of the properties related to this source type
		addProperty( rttiToProperty[ (*csi)->rtti() ] );
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

    if ( RenderingManager::getInstance()->notAtEnd(RenderingManager::getInstance()->getCurrentSource()) ) {
		Source *currentItem = *RenderingManager::getInstance()->getCurrentSource();

		if ( property == idToProperty["Name"] ) {
			currentItem->setName(value);
		}
    }
}


void SourcePropertyBrowser::valueChanged(QtProperty *property, const QPointF &value){

    if ( RenderingManager::getInstance()->notAtEnd(RenderingManager::getInstance()->getCurrentSource()) ) {
		Source *currentItem = *RenderingManager::getInstance()->getCurrentSource();

		if ( property == idToProperty["Position"] ) {
			currentItem->setX( value.x() * SOURCE_UNIT);
			currentItem->setY( value.y() * SOURCE_UNIT);
		}
		else if ( property == idToProperty["Scale"] ) {
			currentItem->setScaleX( value.x() * SOURCE_UNIT );
			currentItem->setScaleY( value.y() * SOURCE_UNIT);
		}
    }
}


void SourcePropertyBrowser::valueChanged(QtProperty *property, const QColor &value){

    if ( RenderingManager::getInstance()->notAtEnd(RenderingManager::getInstance()->getCurrentSource()) ) {
		Source *currentItem = *RenderingManager::getInstance()->getCurrentSource();

		if ( property == idToProperty["Color"] ) {
			currentItem->setColor(value);
		}
    }
}


void SourcePropertyBrowser::valueChanged(QtProperty *property, double value){

    if ( RenderingManager::getInstance()->notAtEnd(RenderingManager::getInstance()->getCurrentSource()) ) {

		if ( property == idToProperty["Depth"] ) {
			// ask the rendering manager to change the depth of the source
			SourceSet::iterator c = RenderingManager::getInstance()->changeDepth(RenderingManager::getInstance()->getCurrentSource(), value);
			// we need to set current again (the list changed)
			RenderingManager::getInstance()->setCurrentSource(c);

			// forces the update of the value, without calling valueChanded again.
			disconnect(doubleManager, SIGNAL(valueChanged(QtProperty *, double)), this, SLOT(valueChanged(QtProperty *, double)));
			doubleManager->setValue(idToProperty["Depth"], (*c)->getDepth() );
			connect(doubleManager, SIGNAL(valueChanged(QtProperty *, double)), this, SLOT(valueChanged(QtProperty *, double)));
		}
		else if ( property == idToProperty["Alpha"] ) {
			Source *currentItem = *RenderingManager::getInstance()->getCurrentSource();
			currentItem->setAlpha(value);
		}

    }
}

void SourcePropertyBrowser::valueChanged(QtProperty *property,  bool value){

    if ( RenderingManager::getInstance()->notAtEnd(RenderingManager::getInstance()->getCurrentSource()) ) {
		Source *currentItem = *RenderingManager::getInstance()->getCurrentSource();

		if ( property == idToProperty["Greyscale"] ) {
			currentItem->setGreyscale(value);
		} else if ( property == idToProperty["Color Invert"] ) {
			currentItem->setInvertcolors(value);
		}
		// indicate that this change affects performance
		property->setModified(value);
		// update the current frame
		currentItem->requestUpdate();
    }
}

void SourcePropertyBrowser::valueChanged(QtProperty *property,  int value){

    if ( RenderingManager::getInstance()->notAtEnd(RenderingManager::getInstance()->getCurrentSource()) ) {
		Source *currentItem = *RenderingManager::getInstance()->getCurrentSource();

		if ( property == idToProperty["Brightness"] ) {
			// use pre-filter filtering if we can on video source
			if (currentItem->rtti() == Source::VIDEO_SOURCE) {
				VideoSource *vs = dynamic_cast<VideoSource *>(currentItem);
				if (vs != 0)
					vs->getVideoFile()->setBrightness(value);
			} else
				currentItem->setBrightness(value);

		} else if ( property == idToProperty["Contrast"] ) {
			if (currentItem->rtti() == Source::VIDEO_SOURCE) {
				VideoSource *vs = dynamic_cast<VideoSource *>(currentItem);
				if (vs != 0)
					vs->getVideoFile()->setContrast(value);
			}
//			else
//			currentItem->setContrast(value);
		} else if ( property == idToProperty["Saturation"] ) {

			if (currentItem->rtti() == Source::VIDEO_SOURCE) {
				VideoSource *vs = dynamic_cast<VideoSource *>(currentItem);
				if (vs != 0)
					vs->getVideoFile()->setSaturation(value);
			}
		}

		currentItem->requestUpdate();
    }
}

void SourcePropertyBrowser::enumChanged(QtProperty *property,  int value){

    if ( RenderingManager::getInstance()->notAtEnd(RenderingManager::getInstance()->getCurrentSource()) ) {
		Source *currentItem = *RenderingManager::getInstance()->getCurrentSource();

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
		else if ( property == idToProperty["Filter"] ) {

			currentItem->setConvolution( (Source::convolutionType) value );
			currentItem->requestUpdate();

			// indicate that this change affects performance
			if (value == 0)
				property->setModified(false);
			else
				property->setModified(true);

		}
		else if ( property == idToProperty["Mask"] ) {

			if ( (Source::maskType) value == Source::CUSTOM_MASK ) {
				static QDir d = QDir::home();
				qDebug("CUSTOM MASK");
				QString fileName = QFileDialog::getOpenFileName(0, tr("Open File"), d.absolutePath(),
															   tr("Image (*.png *.tif *.tiff *.gif *.tga)"));
				if (!fileName.isEmpty()) {
					d.setPath(fileName);
				}
			} else
				currentItem->setMask( (Source::maskType) value );

		}

    }
}


void SourcePropertyBrowser::updateMixingProperties(){

    if ( RenderingManager::getInstance()->notAtEnd(RenderingManager::getInstance()->getCurrentSource()) ) {
		Source *currentItem = *RenderingManager::getInstance()->getCurrentSource();

	    disconnect(doubleManager, SIGNAL(valueChanged(QtProperty *, double)), this, SLOT(valueChanged(QtProperty *, double)));
		doubleManager->setValue(idToProperty["Alpha"], currentItem->getAlpha() );
		connect(doubleManager, SIGNAL(valueChanged(QtProperty *, double)), this, SLOT(valueChanged(QtProperty *, double)));
    }
}


void SourcePropertyBrowser::updateGeometryProperties(){

    if ( RenderingManager::getInstance()->notAtEnd(RenderingManager::getInstance()->getCurrentSource()) ) {
		Source *currentItem = *RenderingManager::getInstance()->getCurrentSource();

	    disconnect(pointManager, SIGNAL(valueChanged(QtProperty *, const QPointF &)), this, SLOT(valueChanged(QtProperty *, const QPointF &)));
		pointManager->setValue(idToProperty["Position"], QPointF( currentItem->getX() / SOURCE_UNIT, currentItem->getY() / SOURCE_UNIT));
		pointManager->setValue(idToProperty["Scale"], QPointF( currentItem->getScaleX() / SOURCE_UNIT, currentItem->getScaleY() / SOURCE_UNIT));
	    connect(pointManager, SIGNAL(valueChanged(QtProperty *, const QPointF &)), this, SLOT(valueChanged(QtProperty *, const QPointF &)));
    }
}

void SourcePropertyBrowser::updateLayerProperties(){

    if ( RenderingManager::getInstance()->notAtEnd(RenderingManager::getInstance()->getCurrentSource()) ) {
		Source *currentItem = *RenderingManager::getInstance()->getCurrentSource();

	    disconnect(doubleManager, SIGNAL(valueChanged(QtProperty *, double)), this, SLOT(valueChanged(QtProperty *, double)));
		doubleManager->setValue(idToProperty["Depth"], currentItem->getDepth() );
		connect(doubleManager, SIGNAL(valueChanged(QtProperty *, double)), this, SLOT(valueChanged(QtProperty *, double)));
    }
}


void SourcePropertyBrowser::ctxMenuGroup(const QPoint &pos){

    QMenu *menu = new QMenu;
    menu->addAction(tr("Switch to Tree view"), this, SLOT(switchToTreeView()));
    menu->exec(mapToGlobal(pos));

}


void SourcePropertyBrowser::ctxMenuTree(const QPoint &pos){

    QMenu *menu = new QMenu;
    menu->addAction(tr("Expand All"), this, SLOT(expandAll()));
    menu->addAction(tr("Collapse All"), this, SLOT(collapseAll()));
    menu->addAction(tr("Switch to Groups view"), this, SLOT(switchToGroupView()));
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


