/*
 * RenderingEncoder.cpp
 *
 *  Created on: Mar 13, 2011
 *      Author: bh
 */

#include "RenderingEncoder.moc"

#include "common.h"
#include "RenderingManager.h"

#include <QDir>
#include <QFileInfo>
#include <QFileDialog>
#include <QMessageBox>
#include <QGLFramebufferObject>


RenderingEncoder::RenderingEncoder(QObject * parent): QObject(parent), started(false), fbohandle(0) {
	// set default format
	setFormat(RenderingEncoder::FFVHUFF);
	setFormat(RenderingEncoder::MPEG1);

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
		// TODO : warning message
	}
}


void RenderingEncoder::setActive(bool on)
{
	if (on) {
		if (!start())
			qCritical("Could not start video recording.");
	} else {
		if (close())
			saveFileAs();
		else
			qCritical("Could not stop video recording.");
	}
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


	switch (format) {
	case RenderingEncoder::MPEG1:
		tmpframe = (char *) malloc(fbosize.width() * fbosize.height() * 3);
		recorder = mpeg_rec_init(qPrintable( QDir::temp().absoluteFilePath(temporaryFileName)), fbosize.width(), fbosize.height(), 25);
		break;
	case RenderingEncoder::FFVHUFF:
	default:
		tmpframe = (char *) malloc(fbosize.width() * fbosize.height() * 4);
		recorder = ffvhuff_rec_init(qPrintable( QDir::temp().absoluteFilePath(temporaryFileName)), fbosize.width(), fbosize.height(), 25);
	}

	if (!tmpframe)
		return false;
	if (recorder == NULL)
		return false;

	started = true;

	return true;
}

// Add a frame to the stream
// - encode
// - save to file
void RenderingEncoder::addFrame(){

	if (!started)
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

	started = false;
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
	}
}
