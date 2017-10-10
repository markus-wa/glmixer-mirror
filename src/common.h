/*
 * common.h
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
 *   Copyright 2009, 2012 Bruno Herbelin
 *
 */

#ifndef COMMON_H_
#define COMMON_H_

#include <QtGlobal>

#ifdef Q_OS_WIN
#define WIN32_LEAN_AND_MEAN
#define GLEWAPI extern
#include <windows.h>
#endif

#include <GL/glew.h>

#ifdef Q_OS_MAC
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
#include <OpenGL/gl.h>
#include <OpenGL/glu.h>
#include <OpenGL/glext.h>
#else
#include <GL/gl.h>
#include <GL/glu.h>
#include <GL/glext.h>
#endif

#include <QDir>
#include <QGLWidget>
#include <QGLFramebufferObject>
#include <QGLShaderProgram>
#include <QGLPixelBuffer>
#include <QUrl>

void initListOfExtension();
QGLFormat glRenderWidgetFormat();
QStringList glSupportedExtensions();
GLint glMaximumTextureWidth();
GLint glMaximumTextureHeight();
GLint glMaximumFramebufferWidth();
GLint glMaximumFramebufferHeight();

GLenum blendfunctionFromInt(int);
int intFromBlendfunction(GLenum);
GLenum blendequationFromInt(int);
int intFromBlendequation(GLenum);
QPair<int, int> blendingPresetFromInt(int);
QString namePresetFromInt(int);
int intFromBlendingPreset(GLenum, GLenum);

void addPathToSystemPath(QByteArray path);

QString getByteSizeString(double numbytes);

void initApplicationFonts();
QString getMonospaceFont();

QFileInfo getFileInfoFromURL(QUrl url);

#endif /*  COMMON_H_ */
