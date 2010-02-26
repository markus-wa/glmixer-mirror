/*
 * system.h
 *
 *  Created on: Dec 15, 2008
 *      Author: bh
 */

#ifndef COMMON_H_
#define COMMON_H_

#define MINI(a, b)  (((a) < (b)) ? (a) : (b))
#define MAXI(a, b)  (((a) > (b)) ? (a) : (b))
#define ABS(a)	   (((a) < 0) ? -(a) : (a))
#define CLAMP(x, low, high)  (((x) > (high)) ? (high) : (((x) < (low)) ? (low) : (x)))
#define EPSILON 0.00001
#define FRAME_DURATION 15

#if defined(__WIN32__)
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <GL/glew.h>
#endif

#include <QtOpenGL>

bool glSupportsExtension(QString extname);
QStringList glSupportedExtensions();


#ifndef GLEWAPI
/* ------------------------ GL_EXT_framebuffer_blit ------------------------ */

#ifdef GL_EXT_framebuffer_blit
extern "C" {
	extern void glBindFramebufferEXT(GLenum target, GLuint framebuffer);
	extern void glBlitFramebufferEXT(GLint srcX0, GLint srcY0, GLint srcX1, GLint srcY1, GLint dstX0, GLint dstY0, GLint dstX1, GLint dstY1, GLbitfield mask, GLenum filter);
}
#endif /* GL_EXT_framebuffer_blit */

#endif


#define SOURCE_UNIT 10.0
#define CIRCLE_SIZE 7.0
#define SELECTBUFSIZE 64
#define MIN_DEPTH_LAYER 0.0
#define MAX_DEPTH_LAYER 150.0
#define MIN_SCALE 0.31
#define MAX_SCALE 5.0

// #include <cmath>
// #include <string>
// #include <iostream>
// #include <sstream>
// #include <stdexcept>


#endif /*  COMMON_H_ */
