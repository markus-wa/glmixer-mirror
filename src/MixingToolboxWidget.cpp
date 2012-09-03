/*
 * MixingToolboxWidget.cpp
 *
 *  Created on: Sep 2, 2012
 *      Author: bh
 */


#include "RenderingManager.h"
#include "GammaLevelsWidget.h"

#include "MixingToolboxWidget.moc"

MixingToolboxWidget::MixingToolboxWidget(QWidget *parent) : QWidget(parent), source(0)
{
    setupUi(this);
	setEnabled(false);


	// Setup the gamma levels toolbox
	gammaAdjust = new GammaLevelsWidget(this);
    Q_CHECK_PTR(gammaAdjust);
	gammaContentsLayout->addWidget(gammaAdjust);


}

void MixingToolboxWidget::connectSource(SourceSet::iterator csi){

	gammaAdjust->connectSource(csi);

	if ( RenderingManager::getInstance()->isValid(csi) ) {
		setEnabled(true);
		source = *csi;

	} else {
		setEnabled(false);
		source = 0;
	}
}


void MixingToolboxWidget::updateSource()
{

	// set the source to preview
//	sourcepreview->setSource(*csi);
}

void MixingToolboxWidget::setAntialiasing(bool antialiased)
{
	gammaAdjust->setAntialiasing(antialiased);
}

//    // Create source preview widget
//    sourcepreview = new SourceDisplayWidget(mixingDockWidgetContent);
//    mixingDockWidgetContentSplitter->insertWidget(0, sourcepreview);
//    sourcepreview->setSizePolicy( QSizePolicy::Preferred, QSizePolicy::Expanding);
//
//    // Bidirectional link between mixing toolbox and source control
//    propertyBrowser->linkToProperty( blendeingPixelatedButton, "Pixelated");
//    propertyBrowser->linkToProperty( blendingBox, "Blending");
//    propertyBrowser->linkToProperty( blendingMaskListWidget, "Mask");
//
