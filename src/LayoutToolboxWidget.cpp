#include "RenderingManager.h"
#include "ViewRenderWidget.h"

#include "LayoutToolboxWidget.moc"

LayoutToolboxWidget::LayoutToolboxWidget(QWidget *parent) : QWidget(parent)
{
    setupUi(this);
}

LayoutToolboxWidget::~LayoutToolboxWidget()
{

}

void LayoutToolboxWidget::setViewMode(View::viewMode a)
{
    // set status of alignment tools depending on view
    rotateBox->setVisible(a == View::GEOMETRY);
    resizeBox->setVisible(a == View::GEOMETRY);
    alignBox->setVisible(a == View::GEOMETRY || a == View::MIXING );
    distributeBox->setVisible( a != View::RENDERING );

    // dristribute tools are not available for all views
    distributeHorizontalLeftButton->setEnabled(a == View::GEOMETRY);
    distributeHorizontalRightButton->setEnabled(a == View::GEOMETRY);
    distributeHorizontalGapsButton->setEnabled(a == View::GEOMETRY);
    distributeVerticalBottomButton->setEnabled(a == View::GEOMETRY);
    distributeVerticalCenterButton->setDisabled(a == View::LAYER);
    distributeVerticalTopButton->setEnabled(a == View::GEOMETRY);
    distributeVerticalGapsButton->setEnabled(a == View::GEOMETRY);
}

void LayoutToolboxWidget::on_alignHorizontalLeftButton_clicked(){

    RenderingManager::getRenderingWidget()->alignSelection(View::AXIS_HORIZONTAL, View::ALIGN_BOTTOM_LEFT, View::REFERENCE_SOURCES);
}

void LayoutToolboxWidget::on_alignHorizontalCenterButton_clicked(){

    RenderingManager::getRenderingWidget()->alignSelection(View::AXIS_HORIZONTAL, View::ALIGN_CENTER, View::REFERENCE_SOURCES);
}

void LayoutToolboxWidget::on_alignHorizontalRightButton_clicked(){

    RenderingManager::getRenderingWidget()->alignSelection(View::AXIS_HORIZONTAL, View::ALIGN_TOP_RIGHT, View::REFERENCE_SOURCES);
}

void LayoutToolboxWidget::on_alignVerticalBottomButton_clicked(){

    RenderingManager::getRenderingWidget()->alignSelection(View::AXIS_VERTICAL, View::ALIGN_BOTTOM_LEFT, View::REFERENCE_SOURCES);
}

void LayoutToolboxWidget::on_alignVerticalCenterButton_clicked(){

    RenderingManager::getRenderingWidget()->alignSelection(View::AXIS_VERTICAL, View::ALIGN_CENTER, View::REFERENCE_SOURCES);
}

void LayoutToolboxWidget::on_alignVerticalTopButton_clicked(){

    RenderingManager::getRenderingWidget()->alignSelection(View::AXIS_VERTICAL, View::ALIGN_TOP_RIGHT, View::REFERENCE_SOURCES);
}

void LayoutToolboxWidget::on_alignHorizontalLeftFrameButton_clicked(){

    RenderingManager::getRenderingWidget()->alignSelection(View::AXIS_HORIZONTAL, View::ALIGN_BOTTOM_LEFT, View::REFERENCE_FRAME);
}

void LayoutToolboxWidget::on_alignHorizontalCenterFrameButton_clicked(){

    RenderingManager::getRenderingWidget()->alignSelection(View::AXIS_HORIZONTAL, View::ALIGN_CENTER, View::REFERENCE_FRAME);
}

void LayoutToolboxWidget::on_alignHorizontalRightFrameButton_clicked(){

    RenderingManager::getRenderingWidget()->alignSelection(View::AXIS_HORIZONTAL, View::ALIGN_TOP_RIGHT, View::REFERENCE_FRAME);
}

void LayoutToolboxWidget::on_alignVerticalBottomFrameButton_clicked(){

    RenderingManager::getRenderingWidget()->alignSelection(View::AXIS_VERTICAL, View::ALIGN_BOTTOM_LEFT, View::REFERENCE_FRAME);
}

void LayoutToolboxWidget::on_alignVerticalCenterFrameButton_clicked(){

    RenderingManager::getRenderingWidget()->alignSelection(View::AXIS_VERTICAL, View::ALIGN_CENTER, View::REFERENCE_FRAME);
}

void LayoutToolboxWidget::on_alignVerticalTopFrameButton_clicked(){

    RenderingManager::getRenderingWidget()->alignSelection(View::AXIS_VERTICAL, View::ALIGN_TOP_RIGHT, View::REFERENCE_FRAME);
}

void LayoutToolboxWidget::on_distributeHorizontalLeftButton_clicked(){

    RenderingManager::getRenderingWidget()->distributeSelection(View::AXIS_HORIZONTAL, View::ALIGN_BOTTOM_LEFT);
}

void LayoutToolboxWidget::on_distributeHorizontalCenterButton_clicked(){

    RenderingManager::getRenderingWidget()->distributeSelection(View::AXIS_HORIZONTAL, View::ALIGN_CENTER);
}

void LayoutToolboxWidget::on_distributeHorizontalGapsButton_clicked(){

    RenderingManager::getRenderingWidget()->distributeSelection(View::AXIS_HORIZONTAL, View::ALIGN_EQUAL_GAPS);
}

void LayoutToolboxWidget::on_distributeHorizontalRightButton_clicked(){

    RenderingManager::getRenderingWidget()->distributeSelection(View::AXIS_HORIZONTAL, View::ALIGN_TOP_RIGHT);
}

void LayoutToolboxWidget::on_distributeVerticalBottomButton_clicked(){

    RenderingManager::getRenderingWidget()->distributeSelection(View::AXIS_VERTICAL, View::ALIGN_BOTTOM_LEFT);
}

void LayoutToolboxWidget::on_distributeVerticalCenterButton_clicked(){

    RenderingManager::getRenderingWidget()->distributeSelection(View::AXIS_VERTICAL, View::ALIGN_CENTER);
}

void LayoutToolboxWidget::on_distributeVerticalGapsButton_clicked(){

    RenderingManager::getRenderingWidget()->distributeSelection(View::AXIS_VERTICAL, View::ALIGN_EQUAL_GAPS);
}

void LayoutToolboxWidget::on_distributeVerticalTopButton_clicked(){

    RenderingManager::getRenderingWidget()->distributeSelection(View::AXIS_VERTICAL, View::ALIGN_TOP_RIGHT);
}


void LayoutToolboxWidget::on_sizeHorizontalButton_clicked(){

    RenderingManager::getRenderingWidget()->transformSelection(View::TRANSFORM_SCALE, View::AXIS_HORIZONTAL, View::REFERENCE_SOURCES);
    // update GUI
    RenderingManager::getInstance()->refreshCurrentSource();
}

void LayoutToolboxWidget::on_sizeHorizontalFrameButton_clicked(){

    RenderingManager::getRenderingWidget()->transformSelection(View::TRANSFORM_SCALE, View::AXIS_HORIZONTAL, View::REFERENCE_FRAME);
    // update GUI
    RenderingManager::getInstance()->refreshCurrentSource();
}

void LayoutToolboxWidget::on_sizeVerticalButton_clicked(){

    RenderingManager::getRenderingWidget()->transformSelection(View::TRANSFORM_SCALE, View::AXIS_VERTICAL, View::REFERENCE_SOURCES);
    // update GUI
    RenderingManager::getInstance()->refreshCurrentSource();
}

void LayoutToolboxWidget::on_sizeVerticalFrameButton_clicked(){

    RenderingManager::getRenderingWidget()->transformSelection(View::TRANSFORM_SCALE, View::AXIS_VERTICAL, View::REFERENCE_FRAME);
    // update GUI
    RenderingManager::getInstance()->refreshCurrentSource();
}

void LayoutToolboxWidget::on_sizeRenderingAspectButton_clicked(){

    RenderingManager::getRenderingWidget()->transformSelection(View::TRANSFORM_SCALE, View::AXIS_BOTH, View::REFERENCE_FRAME);
    // update GUI
    RenderingManager::getInstance()->refreshCurrentSource();
}

void LayoutToolboxWidget::on_sizeOriginalAspectButton_clicked(){

    RenderingManager::getRenderingWidget()->transformSelection(View::TRANSFORM_SCALE, View::AXIS_BOTH, View::REFERENCE_SOURCES);
    // update GUI
    RenderingManager::getInstance()->refreshCurrentSource();
}

void LayoutToolboxWidget::on_sizePixelAspectButton_clicked(){

    RenderingManager::getRenderingWidget()->transformSelection(View::TRANSFORM_SCALE, View::AXIS_BOTH, View::REFERENCE_PIXEL);
    // update GUI
    RenderingManager::getInstance()->refreshCurrentSource();
}

void LayoutToolboxWidget::on_sizeSquareAspectButton_clicked(){

    RenderingManager::getRenderingWidget()->transformSelection(View::TRANSFORM_SCALE, View::AXIS_HORIZONTAL, View::REFERENCE_PIXEL);
    // update GUI
    RenderingManager::getInstance()->refreshCurrentSource();
}

void LayoutToolboxWidget::on_rotateReset_clicked(){

    RenderingManager::getRenderingWidget()->transformSelection(View::TRANSFORM_ROTATE, View::AXIS_HORIZONTAL, View::REFERENCE_FRAME);
    // update GUI
    RenderingManager::getInstance()->refreshCurrentSource();
}

void LayoutToolboxWidget::on_rotateClockwiseButton_clicked(){

    RenderingManager::getRenderingWidget()->transformSelection(View::TRANSFORM_ROTATE, View::AXIS_HORIZONTAL, View::REFERENCE_SOURCES);
    // update GUI
    RenderingManager::getInstance()->refreshCurrentSource();
}

void LayoutToolboxWidget::on_rotateCounterclockwiseButton_clicked(){

    RenderingManager::getRenderingWidget()->transformSelection(View::TRANSFORM_ROTATE, View::AXIS_VERTICAL, View::REFERENCE_SOURCES);
    // update GUI
    RenderingManager::getInstance()->refreshCurrentSource();
}

void LayoutToolboxWidget::on_flipHorizontalButton_clicked(){

    RenderingManager::getRenderingWidget()->transformSelection(View::TRANSFORM_FLIP, View::AXIS_HORIZONTAL, View::REFERENCE_SOURCES);
    // update GUI
    RenderingManager::getInstance()->refreshCurrentSource();
}

void LayoutToolboxWidget::on_flipVerticalButton_clicked(){

    RenderingManager::getRenderingWidget()->transformSelection(View::TRANSFORM_FLIP, View::AXIS_VERTICAL, View::REFERENCE_SOURCES);
    // update GUI
    RenderingManager::getInstance()->refreshCurrentSource();
}

void LayoutToolboxWidget::on_flipReset_clicked(){

    RenderingManager::getRenderingWidget()->transformSelection(View::TRANSFORM_FLIP, View::AXIS_VERTICAL, View::REFERENCE_FRAME);
    // update GUI
    RenderingManager::getInstance()->refreshCurrentSource();
}
