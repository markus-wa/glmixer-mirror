/*
 * SharedMemorySource.h
 *
 *  Created on: Jul 13, 2010
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

#ifndef SHMSOURCE_H_
#define SHMSOURCE_H_

#include "Source.h"
#include "RenderingManager.h"


class SharedMemoryAttachException : public SourceConstructorException {
public:
	virtual QString message() { return "Cannot attach to shared memory."; }
	void raise() const { throw *this; }
	Exception *clone() const { return new SharedMemoryAttachException(*this); }
};

class InvalidFormatException : public SourceConstructorException {
public:
	virtual QString message() { return "Invalid image format from memory."; }
	void raise() const { throw *this; }
	Exception *clone() const { return new InvalidFormatException(*this); }
};

class SharedMemorySource: public Source
{
	friend class SharedMemoryDialog;
	friend class RenderingManager;
    friend class OutputRenderWidget;

public:

	static RTTI type;
	RTTI rtti() const { return type; }

	void setAttached(bool attach);
	bool isAttached();
	int getFrameWidth() const { return width; }
	int getFrameHeight() const { return height; }

	QString getKey() { return shmKey; }
	QString getProgram() { return programName; }
	QString getInfo() { return infoString; }
	QImage::Format getFormat() { return format; }

    // only friends can create a source
protected:

	SharedMemorySource(GLuint texture, double d, QString key, QSize s, QImage::Format f, QString process = QString(), QString info = QString());
	virtual ~SharedMemorySource();
	void update();

private:
	QString shmKey, programName, infoString;
	class QSharedMemory *shm;

	int width, height;
	QImage::Format format;
	void setGLFormat(QImage::Format f);
	GLenum glformat, gltype;
	GLint glunpackalign;
};

#endif /* SHMSOURCE_H_ */