/*
 * MixSource.cpp
 *
 *  Created on: Jul 13, 2010
 *      Author: bh
 */

#include <MixSource.h>

Source::RTTI MixSource::type = Source::MIX_SOURCE;

MixSource::MixSource(GLuint texture, double d): Source(texture, d) {


}

MixSource::~MixSource()
{

}

void MixSource::update(){

	Source::update();


}
