/*
 * RenderingEncoder.cpp
 *
 *  Created on: Mar 13, 2011
 *      Author: bh
 */

#include "RenderingEncoder.moc"

#include "common.h"
#include "RenderingManager.h"
#include "ViewRenderWidget.h"

#include <QDir>
#include <QFileInfo>
#include <QFileDialog>
#include <QMessageBox>
#include <QGLFramebufferObject>

extern "C" {
#include "video_rec.h"
}

RenderingEncoder::RenderingEncoder(QObject * parent): QObject(parent), started(false), fbohandle(0), update(40), displayupdate(33) {

	// set default format
	setFormat(RenderingEncoder::FFVHUFF);
}


void RenderingEncoder::setFormat(encoder_format f){

	if (!started) {
		format = f;

		switch (format) {
		case RenderingEncoder::MPEG1:
				temporaryFileName = "glmixeroutput.mpg";
			break;
		case RenderingEncoder::FFVHUFF:
			default:
				temporaryFileName = "glmixeroutput.avi";
		}
	} else {
		qCritical("ERROR setting video recording format.\n\nRecorder is busy.");
	}
}


void RenderingEncoder::setActive(bool on)
{
	if (on) {
		if (!start())
			qCritical("ERROR starting video recording.\n\n%s\n", errormessage);
	} else {
		if (close())
			saveFileAs();
	}

	// inform if we could be activated
    emit activated(started);
}

// Start the encoding process
// - Create codec
// - Create the temporary file
bool RenderingEncoder::start(){

	if (started)
		return false;

	// if the temporary file already exists, delete it.
	if (QDir::temp().exists(temporaryFileName)){
		QDir::temp().remove(temporaryFileName);
	}

	// get access to the size of the renderrer fbo
	fbosize = RenderingManager::getInstance()->getFrameBufferResolution();
	fbohandle =  RenderingManager::getInstance()->getFrameBufferHandle();

	// setup update frequency
	displayupdate = RenderingManager::getRenderingWidget()->updatePeriod();
	RenderingManager::getRenderingWidget()->setUpdatePeriod( update );
	int freq = (int) ( 1000.0 / double(update) );

	switch (format) {
	case RenderingEncoder::MPEG1:
		tmpframe = (char *) malloc(fbosize.width() * fbosize.height() * 3);
		recorder = mpeg_rec_init(qPrintable( QDir::temp().absoluteFilePath(temporaryFileName)), fbosize.width(), fbosize.height(), freq, errormessage);
		break;
	case RenderingEncoder::FFVHUFF:
	default:
		tmpframe = (char *) malloc(fbosize.width() * fbosize.height() * 4);
		recorder = ffvhuff_rec_init(qPrintable( QDir::temp().absoluteFilePath(temporaryFileName)), fbosize.width(), fbosize.height(), freq, errormessage);
	}

	// test success of initialization
	if (!tmpframe)
		return false;
	if (recorder == NULL)
		return false;

	// start
    emit status(tr("Start recording."), 1000);
	timer.start();
	elapseTimer = startTimer(1000); // emit the duration of recording every second
	started = true;

	return true;
}

void RenderingEncoder::timerEvent(QTimerEvent *event)
{
    emit status(tr("Recording time: %1 s").arg(timer.elapsed()/1000), 1000);
}

int RenderingEncoder::getRecodingTime() {

	if (started)
		return timer.elapsed();
	else
		return 0;
}

// Add a frame to the stream
// - encode
// - save to file
void RenderingEncoder::addFrame(){

	if (!started || recorder == NULL)
		return;

	switch (format) {
	case RenderingEncoder::MPEG1:
		// bind rendering frame buffer object
		glBindFramebuffer(GL_READ_FRAMEBUFFER, fbohandle);
		// read the pixels and store into the temporary buffer
		glReadPixels((GLint)0, (GLint)0, (GLint)fbosize.width(), (GLint)fbosize.height(), GL_RGB, GL_UNSIGNED_BYTE, tmpframe);
		// give the frame to the encoder
		mpeg_rec_deliver_vframe(recorder, tmpframe);
		break;
	case RenderingEncoder::FFVHUFF:
	default:
		// bind rendering frame buffer object
		glBindFramebuffer(GL_READ_FRAMEBUFFER, fbohandle);
		// read the pixels and store into the temporary buffer
		glReadPixels((GLint)0, (GLint)0, (GLint)fbosize.width(), (GLint)fbosize.height(), GL_BGRA, GL_UNSIGNED_BYTE, tmpframe);
		// give the frame to the encoder
		ffvhuff_rec_deliver_vframe(recorder, tmpframe);
	}

}

// Close the encoding process
// - get the delayed frames
// - add sequence end code to have a real mpeg file
// - close file
bool RenderingEncoder::close(){

	if (!started)
		return false;

	switch (format) {
	case RenderingEncoder::MPEG1:
		mpeg_rec_stop(recorder);
		break;
	case RenderingEncoder::FFVHUFF:
	default:
		ffvhuff_rec_stop(recorder);
	}

    // free opengl buffer
	free(tmpframe);

	// done
	killTimer(elapseTimer);
	started = false;

	// restore former display update period
	RenderingManager::getRenderingWidget()->setUpdatePeriod( displayupdate );

	return true;
}

void RenderingEncoder::saveFileAs(){

	static QDir dir(QDir::currentPath());

	// Select file name
	QString newFileName;

	switch (format) {
	case RenderingEncoder::MPEG1:
		newFileName = QFileDialog::getSaveFileName ( 0, tr("Save captured video"), dir.absolutePath(), tr("MPEG Video (*.mpg)"));
		break;
	case RenderingEncoder::FFVHUFF:
		default:
		newFileName = QFileDialog::getSaveFileName ( 0, tr("Save captured video"), dir.absolutePath(), tr("AVI Video (*.avi)"));
	}

	// remember path
	dir = QFileInfo(newFileName).dir();

	if (!newFileName.isEmpty()) {
		// delete file if exists
		QFileInfo infoFileDestination(newFileName);
		if (infoFileDestination.exists()){
			infoFileDestination.dir().remove(infoFileDestination.fileName ());
		}
		// move the temporaryFileName to newFileName
		QDir::temp().rename(temporaryFileName, newFileName);
	    emit status(tr("File %1 saved.").arg(newFileName), 2000);
	}
}
