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

// Header Files and directives for Microsoft Windows
// #ifdef WIN32
// #define WIN32_LEAN_AND_MEAN
// #include <windows.h>
// #include <tchar.h>
// #endif


#if defined(__WIN32__)
#include <GL/gl.h>
#include <GL/glu.h>
#include <GL/glext.h>
#endif

#include <QtOpenGL>

#if QT_VERSION < 0x040600

#ifndef GL_EXT_framebuffer_blit
#define GL_READ_FRAMEBUFFER_EXT           0x8CA8
#define GL_DRAW_FRAMEBUFFER_EXT           0x8CA9
#define GL_DRAW_FRAMEBUFFER_BINDING_EXT   GL_FRAMEBUFFER_BINDING_EXT
#define GL_READ_FRAMEBUFFER_BINDING_EXT   0x8CAA
#endif

extern "C" {
	extern void glBindFramebufferEXT(GLenum target, GLuint framebuffer);
	extern void glBlitFramebufferEXT(GLint srcX0, GLint srcY0, GLint srcX1, GLint srcY1, GLint dstX0, GLint dstY0, GLint dstX1, GLint dstY1, GLbitfield mask, GLenum filter);
}

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
