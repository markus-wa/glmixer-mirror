/*
 * RenderingEncoder.h
 *
 *  Created on: Mar 13, 2011
 *      Author: bh
 */

#ifndef RENDERINGENCODER_H_
#define RENDERINGENCODER_H_

#include <QObject>
#include <QString>
#include <QSize>

extern "C" {
#include "video_rec.h"
}


class RenderingEncoder: public QObject {

	Q_OBJECT

public:
	RenderingEncoder(QObject * parent = 0);

	bool start();
	void addFrame();
	bool close();

	typedef enum {
		FFVHUFF = 0,
		MPEG1
	} encoder_format;

	void setFormat(encoder_format f);

public Q_SLOTS:
	void setActive(bool on);
	void saveFileAs();

private:
	// temp file location
	QString temporaryFileName;

	// state machine
	bool started;

	// opengl
	char * tmpframe;
	QSize fbosize;
	unsigned int fbohandle;

	// encoder
	encoder_format format;
	video_rec_t *recorder;

};

#endif /* RENDERINGENCODER_H_ */
