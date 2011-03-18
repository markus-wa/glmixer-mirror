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


RenderingEncoder::RenderingEncoder(QObject * parent): QObject(parent), started(false) {
	// TODO Auto-generated constructor stub

	temporaryFileName = "glmixeroutput.mpg";

}

RenderingEncoder::~RenderingEncoder() {
	// TODO Auto-generated destructor stub
}


void RenderingEncoder::setActive(bool on)
{
	if (on) {
		if (!start())
			qCritical("Could not stop video recording.");
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

	/* find the mpeg1 video encoder */
	codec = avcodec_find_encoder(CODEC_ID_MPEG1VIDEO);
	if (!codec) {
		qDebug("codec not found\n");
		return false;
	}

	c= avcodec_alloc_context();
	picture= avcodec_alloc_frame();

	/* put sample parameters */
	c->bit_rate = 400000;
	/* resolution must be a multiple of two */
	c->width = fbosize.width();
	c->height = fbosize.height();
	/* frames per second */
	// TODO : read frame rate from glRenderWidget
	c->time_base= (AVRational){1,25};
	/* emit one intra frame every ten frames */
	c->gop_size = 10;
	c->max_b_frames=1;
	c->pix_fmt = PIX_FMT_YUV420P;

	/* open it */
	if (avcodec_open(c, codec) < 0) {
		qDebug("could not open codec\n");
		return false;
	}

	f = fopen( qPrintable( QDir::temp().absoluteFilePath(temporaryFileName)), "wb");
	if (!f) {
		qDebug("could not open %s\n", qPrintable(QDir::temp().absoluteFilePath(temporaryFileName)) );
		return false;
	}

	/* alloc image and output buffer */
	outbuf_size = 1000000;
	outbuf = (uint8_t *) malloc(outbuf_size);
	size = c->width * c->height;
	picture_buf = (uint8_t *) malloc((size * 3) / 2); /* size for YUV 420 */

	picture->data[0] = picture_buf;
	picture->data[1] = picture->data[0] + size;
	picture->data[2] = picture->data[1] + size / 4;
	picture->linesize[0] = c->width;
	picture->linesize[1] = c->width / 2;
	picture->linesize[2] = c->width / 2;

	// reset frame counter
	framenum = 0;

	// create a temporary buffer for gl read
	tmpframe = (char *) malloc(fbosize.width() * fbosize.height() * 3);
	if (!tmpframe)
		return false;

	// setup the frame pointers to tmpframe for reading the frame backward (y inverted in opengl)
	linesize[0] = - fbosize.width() * 3;
	data[0] = (uint8_t *) tmpframe + (fbosize.width()) * (fbosize.height() - 1) * 3;
	linesize[1] = 0;
	data[1] = 0;
	linesize[2] = 0;
	data[2] = 0;
	linesize[3] = 0;
	data[3] = 0;


	// create conversion context
	img_convert_ctx = sws_getContext(fbosize.width(), fbosize.height(), PIX_FMT_RGB24,
										c->width, c->height, PIX_FMT_YUV420P, SWS_POINT,
										NULL, NULL, NULL);
	if (img_convert_ctx == NULL)
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

	// bind rendering frame buffer object
	glBindFramebuffer(GL_READ_FRAMEBUFFER, RenderingManager::getInstance()->getFrameBufferHandle());
	// read the pixels and store into the temporary buffer
	glReadPixels((GLint)0, (GLint)0, (GLint)fbosize.width(), (GLint)fbosize.height(), GL_RGB, GL_UNSIGNED_BYTE, tmpframe);
	// unbind fbo
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);

	// convert buffer to avcodec frame
	sws_scale(img_convert_ctx, data, linesize, 0, fbosize.height(), picture->data, picture->linesize);

	// set timing for frame and increment frame counter
	picture->pts = 1000000LL * framenum / 25;
	framenum++;

	/* encode the image */
	out_size = avcodec_encode_video(c, outbuf, outbuf_size, picture);
	fwrite(outbuf, 1, out_size, f);

}

// Close the encoding process
// - get the delayed frames
// - add sequence end code to have a real mpeg file
// - close file
bool RenderingEncoder::close(){

	if (!started)
		return false;

	/* get the delayed frames */
	for(; out_size;) {
		out_size = avcodec_encode_video(c, outbuf, outbuf_size, NULL);
		fwrite(outbuf, 1, out_size, f);
	}

	/* add sequence end code to have a real mpeg file */
	outbuf[0] = 0x00;
	outbuf[1] = 0x00;
	outbuf[2] = 0x01;
	outbuf[3] = 0xb7;
	fwrite(outbuf, 1, 4, f);
	fclose(f);
	free(picture_buf);
	free(outbuf);

	// free avcodec stuff
	avcodec_close(c);
	av_free(c);
	av_free(picture);

    // free context converter
    if (img_convert_ctx)
        sws_freeContext(img_convert_ctx);

    // free opengl buffer
	free(tmpframe);

	started = false;
	return true;
}

void RenderingEncoder::saveFileAs(){

	static QDir dir(QDir::currentPath());

	// Select file name
	QString newFileName = QFileDialog::getSaveFileName ( 0, tr("Save captured video"), dir.absolutePath(), tr("MPEG1 video (*.mpg *.mpeg)"));

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
