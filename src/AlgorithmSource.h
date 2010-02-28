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

public:

	typedef enum {FLAT = 0, BW_NOISE, COLOR_NOISE, PERLIN_BW_NOISE, PERLIN_COLOR_NOISE, WATER} algorithmType;

    inline algorithmType getAlgorithmType() const { return algotype; }
	inline int getFrameWidth() const { return width; }
	inline int getFrameHeight() const { return height; }
	inline double getFrameRate() const { return framerate; }

	static QString getAlgorithmDescription(algorithmType t);

private:

	void initBuffer();

	algorithmType algotype;
	unsigned char *buffer;
	int width, height;
	double framerate;
};

#endif /* ALGORITHMSOURCE_H_ */
