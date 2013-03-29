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

#include "MixingToolboxWidget.moc"

//QMap<QString, Source *> MixingToolboxWidget::_defaultPresets;


class CustomBlendingWidget : public QDialog {

public:

	QComboBox *functionBox;
	QComboBox *equationBox;

	CustomBlendingWidget(QWidget *parent, Source *s): QDialog(parent) {

		setObjectName(QString::fromUtf8("CustomBlendingWidget"));
		setWindowTitle(tr( "Custom blending"));

		QGridLayout *gridLayout = new QGridLayout(this);
		QSizePolicy sizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

		QLabel *label = new QLabel(tr("On white"), this);
        gridLayout->addWidget(label, 0, 2, 1, 1);
		label = new QLabel(tr("Transparent"), this);
        gridLayout->addWidget(label, 0, 1, 1, 1);
		label = new QLabel(tr("On black"), this);
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

	    QLabel *labelEquation = new QLabel(tr("Equation :"), this);
        gridLayout->addWidget(labelEquation, 2, 0, 1, 1);
	    equationBox = new QComboBox(this);
        equationBox->insertItems(0, QStringList()
				 << tr("Add")
				 << tr("Subtract")
				 << tr("Reverse")
				 << tr("Minimum")
				 << tr("Maximum")
        );
        gridLayout->addWidget(equationBox, 2, 1, 1, 2);

	    QLabel *labelFunction = new QLabel(tr("Destination :"), this);
        gridLayout->addWidget(labelFunction, 3, 0, 1, 1);
	    functionBox = new QComboBox(this);
		functionBox->insertItems(0, QStringList()
	             << tr("Zero")
	             << tr("One")
	             << tr("Source Color")
	             << tr("Invert Source Color ")
	             << tr("Background color")
	             << tr("Invert Background Color")
	             << tr("Source Alpha")
	             << tr("Invert Source Alpha")
	             << tr("Background Alpha")
	             << tr("Invert Background Alpha")
	            );
        gridLayout->addWidget(functionBox, 3, 1, 1, 2);

	    QLabel *labelWarning = new QLabel(tr("Warning: some configurations do not allow to change\nthe transparency of the source anymore.\n"), this);
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

	// Setup the gamma levels toolbox
	gammaAdjust = new GammaLevelsWidget(this);
	Q_CHECK_PTR(gammaAdjust);
	gammaContentsLayout->addWidget(gammaAdjust);

	// hide custom blending button
	blendingCustomButton->setVisible(false);

	// create presets

//	_defaultPresets["Impressionnist"] = new Source();

	presetsList->insertItem(0, "Hypersaturated");
	_defaultPresets[presetsList->item(0)] = new Source();
	_defaultPresets[presetsList->item(0)]->setSaturation(100);

	presetsList->insertItem(0, "Desaturated");
	_defaultPresets[presetsList->item(0)] = new Source();
	_defaultPresets[presetsList->item(0)]->setSaturation(-100);

	presetsList->insertItem(0, "Original");
	_defaultPresets[presetsList->item(0)] = new Source();



}

MixingToolboxWidget::~MixingToolboxWidget()
{
	// clean presets list
	foreach (Source *s, _defaultPresets)
		delete s;

	foreach (Source *s, _userPresets)
		delete s;

}

void MixingToolboxWidget::connectSource(SourceSet::iterator csi)
{
	// show or hide Filter effetcs section
	mixingToolBox->setItemEnabled(3, ViewRenderWidget::filteringEnabled());

	// connect gamma adjustment to the current source
	gammaAdjust->connectSource(csi);

	// enable / disable toolbox depending on availability of current source
	if (RenderingManager::getInstance()->isValid(csi)) {
		setEnabled(true);
		source = *csi;
		propertyChanged("Color", source->getColor());
	} else {
		setEnabled(false);
		presetsList->setCurrentItem(0);
		source = 0;
		propertyChanged("Color", palette().color(QPalette::Window));
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
			source->importProperties( *_defaultPresets[presetsList->currentItem()], false );
		else if ( _userPresets.contains(presetsList->currentItem()) )
			source->importProperties( *_userPresets[presetsList->currentItem()], false );
	}
}

void MixingToolboxWidget::on_presetReApply_pressed()
{
	QStringList tooltip;
	if ( source->getColor() != QColor(255, 255, 255, 255)  )
		tooltip << QString("R%1:G%2:B%3").arg(source->getColor().red()).arg(source->getColor().green()).arg(source->getColor().blue());
	if ( source->getInvertMode() == Source::INVERT_COLOR )
		tooltip << QString("Invert RGB");
	else if ( source->getInvertMode() == Source::INVERT_LUMINANCE )
		tooltip << QString("Invert Luminance");
	if ( qAbs(source->getGamma() - 1.0) > 0.001  )
		tooltip << QString("%1 \tGamma").arg(source->getGamma());
	if ( source->getSaturation() != 0 )
		tooltip << QString("%1 % \tSaturation").arg(source->getSaturation());
	if ( source->getBrightness() != 0 )
		tooltip << QString("%1 % \tBrightness").arg(source->getBrightness());
	if ( source->getContrast() != 0 )
		tooltip << QString("%1 % \tContrast").arg(source->getContrast());
	if ( source->getHueShift() != 0 )
		tooltip << QString("%1 \tHue Shift").arg(source->getHueShift());
	if ( source->getLuminanceThreshold() != 0 )
		tooltip << QString("%1 % \tThreshold").arg(source->getLuminanceThreshold());
	if ( source->getNumberOfColors() != 0 )
		tooltip << QString("%1 \tPosterize").arg(source->getNumberOfColors());
	if ( source->getFilter() != Source::FILTER_NONE )
		tooltip << Source::getFilterName( source->getFilter() ) + " filter";

	 presetsList->currentItem()->setToolTip(tooltip.join("\n"));

	_userPresets[ presetsList->currentItem() ]->importProperties( *source, false);
}

void MixingToolboxWidget::on_presetAdd_pressed()
{
	if (source) {
		// create list item with default name
		QListWidgetItem *item = new QListWidgetItem( source->getName(), presetsList);
		item->setFlags( presetsList->item(0)->flags () | Qt::ItemIsEditable );

		// add the item to the list and offer user to edit the name
		presetsList->addItem( item );
		presetsList->setCurrentItem( item );
		presetsList->editItem( item );

		// associate the properties of a source imported from the current source
		_userPresets[item] = new Source();
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

	return ba;
}


bool MixingToolboxWidget::restoreState(const QByteArray &state) {

	return true;
}


