/*
 * MixingToolboxWidget.h
 *
 *  Created on: Sep 2, 2012
 *      Author: bh
 */

#ifndef MIXINGTOOLBOXWIDGET_H_
#define MIXINGTOOLBOXWIDGET_H_

#include <qwidget.h>
#include "ui_MixingToolboxWidget.h"


#include "Source.h"

class MixingToolboxWidget: public QWidget, public Ui::MixingToolboxWidget {

    Q_OBJECT

public:

    MixingToolboxWidget(QWidget *parent, QSettings *settings = 0);
    ~MixingToolboxWidget();
    void setAntialiasing(bool antialiased);

public slots:

    // apply preset
    void applyPreset(Source *s, QListWidgetItem *itemListPreset);
    void applyPreset(QString sourceName, QString nameItemListPreset);

    // get informed when a property is changed
    void connectSource(SourceSet::iterator);
    void propertyChanged(QString propertyname);
    void propertyChanged(QString propertyname, bool);
    void propertyChanged(QString propertyname, int);
    void propertyChanged(QString propertyname, const QColor &);

    //
    // setup connections with property manager
    //

    // Presets Page
    void on_presetsList_itemDoubleClicked(QListWidgetItem *);
    void on_presetsList_currentItemChanged(QListWidgetItem *item);
    void on_presetsList_itemChanged(QListWidgetItem *item);
    void on_presetAdd_pressed();
    void on_presetButton_clicked(bool);

    void applyPreset();
    void reApplyPreset();
    void removePreset();
    void removeAllPresets();
    void renamePreset();

    // Blending Page
    void on_blendingBox_currentIndexChanged(int);
    void on_blendingColorButton_pressed();
    void on_blendingCustomButton_pressed();
    void on_blendingPixelatedButton_toggled(bool);
    void on_blendingMaskList_currentRowChanged(int);
    void on_blendingMaskList_itemDoubleClicked(QListWidgetItem * item);
    void on_resetBlending_pressed();
    void on_blendingButton_clicked(bool);

    // Gamma page
    void on_resetGamma_pressed();
    void on_gammaButton_clicked(bool);

    // Color page
    void on_saturationSlider_valueChanged(int);
    void on_brightnessSlider_valueChanged(int);
    void on_contrastSlider_valueChanged(int);
    void on_hueSlider_valueChanged(int);
    void on_thresholdSlider_valueChanged(int);
    void on_lumakeySlider_valueChanged(int);
    void on_posterizeSlider_valueChanged(int);
    void on_saturationReset_pressed();
    void on_brightnessReset_pressed();
    void on_contrastReset_pressed();
    void on_hueReset_pressed();
    void on_thresholdReset_pressed();
    void on_lumakeyReset_pressed();
    void on_posterizeReset_pressed();
    void on_invertReset_pressed();
    void on_EffectsInvertBox_currentIndexChanged(int);
    void on_chromakeyEnable_toggled(bool);
    void on_chromakeyColor_pressed();
    void on_chromakeySlider_valueChanged(int);
    void on_resetColor_pressed();
    void on_colorButton_clicked(bool);
    void on_colorPreview_pressed();
    void on_colorPreview_released();

    // Filter page
    void on_filterList_currentRowChanged(int);
    void on_resetFilter_pressed();
    void on_filterButton_clicked(bool);

#ifdef GLM_FFGL
    // Plugin page
    void on_addPlugin_pressed();
    void on_addShadertoyPlugin_pressed();
#endif
    void on_resetPlugins_pressed();
    void on_pluginButton_clicked(bool);

    // inform something changed
    void changed();

    // state restoration
    QByteArray getPresets() const;
    bool restorePresets(const QByteArray &state);

signals:
    // inform property manager when a property is modified here
    void valueChanged(QString propertyname, bool value);
    void valueChanged(QString propertyname, int value);
    void valueChanged(QString propertyname, const QColor &value);
    void enumChanged(QString propertyname, int value);
    // inform when a preset is applied (to refresh the GUI)
    void sourceChanged(SourceSet::iterator);

private:

    class GammaLevelsWidget *gammaAdjust;
    Source *source;

    QSettings *appSettings;

    QAction *applyAction;
    QAction *reapplyAction;
    QAction *removeAction;
    QAction *renameAction;
    QAction *clearAction;


#ifdef GLM_FFGL
    class FFGLPluginBrowser *pluginBrowser;
#endif
};

#endif /* MIXINGTOOLBOXWIDGET_H_ */
