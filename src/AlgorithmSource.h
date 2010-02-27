/*
 * AlgorithmSource.h
 *
 *  Created on: Feb 27, 2010
 *      Author: bh
 */

#ifndef ALGORITHMSOURCE_H_
#define ALGORITHMSOURCE_H_

#include "Source.h"
#include "RenderingManager.h"

class AlgorithmSource: public Source {

	friend class RenderingManager;

    // only RenderingManager can create a source
protected:
	AlgorithmSource(int type, GLuint texture, double d, int w = 256, int h = 256);
	~AlgorithmSource();

	void update();

private:

	void initBuffer();

	typedef enum {FLAT = 0, BW_NOISE, COLOR_NOISE, PERLIN_BW_NOISE, PERLIN_COLOR_NOISE, WATER} algorithmType;
	algorithmType algotype;
	unsigned char *buffer;
	int width, height;
};

#endif /* ALGORITHMSOURCE_H_ */
