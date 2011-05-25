/*
 * system.h
 *
 *  Created on: Dec 15, 2008
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

#ifndef COMMON_H_
#define COMMON_H_

#define MINI(a, b)  (((a) < (b)) ? (a) : (b))
#define MAXI(a, b)  (((a) > (b)) ? (a) : (b))
#define ABS(a)	   (((a) < 0) ? -(a) : (a))
#define SIGN(a)	   (((a) < 0) ? -1 : 1)
#define CLAMP(x, low, high)  (((x) > (high)) ? (high) : (((x) < (low)) ? (low) : (x)))
#define EPSILON 0.00001
#define FRAME_DURATION 15

#if defined(__WIN32__)
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif

#include <GL/glew.h>

#ifdef __APPLE__
#include <OpenGL/gl.h>
#include <OpenGL/glu.h>
#include <OpenGL/glext.h>
#else
#include <GL/gl.h>
#include <GL/glu.h>
#include <GL/glext.h>
#endif

#include <QtOpenGL>

bool glSupportsExtension(QString extname);
QStringList glSupportedExtensions();

#define MAX_SOURCE_COUNT 125
#define SELECTBUFSIZE 512   // > MAX_SOURCE_COUNT * 4 + 3
#define SOURCE_UNIT 10.0
#define CIRCLE_SIZE 8.0
#define MIN_DEPTH_LAYER 0.0
#define MAX_DEPTH_LAYER 30.0
#define MIN_SCALE 0.31
#define MAX_SCALE 100.0
#define DEPTH_EPSILON 0.1
#define BORDER_SIZE 0.4

#define XML_GLM_VERSION "0.6"

#define COLOR_SOURCE 230, 230, 0
//#define COLOR_SELECTION 240, 80, 220
//#define COLOR_SELECTION_AREA 230, 150, 220
//#define COLOR_SELECTION 230, 105, 10
//#define COLOR_SELECTION_AREA 230, 145, 85
#define COLOR_SELECTION 0, 180, 50
#define COLOR_SELECTION_AREA 40, 190, 80
#define COLOR_BACKGROUND 52, 52, 52
#define COLOR_DRAWINGS 150, 150, 150
#define COLOR_LIMBO 35, 35, 35
#define COLOR_FADING 25, 25, 25
#define COLOR_FRAME 210, 30, 210
#define COLOR_CURSOR 13, 148, 224

#include <QDir>
#include <QValidator>

class folderValidator : public QValidator
{
  public:
    folderValidator(QObject *parent) : QValidator(parent) { }

    QValidator::State validate ( QString & input, int & pos ) const {
      QDir d(input);
      if( d.exists() )
	  	  return QValidator::Acceptable;
      if( d.isAbsolute() )
    	  return QValidator::Intermediate;
      return QValidator::Invalid;
    }
};


#endif /*  COMMON_H_ */
