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
 *   Copyright 2009, 2010 Bruno Herbelin
 *
 */

#include "UserPreferencesDialog.moc"

#include "common.h"
#include "Source.h"

UserPreferencesDialog::UserPreferencesDialog(QWidget *parent): QDialog(parent)
{
    setupUi(this);
	IntroTextLabel->setVisible(false);

    // the default source property browser
    defaultSource = new Source;
    defaultProperties->showProperties(defaultSource);
    defaultProperties->setPropertyEnabled("Type", false);
    defaultProperties->setPropertyEnabled("Scale", false);
    defaultProperties->setPropertyEnabled("Depth", false);
    defaultProperties->setPropertyEnabled("Frames size", false);
    defaultProperties->setPropertyEnabled("Aspect ratio", false);

    // the rendering option for BLIT of frame buffer makes no sense if the computer does not supports it
    activateBlitFrameBuffer->setEnabled(glSupportsExtension("GL_EXT_framebuffer_blit"));

    // add a validator for folder selection in recording preference
    recordingFolderLine->setValidator(new folderValidator(this));
	recordingFolderLine->setProperty("exists", true);
    QObject::connect(recordingFolderLine, SIGNAL(textChanged(const QString &)), this, SLOT(recordingFolderPathChanged(const QString &)));
}

UserPreferencesDialog::~UserPreferencesDialog()
{
	delete defaultSource;
}


void UserPreferencesDialog::setModeMinimal(bool on)
{
	listWidget->setVisible(!on);
	IntroTextLabel->setVisible(on);

	if (on){
		resolutionTable->selectRow(0);
		stackedPreferences->setCurrentIndex(0);
        DecisionButtonBox->setStandardButtons(QDialogButtonBox::Save);
	} else {
        DecisionButtonBox->setStandardButtons(QDialogButtonBox::Cancel|QDialogButtonBox::Save);
	}
}

void UserPreferencesDialog::restoreDefaultPreferences() {

	if (stackedPreferences->currentWidget() == PageRendering) {
		resolutionTable->selectRow(0);
	    activateBlitFrameBuffer->setChecked(glSupportsExtension("GL_EXT_framebuffer_blit"));
		updatePeriod->setValue(33);
	}

	if (stackedPreferences->currentWidget() == PageRecording) {
		recordingFormatSelection->setCurrentIndex(0);
		recordingUpdatePeriod->setValue(40);
		recordingFolderBox->setChecked(false);
		recordingFolderLine->clear();
	}

	if (stackedPreferences->currentWidget() == PageSources) {
		if(defaultSource)
			delete defaultSource;
		defaultSource = new Source;
		defaultProperties->showProperties(defaultSource);

		defaultStartPlaying->setChecked(true);
		scalingModeSelection->setCurrentIndex(0);
		numberOfFramesRendering->setValue(1);
	}

	if (stackedPreferences->currentWidget() == PageInterface){
		FINE->setChecked(true);
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
	activateBlitFrameBuffer->setChecked(useBlitFboExtension);

	int tfr = 33;
	stream >> tfr;
	updatePeriod->setValue(tfr);

	// b. Read and setup the default source properties
	stream >> defaultSource;
    defaultProperties->showProperties(defaultSource);

	// c. Default scaling mode
    uint sm = 0;
    stream >> sm;
    scalingModeSelection->setCurrentIndex(sm);

    // d. DefaultPlayOnDrop
    bool DefaultPlayOnDrop = false;
    stream >> DefaultPlayOnDrop;
    defaultStartPlaying->setChecked(DefaultPlayOnDrop);

	// e.  PreviousFrameDelay
	uint  PreviousFrameDelay = 1;
	stream >> PreviousFrameDelay;
	numberOfFramesRendering->setValue( (int) PreviousFrameDelay);

	// f. Mixing icons stippling
	uint  stippling = 0;
	stream >> stippling;
	switch (stippling) {
	case 3:
		TRIANGLE->setChecked(true);
		break;
	case 2:
		CHECKERBOARD->setChecked(true);
		break;
	case 1:
		GROSS->setChecked(true);
		break;
	default:
		FINE->setChecked(true);
		break;
	}

	// g. recording format
	uint recformat = 0;
	stream >> recformat;
	recordingFormatSelection->setCurrentIndex(recformat);
	uint rtfr = 40;
	stream >> rtfr;
	recordingUpdatePeriod->setValue(rtfr > 0 ? rtfr : 40);

	// e. recording folder
	bool automaticSave = false;
	stream >> automaticSave;
	recordingFolderBox->setChecked(automaticSave);
	QString automaticSaveFolder;
	stream >> automaticSaveFolder;
	recordingFolderLine->setText(automaticSaveFolder);

}

QByteArray UserPreferencesDialog::getUserPreferences() const {

    QByteArray data;
    QDataStream stream(&data, QIODevice::WriteOnly);
    const quint32 magicNumber = MAGIC_NUMBER;
    quint16 majorVersion = QSETTING_PREFERENCE_VERSION;
	stream << magicNumber << majorVersion;

	// a. write the rendering preferences
	stream << resolutionTable->currentRow() << activateBlitFrameBuffer->isChecked();
	stream << updatePeriod->value();

	// b. Write the default source properties
	stream 	<< defaultSource;

	// c. Default scaling mode
	stream << (uint) scalingModeSelection->currentIndex();

	// d. defaultStartPlaying
	stream << defaultStartPlaying->isChecked();

	// e. PreviousFrameDelay
	stream << (uint) numberOfFramesRendering->value();

	// f. Mixing icons stippling
	if (FINE->isChecked())
		stream << (uint) 0;
	if (GROSS->isChecked())
		stream << (uint) 1;
	if (CHECKERBOARD->isChecked())
		stream << (uint) 2;
	if (TRIANGLE->isChecked())
		stream << (uint) 3;

	// g. recording format
	stream << (uint) recordingFormatSelection->currentIndex();
	stream << (uint) recordingUpdatePeriod->value();

	// e. recording folder
	stream << recordingFolderBox->isChecked();
	stream << recordingFolderLine->text();

	return data;
}


void UserPreferencesDialog::on_updatePeriod_valueChanged(int period)
{
	frameRateString->setText(QString("%1 fps").arg((int) ( 1000.0 / double(updatePeriod->value()) ) ) );
}


void UserPreferencesDialog::on_recordingUpdatePeriod_valueChanged(int period)
{
	recordingFrameRateString->setText(QString("%1 fps").arg((int) ( 1000.0 / double(recordingUpdatePeriod->value()) ) ) );
}


void UserPreferencesDialog::on_recordingFolderButton_clicked(){

	  QString dirName = QFileDialog::getExistingDirectory(this, tr("Select a directory"), recordingFolderLine->text().isEmpty()?QDir::currentPath():recordingFolderLine->text());
	  if ( ! dirName.isEmpty() )
		  recordingFolderLine->setText(dirName);

}

void UserPreferencesDialog::recordingFolderPathChanged(const QString &s)
{
	if( recordingFolderLine->hasAcceptableInput ())
		recordingFolderLine->setStyleSheet("");
	else
		recordingFolderLine->setStyleSheet("color: red");
}

