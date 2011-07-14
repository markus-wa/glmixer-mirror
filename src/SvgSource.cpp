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

Source::RTTI SvgSource::type = Source::SVG_SOURCE;

SvgSource::SvgSource(QGraphicsSvgItem *svg, GLuint texture, double d): Source(texture, d), _svg(svg) {

	// if the svg renderer could load the file
	if (_svg) {

		_rendered = QImage(1024, 768, QImage::Format_ARGB32_Premultiplied);

//		_svg->setSharedRenderer(new QSvgRenderer);

		// render an image from the svg
		QPainter imagePainter(&_rendered);
		imagePainter.setRenderHint(QPainter::HighQualityAntialiasing, true);

	    QStyleOptionGraphicsItem style;
		_svg->paint(&imagePainter, &style);
		imagePainter.end();

		// generate a texture from the rendered image
		if (!_rendered.isNull()) {
			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_2D, textureIndex);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
#if QT_VERSION >= 0x040700
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA,  _rendered.width(), _rendered. height(),
					  0, GL_BGRA, GL_UNSIGNED_BYTE, _rendered.constBits() );
#else
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA,  _rendered.width(), _rendered. height(),
					  0, GL_BGRA, GL_UNSIGNED_BYTE, _rendered.bits() );
#endif

			aspectratio = double(_rendered.width()) / double(_rendered.height());
		}
		// TODO : else through exception

	}
	// TODO : else through exception
}

SvgSource::~SvgSource()
{
	// free the OpenGL texture
	glDeleteTextures(1, &textureIndex);

	if (_svg)
		delete _svg;
}

