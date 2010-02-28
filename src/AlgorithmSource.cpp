/*
 * AlgorithmSource.cpp
 *
 *  Created on: Feb 27, 2010
 *      Author: bh
 */

#include "AlgorithmSource.h"
#include <limits>
#include <iostream>
#include <ctime>

AlgorithmSource::AlgorithmSource(int type, GLuint texture, double d, int w, int h) : Source(texture, d), width(w), height(h), framerate(0) {

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


	algotype = COLOR_NOISE;
}

AlgorithmSource::~AlgorithmSource() {
	delete [] buffer;

	// free the OpenGL texture
	glDeleteTextures(1, &textureIndex);
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
		srand( clock () );
		if ( algotype == AlgorithmSource::BW_NOISE ){
			for (int i = 0; i < (width * height); ++i) {
				buffer[i * 4 + 0] = (unsigned char) ( rand() % std::numeric_limits<unsigned char>::max());
				buffer[i * 4 + 1] = buffer[i * 4];
				buffer[i * 4 + 2] = buffer[i * 4];
				buffer[i * 4 + 3] = buffer[i * 4];
			}

		} else
		if ( algotype == AlgorithmSource::COLOR_NOISE ){
			for (int i = 0; i < (width * height * 4); ++i)
//				buffer[i] = (unsigned char) (  rand() % std::numeric_limits<unsigned char>::max() );
			buffer[i] = (unsigned char) ((  rand() % 2 ) * std::numeric_limits<unsigned char>::max());

		}

		glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, width, height, GL_BGRA, GL_UNSIGNED_BYTE, (unsigned char*) buffer);
	}

}


QString AlgorithmSource::getAlgorithmDescription(algorithmType t) {

	QString description;
	switch (t) {
	case FLAT:
		description = QString("Flat color");
		break;
	case BW_NOISE:
		description = QString("Black and white noise");
		break;
	case COLOR_NOISE:
		description = QString("Color noise");
		break;
	case PERLIN_BW_NOISE:
		description = QString("Black and white Perlin noise");
		break;
	case PERLIN_COLOR_NOISE:
		description = QString("Color Perlin noise");
		break;
	case WATER:
		description = QString("Water effect");
		break;
	}

	return description;
}

