/*
 * FFGLSource.cpp
 *
 *  Created on: June 23 2013
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

#include <limits>

#include "FFGLSource.moc"

#include "FFGLPluginSource.h"

Source::RTTI FFGLSource::type = Source::FFGL_SOURCE;
bool FFGLSource::playable = true;


FFGLSource::FFGLSource(QString pluginFileName, GLuint texture, double d, int w, int h):
    QObject(), Source(texture, d), _plugin(0), _playing(true), _buffer(0)
{
    aspectratio = double(w) / double(h);

    if ( !QFileInfo(pluginFileName).isFile()) {
        qCritical() << tr("FFGLSource given an invalid file") << " ("<< pluginFileName <<")";
        SourceConstructorException().raise();
    }

    // create new plugin with this file
    FFGLTextureStruct it;
    it.Handle = texture;
    it.Width = 0;
    it.Height = 0;
    it.HardwareWidth = 0;
    it.HardwareHeight = 0;
    _plugin = new FFGLPluginSource(pluginFileName, w, h, it);

    // if plugin not of type source, create a buffer and a texture for applying effect
    if (!_plugin->isSourceType()) {
        _buffer = new unsigned char[w * h * 4];
        CHECK_PTR_EXCEPTION(_buffer);
        // CLEAR the buffer to transparent black
        memset((void *) _buffer, 0, w * h * 4);
        // apply buffer to the texture
        glBindTexture(GL_TEXTURE_2D, texture);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, w, h, 0, GL_BGRA,
                GL_UNSIGNED_INT_8_8_8_8_REV, (unsigned char*) _buffer);
    }

    // no exceptions raised, continue with the plugin
    // try to update
    _plugin->update();

    // this source behaves like a normal source, except the texture index
    // comes from the plugin's FBO
    _sourceTextureIndex = texture;
    textureIndex = _plugin->getOutputTextureStruct().Handle;

}

FFGLSource::~FFGLSource()
{
    // free the OpenGL texture
    glDeleteTextures(1, &_sourceTextureIndex);

    // delete picture buffer
    if (_buffer)
        delete[] _buffer;

    if (_plugin)
        delete _plugin;
}

int FFGLSource::getFrameWidth() const {

    return _plugin->getOutputTextureStruct().Width;
}

int FFGLSource::getFrameHeight() const {

    return _plugin->getOutputTextureStruct().Height;
}


bool FFGLSource::isPlaying() const
{
    return _playing;
}

void FFGLSource::play(bool on)
{
    _playing = on;

    if (_plugin)
        _plugin->setPaused(!on);
}

void FFGLSource::update() {

    try {
        // call the update on the ffgl plugin
        if (_plugin && _playing)
            _plugin->update();

    }
    catch (FFGLPluginException &e) {
        this->play(false);
        qCritical() <<  e.message() << QObject::tr("\nThe source was stopped.");
    }

    // normal update (other ffgl plugins)
    Source::update();
}

//FFGLPluginSourceStack *FFGLSource::getFreeframeGLPluginStack() {

//    FFGLPluginSourceStack tmp(_ffgl_plugins);
//    tmp.push_front(_plugin);
//    return  tmp;
//}

//bool FFGLSource::hasFreeframeGLPlugin() {

//    return true;
//}
