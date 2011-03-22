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

#include <QSize>
#include <QFileInfo>
#include <QFileDialog>
#include <QMessageBox>
#include <QGLFramebufferObject>


RenderingEncoder::RenderingEncoder(QObject * parent): QObject(parent), started(false), fbohandle(0), update(40), displayupdate(33) {

	// set default format
	temporaryFileName = "glmixeroutput";
	setEncodingFormat(FORMAT_AVI_FFVHUFF);
	// init file saving dir
	sfa.setDirectory(QDir::currentPath());
	sfa.setAcceptMode(QFileDialog::AcceptSave);
	sfa.setFileMode(QFileDialog::AnyFile);
}


void RenderingEncoder::setEncodingFormat(encodingformat f){

	if (!started) {
		format = f;
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
	QSize fbosize = RenderingManager::getInstance()->getFrameBufferResolution();
	fbohandle =  RenderingManager::getInstance()->getFrameBufferHandle();

	// setup update frequency
	displayupdate = RenderingManager::getRenderingWidget()->updatePeriod();
	RenderingManager::getRenderingWidget()->setUpdatePeriod( update );
	int freq = (int) ( 1000.0 / double(update) );

	// BGRA temporary frame for read pixels of FBO
	tmpframe = (char *) malloc(fbosize.width() * fbosize.height() * 4);
	if (!tmpframe)
		return false;

	// initialization of ffmpeg recorder
	recorder = video_rec_init(qPrintable( QDir::temp().absoluteFilePath(temporaryFileName)), format, fbosize.width(), fbosize.height(), freq, errormessage);
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
void RenderingEncoder::addFrame(){

	if (!started || recorder == NULL)
		return;

	// bind rendering frame buffer object
	glBindFramebuffer(GL_READ_FRAMEBUFFER, fbohandle);
	// read the pixels and store into the temporary buffer
	glReadPixels((GLint)0, (GLint)0, (GLint) recorder->width, (GLint) recorder->height, GL_BGRA, GL_UNSIGNED_BYTE, tmpframe);
	// give the frame to the encoder by calling the function specified in the recorder
	(*recorder->pt2RecordingFunction)(recorder, tmpframe);

}

// Close the encoding process
bool RenderingEncoder::close(){

	if (!started)
		return false;

	// stop recorder
	video_rec_stop(recorder);

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

	// Select file format
	switch (format) {
	case FORMAT_MPG_MPEG1:
		sfa.setFilter(tr("MPEG 1 Video (*.mpg *.mpeg)"));
		sfa.setDefaultSuffix("mpg");
		break;
	case FORMAT_MP4_MPEG4:
		sfa.setFilter(tr("MPEG 4 Video (*.mp4)"));
		sfa.setDefaultSuffix("mp4");
		break;
	case FORMAT_WMV_WMV1:
		sfa.setFilter(tr("Windows Media Video (*.wmv)"));
		sfa.setDefaultSuffix("wmv");
		break;
	default:
	case FORMAT_AVI_FFVHUFF:
		sfa.setFilter(tr("AVI Video (*.avi)"));
		sfa.setDefaultSuffix("avi");
	}

	// get file name
	if (sfa.exec()) {
	    QString newFileName = sfa.selectedFiles().front();
		// now we got a filename, save the file:
		if (!newFileName.isEmpty()) {

			// delete file if exists
			QFileInfo infoFileDestination(newFileName);
			if (infoFileDestination.exists()){
				infoFileDestination.dir().remove(infoFileDestination.fileName());
			}
			// move the temporaryFileName to newFileName
			QDir::temp().rename(temporaryFileName, newFileName);
			emit status(tr("File %1 saved.").arg(newFileName), 2000);
		}
	}

}
