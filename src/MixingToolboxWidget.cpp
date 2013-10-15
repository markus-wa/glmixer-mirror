/*
 * MixingToolboxWidget.cpp
 *
 *  Created on: Sep 2, 2012
 *      Author: bh
 */

#include <QColorDialog>
#include <QtGui/QButtonGroup>
#include <QtGui/QComboBox>
#include <QtGui/QDialog>
#include <QtGui/QDialogButtonBox>
#include <QtGui/QGridLayout>
#include <QtGui/QHeaderView>
#include <QtGui/QLabel>

#include "RenderingManager.h"
#include "GammaLevelsWidget.h"
#include "SourcePropertyBrowser.h"
#include "SourceDisplayWidget.h"
#include "ViewRenderWidget.h"
#include "glmixer.h"

#ifdef FFGL
#include "FFGLPluginBrowser.h"
#endif

#include "MixingToolboxWidget.moc"

// some presets as a string of Hex values: to get from console on program exit
QByteArray static_presets =
QByteArray::fromHex("00000001000000000000000000000000000000000000000000000000000000000000000000000000000000003ff0000000000000000000010000800"
                    "6000000000000000000000000000000003ff00000000000003ff000000000000001ffffffffffffffff000000000000000000000000000000000000"
                    "0000000000ffffff9c3ff000000000000000000000000000003ff000000000000000000000000000003ff0000000000000000000000000000000000"
                    "0000001ffff0000ffff000000000000000a010000000016004400650073006100740075007200610074006500640000000100000000000000000000"
                    "0000000000000000000000000000000000000000000000000000000000003ff00000000000000000000100008006000000000000000000000000000"
                    "000003ff00000000000003ff000000000000001ffffffffffffffff0000000000000000000000000000000000000000000000000000643ff0000000"
                    "00000000000000000000003ff000000000000000000000000000003ff00000000000000000000000000000000000000001ffff0000ffff000000000"
                    "000000a01000000001c0048007900700065007200730061007400750072006100740065006400000001000000000000000000000000000000000000"
                    "000000000000000000000000000000000000000000003ff00000000000000000000100008006000000000000000000000000000000003ff00000000"
                    "000003ff000000000000001ffffffffffffffff00000000000008000000010000000f00000002ffffffecffffff9c3ff00000000000000000000000"
                    "0000003ff000000000000000000000000000003ff00000000000000000000000000028000000000001ffff0000ffff000000000000000a010000000"
                    "01c00440072006100770069006e006700200043007200610079006f006e000000010000000000000000000000000000000000000000000000000000"
                    "00000000000000000000000000003ff00000000000000000000100008006000000000000000000000000000000003ff00000000000003ff00000000"
                    "0000001ffffffffffffffff0000000000000a00000000000000000000001400000032ffffff9c3ff000000000000000000000000000003ff0000000"
                    "00000000000000000000003ff00000000000000000000000000000000000050001ffff0000ffff000000000000000a0100000000160047007200650"
                    "0790020006c006500760065006c00730000000100000000000000000000000000000000000000000000000000000000000000000000000000000000"
                    "3ff00000000000000000000100008006000000000000000000000000000000003ff00000000000003ff000000000000001ffffffffffffffff00000"
                    "00000000000000000000000110000000000000014ffffff9c3ff2ccccc000000000000000000000003ff000000000000000000000000000003ff000"
                    "00000000000000000000000000000000000001ffff0000ffff000000000000000a01000000001e00500068006f0074006f006700720061007000680"
                    "079002000420026005700000001000000000000000000000000000000000000000000000000000000000000000000000000000000003ff000000000"
                    "00000000000100008006000000000000000000000000000000003ff00000000000003ff000000000000001ffffffffffffffff00000000000000000"
                    "000000000001200000000ffffffeb00000000401195b0400000003fd5b1e5c00000003fe94f8b600000003fb99999a00000003feba2e80000000000"
                    "00000300000000000000000001ffff0000ffff000000000000000a01000000000a0044006900610070006f000000010000000000000000000000000"
                    "00000000000000000000000000000000000000000000000000000003ff0000000000000000000010000800600000000000000000000000000000000"
                    "3ff00000000000003ff000000000000001ffffffffffffffff0000000000000000000000000000000000000000000000000000003ff000000000000"
                    "000000000000000003ff000000000000000000000000000003ff00000000000000000000000000000000000000001ffff0000ffff00000000000000"
                    "0a010000000010004f0072006900670069006e0061006c0000000100000000000000000000000000000000000000000000000000000000000000000"
                    "0000000000000003ff00000000000000000000100008006000000000000000000000000000000003ff00000000000003ff000000000000001ffffff"
                    "ffffffffff00000000000001000000020000001300000000fffffff6ffffff9c3ff000000000000000000000000000003ff00000000000000000000"
                    "0000000003ff00000000000000000000000000041000000000001ffff0000ffff000000000000000a01000000001600440072006100770069006e00"
                    "6700200049006e006b00000001000000000000000000000000000000000000000000000000000000000000000000000000000000003ff0000000000"
                    "0000000000100008006000000000000000000000000000000003ff00000000000003ff000000000000001ffffffffffffffff0000000000000f0000"
                    "00000000000c0000000000000024000000003ff50bcbe00000003fb5b1e6a00000003ff000000000000000000000000000003ff0000000000000000"
                    "0000000000000000000000001ffff0000ffff000000000000000a010000000010005000610069006e00740069006e00670000000100000000000000"
                    "0000000000000000000000000000000000000000000000000000000000000000003ff00000000000000000000100008006000000000000000000000"
                    "000000000003ff00000000000003ff000000000000001ffffbcbcddddffff00000100000000000000000000000e0000000000000000000000004001"
                    "306f6000000000000000000000003ff00000000000003f829e40000000003ff00000000000000000000000000000000000000001ffff0000ffff000"
                    "000000000000a01000000001400540065006c00650076006900730069006f006e000000010000000000000000000000000000000000000000000000"
                    "00000000000000000000000000000000003ff00000000000000000000100008006000000000000000000000000000000003ff00000000000003ff00"
                    "0000000000001ffff9a9a8383686800000000000002000000000000000d0000001900000014ffffff9c3fedf3b6400000003fb99999a00000003fec"
                    "ccccc000000000000000000000003ff00000000000000000000000000000000000000001ffff0000ffff000000000000000a0100000000220050006"
                    "8006f0074006f006700720061007000680079002000530065007000690061"
);

void setPresetItemTooltip(QListWidgetItem *item, Source *source)
{
    QStringList tooltip;
    tooltip << QString("Preset '%1'\n").arg(source->getName());
    tooltip << QString("Blending\t\t%1").arg( namePresetFromInt(intFromBlendingPreset( source->getBlendFuncDestination(), source->getBlendEquation() ) )  );

    if ( source->isPixelated() )
        tooltip << QString("Pixelated\tON");
    if ( source->getColor() != QColor(255, 255, 255, 255)  )
        tooltip << QString("Color\t\t(%1,%2,%3)").arg(source->getColor().red()).arg(source->getColor().green()).arg(source->getColor().blue());
    if ( source->getMask() != 0 )
        tooltip << QString("Mask\t\t%1").arg(ViewRenderWidget::getMaskDecription()[source->getMask()].first  );
    if ( source->getInvertMode() == Source::INVERT_COLOR )
        tooltip << QString("Inversion \tRGB");
    else if ( source->getInvertMode() == Source::INVERT_LUMINANCE )
        tooltip << QString("Inversion \tLuminance");
    if ( qAbs(source->getGamma() - 1.0) > 0.001  )
        tooltip << QString("Gamma     \t%1").arg(source->getGamma());
    if ( source->getSaturation() != 0 )
        tooltip << QString("Saturation\t%1").arg(source->getSaturation());
    if ( source->getBrightness() != 0 )
        tooltip << QString("Brightness\t%1").arg(source->getBrightness());
    if ( source->getContrast() != 0 )
        tooltip << QString("Contrast  \t%1").arg(source->getContrast());
    if ( source->getHueShift() != 0 )
        tooltip << QString("Hue Shift \t%1").arg(source->getHueShift());
    if ( source->getLuminanceThreshold() != 0 )
        tooltip << QString("Threshold \t%1").arg(source->getLuminanceThreshold());
    if ( source->getNumberOfColors() != 0 )
        tooltip << QString("Posterize \t%1").arg(source->getNumberOfColors());
    if ( source->getFilter() != Source::FILTER_NONE )
        tooltip << QString("Filter\t\t%1").arg(Source::getFilterName( source->getFilter() ) );

    item->setToolTip(tooltip.join("\n"));
}



class CustomBlendingWidget : public QDialog {

public:

	QComboBox *functionBox;
	QComboBox *equationBox;

	CustomBlendingWidget(QWidget *parent, Source *s): QDialog(parent) {

		setObjectName(QString::fromUtf8("CustomBlendingWidget"));
        setWindowTitle(QObject::tr( "Custom blending"));

		QGridLayout *gridLayout = new QGridLayout(this);
		QSizePolicy sizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

        QLabel *label = new QLabel(QObject::tr("On white"), this);
        gridLayout->addWidget(label, 0, 2, 1, 1);
        label = new QLabel(QObject::tr("Transparent"), this);
        gridLayout->addWidget(label, 0, 1, 1, 1);
        label = new QLabel(QObject::tr("On black"), this);
        gridLayout->addWidget(label, 0, 0, 1, 1);

		SourceDisplayWidget *previewWhitebg = new SourceDisplayWidget(this, SourceDisplayWidget::WHITE);
		previewWhitebg->setSource(s);
        previewWhitebg->setMinimumSize(QSize(150, 100));
		previewWhitebg->setSizePolicy(sizePolicy);
        gridLayout->addWidget(previewWhitebg, 1, 2, 1, 1);
		SourceDisplayWidget *previewTransparentbg = new SourceDisplayWidget(this, SourceDisplayWidget::GRID);
		previewTransparentbg->setSource(s);
		previewTransparentbg->setMinimumSize(QSize(150, 100));
		previewTransparentbg->setSizePolicy(sizePolicy);
        gridLayout->addWidget(previewTransparentbg, 1, 1, 1, 1);
		SourceDisplayWidget *previewBlackbg = new SourceDisplayWidget(this, SourceDisplayWidget::BLACK);
		previewBlackbg->setSource(s);
		previewBlackbg->setMinimumSize(QSize(150, 100));
		previewBlackbg->setSizePolicy(sizePolicy);
        gridLayout->addWidget(previewBlackbg, 1, 0, 1, 1);

        QLabel *labelEquation = new QLabel(QObject::tr("Equation :"), this);
        gridLayout->addWidget(labelEquation, 2, 0, 1, 1);
	    equationBox = new QComboBox(this);
        equationBox->insertItems(0, QStringList()
                 << QObject::tr("Add")
                 << QObject::tr("Subtract")
                 << QObject::tr("Reverse")
                 << QObject::tr("Minimum")
                 << QObject::tr("Maximum")
        );
        gridLayout->addWidget(equationBox, 2, 1, 1, 2);

        QLabel *labelFunction = new QLabel(QObject::tr("Destination :"), this);
        gridLayout->addWidget(labelFunction, 3, 0, 1, 1);
	    functionBox = new QComboBox(this);
		functionBox->insertItems(0, QStringList()
                 << QObject::tr("Zero")
                 << QObject::tr("One")
                 << QObject::tr("Source Color")
                 << QObject::tr("Invert Source Color ")
                 << QObject::tr("Background color")
                 << QObject::tr("Invert Background Color")
                 << QObject::tr("Source Alpha")
                 << QObject::tr("Invert Source Alpha")
                 << QObject::tr("Background Alpha")
                 << QObject::tr("Invert Background Alpha")
	            );
        gridLayout->addWidget(functionBox, 3, 1, 1, 2);

        QLabel *labelWarning = new QLabel(QObject::tr("Warning: some configurations do not allow to change\nthe transparency of the source anymore.\n"), this);
        gridLayout->addWidget(labelWarning, 4, 0, 1, 3);

	    QDialogButtonBox *dialogBox = new QDialogButtonBox(this);
        dialogBox->setOrientation(Qt::Horizontal);
        dialogBox->setStandardButtons(QDialogButtonBox::Cancel|QDialogButtonBox::Ok);
        gridLayout->addWidget(dialogBox, 5, 0, 1, 3);

        QObject::connect(functionBox, SIGNAL(currentIndexChanged(int)), previewWhitebg, SLOT(setBlendingFunction(int)));
        QObject::connect(equationBox, SIGNAL(currentIndexChanged(int)), previewWhitebg, SLOT(setBlendingEquation(int)));
        QObject::connect(functionBox, SIGNAL(currentIndexChanged(int)), previewTransparentbg, SLOT(setBlendingFunction(int)));
        QObject::connect(equationBox, SIGNAL(currentIndexChanged(int)), previewTransparentbg, SLOT(setBlendingEquation(int)));
        QObject::connect(functionBox, SIGNAL(currentIndexChanged(int)), previewBlackbg, SLOT(setBlendingFunction(int)));
        QObject::connect(equationBox, SIGNAL(currentIndexChanged(int)), previewBlackbg, SLOT(setBlendingEquation(int)));

        QObject::connect(dialogBox, SIGNAL(accepted()), this, SLOT(accept()));
        QObject::connect(dialogBox, SIGNAL(rejected()), this, SLOT(reject()));

        functionBox->setCurrentIndex( intFromBlendfunction(s->getBlendFuncDestination()) );
        equationBox->setCurrentIndex( intFromBlendequation(s->getBlendEquation()) );
	}
};

MixingToolboxWidget::MixingToolboxWidget(QWidget *parent) : QWidget(parent), source(0)
{
	setupUi(this);
    setEnabled(false);

    // fill the list of masks
    QMapIterator<int, QPair<QString, QString> > i(ViewRenderWidget::getMaskDecription());
    while (i.hasNext()) {
        i.next();
        QListWidgetItem *item = new QListWidgetItem( QIcon(i.value().second), i.value().first);
        item->setToolTip(i.value().first);
        blendingMaskList->addItem( item );
    }

	// Setup the gamma levels toolbox
	gammaAdjust = new GammaLevelsWidget(this);
	Q_CHECK_PTR(gammaAdjust);
	gammaContentsLayout->addWidget(gammaAdjust);

	// hide custom blending button
    blendingCustomButton->setVisible(false);

    // make sure it is not sorting alphabetically the list
    presetsList->setSortingEnabled(false);

#ifdef FFGL
    // Setup the FFGL plugin property browser
    pluginBrowser = new FFGLPluginBrowser(Plugin);
    pluginBrowserLayout->addWidget(pluginBrowser);
#else
    mixingToolBox->removeTab( mixingToolBox->indexOf(Plugin) );
#endif

    // create default presets
    QByteArray ba = static_presets;
    QDataStream stream(&ba, QIODevice::ReadOnly);
    while(! stream.atEnd()) {
        Source *s = new Source();
        stream >> s;
        presetsList->insertItem(0, s->getName());
        _defaultPresets[presetsList->item(0)] = s;
        setPresetItemTooltip(presetsList->item(0), s);
        QFont f = presetsList->item(0)->font();
        f.setItalic(true);
        presetsList->item(0)->setFont( f );
    }
}


MixingToolboxWidget::~MixingToolboxWidget()
{
	// clean presets list
	foreach (Source *s, _defaultPresets)
		delete s;

	foreach (Source *s, _userPresets)
        delete s;

    delete gammaAdjust;

#ifdef FFGL
    delete pluginBrowser;
#endif
}

void MixingToolboxWidget::connectSource(SourceSet::iterator csi)
{
	// show or hide Filter effetcs section
    mixingToolBox->setTabEnabled(3, ViewRenderWidget::filteringEnabled());

	// connect gamma adjustment to the current source
	gammaAdjust->connectSource(csi);

	// enable / disable toolbox depending on availability of current source
	if (RenderingManager::getInstance()->isValid(csi)) {
		setEnabled(true);
		source = *csi;
        propertyChanged("Color", source->getColor());
#ifdef FFGL
        pluginBrowser->showProperties( source->getFreeframeGLPluginStack() );
#endif
	} else {
		setEnabled(false);
		presetsList->setCurrentItem(0);
		source = 0;
        propertyChanged("Color", palette().color(QPalette::Window));
#ifdef FFGL
        pluginBrowser->clear();
#endif
	}

}

void MixingToolboxWidget::setAntialiasing(bool antialiased)
{
	gammaAdjust->setAntialiasing(antialiased);
}


void MixingToolboxWidget::propertyChanged(QString propertyname, bool value)
{
	if (propertyname == "Pixelated")
		blendingPixelatedButton->setChecked(value);
}

void MixingToolboxWidget::propertyChanged(QString propertyname, int value)
{
	if (propertyname == "Mask")
		blendingMaskList->setCurrentRow(value);
	else if (propertyname == "Blending")
		blendingBox->setCurrentIndex(value);
	else if (propertyname == "Color inversion")
		EffectsInvertBox->setCurrentIndex(value);
	else if (propertyname == "Saturation")
		saturationSlider->setValue(value);
	else if (propertyname == "Brightness")
		brightnessSlider->setValue(value);
	else if (propertyname == "Contrast")
		contrastSlider->setValue(value);
	else if (propertyname == "Threshold")
		thresholdSlider->setValue(value);
	else if (propertyname == "Posterize")
		posterizeSlider->setValue(value < 1 ? 255 : value);
	else if (propertyname == "Hue shift")
		hueSlider->setValue(value);
	else if (propertyname == "Filter")
		filterList->setCurrentRow(value);

}


void MixingToolboxWidget::propertyChanged(QString propertyname, const QColor &c)
{
	if (propertyname == "Color") {
		QString stylesheet =
				QString("QPushButton{ border-width: 1px; border-style: solid; border-color: palette(button); "
						"background-color: rgb(%1, %2, %3); border-radius: 5px;}\nQPushButton:pressed { border-width: 1px; "
						"border-style: inset; border-color: palette(dark);}\n").arg(c.red()).arg(c.green()).arg(c.blue());

		blendingColorButton->setStyleSheet(stylesheet);
	}
}

void MixingToolboxWidget::on_blendingMaskList_currentRowChanged(int value)
{
	emit(enumChanged("Mask", value));
}


void MixingToolboxWidget::on_blendingBox_currentIndexChanged(int value)
{
	emit(enumChanged("Blending", value));
	blendingCustomButton->setVisible(value==0);
}

void MixingToolboxWidget::on_blendingCustomButton_pressed(){

	CustomBlendingWidget cbw(this, source);

	if (cbw.exec() == QDialog::Accepted) {
		emit(enumChanged("Equation", cbw.equationBox->currentIndex()));
		emit(enumChanged("Destination", cbw.functionBox->currentIndex()));
	}
}

void MixingToolboxWidget::on_blendingPixelatedButton_toggled(bool value){

	emit(valueChanged("Pixelated", value));
}

void MixingToolboxWidget::on_blendingColorButton_pressed() {

	if (source) {
		QColor color;
		if (GLMixer::getInstance()->useSystemDialogs())
			color = QColorDialog::getColor(source->getColor(), this);
		else
			color = QColorDialog::getColor(source->getColor(), this, "Select Color", QColorDialog::DontUseNativeDialog);

		if (color.isValid())
			emit( valueChanged("Color", color));

	}
}

void MixingToolboxWidget::on_EffectsInvertBox_currentIndexChanged(int value)
{
	emit(enumChanged("Color inversion", value));
}

void MixingToolboxWidget::on_saturationSlider_valueChanged(int value)
{
	emit(valueChanged("Saturation", value));
}

void MixingToolboxWidget::on_brightnessSlider_valueChanged(int value)
{
	emit(valueChanged("Brightness", value));
}

void MixingToolboxWidget::on_contrastSlider_valueChanged(int value)
{
	emit(valueChanged("Contrast", value));
}

void MixingToolboxWidget::on_hueSlider_valueChanged(int value)
{
	emit(valueChanged("Hue shift", value));
}

void MixingToolboxWidget::on_thresholdSlider_valueChanged(int value)
{
	emit(valueChanged("Threshold", value));
}

void MixingToolboxWidget::on_posterizeSlider_valueChanged(int value)
{
	emit(valueChanged("Posterize", value > 254 ? 0 : value));
}

void MixingToolboxWidget::on_saturationReset_pressed()
{
	saturationSlider->setValue(0);
}

void MixingToolboxWidget::on_brightnessReset_pressed()
{
	brightnessSlider->setValue(0);
}

void MixingToolboxWidget::on_contrastReset_pressed()
{
	contrastSlider->setValue(0);
}

void MixingToolboxWidget::on_hueReset_pressed()
{
	hueSlider->setValue(0);
}

void MixingToolboxWidget::on_thresholdReset_pressed()
{
	thresholdSlider->setValue(0);
}

void MixingToolboxWidget::on_posterizeReset_pressed()
{
	posterizeSlider->setValue(255);
}

void MixingToolboxWidget::on_filterList_currentRowChanged(int value)
{
	emit(enumChanged("Filter", value));
}

void MixingToolboxWidget::on_presetsList_itemDoubleClicked(QListWidgetItem *item){

	on_presetApply_pressed();
}

void MixingToolboxWidget::on_presetApply_pressed()
{
	if (source) {
		if (_defaultPresets.contains(presetsList->currentItem()) )
            source->importProperties( _defaultPresets[presetsList->currentItem()], false );
		else if ( _userPresets.contains(presetsList->currentItem()) )
            source->importProperties( _userPresets[presetsList->currentItem()], false );

        emit presetApplied( RenderingManager::getInstance()->getById( source->getId()) );
	}
}

void MixingToolboxWidget::on_presetReApply_pressed()
{    
    if (source) {
        setPresetItemTooltip(presetsList->currentItem(), source);
        _userPresets[ presetsList->currentItem() ]->importProperties(source, false);
    }
}

void MixingToolboxWidget::on_presetAdd_pressed()
{
	if (source) {
        // create list item with default name
        presetsList->insertItem(0, source->getName());
        presetsList->item(0)->setFlags( Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsEditable );

        // Set as current and offer user to edit the name
        presetsList->setCurrentItem(presetsList->item(0));
        presetsList->editItem(presetsList->item(0));

        // create a new source for this preset
        _userPresets[presetsList->item(0)] = new Source();

        // associate the properties of a source imported from the current source
		on_presetReApply_pressed();

		// ready GUI
		presetRemove->setEnabled(true);
		presetReApply->setEnabled(true);
	}
}

void MixingToolboxWidget::on_presetRemove_pressed()
{
	// take the element out of the list
	QListWidgetItem *it = presetsList->takeItem( presetsList->currentRow() );

	QMap<QListWidgetItem *, Source *>::iterator i = _userPresets.find(it);
	if ( i != _userPresets.end() ) {
		// free the source
		delete _userPresets[it];
		// remove element
		_userPresets.erase(i);
	}
}

void MixingToolboxWidget::on_presetsList_itemChanged(QListWidgetItem *item){

    if ( presetsList->findItems( item->text(), Qt::MatchFixedString ).length() > 1 )
		item->setText( item->text() + "_bis" );

    if ( _userPresets.contains(item) ) {
        // associate label to preset source name
        _userPresets[item]->setName(item->text());
        // update tooltip
        setPresetItemTooltip(item, _userPresets[item]);
    }
}


void MixingToolboxWidget::on_presetsList_currentItemChanged(QListWidgetItem *item){

	if (item) {
		presetApply->setEnabled(true);

		if ( _userPresets.contains(item) ) {
			presetRemove->setEnabled(true);
			presetReApply->setEnabled(true);
		} else {
			presetRemove->setEnabled(false);
			presetReApply->setEnabled(false);
		}
	} else {
		presetApply->setEnabled(false);
		presetRemove->setEnabled(false);
		presetReApply->setEnabled(false);
	}
}

QByteArray MixingToolboxWidget::saveState() const {
	QByteArray ba;
    QDataStream stream(&ba, QIODevice::WriteOnly);

    // store all user preset sources
    QMapIterator<QListWidgetItem *, Source *> i(_userPresets);
    while (i.hasNext()) {
        i.next();
        stream << i.value();
    }
    // for copy paste of user presets into Hex of default presets
 //   qDebug() << "User presets" << ba.toHex();

	return ba;
}


bool MixingToolboxWidget::restoreState(const QByteArray &state) {

    QByteArray sd = state;
    QDataStream stream(&sd, QIODevice::ReadOnly);

    // read all source stored and add to user presets
    while(! stream.atEnd()) {
        Source *s = new Source();
        stream >> s;
        presetsList->insertItem(0, s->getName());
        presetsList->item(0)->setFlags( Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsEditable );
        _userPresets[presetsList->item(0)] = s;
        setPresetItemTooltip(presetsList->item(0), s);
    }

    qDebug() << tr("Mixing presets restored (") << _userPresets.count() << QObject::tr(" user presets)");
	return true;
}

void MixingToolboxWidget::on_addPlugin_pressed(){

#ifdef FFGL

    #ifdef Q_OS_MAC
    QString ext = tr("Freeframe GL Plugin (*.bundle)");
    #else
    #ifdef Q_OS_WIN
    QString ext = tr("Freeframe GL Plugin (*.dll)");
    #else
    QString ext = tr("Freeframe GL Plugin (*.so)");
    #endif
    #endif
    // browse for a plugin file
    QString fileName = GLMixer::getInstance()->getFileName(tr("Open FFGL Plugin file"), ext);

    QFileInfo pluginfile(fileName);
    if (source && pluginfile.isFile()) {
        source->addFreeframeGLPlugin(fileName);
        pluginBrowser->showProperties( source->getFreeframeGLPluginStack() );
    }
#endif
}



