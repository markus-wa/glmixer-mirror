/*
 * RenderingEncoder.h
 *
 *  Created on: Mar 13, 2011
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

#ifndef RENDERINGENCODER_H_
#define RENDERINGENCODER_H_

#include <QObject>
#include <QTime>
#include <QString>
#include <QFileDialog>


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

	void setEncodingFormat(encodingformat f);
	encodingformat encodingFormat() { return format; }
	void setUpdatePeriod(int ms) { update=ms; }
	int updatePeriod() { return update; }

	bool isActive() { return started; }
	int getRecodingTime();

	void setSavingDirectory(QString path) { sfa.setDirectory(path); }
	QString savingDirectory() { return sfa.directory().absolutePath(); }

public Q_SLOTS:
	void setActive(bool on);
	void saveFileAs();

Q_SIGNALS:
	void activated(bool);
	void status(const QString &, int);

protected:
    void timerEvent(QTimerEvent *event);

private:
	// temp file location
	QString temporaryFileName;
	QFileDialog sfa;

	// state machine
	bool started;
	QTime timer;
	int elapseTimer;

	// opengl
	char * tmpframe;
	unsigned int fbohandle;

	// encoder
	int update, displayupdate;
	encodingformat format;
	video_rec_t *recorder;
	char errormessage[256];
};

#endif /* RENDERINGENCODER_H_ */
