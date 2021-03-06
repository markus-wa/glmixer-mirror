/*
 * UserPreferencesDialog.cpp
 *
 *  Created on: Jul 16, 2010
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

#include "UserPreferencesDialog.moc"

#include "common.h"
#include "glmixer.h"
#include "Source.h"
#include "OutputRenderWindow.h"
#include "VideoFile.h"
#include "RenderingEncoder.h"
#include "CodecManager.h"

#include <QFileDialog>
#include <QApplication>
#include <QDesktopWidget>
//#include <QDesktopServices>

UserPreferencesDialog::UserPreferencesDialog(QWidget *parent): QDialog(parent)
{
    setupUi(this);
    IntroTextLabel->setVisible(false);
    titleTextLabel->setVisible(false);

    // the default source properties
    defaultSource = new Source();

    // the rendering option for BLIT of frame buffer makes no sense if the computer does not supports it
    disableBlitFrameBuffer->setEnabled( glewIsSupported("GL_EXT_framebuffer_blit GL_EXT_framebuffer_multisample") );
    disablePixelBufferObject->setEnabled( glewIsSupported("GL_ARB_pixel_buffer_object") );

    // add a validator for folder selection in recording preference
    recordingFolderLine->setValidator(new folderValidator(this));
    recordingFolderLine->setProperty("exists", true);
    QObject::connect(recordingFolderLine, SIGNAL(textChanged(const QString &)), this, SLOT(recordingFolderPathChanged(const QString &)));

    // connect to output preview
    QObject::connect(disablePixelBufferObject, SIGNAL(toggled(bool)), previewOutput, SLOT(disablePBO(bool)));
    QObject::connect(disableBlitFrameBuffer, SIGNAL(toggled(bool)), previewOutput, SLOT(disableBlitFBO(bool)));
    QObject::connect(updatePeriod, SIGNAL(valueChanged(int)), previewOutput, SLOT(setUpdatePeriod(int)));

    // TODO fill in the list of available languages


#ifndef GLM_SHM
    sharedMemoryBox->setVisible(false);
#endif

    // fill in list of modes of update intervals
    updateIntervalModes.append(16);
    updateIntervalModes.append(20);
    updateIntervalModes.append(25);
    updateIntervalModes.append(33);
    updateIntervalModes.append(40);
    updateIntervalModes.append(50);
    updateIntervalModes.append(80);
    updateIntervalModes.append(100);
    updateIntervalModes.append(200);
    updateIntervalModes.append(500);
    updateIntervalModes.append(1000);
}

UserPreferencesDialog::~UserPreferencesDialog()
{
    delete defaultSource;
}

void UserPreferencesDialog::showEvent(QShowEvent *e){

    // update labels
    on_updatePeriod_valueChanged( updatePeriod->value() );

    // (re)set number of available monitors
    if (OutputRenderWindow::getInstance()->getFullScreenCount() != fullscreenMonitor->count()) {
        fullscreenMonitor->clear();
        for( int i = 0; i < OutputRenderWindow::getInstance()->getFullScreenCount(); ++i)
            fullscreenMonitor->addItem(QString("Monitor %1").arg(i+1));
    }
    // make sure we display the correct index
    int qwe = OutputRenderWindow::getInstance()->getFullScreenMonitor();
    fullscreenMonitor->setCurrentIndex(qwe);

    // refresh opengl preview
    previewOutput->setUpdatePeriod(updatePeriod->value());

    // improve table appearance
    resolutionTable->horizontalHeader()->resizeSections(QHeaderView::Stretch);
    resolutionTable->verticalHeader()->resizeSections(QHeaderView::Stretch);

    QWidget::showEvent(e);
}

void UserPreferencesDialog::hideEvent(QHideEvent *e){

    // suspend opengl preview
    previewOutput->setUpdatePeriod(0);

    QWidget::hideEvent(e);
}

void UserPreferencesDialog::setModeMinimal(bool on)
{
    listWidget->setVisible(!on);
    factorySettingsButton->setVisible(!on);
    IntroTextLabel->setVisible(on);
    titleTextLabel->setVisible(on);

    if (on){
        stackedPreferences->setCurrentIndex(0);
        DecisionButtonBox->setStandardButtons(QDialogButtonBox::Save);
    } else {
        DecisionButtonBox->setStandardButtons(QDialogButtonBox::Cancel|QDialogButtonBox::Save);
        // try to adjust the size to fit content
        adjustSize();
    }

}


void UserPreferencesDialog::factoryResetPreferences() {

    QString msg = tr("Do you want to reset GLMixer to factory settings and appearance?");
    if ( QMessageBox::question(this, tr("%1 - Are you sure?").arg(QCoreApplication::applicationName()), msg,
    QMessageBox::Yes | QMessageBox::No, QMessageBox::Yes) == QMessageBox::Yes) {
        // reset all
        restoreAllDefaultPreferences();
        GLMixer::getInstance()->on_actionResetToolbars_triggered();
    }
}

void UserPreferencesDialog::restoreAllDefaultPreferences() {

    // reset default for every preference page
    for (int r = listWidget->count(); r >= 0; listWidget->setCurrentRow(r--))
        restoreDefaultPreferences();
}

void UserPreferencesDialog::restoreDefaultPreferences() {

    if (stackedPreferences->currentWidget() == PageRendering) {
        resolutionTable->selectRow(1); // default to HD
        updatePeriod->setValue(20); // default fps at 50
        on_updatePeriod_valueChanged( updatePeriod->value() );
        disableFiltering->setChecked(false);
        disableBlitFrameBuffer->setChecked(!GLEW_EXT_framebuffer_blit);
        disablePixelBufferObject->setChecked(!GLEW_EXT_pixel_buffer_object);
        disableHWCodec->setChecked(false);
    }

    if (stackedPreferences->currentWidget() == PageRecording) {
        fullscreenMonitor->setCurrentIndex(0);
        outputSkippedFrames->setValue(1);
        on_outputSkippedFrames_valueChanged( outputSkippedFrames->value() );
        disableOutputRecording->setChecked(false);
        recordingFormatSelection->setCurrentIndex(0);
        recordingQualitySelection->setCurrentIndex(0);
        recordingFramerateMode->setValue(4);
        recordingFolderBox->setChecked(false);
        recordingFolderLine->clear();
        sharedMemoryColorDepth->setCurrentIndex(0);
        recordingBufferSize->setValue(10);
        outputFadingDuration->setValue(500);
    }

    if (stackedPreferences->currentWidget() == PageSources) {
        if(defaultSource)
            delete defaultSource;
        defaultSource = new Source();
        sourceDefaultName->setText(defaultSource->getName());
        sourceDefaultAspectRatio->setChecked( defaultSource->isFixedAspectRatio());
        sourceDefaultBlending->setCurrentIndex( intFromBlendingPreset( defaultSource->getBlendFuncDestination(), defaultSource->getBlendEquation() ) - 1);

        defaultStartPlaying->setChecked(true);
        scalingModeSelection->setCurrentIndex(0);
        loopbackSkippedFrames->setValue(1);
        on_loopbackSkippedFrames_valueChanged( loopbackSkippedFrames->value() );

        MemoryUsagePolicySlider->setValue(DEFAULT_MEMORY_USAGE_POLICY);
        displayTimeAsFrame->setChecked(false);
    }

    if (stackedPreferences->currentWidget() == PageInterface){
        ButtonTestFrame->reset();
        speedZoom->setValue(120);
        centeredZoom->setChecked(false);
        selectionViewContextMenu->setCurrentIndex(0);
    }

    if (stackedPreferences->currentWidget() == PageOptions){
        stipplingSlider->setValue(10);
        antiAliasing->setChecked(true);
        displayFramerate->setChecked(false);
        restoreLastSession->setChecked(true);
        useCustomDialogs->setChecked(true);
        saveExitSession->setChecked(false);
        iconSizeSlider->setValue(50);
        maximumUndoLevels->setValue(100);
        snapTool->setChecked(false);
        allowOneInstance->setChecked(true);
        useCustomTimer->setChecked(false);
    }
}

void UserPreferencesDialog::showPreferences(const QByteArray & state){

    if (state.isEmpty())
            return;

    QByteArray sd = state;
    QDataStream stream(&sd, QIODevice::ReadOnly);

    const quint32 magicNumber = MAGIC_NUMBER;
    const quint16 currentMajorVersion = QSETTING_PREFERENCE_VERSION;
    quint32 storedMagicNumber;
    quint16 majorVersion = 0;
    stream >> storedMagicNumber >> majorVersion;
    if (storedMagicNumber != magicNumber || majorVersion != currentMajorVersion)
        return;

    // a. Read and show the rendering preferences
    uint RenderingQuality;
    stream  >> RenderingQuality;
    resolutionTable->selectRow(RenderingQuality);

    bool useBlitFboExtension = true;
    stream >> useBlitFboExtension;
    disableBlitFrameBuffer->setChecked(!useBlitFboExtension);

    int tfr = 16;
    stream >> tfr;
    updatePeriod->setValue(tfr);

    // b. Read and setup the default source properties
    stream >> defaultSource;
    sourceDefaultName->setText(defaultSource->getName());
    sourceDefaultAspectRatio->setChecked( defaultSource->isFixedAspectRatio());
    sourceDefaultBlending->setCurrentIndex( intFromBlendingPreset( defaultSource->getBlendFuncDestination(), defaultSource->getBlendEquation() ) - 1 );

    // c. Default scaling mode
    uint sm = 0;
    stream >> sm;
    scalingModeSelection->setCurrentIndex(sm);

    // d. DefaultPlayOnDrop
    bool DefaultPlayOnDrop = false;
    stream >> DefaultPlayOnDrop;
    defaultStartPlaying->setChecked(DefaultPlayOnDrop);

    // e.  PreviousFrameDelay
    uint  previous_frame_period = 1;
    stream >> previous_frame_period;
    loopbackSkippedFrames->setValue( (int) previous_frame_period);

    // f. Mixing icons stippling
    uint  stippling = 0;
    stream >> stippling;
    stipplingSlider->setValue(stippling / 10);

    // g. recording format
    uint recformat = 0;
    stream >> recformat;
    recordingFormatSelection->setCurrentIndex(recformat);
    uint rtfr = 40;
    stream >> rtfr;
    recordingFramerateMode->setValue( getModeFromRecordingUpdateInterval(rtfr) );

    // h. recording folder
    bool automaticSave = false;
    stream >> automaticSave;
    recordingFolderBox->setChecked(automaticSave);
    QString automaticSaveFolder;
    stream >> automaticSaveFolder;
    recordingFolderLine->setText(automaticSaveFolder);

    // i. disable filtering
    bool disablefilter = false;
    stream >> disablefilter;
    disableFiltering->setChecked(disablefilter);

    // j. antialiasing
    bool antialiasing = true;
    stream >> antialiasing;
    antiAliasing->setChecked(antialiasing);

    // k. mouse buttons and modifiers
    QMap<int, int> mousemap;
    stream >> mousemap;
    QMap<int, int> modifiermap;
    stream >> modifiermap;
    ButtonTestFrame->setConfiguration(mousemap, modifiermap);

    // l. zoom config
    int zoomspeed = 120;
    stream >> zoomspeed;
    speedZoom->setValue(zoomspeed);
    bool zoomcentered = true;
    stream >> zoomcentered;
    centeredZoom->setChecked(zoomcentered);

    // m. useCustomDialogs
    bool usesystem = false;
    stream >> usesystem;
    useCustomDialogs->setChecked(!usesystem);

    // n. shared memory depth
    uint shmdepth = 0;
    stream >> shmdepth;
    sharedMemoryColorDepth->setCurrentIndex(shmdepth);

    // o. fullscreen monitor index
    int monitorindex = 0;
    stream >> monitorindex;
    fullscreenMonitor->setCurrentIndex(monitorindex);

    // p. options
    bool fs, taf, rs = false;
    stream >> fs >> taf >> rs;
    displayFramerate->setChecked(fs);
    displayTimeAsFrame->setChecked(taf);
    restoreLastSession->setChecked(rs);

    // q. view context menu
    int vcm = 0;
    stream >> vcm;
    selectionViewContextMenu->setCurrentIndex(vcm);

    // r. Memory usage policy
    int mem = 50;
    stream >> mem;
    MemoryUsagePolicySlider->setValue(mem);

    // s. save session on exit
    bool save = false;
    stream >> save;
    saveExitSession->setChecked(save);

    // t. disable PBO
    bool usePBO = true;
    bool disableoutput = false;
    stream >> usePBO >> disableoutput;
    disablePixelBufferObject->setChecked(!usePBO);
    disableOutputRecording->setChecked(disableoutput);

    // u. recording buffer
    int percent = 20;
    stream >> percent;
    recordingBufferSize->setValue(percent);

    // v. icon size
    int isize = 50;
    stream >> isize;
    iconSizeSlider->setValue(isize);

    // w. Undo level
    int undolevel = 100;
    stream >> undolevel;
    maximumUndoLevels->setValue(undolevel);

    // x.  output frame periodicity
    uint  display_frame_period = 1;
    stream >> display_frame_period;
    outputSkippedFrames->setValue( (int) display_frame_period);

    // y. recording quality
    uint recquality = 0;
    stream >> recquality;
    recordingQualitySelection->setCurrentIndex(recquality);
    recordingQualitySelection->setEnabled(recformat < 5);

    // z. snap tools
    bool snap = true;
    stream >> snap;
    snapTool->setChecked(snap);

    // aa. Single Instance & custom timer
    bool oneinstance = true, customtimer = false;
    stream >> oneinstance >> customtimer;
    allowOneInstance->setChecked(oneinstance);
    useCustomTimer->setChecked(customtimer);

    // ab. Hardware Codec
    bool hwcodec = true;
    stream >> hwcodec;
    disableHWCodec->setChecked(!hwcodec);

    // ac. Output fading duration
    int duration = 500;
    stream >> duration;
    outputFadingDuration->setValue(duration);
}

QByteArray UserPreferencesDialog::getUserPreferences() const {

    QByteArray data;
    QDataStream stream(&data, QIODevice::WriteOnly);
    const quint32 magicNumber = MAGIC_NUMBER;
    quint16 majorVersion = QSETTING_PREFERENCE_VERSION;
    stream << magicNumber << majorVersion;

    // a. write the rendering preferences
    stream << resolutionTable->currentRow() << !disableBlitFrameBuffer->isChecked();
    stream << updatePeriod->value();

    // b. Write the default source properties
    stream 	<< defaultSource;

    // c. Default scaling mode
    stream << (uint) scalingModeSelection->currentIndex();

    // d. defaultStartPlaying
    stream << defaultStartPlaying->isChecked();

    // e. PreviousFrameDelay
    stream << (uint) loopbackSkippedFrames->value();

    // f. Mixing icons stippling
    stream << (uint) stipplingSlider->value() * 10;

    // g. recording format
    stream << (uint) recordingFormatSelection->currentIndex();
    stream <<  getRecordingUpdateIntervalFromMode( recordingFramerateMode->value() );

    // h. recording folder
    stream << recordingFolderBox->isChecked();
    stream << recordingFolderLine->text();

    // i. disable filter
    stream << disableFiltering->isChecked();

    // j. antialiasing
    stream << antiAliasing->isChecked();

    // k. mouse buttons and modifiers
    stream << View::getMouseButtonsMap(ButtonTestFrame->buttonMap());
    stream << View::getMouseModifiersMap(ButtonTestFrame->modifierMap());

    // l. zoom config
    stream << speedZoom->value();
    stream << centeredZoom->isChecked();

    // m. useCustomDialogs
    stream << !useCustomDialogs->isChecked();

    // n. shared memory depth
    stream << (uint) sharedMemoryColorDepth->currentIndex();

    // o. fullscreen monitor index
    stream << (uint) fullscreenMonitor->currentIndex();

    // p. options
    stream << displayFramerate->isChecked() << displayTimeAsFrame->isChecked() << restoreLastSession->isChecked();

    // q. view context menu
    stream << selectionViewContextMenu->currentIndex();

    // r. memory usage policy
    stream << MemoryUsagePolicySlider->value();

    // s. save session on exit
    stream << saveExitSession->isChecked();

    // t. disable pbo
    stream << !disablePixelBufferObject->isChecked();
    stream << disableOutputRecording->isChecked();

    // u. recording buffer
    stream << recordingBufferSize->value();

    // v. icon size
    stream << iconSizeSlider->value();

    // w. Undo level
    stream << maximumUndoLevels->value();

    // x. output frame periodicity
    stream << (uint) outputSkippedFrames->value();

    // y. recording quality
    stream << (uint) recordingQualitySelection->currentIndex();

    // z. snap tools
    stream << snapTool->isChecked();

    // aa. Single Instance & custom timer
    stream << allowOneInstance->isChecked();
    stream << useCustomTimer->isChecked();

    // ab. Hardware Codec
    stream << !disableHWCodec->isChecked();

    // ac. Output fading duration
    stream << outputFadingDuration->value();

    return data;
}


void UserPreferencesDialog::on_updatePeriod_valueChanged(int period)
{
    double fps = qBound(1.0, 1000.0 / double(period), 62.0);
    frameRateString->setText(QString("%1 fps").arg(qRound(fps)) );
    on_loopbackSkippedFrames_valueChanged( loopbackSkippedFrames->value() );
    on_outputSkippedFrames_valueChanged( outputSkippedFrames->value() );
}


void UserPreferencesDialog::on_recordingFormatSelection_currentIndexChanged(int i)
{
    recordingQualitySelection->setEnabled(i < 5);
}

void UserPreferencesDialog::on_recordingFramerateMode_valueChanged(int mode)
{
    int interval = getRecordingUpdateIntervalFromMode(mode);
    double fps = qBound(1.0, 1000.0 / double(interval), 60.0);
    recordingFrameRateString->setText(QString("%1 fps").arg(qRound(fps)) );
}


void UserPreferencesDialog::on_recordingFolderButton_clicked(){

    QString dirName = QFileDialog::getExistingDirectory(this, QObject::tr("Select a directory"),
                              recordingFolderLine->text().isEmpty() ? QDesktopServices::storageLocation(QDesktopServices::MoviesLocation) : recordingFolderLine->text(),
                              GLMixer::getInstance()->useSystemDialogs() ? QFileDialog::ShowDirsOnly : QFileDialog::ShowDirsOnly | QFileDialog::DontUseNativeDialog);
    if ( ! dirName.isEmpty() )
      recordingFolderLine->setText(dirName);

}

void UserPreferencesDialog::recordingFolderPathChanged(const QString &s)
{
    if( recordingFolderLine->hasAcceptableInput ())
        recordingFolderLine->setStyleSheet("");
    else
        recordingFolderLine->setStyleSheet("color: rgb(135, 0, 2)");
}

// TODO ; GUI configuration for key shortcuts
//// List of actions registered in GLMixer
//QList<QAction *>actions = getActionsList( GLMixer::getInstance()->actions() );
//actions += getActionsList( GLMixer::getInstance()->menuBar()->actions() );
//qDebug("%d actions registered",actions.length());

QList<QAction *> UserPreferencesDialog::getActionsList(QList<QAction *> actionlist)
{
    QList<QAction *> buildlist;
     for (int i = 0; i < actionlist.size(); ++i) {
         if (actionlist.at(i)->menu())
             buildlist += getActionsList(actionlist.at(i)->menu()->actions());
         else if (!actionlist.at(i)->isSeparator())
             buildlist += actionlist.at(i);
     }
     return buildlist;
}


void UserPreferencesDialog::on_MemoryUsagePolicySlider_valueChanged(int mem)
{
    MemoryUsageMaximumLabel->setText(QString("%1 MB").arg(VideoFile::getMemoryUsageMaximum(mem)));
}


void UserPreferencesDialog::on_loopbackSkippedFrames_valueChanged(int i)
{
    double fps = qBound(1.0, 1000.0 / double(updatePeriod->value()), 62.0);
    fps /= (double) i;

    loopbackFPS->setText( QString("%1 fps").arg(QString::number(qRound(fps*2.0)/2.0, 'f', 1 )));
}

void UserPreferencesDialog::on_outputSkippedFrames_valueChanged(int i)
{
    double fps = qBound(1.0, 1000.0 / double(updatePeriod->value()), 62.0);
    fps /= (double) i;

    outputFPS->setText( QString("%1 fps").arg(QString::number(qRound(fps*2.0)/2.0, 'f', 1 )));
}


void UserPreferencesDialog::on_recordingBufferSize_valueChanged(int percent)
{
    recordingBuffersizeString->setText(getByteSizeString(RenderingEncoder::computeBufferSize(percent)));
}

void UserPreferencesDialog::on_sourceDefaultBlending_currentIndexChanged(int i)
{
    QPair<int, int> preset = blendingPresetFromInt(i+1);
    defaultSource->setBlendFunc(GL_SRC_ALPHA, blendfunctionFromInt( preset.first ) );
    defaultSource->setBlendEquation( blendequationFromInt( preset.second ) );
}

void UserPreferencesDialog::on_sourceDefaultAspectRatio_toggled(bool on)
{
    defaultSource->setFixedAspectRatio(on);
}

void UserPreferencesDialog::on_sourceDefaultName_textEdited(const QString & text)
{
    defaultSource->setName(text);
}

uint UserPreferencesDialog::getRecordingUpdateIntervalFromMode(int m) const
{
    return updateIntervalModes[m];
}

int UserPreferencesDialog::getModeFromRecordingUpdateInterval(uint u) const
{
    int i = updateIntervalModes.indexOf(u);
    // all good, found the right index
    if (i>0)
        return i;
    // browse to find closest index
    for (i = updateIntervalModes.size()-1; i > 0; --i) {
        if ( updateIntervalModes[i] < u )
            break;
    }
    return i;
}
