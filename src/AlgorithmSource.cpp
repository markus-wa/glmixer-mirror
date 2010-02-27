/*
 * AlgorithmSource.cpp
 *
 *  Created on: Feb 27, 2010
 *      Author: bh
 */

#include "AlgorithmSource.h"
#include <limits>

AlgorithmSource::AlgorithmSource(int type, GLuint texture, double d, int w, int h) : Source(texture, d), width(w), height(h) {

	algotype = CLAMP(AlgorithmSource::algorithmType(type), AlgorithmSource::FLAT, AlgorithmSource::WATER);
	// allocate and initialize the buffer
	buffer = new unsigned char [width * height * 4];
	initBuffer();

	aspectratio = (float)width / (float)height;

	// apply the texture
	glBindTexture(GL_TEXTURE_2D, textureIndex);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height,0, GL_BGRA, GL_UNSIGNED_BYTE, (unsigned char*) buffer);

}

AlgorithmSource::~AlgorithmSource() {
	delete [] buffer;
}

void AlgorithmSource::initBuffer(){

	if (algotype == AlgorithmSource::FLAT) {
		// CLEAR the buffer to white
		for (int i = 0; i < (width * height * 4); ++i)
			buffer[i] = std::numeric_limits<unsigned char>::max();
	}
}


void AlgorithmSource::update(){

	Source::update();

	// Immediately discard the FLAT 'algo' ; it is the "do nothing" algorithm :)
	if( algotype != AlgorithmSource::FLAT )
	{

	}

}
