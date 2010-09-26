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

#include "Source.h"

UserPreferencesDialog::UserPreferencesDialog(QWidget *parent): QDialog(parent), m_iconSize(64, 64)
{
    setupUi(this);

    // create the curves into the transition easing curve selector
    easingCurvePicker->setIconSize(m_iconSize);
    easingCurvePicker->setMinimumHeight(m_iconSize.height() + 50);
    createCurveIcons();

    // the default source property browser
    defaultSource = new Source;
    defaultProperties->showProperties(defaultSource);
    defaultProperties->setPropertyEnabled("Type", false);
    defaultProperties->setPropertyEnabled("Scale", false);
    defaultProperties->setPropertyEnabled("Depth", false);
    defaultProperties->setPropertyEnabled("Frames size", false);
    defaultProperties->setPropertyEnabled("Aspect ratio", false);

    activateBlitFrameBuffer->setEnabled(glSupportsExtension("GL_EXT_framebuffer_blit"));

}

UserPreferencesDialog::~UserPreferencesDialog()
{
	delete defaultSource;
}

void UserPreferencesDialog::createCurveIcons()
{
    QPixmap pix(m_iconSize);
    QPainter painter(&pix);
    QLinearGradient gradient(0,0, 0, m_iconSize.height());
    gradient.setColorAt(0.0, QColor(240, 240, 240));
    gradient.setColorAt(1.0, QColor(224, 224, 224));
    QBrush brush(gradient);
    const QMetaObject &mo = QEasingCurve::staticMetaObject;
    QMetaEnum metaEnum = mo.enumerator(mo.indexOfEnumerator("Type"));
    // Skip QEasingCurve::Custom
    for (int i = 0; i < QEasingCurve::NCurveTypes - 3; ++i) {
        painter.fillRect(QRect(QPoint(0, 0), m_iconSize), brush);
        QEasingCurve curve((QEasingCurve::Type)i);
        painter.setPen(QColor(0, 0, 255, 64));
        qreal xAxis = m_iconSize.height()/1.5;
        qreal yAxis = m_iconSize.width()/3;
        painter.drawLine(0, xAxis, m_iconSize.width(),  xAxis);
        painter.drawLine(yAxis, 0, yAxis, m_iconSize.height());

        qreal curveScale = m_iconSize.height()/2;

        painter.setPen(Qt::NoPen);

        // start point
        painter.setBrush(Qt::red);
        QPoint start(yAxis, xAxis - curveScale * curve.valueForProgress(0));
        painter.drawRect(start.x() - 1, start.y() - 1, 3, 3);

        // end point
        painter.setBrush(Qt::blue);
        QPoint end(yAxis + curveScale, xAxis - curveScale * curve.valueForProgress(1));
        painter.drawRect(end.x() - 1, end.y() - 1, 3, 3);

        QPainterPath curvePath;
        curvePath.moveTo(start);
        for (qreal t = 0; t <= 1.0; t+=1.0/curveScale) {
            QPoint to;
            to.setX(yAxis + curveScale * t);
            to.setY(xAxis - curveScale * curve.valueForProgress(t));
            curvePath.lineTo(to);
        }
        painter.setRenderHint(QPainter::Antialiasing, true);
        painter.strokePath(curvePath, QColor(32, 32, 32));
        painter.setRenderHint(QPainter::Antialiasing, false);
        QListWidgetItem *item = new QListWidgetItem;
        item->setIcon(QIcon(pix));
        item->setText(metaEnum.key(i));
        easingCurvePicker->addItem(item);
    }
}


void UserPreferencesDialog::restoreDefaultPreferences() {

	if (stackedPreferences->currentWidget() == PageRendering) {
		r640x480->setChecked(true);
	    activateBlitFrameBuffer->setChecked(glSupportsExtension("GL_EXT_framebuffer_blit"));
		updatePeriod->setValue(16);
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
		transitionDuration->setValue(1000);
		easingCurvePicker->setCurrentRow(3);
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
	QSize RenderingSize;
	stream  >> RenderingSize;
	sizeToSelection(RenderingSize);

	bool useBlitFboExtension = true;
	stream >> useBlitFboExtension;
	activateBlitFrameBuffer->setChecked(useBlitFboExtension);

	int tfr = 16;
	stream >> tfr;
	updatePeriod->setValue(tfr);

	// b. Read and setup the default source properties
	stream >> defaultSource;
    defaultProperties->showProperties(defaultSource);

	// c. Default scaling mode
    unsigned int sm = 0;
    stream >> sm;
    scalingModeSelection->setCurrentIndex(sm);

    // d. DefaultPlayOnDrop
    bool DefaultPlayOnDrop = false;
    stream >> DefaultPlayOnDrop;
    defaultStartPlaying->setChecked(DefaultPlayOnDrop);

	// e.  PreviousFrameDelay
	unsigned int  PreviousFrameDelay = 1;
	stream >> PreviousFrameDelay;
	numberOfFramesRendering->setValue( (unsigned int) PreviousFrameDelay);

	// f. Mixing icons stippling
	unsigned int  stippling = 0;
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

	// g. transition time and Easing curve
	int duration = 0;
	stream >> duration;
	transitionDuration->setValue(duration);
	int curve = 1;
	stream >> curve;
	easingCurvePicker->setCurrentRow(curve);
}

QByteArray UserPreferencesDialog::getUserPreferences() const {

    QByteArray data;
    QDataStream stream(&data, QIODevice::WriteOnly);
    const quint32 magicNumber = MAGIC_NUMBER;
    quint16 majorVersion = QSETTING_PREFERENCE_VERSION;
	stream << magicNumber << majorVersion;

	// a. write the rendering preferences
	stream << selectionToSize() << activateBlitFrameBuffer->isChecked();
	stream << updatePeriod->value();

	// b. Write the default source properties
	stream 	<< defaultSource;

	// c. Default scaling mode
	stream << (unsigned int) scalingModeSelection->currentIndex();

	// d. defaultStartPlaying
	stream << defaultStartPlaying->isChecked();

	// e. PreviousFrameDelay
	stream << (unsigned int) numberOfFramesRendering->value();

	// f. Mixing icons stippling
	if (FINE->isChecked())
		stream << (unsigned int) 0;
	if (GROSS->isChecked())
		stream << (unsigned int) 1;
	if (CHECKERBOARD->isChecked())
		stream << (unsigned int) 2;
	if (TRIANGLE->isChecked())
		stream << (unsigned int) 3;

	// g. transition time and Easing curve
	stream << transitionDuration->value();
	stream << easingCurvePicker->currentRow();

	return data;
}

void UserPreferencesDialog::sizeToSelection(QSize s){

	if (s==QSize(640, 480)) r640x480->setChecked(true);
    if (s==QSize(600, 400)) r600x400->setChecked(true);
    if (s==QSize(640, 400)) r640x400->setChecked(true);
    if (s==QSize(854, 480)) r854x480->setChecked(true);
    if (s==QSize(768, 576)) r768x576->setChecked(true);
    if (s==QSize(720, 480)) r720x480->setChecked(true);
    if (s==QSize(800, 600)) r800x600->setChecked(true);
    if (s==QSize(960, 600)) r960x600->setChecked(true);
    if (s==QSize(1042, 768)) r1042x768->setChecked(true);
    if (s==QSize(1152, 768)) r1152x768->setChecked(true);
    if (s==QSize(1280, 800)) r1280x800->setChecked(true);
    if (s==QSize(1280, 720)) r1280x720->setChecked(true);
    if (s==QSize(1600, 1200)) r1600x1200->setChecked(true);
    if (s==QSize(1920, 1200)) r1920x1200->setChecked(true);
    if (s==QSize(1920, 1080)) r1920x1080->setChecked(true);

}

QSize UserPreferencesDialog::selectionToSize() const {

    if(r600x400->isChecked())
    	return QSize(600, 400);
    if(r640x400->isChecked())
    	return QSize(640, 400);
    if(r854x480->isChecked())
    	return QSize(854, 480);
    if(r768x576->isChecked())
    	return QSize(798, 576);
    if(r720x480->isChecked())
    	return QSize(720, 480);
    if(r800x600->isChecked())
    	return QSize(800, 600);
    if(r960x600->isChecked())
    	return QSize(960, 600);
    if(r1042x768->isChecked())
    	return QSize(1024, 768);
    if(r1152x768->isChecked())
    	return QSize(1152, 768);
    if(r1280x800->isChecked())
    	return QSize(1280, 800);
    if(r1280x720->isChecked())
    	return QSize(1280, 720);
    if(r1600x1200->isChecked())
    	return QSize(1600, 1200);
    if(r1920x1200->isChecked())
    	return QSize(1920, 1200);
    if(r1920x1080->isChecked())
    	return QSize(1920, 1080);

    return QSize(640, 480);
}


void UserPreferencesDialog::on_updatePeriod_valueChanged(int period)
{
	frameRateString->setText(QString("%1 fps").arg((int) ( 1000.0 / double(updatePeriod->value()) ) ) );
}
