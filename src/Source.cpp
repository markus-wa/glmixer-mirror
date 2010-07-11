/*
 * Source.cpp
 *
 *  Created on: Jun 29, 2009
 *      Author: bh
 */

#include "SourceSet.h"
#include "ViewRenderWidget.h"
#include "OutputRenderWindow.h"

#include <QtProperty>
#include <QtVariantPropertyManager>

GLuint Source::lastid = 1;
bool Source::imaging_extension = true;
Source::RTTI Source::type = Source::SIMPLE_SOURCE;

Source::Source(GLuint texture, double depth) :
	active(false), culled(false), frameChanged(false), textureIndex(texture),
			maskTextureIndex(0), iconIndex(0), x(0.0), y(0.0), z(depth),
			scalex(SOURCE_UNIT), scaley(SOURCE_UNIT), alphax(0.0), alphay(0.0),
			centerx(0.0), centery(0.0), rotangle(0.0), rothandle(SOURCE_UNIT * 2.0),
			aspectratio(1.0), texalpha(1.0), pixelated(false),
			convolution(NO_CONVOLUTION), colorTable(NO_COLORTABLE), mask_type(NO_MASK),
			brightness(0), contrast(0),	saturation(0) {

	z = CLAMP(z, MIN_DEPTH_LAYER, MAX_DEPTH_LAYER);

	texcolor = Qt::white;
	source_blend = GL_SRC_ALPHA;
	destination_blend = GL_ONE;
	blend_eq = GL_FUNC_ADD;

	// give it a unique identifying name
	id = lastid++;
//	name = QString("Source %1").arg(id);
	name = QString("Source");

	clones = new SourceList;
}

Source::~Source() {

	delete clones;
}

void Source::setName(QString n) {
	name = n;
}

void Source::testCulling() {

	// if all coordinates of center are between viewport limits, it is obviously visible
	if (x > -SOURCE_UNIT && x < SOURCE_UNIT && y > -SOURCE_UNIT && y
			< SOURCE_UNIT)
		culled = false;
	else {
		// not obviously visible
		// but it might still be parly visible if the distance from the center to the borders is less than the width
		if ((x + ABS(scalex) < -SOURCE_UNIT) || (x - ABS(scalex) > SOURCE_UNIT))
			culled = true;
		else if ((y + ABS(scaley) < -SOURCE_UNIT) || (y - ABS(scaley) > SOURCE_UNIT))
			culled = true;
			else
				culled = false;
	}
}

void Source::setDepth(GLdouble v) {
	z = CLAMP(v, MIN_DEPTH_LAYER, MAX_DEPTH_LAYER);
}

void Source::moveTo(GLdouble posx, GLdouble posy) {
	x = posx;
	y = posy;
}

void Source::setScale(GLdouble sx, GLdouble sy) {
	scalex = sx;
	scaley = sy;
}

void Source::scaleBy(float fx, float fy) {
	scalex *= fx;
	scaley *= fy;
}

void Source::clampScale() {

	scalex = (scalex > 0 ? 1.0 : -1.0)
			* CLAMP( ABS(scalex), MIN_SCALE, MAX_SCALE);
	scaley = (scaley > 0 ? 1.0 : -1.0)
			* CLAMP( ABS(scaley), MIN_SCALE, MAX_SCALE);
}

void Source::setAlphaCoordinates(double x, double y) {

	// set new alpha coordinates
	alphax = x;
	alphay = y;

	// Compute distance to the center
	double d = ((x * x) + (y * y)) / (SOURCE_UNIT * SOURCE_UNIT * CIRCLE_SIZE
			* CIRCLE_SIZE); // QUADRATIC
	// adjust alpha according to distance to center
	if (d < 1.0)
		texalpha = 1.0 - d;
	else
		texalpha = 0.0;

}

void Source::setAlpha(GLfloat a) {

	texalpha = CLAMP(a, 0.0, 1.0);

	// compute new alpha coordinates to match this alpha
	GLdouble dx = 0, dy = 0;

	// special case when source at the center
	if (ABS(alphax) < EPSILON && ABS(alphay) < EPSILON)
		dy = 1.0;
	else { // general case ; compute direction of the alpha coordinates
		dx = alphax / sqrt(alphax * alphax + alphay * alphay);
		dy = alphay / sqrt(alphax * alphax + alphay * alphay);
	}

	GLfloat da = sqrt((1.0 - texalpha) * (SOURCE_UNIT * SOURCE_UNIT
			* CIRCLE_SIZE * CIRCLE_SIZE));

	// set new alpha coordinates
	alphax = dx * da;
	alphay = dy * da;
}

void Source::resetScale() {
	scalex = SOURCE_UNIT * aspectratio;
	scaley = SOURCE_UNIT;

//	float renderingAspectRatio = OutputRenderWindow::getInstance()->getAspectRatio();
//
//	if (aspectratio < renderingAspectRatio)
//		scalex *= aspectratio / renderingAspectRatio;
//	else
//		scaley *= renderingAspectRatio / aspectratio;


}

void Source::setColor(QColor c) {
	texcolor = c;
}

void Source::draw(bool withalpha, GLenum mode) const {
	// set id in select mode, avoid texturing if not rendering.
	if (mode == GL_SELECT)
		glLoadName(id);
	else {
		// set transparency and color
		glColor4f(texcolor.redF(), texcolor.greenF(), texcolor.blueF(),
				withalpha ? texalpha : 1.0);
	}
	// draw
	glCallList(ViewRenderWidget::quad_texured);
}

// color tables
static GLubyte invertTable[256][3] = { { 255, 255, 255 }, { 254, 254, 254 }, {253, 253, 253 }, { 252, 252, 252 }, { 251, 251, 251 },{ 250, 250, 250 }, { 249, 249, 249 }, { 248, 248, 248 }, { 247, 247,247 }, { 246, 246, 246 }, { 245, 245, 245 }, { 244, 244, 244 },{ 243, 243, 243 }, { 242, 242, 242 }, { 241, 241, 241 }, { 240, 240,240 }, { 239, 239, 239 }, { 238, 238, 238 }, { 237, 237, 237 },{ 236, 236, 236 }, { 235, 235, 235 }, { 234, 234, 234 }, { 233, 233,233 }, { 232, 232, 232 }, { 231, 231, 231 }, { 230, 230, 230 },{ 229, 229, 229 }, { 228, 228, 228 }, { 227, 227, 227 }, { 226, 226,226 }, { 225, 225, 225 }, { 224, 224, 224 }, { 223, 223, 223 },{ 222, 222, 222 }, { 221, 221, 221 }, { 220, 220, 220 }, { 219, 219,219 }, { 218, 218, 218 }, { 217, 217, 217 }, { 216, 216, 216 },{ 215, 215, 215 }, { 214, 214, 214 }, { 213, 213, 213 }, { 212, 212,212 }, { 211, 211, 211 }, { 210, 210, 210 }, { 209, 209, 209 },{ 208, 208, 208 }, { 207, 207, 207 }, { 206, 206, 206 }, { 205, 205,205 }, { 204, 204, 204 }, { 203, 203, 203 }, { 202, 202, 202 },{ 201, 201, 201 }, { 200, 200, 200 }, { 199, 199, 199 }, { 198, 198,198 }, { 197, 197, 197 }, { 196, 196, 196 }, { 195, 195, 195 },{ 194, 194, 194 }, { 193, 193, 193 }, { 192, 192, 192 }, { 191, 191,191 }, { 190, 190, 190 }, { 189, 189, 189 }, { 188, 188, 188 },{ 187, 187, 187 }, { 186, 186, 186 }, { 185, 185, 185 }, { 184, 184,184 }, { 183, 183, 183 }, { 182, 182, 182 }, { 181, 181, 181 },{ 180, 180, 180 }, { 179, 179, 179 }, { 178, 178, 178 }, { 177, 177,177 }, { 176, 176, 176 }, { 175, 175, 175 }, { 174, 174, 174 },{ 173, 173, 173 }, { 172, 172, 172 }, { 171, 171, 171 }, { 170, 170,170 }, { 169, 169, 169 }, { 168, 168, 168 }, { 167, 167, 167 },{ 166, 166, 166 }, { 165, 165, 165 }, { 164, 164, 164 }, { 163, 163,163 }, { 162, 162, 162 }, { 161, 161, 161 }, { 160, 160, 160 },{ 159, 159, 159 }, { 158, 158, 158 }, { 157, 157, 157 }, { 156, 156,156 }, { 155, 155, 155 }, { 154, 154, 154 }, { 153, 153, 153 },{ 152, 152, 152 }, { 151, 151, 151 }, { 150, 150, 150 }, { 149, 149,149 }, { 148, 148, 148 }, { 147, 147, 147 }, { 146, 146, 146 },{ 145, 145, 145 }, { 144, 144, 144 }, { 143, 143, 143 }, { 142, 142,142 }, { 141, 141, 141 }, { 140, 140, 140 }, { 139, 139, 139 },{ 138, 138, 138 }, { 137, 137, 137 }, { 136, 136, 136 }, { 135, 135,135 }, { 134, 134, 134 }, { 133, 133, 133 }, { 132, 132, 132 },{ 131, 131, 131 }, { 130, 130, 130 }, { 129, 129, 129 }, { 128, 128,128 }, { 127, 127, 127 }, { 126, 126, 126 }, { 125, 125, 125 },{ 124, 124, 124 }, { 123, 123, 123 }, { 122, 122, 122 }, { 121, 121,121 }, { 120, 120, 120 }, { 119, 119, 119 }, { 118, 118, 118 },{ 117, 117, 117 }, { 116, 116, 116 }, { 115, 115, 115 }, { 114, 114,114 }, { 113, 113, 113 }, { 112, 112, 112 }, { 111, 111, 111 },{ 110, 110, 110 }, { 109, 109, 109 }, { 108, 108, 108 }, { 107, 107,107 }, { 106, 106, 106 }, { 105, 105, 105 }, { 104, 104, 104 },{ 103, 103, 103 }, { 102, 102, 102 }, { 101, 101, 101 }, { 100, 100,100 }, { 99, 99, 99 }, { 98, 98, 98 }, { 97, 97, 97 }, { 96,96, 96 }, { 95, 95, 95 }, { 94, 94, 94 }, { 93, 93, 93 }, { 92,92, 92 }, { 91, 91, 91 }, { 90, 90, 90 }, { 89, 89, 89 }, { 88,88, 88 }, { 87, 87, 87 }, { 86, 86, 86 }, { 85, 85, 85 }, { 84,84, 84 }, { 83, 83, 83 }, { 82, 82, 82 }, { 81, 81, 81 }, { 80,80, 80 }, { 79, 79, 79 }, { 78, 78, 78 }, { 77, 77, 77 }, { 76,76, 76 }, { 75, 75, 75 }, { 74, 74, 74 }, { 73, 73, 73 }, { 72,72, 72 }, { 71, 71, 71 }, { 70, 70, 70 }, { 69, 69, 69 }, { 68,68, 68 }, { 67, 67, 67 }, { 66, 66, 66 }, { 65, 65, 65 }, { 64,64, 64 }, { 63, 63, 63 }, { 62, 62, 62 }, { 61, 61, 61 }, { 60,60, 60 }, { 59, 59, 59 }, { 58, 58, 58 }, { 57, 57, 57 }, { 56,56, 56 }, { 55, 55, 55 }, { 54, 54, 54 }, { 53, 53, 53 }, { 52,52, 52 }, { 51, 51, 51 }, { 50, 50, 50 }, { 49, 49, 49 }, { 48,48, 48 }, { 47, 47, 47 }, { 46, 46, 46 }, { 45, 45, 45 }, { 44,44, 44 }, { 43, 43, 43 }, { 42, 42, 42 }, { 41, 41, 41 }, { 40,40, 40 }, { 39, 39, 39 }, { 38, 38, 38 }, { 37, 37, 37 }, { 36,36, 36 }, { 35, 35, 35 }, { 34, 34, 34 }, { 33, 33, 33 }, { 32,32, 32 }, { 31, 31, 31 }, { 30, 30, 30 }, { 29, 29, 29 }, { 28,28, 28 }, { 27, 27, 27 }, { 26, 26, 26 }, { 25, 25, 25 }, { 24,24, 24 }, { 23, 23, 23 }, { 22, 22, 22 }, { 21, 21, 21 }, { 20,20, 20 }, { 19, 19, 19 }, { 18, 18, 18 }, { 17, 17, 17 }, { 16,16, 16 }, { 15, 15, 15 }, { 14, 14, 14 }, { 13, 13, 13 }, { 12,12, 12 }, { 11, 11, 11 }, { 10, 10, 10 }, { 9, 9, 9 }, { 8, 8,8 }, { 7, 7, 7 }, { 6, 6, 6 }, { 5, 5, 5 }, { 4, 4, 4 }, { 3,3, 3 }, { 2, 2, 2 }, { 1, 1, 1 } };

static GLubyte colorTable16[16][3] = { { 0, 0, 0 }, { 17, 17, 17 }, { 34, 34, 34 }, { 51, 51, 51 }, { 68, 68, 68 }, { 85, 85, 85 }, { 102, 102, 102 }, { 119, 119, 119 }, { 136, 136, 136 }, { 153, 153, 153 }, { 170, 170, 170 }, { 187, 187, 187 }, { 204, 204, 204 }, { 221, 221, 221 }, { 238, 238, 238 }, { 255, 255, 255 } };

static GLubyte colorTable8[8][3] = { {0, 0, 0},  {36, 36, 36},  {72, 72, 72},  {108, 108, 108},  {144, 144, 144},  {180, 180, 180},  {216, 216, 216},  {252, 252, 252} };

static GLubyte colorTable4[4][3] = { { 0, 0, 0 },  { 64, 64, 64 },  { 160, 160, 160 },  { 255, 255, 255 } };

static GLubyte colorTable2[2][3] = { { 0, 0, 0 }, { 255, 255, 255 } };

static GLubyte testTable[4][3] = { { 1, 1, 1 },  { 64, 64, 64 },  { 160, 160, 160 },  { 255, 255, 255 } };


// Sharpen convolution kernel
static GLfloat mSharpen[3][3] = { { 0.0f, -1.0f, 0.0f },
		{ -1.0f, 5.0f, -1.0f }, { 0.0f, -1.0f, 0.0f } };
// Blur convolution kernel
static GLfloat mBlur[3][3] = { { 1.0f, 1.0f, 1.0f }, { 1.0f, 2.0f, 1.f }, {
		1.0f, 1.f, 1.0f } };
// Blur convolution kernel
static GLfloat mEdge[3][3] = { { 1.0f, 1.0f, 1.0f }, { 1.0f, -9.0, 1.0f }, {
		1.0f, 1.0f, 1.0f } };
// Emboss convolution kernel
static GLfloat mEmboss[3][3] = { { -2.0f, -1.0f, 0.0f }, { -1.0f, 1.0f, 1.0f },
		{ 0.0f, 1.0f, 2.0f } };

void Source::update() {

	glBindTexture(GL_TEXTURE_2D, textureIndex);

	if (pixelated) {
    	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	}
}

// Greyscale convert matrix
//static GLfloat lumMat[16] = { 0.30f, 0.30f, 0.30f, 0.0f,
//                              0.59f, 0.59f, 0.59f, 0.0f,
//                              0.11f, 0.11f, 0.11f, 0.0f,
//                              0.0f,  0.0f,  0.0f,  1.0f };
// interpolation matrix
//		GLfloat saturationMatrix[16] = { 1.f * (1.f + s) - s * 0.3f, -s * 0.3f, -s * 0.3f,
//				0.0f, -s * 0.59f, 1.f * (1.f + s) - s * 0.59f, -s * 0.59f,
//				0.0f, -s * 0.11f, -s * 0.11f, 1.f * (1.f + s) - s * 0.11f,
//				0.0f, 0.0f, 0.0f, 0.0f, 1.0f };
void Source::setSaturation(int sat) {
	saturation = sat;
	GLfloat s = float(saturation) / 100.f;

	saturationMatrix[0] = 1.f * (1.f + s) - s * 0.3f;
	saturationMatrix[1] = -s * 0.3f;
	saturationMatrix[2] = -s * 0.3f;
	saturationMatrix[3] = 0.f;
	saturationMatrix[4] = -s * 0.59f;
	saturationMatrix[5] = 1.f * (1.f + s) - s * 0.59f;
	saturationMatrix[6] = -s * 0.59f;
	saturationMatrix[7] = 0.f;
	saturationMatrix[8] = -s * 0.11f;
	saturationMatrix[9] = -s * 0.11f;
	saturationMatrix[10] = 1.f * (1.f + s) - s * 0.11f;
	saturationMatrix[11] = 0.f;
	saturationMatrix[12] = 0.f;
	saturationMatrix[13] = 0.f;
	saturationMatrix[14] = 0.f;
	saturationMatrix[15] = 1.f;

}

void Source::beginEffectsSection() const {

	if (brightness != 0) {
		float b = float(brightness) / 100.f;
		glPixelTransferf(GL_RED_BIAS, b);
		glPixelTransferf(GL_GREEN_BIAS, b);
		glPixelTransferf(GL_BLUE_BIAS, b);
	}

	if (contrast != 0) {
		float b = float(contrast) / 100.f + 1.f;
		glPixelTransferf(GL_RED_SCALE, b);
		glPixelTransferf(GL_GREEN_SCALE, b);
		glPixelTransferf(GL_BLUE_SCALE, b);
	}

	// the other options require the GL_ARB_IMAGING extension
	if (!imaging_extension)
		return;

	if (convolution != Source::NO_CONVOLUTION) {
		glEnable(GL_CONVOLUTION_2D);

		if (convolution == Source::SHARPEN_CONVOLUTION) {
			glConvolutionFilter2D(GL_CONVOLUTION_2D, GL_RGB, 3, 3,	GL_LUMINANCE, GL_FLOAT, mSharpen);
		} else if (convolution == Source::BLUR_CONVOLUTION) {
			glConvolutionFilter2D(GL_CONVOLUTION_2D, GL_RGB, 3, 3,	GL_LUMINANCE, GL_FLOAT, mBlur);
			glMatrixMode(GL_COLOR);
			glScalef(0.1, 0.1, 0.1);
			glMatrixMode(GL_MODELVIEW);
		} else if (convolution == Source::EMBOSS_CONVOLUTION) {
			glConvolutionFilter2D(GL_CONVOLUTION_2D, GL_RGB, 3, 3,	GL_LUMINANCE, GL_FLOAT, mEmboss);
		} else if (convolution == Source::EDGE_CONVOLUTION) {
			glConvolutionFilter2D(GL_CONVOLUTION_2D, GL_RGB, 3, 3,	GL_LUMINANCE, GL_FLOAT, mEdge);
		}

	}

	if (colorTable != Source::NO_COLORTABLE) {
		glEnable(GL_COLOR_TABLE);

		if (colorTable == Source::INVERT_COLORTABLE) {
			glColorTable(GL_COLOR_TABLE, GL_RGB, 256, GL_RGB, GL_UNSIGNED_BYTE, invertTable);
		} else if (colorTable == Source::COLOR_16_COLORTABLE) {
			glColorTable(GL_COLOR_TABLE, GL_RGB, 16, GL_RGB, GL_UNSIGNED_BYTE, colorTable16);
		} else if (colorTable == Source::COLOR_8_COLORTABLE) {
			glColorTable(GL_COLOR_TABLE, GL_RGB, 8, GL_RGB, GL_UNSIGNED_BYTE, colorTable8);
		} else if (colorTable == Source::COLOR_4_COLORTABLE) {
			glColorTable(GL_COLOR_TABLE, GL_RGB, 4, GL_RGB, GL_UNSIGNED_BYTE, colorTable4);
		} else if (colorTable == Source::COLOR_2_COLORTABLE) {
			glColorTable(GL_COLOR_TABLE, GL_RGB, 2, GL_RGB, GL_UNSIGNED_BYTE, colorTable2);
//			glColorTable(GL_COLOR_TABLE, GL_RGB, 4, GL_RGB, GL_UNSIGNED_BYTE, testTable);
		}

	}

	if (saturation != 0) {
		glMatrixMode(GL_COLOR);
		glLoadMatrixf(saturationMatrix);
		glMatrixMode(GL_MODELVIEW);
	}

}

void Source::endEffectsSection() const {

	// standard transparency blending
//	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
//	glBlendEquation(GL_FUNC_ADD);

	if (brightness != 0) {
		glPixelTransferf(GL_RED_BIAS, 0.0f);
		glPixelTransferf(GL_GREEN_BIAS, 0.0f);
		glPixelTransferf(GL_BLUE_BIAS, 0.0f);
	}

	if (contrast != 0) {
		//		glPixelTransferi(GL_MAP_COLOR, GL_FALSE);
		glPixelTransferf(GL_RED_SCALE, 1.0f);
		glPixelTransferf(GL_GREEN_SCALE, 1.0f);
		glPixelTransferf(GL_BLUE_SCALE, 1.0f);
	}

	if (pixelated) {
    	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	}

	// the other options require the GL_ARB_IMAGING extension
	if (!imaging_extension)
		return;

	if (convolution != Source::NO_CONVOLUTION) {
		glMatrixMode(GL_COLOR);
		glLoadIdentity();
		glMatrixMode(GL_MODELVIEW);
		glDisable(GL_CONVOLUTION_2D);
	}

	if (saturation != 0) {
		glMatrixMode(GL_COLOR);
		glLoadIdentity();
		glMatrixMode(GL_MODELVIEW);
	}

	if (colorTable != Source::NO_COLORTABLE) {
		glDisable(GL_COLOR_TABLE);
	}

	if (mask_type != Source::NO_MASK) {
		glActiveTexture(GL_TEXTURE1);
		glDisable(GL_TEXTURE_2D);
		glActiveTexture(GL_TEXTURE0);
	}
}

void Source::blend() const {

	glBlendEquation(blend_eq);
	glBlendFunc(source_blend, destination_blend);

	if (mask_type != Source::NO_MASK) {
		// activate texture 1 ; double texturing of the mask
		glActiveTexture(GL_TEXTURE1);
		// select and enable the texture corresponding to the mask
		glBindTexture(GL_TEXTURE_2D, maskTextureIndex);
		glEnable(GL_TEXTURE_2D);

		// back to texture 0 for the following
		glActiveTexture(GL_TEXTURE0);
	}
}

void Source::setMask(maskType t, GLuint texture) {

	mask_type = t;

	switch (t) {
	case Source::ROUNDCORNER_MASK:
		maskTextureIndex = ViewRenderWidget::mask_textures[0];
		break;
	case Source::CIRCLE_MASK:
		maskTextureIndex = ViewRenderWidget::mask_textures[1];
		break;
	case Source::GRADIENT_CIRCLE_MASK:
		maskTextureIndex = ViewRenderWidget::mask_textures[2];
		break;
	case Source::GRADIENT_SQUARE_MASK:
		maskTextureIndex = ViewRenderWidget::mask_textures[3];
		break;
	case Source::CUSTOM_MASK:
		if (texture != 0)
			maskTextureIndex = texture;
	default:
	case Source::NO_MASK:
		mask_type = Source::NO_MASK;
	}

}
