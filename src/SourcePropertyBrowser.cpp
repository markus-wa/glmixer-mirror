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

    // connect the managers to the corresponding value change
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

	presetBlending[1] = qMakePair( 8, 0 );
	presetBlending[2] = qMakePair( 8, 2 );
	presetBlending[3] = qMakePair( 1, 0 );
	presetBlending[4] = qMakePair( 1, 2 );
	presetBlending[5] = qMakePair( 7, 0 );

	QtProperty *property;

	// the top property holding all the source sub-properties
	root = groupManager->addProperty( QLatin1String("Source root") );

	// Name
	property = stringManager->addProperty( QLatin1String("Name") );
	idToProperty[property->propertyName()] = property;
	root->addSubProperty(property);
	// Type (not editable)
	property = infoManager->addProperty( QLatin1String("[Type]") );
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
	enumNames << "Custom" << "Soft color mix (default)" << "Soft inverse color mix" << "Hard color mix" << "Hard inverse color mix" << "Layer opacity";
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
	enumNames << "None" << "Sharpen" << "Blur" << "Edge detect" << "Emboss";
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
	// AspectRatio
	property = infoManager->addProperty("[Aspect ratio]");
	idToProperty[property->propertyName()] = property;
	root->addSubProperty(property);


	rttiToProperty[Source::VIDEO_SOURCE] = groupManager->addProperty( QLatin1String("Media file properties"));

		// File Name
		property = infoManager->addProperty( QLatin1String("[File name]") );
		idToProperty[property->propertyName()] = property;
		rttiToProperty[Source::VIDEO_SOURCE]->addSubProperty(property);
		// Codec
		property = infoManager->addProperty( QLatin1String("[Codec]") );
		idToProperty[property->propertyName()] = property;
		rttiToProperty[Source::VIDEO_SOURCE]->addSubProperty(property);

		// Frames size
		QtProperty *fs = sizeManager->addProperty( QLatin1String("[Frames size]") );
		idToProperty[fs->propertyName()] = fs;
		rttiToProperty[Source::VIDEO_SOURCE]->addSubProperty(fs);

		// Frames size special case when power of two dimensions are generated
		property = sizeManager->addProperty( QLatin1String("[Converted size]") );
		idToProperty[property->propertyName()] = property;
		rttiToProperty[Source::VIDEO_SOURCE]->addSubProperty(property);

		// Frame rate
		QtProperty *fr = infoManager->addProperty( QLatin1String("[Frame rate]") );
		idToProperty[fr->propertyName()] = fr;
		rttiToProperty[Source::VIDEO_SOURCE]->addSubProperty(fr);
		// Duration
		property = infoManager->addProperty( QLatin1String("[Duration]") );
		idToProperty[property->propertyName()] = property;
		rttiToProperty[Source::VIDEO_SOURCE]->addSubProperty(property);

		// pre filtering options (brightness, contrast, etc.)
		QtProperty *prefilterItem = groupManager->addProperty( QLatin1String("Pre-filtering"));

		// Brightness
		property = intManager->addProperty( QLatin1String("Brightness") );
		idToProperty[QString("pre-") + property->propertyName()] = property;
		intManager->setRange(property, -100, 100);
		prefilterItem->addSubProperty(property);
		// Contrast
		property = intManager->addProperty( QLatin1String("Contrast") );
		idToProperty[QString("pre-") + property->propertyName()] = property;
		intManager->setRange(property, -100, 100);
		prefilterItem->addSubProperty(property);
		// Saturation
		property = intManager->addProperty( QLatin1String("Saturation") );
		idToProperty[QString("pre-") + property->propertyName()] = property;
		intManager->setRange(property, -100, 100);
		prefilterItem->addSubProperty(property);

		rttiToProperty[Source::VIDEO_SOURCE]->addSubProperty(prefilterItem);


	rttiToProperty[Source::CAMERA_SOURCE] = groupManager->addProperty( QLatin1String("Camera properties"));

		// Identifier
		property = infoManager->addProperty( QLatin1String("[Identifier]") );
		idToProperty[property->propertyName()] = property;
		rttiToProperty[Source::CAMERA_SOURCE]->addSubProperty(property);
		// Frames size
		rttiToProperty[Source::CAMERA_SOURCE]->addSubProperty(fs);
		// Frame rate
		rttiToProperty[Source::CAMERA_SOURCE]->addSubProperty(fr);


	rttiToProperty[Source::RENDERING_SOURCE] = groupManager->addProperty( QLatin1String("Render loop-back properties"));

		// Identifier
		property = infoManager->addProperty( QLatin1String("[Rendering mechanism]") );
		idToProperty[property->propertyName()] = property;
		rttiToProperty[Source::RENDERING_SOURCE]->addSubProperty(property);
		// Frames size
		rttiToProperty[Source::RENDERING_SOURCE]->addSubProperty(fs);
		// Frame rate
		rttiToProperty[Source::RENDERING_SOURCE]->addSubProperty(fr);


	rttiToProperty[Source::ALGORITHM_SOURCE] = groupManager->addProperty( QLatin1String("Algorithm properties"));

		// Identifier
		property = infoManager->addProperty( QLatin1String("[Algorithm]") );
		idToProperty[property->propertyName()] = property;
		rttiToProperty[Source::ALGORITHM_SOURCE]->addSubProperty(property);
		// Frames size
		rttiToProperty[Source::ALGORITHM_SOURCE]->addSubProperty(fs);
		// Frame rate
		rttiToProperty[Source::ALGORITHM_SOURCE]->addSubProperty(fr);

	rttiToProperty[Source::CLONE_SOURCE] = groupManager->addProperty( QLatin1String("Clone properties"));

		// Identifier
		property = infoManager->addProperty( QLatin1String("[Clone of]") );
		idToProperty[property->propertyName()] = property;
		rttiToProperty[Source::CLONE_SOURCE]->addSubProperty(property);

	rttiToProperty[Source::SIMPLE_SOURCE] = groupManager->addProperty( QLatin1String("Rendering capture properties"));

		// Frames size
		rttiToProperty[Source::SIMPLE_SOURCE]->addSubProperty(fs);


}

void SourcePropertyBrowser::updatePropertyTree(Source *s){

	stringManager->setValue(idToProperty["Name"], s->getName() );
	pointManager->setValue(idToProperty["Position"], QPointF( s->getX() / SOURCE_UNIT, s->getY() / SOURCE_UNIT));
	pointManager->setValue(idToProperty["Scale"], QPointF( s->getScaleX() / SOURCE_UNIT, s->getScaleY() / SOURCE_UNIT));
	doubleManager->setValue(idToProperty["Depth"], s->getDepth() );
	doubleManager->setValue(idToProperty["Alpha"], s->getAlpha() );
	enumManager->setValue(idToProperty["Blending"], presetBlending.key( qMakePair( glblendToEnum[s->getBlendFuncDestination()], glequationToEnum[s->getBlendEquation()] ) ) );
	enumManager->setValue(idToProperty["Destination"], glblendToEnum[ s->getBlendFuncDestination() ]);
	enumManager->setValue(idToProperty["Equation"], glequationToEnum[ s->getBlendEquation() ]);
	colorManager->setValue(idToProperty["Color"], QColor( s->getColor()));
	boolManager->setValue(idToProperty["Greyscale"], s->isGreyscale());
	boolManager->setValue(idToProperty["Color Invert"], s->isInvertcolors());
	enumManager->setValue(idToProperty["Filter"], (int) s->getConvolution());
	// TODO :
	intManager->setValue(idToProperty["Brightness"], 0 );
	intManager->setValue(idToProperty["Contrast"], 0 );
	infoManager->setValue(idToProperty["[Aspect ratio]"], QString::number(s->getAspectRatio()) );

	if (s->rtti() == Source::VIDEO_SOURCE) {
		infoManager->setValue(idToProperty["[Type]"], QLatin1String("Media file") );

		VideoSource *vs = dynamic_cast<VideoSource *>(s);
		VideoFile *vf = vs->getVideoFile();
		infoManager->setValue(idToProperty["[File name]"], QLatin1String(vf->getFileName()) );
		infoManager->setValue(idToProperty["[Codec]"], vf->getCodecName() );
		sizeManager->setValue(idToProperty["[Frames size]"], QSize(vf->getStreamFrameWidth(),vf->getStreamFrameHeight()) );
		// Frames size special case when power of two dimensions are generated
		sizeManager->setValue(idToProperty["[Converted size]"], QSize(vf->getFrameWidth(),vf->getFrameHeight()) );
		if (vf->getStreamFrameWidth() != vf->getFrameWidth() || vf->getStreamFrameHeight() != vf->getFrameHeight())
			idToProperty["[Converted size]"]->setEnabled(true);
		else
			idToProperty["[Converted size]"]->setEnabled(false);
		infoManager->setValue(idToProperty["[Frame rate]"], QString::number( vf->getFrameRate() ) + QString(" fps") );
		infoManager->setValue(idToProperty["[Duration]"], QString::number( vf->getDuration() ) + QString(" s") );
		intManager->setValue(idToProperty["[Pre-Brightness]"], vf->getBrightness() );
		intManager->setValue(idToProperty["[Pre-Contrast]"], vf->getContrast() );
		intManager->setValue(idToProperty["[Pre-Saturation]"], vf->getSaturation() );
	}
	else if (s->rtti() == Source::CAMERA_SOURCE) {

		infoManager->setValue(idToProperty["[Type]"], QLatin1String("Camera device") );
		OpencvSource *cvs = dynamic_cast<OpencvSource *>(s);
		infoManager->setValue(idToProperty["[Identifier]"], QString("OpenCV Camera %1").arg(cvs->getOpencvCameraIndex()) );
		sizeManager->setValue(idToProperty["[Frames size]"], QSize(cvs->getFrameWidth(), cvs->getFrameHeight()) );
		infoManager->setValue(idToProperty["[Frame rate]"], QString::number( cvs->getFrameRate() ) + QString(" fps") );
	}
	else if (s->rtti() == Source::RENDERING_SOURCE) {

		infoManager->setValue(idToProperty["[Type]"], QLatin1String("Rendering loop-back") );
		if (glSupportsExtension("GL_EXT_framebuffer_blit"))
			infoManager->setValue(idToProperty["[Rendering mechanism]"], "Blit to frame buffer object" );
		else
			infoManager->setValue(idToProperty["[Rendering mechanism]"], "Draw to frame buffer object" );
		sizeManager->setValue(idToProperty["[Frames size]"], QSize(RenderingManager::getInstance()->getFrameBufferWidth(), RenderingManager::getInstance()->getFrameBufferHeight()) );
		infoManager->setValue(idToProperty["[Frame rate]"], QString::number( RenderingManager::getRenderingWidget()->getFPS() / float(RenderingManager::getInstance()->getPreviousFrameDelay()) ) + QString(" fps") );
	}
	else if (s->rtti() == Source::ALGORITHM_SOURCE) {

		infoManager->setValue(idToProperty["[Type]"], QLatin1String("Algorithm") );
		AlgorithmSource *as = dynamic_cast<AlgorithmSource *>(s);
		infoManager->setValue(idToProperty["[Algorithm]"], AlgorithmSource::getAlgorithmDescription(as->getAlgorithmType()) );
		sizeManager->setValue(idToProperty["[Frames size]"], QSize(as->getFrameWidth(), as->getFrameHeight()) );
		infoManager->setValue(idToProperty["[Frame rate]"], QString::number(as->getFrameRate() ) + QString(" fps"));
	}
	else if (s->rtti() == Source::CLONE_SOURCE) {

		infoManager->setValue(idToProperty["[Type]"], QLatin1String("Clone") );
		CloneSource *cs = dynamic_cast<CloneSource *>(s);
		infoManager->setValue(idToProperty["[Clone of]"], cs->getOriginalName() );

//		qDebug("Source %d (%s) is clone of %d", cs->getId(), qPrintable(cs->getName()), (cs->getOriginalId())  );
//		qDebug("Source %d is %s", cs->getOriginalId(), qPrintable(cs->getOriginalName()) );
	}
	else {
		infoManager->setValue(idToProperty["[Type]"], QLatin1String("Captured image") );
		sizeManager->setValue(idToProperty["[Frames size]"], QSize(RenderingManager::getInstance()->getFrameBufferWidth(), RenderingManager::getInstance()->getFrameBufferHeight()) );
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

//			qDebug("name of Source %d is %s", currentItem->getId(), qPrintable(currentItem->getName()) );

			currentItem->setName(value);


//			qDebug("name of Source %d is %s", currentItem->getId(), qPrintable(currentItem->getName()) );

//			// update subproperty in clones
//			for (SourceList::iterator c = currentItem->getClones()->begin(); c != currentItem->getClones()->end(); c++){
//				QList<QtProperty *> list = (*c)->getProperty()->subProperties();
//			    QListIterator<QtProperty *> it(list);
//			    while (it.hasNext()) {
//			    	QtProperty *p = it.next();
//			    	if ( p->propertyName() == QString("Clone properties") ) {
//						CloneSource *cs = dynamic_cast<CloneSource *>(currentItem);
//						if (cs)
//							infoManager->setValue(p->subProperties().first(), cs->getOriginalName() );
//					}
//			    }
//			}


		}

    }
}


void SourcePropertyBrowser::valueChanged(QtProperty *property, const QPointF &value){

    if ( RenderingManager::getInstance()->notAtEnd(RenderingManager::getInstance()->getCurrentSource()) ) {
		Source *currentItem = *RenderingManager::getInstance()->getCurrentSource();

		if ( !property->isModified() ) {
			if ( property == idToProperty["Position"] ) {
				currentItem->setX( value.x() * SOURCE_UNIT);
				currentItem->setY( value.y() * SOURCE_UNIT);
			}
			else if ( property == idToProperty["Scale"] ) {
				currentItem->setScaleX( value.x() * SOURCE_UNIT );
				currentItem->setScaleY( value.y() * SOURCE_UNIT);
			}
		} else
			property->setModified(false);
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
		Source *currentItem = *RenderingManager::getInstance()->getCurrentSource();

		if ( !property->isModified() ) {
			if ( property == idToProperty["Depth"] ) {
				currentItem->setDepth(value);
			}
			else if ( property == idToProperty["Alpha"] ) {
				currentItem->setAlpha(value);
			}
		}
		else
			property->setModified(false);
    }
}


void SourcePropertyBrowser::valueChanged(QtProperty *property,  int value){

    if ( RenderingManager::getInstance()->notAtEnd(RenderingManager::getInstance()->getCurrentSource()) ) {
		Source *currentItem = *RenderingManager::getInstance()->getCurrentSource();

		if ( property == idToProperty["pre-Brightness"] ) {
			VideoSource *vs = dynamic_cast<VideoSource *>(currentItem);
			if (vs != 0)
				vs->getVideoFile()->setBrightness(value);
		}
		else if ( property == idToProperty["pre-Contrast"] ) {
			VideoSource *vs = dynamic_cast<VideoSource *>(currentItem);
			if (vs != 0)
				vs->getVideoFile()->setContrast(value);
		}
		else if ( property == idToProperty["pre-Saturation"] ) {
			VideoSource *vs = dynamic_cast<VideoSource *>(currentItem);
			if (vs != 0)
				vs->getVideoFile()->setSaturation(value);
		}

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
    }
}


void SourcePropertyBrowser::updateMixingProperties(){

    if ( RenderingManager::getInstance()->notAtEnd(RenderingManager::getInstance()->getCurrentSource()) ) {
		Source *currentItem = *RenderingManager::getInstance()->getCurrentSource();
		idToProperty["Alpha"]->setModified(true);
		doubleManager->setValue(idToProperty["Alpha"], currentItem->getAlpha() );
    }
}


void SourcePropertyBrowser::updateGeometryProperties(){

    if ( RenderingManager::getInstance()->notAtEnd(RenderingManager::getInstance()->getCurrentSource()) ) {
		Source *currentItem = *RenderingManager::getInstance()->getCurrentSource();
		idToProperty["Position"]->setModified(true);
		idToProperty["Scale"]->setModified(true);
		pointManager->setValue(idToProperty["Position"], QPointF( currentItem->getX() / SOURCE_UNIT, currentItem->getY() / SOURCE_UNIT));
		pointManager->setValue(idToProperty["Scale"], QPointF( currentItem->getScaleX() / SOURCE_UNIT, currentItem->getScaleY() / SOURCE_UNIT));
    }

}

void SourcePropertyBrowser::updateLayerProperties(){

    if ( RenderingManager::getInstance()->notAtEnd(RenderingManager::getInstance()->getCurrentSource()) ) {
		Source *currentItem = *RenderingManager::getInstance()->getCurrentSource();
		idToProperty["Depth"]->setModified(true);
		doubleManager->setValue(idToProperty["Depth"], currentItem->getDepth() );
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


