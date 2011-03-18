/*
 * RenderingEncoder.h
 *
 *  Created on: Mar 13, 2011
 *      Author: bh
 */

#ifndef RENDERINGENCODER_H_
#define RENDERINGENCODER_H_

#include <QDir>
#include <QString>
#include <QSize>


#include <cstdio>
extern "C" {
#include <libavutil/common.h>
#include <libavcodec/avcodec.h>
#include <libavutil/mathematics.h>
#include <libswscale/swscale.h>
}


class RenderingEncoder: public QObject {

	Q_OBJECT

public:
	RenderingEncoder(QObject * parent = 0);
	virtual ~RenderingEncoder();

	bool start();
	void addFrame();
	bool close();

public Q_SLOTS:
	void setActive(bool on);
	void saveFileAs();

private:
    AVCodec *codec;
    AVCodecContext *c;
    int out_size, size, outbuf_size;
    FILE *f;
    AVFrame *picture;
    uint8_t *outbuf, *picture_buf;
    SwsContext *img_convert_ctx;
	uint8_t * 	data [4];
	char *tmpframe;
	int 	linesize [4];
	int framenum;

	QString temporaryFileName;


	// state machine
	bool started;

	// opengl
	QSize fbosize;

};

#endif /* RENDERINGENCODER_H_ */
