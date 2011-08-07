/*
 * SvgSource.cpp
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

#include <SvgSource.h>
#include "RenderingManager.h"

Source::RTTI SvgSource::type = Source::SVG_SOURCE;

SvgSource::SvgSource(QSvgRenderer *svg, GLuint texture, double d): Source(texture, d), _svg(svg) {

	// if the svg renderer could load the file
	if (!_svg || !_svg->isValid())
		SourceConstructorException().raise();

	// get aspect ratio from orifinal box
	QRectF vb = _svg->viewBoxF();
	aspectratio = double(vb.width()) / double(vb.height());

	// setup renderer resolution to match to rendering manager preference
	int w = RenderingManager::getInstance()->getFrameBufferWidth();
	_rendered = QImage(w, w * aspectratio, QImage::Format_ARGB32_Premultiplied);

	// render an image from the svg
	QPainter imagePainter(&_rendered);
	imagePainter.setRenderHint(QPainter::Antialiasing, true);
	imagePainter.setRenderHint(QPainter::SmoothPixmapTransform, true);
	imagePainter.setRenderHint(QPainter::TextAntialiasing, true);
	_svg->render(&imagePainter);
	imagePainter.end();

	if (_rendered.isNull())
		SvgRenderingException().raise();

	// generate a texture from the rendered image
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, textureIndex);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

#if QT_VERSION >= 0x040700
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA,  _rendered.width(), _rendered.height(),
				  0, GL_BGRA, GL_UNSIGNED_BYTE, _rendered.constBits() );
#else
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA,  _rendered.width(), _rendered. height(),
				  0, GL_BGRA, GL_UNSIGNED_BYTE, _rendered.bits() );
#endif

}

SvgSource::~SvgSource()
{
	// free the OpenGL texture
	glDeleteTextures(1, &textureIndex);

	if (_svg)
		delete _svg;
}


QByteArray SvgSource::getDescription(){

	QBuffer dev;

	QSvgGenerator generator;
	generator.setOutputDevice(&dev);
	generator.setTitle(getName());
	generator.setResolution(150);

	QPainter painter;
	painter.begin(&generator);
	_svg->render(&painter,_svg->viewBoxF());
	painter.end();

	return dev.buffer();
}



