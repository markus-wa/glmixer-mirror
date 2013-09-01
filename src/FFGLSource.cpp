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


FFGLSource::FFGLSource(QString pluginFileName, double d, int w, int h):
    QObject(), Source(0, d), _plugin(0), _playing(true)
{
    aspectratio = double(w) / double(h);

    if ( !QFileInfo(pluginFileName).isFile()) {
        qCritical() << tr("FFGLSource given an invalid file") << " ("<< pluginFileName <<")";
        SourceConstructorException().raise();
    }

    // create new plugin with this file
    FFGLTextureStruct it;
    it.Handle = 0;
    it.Width = 0;
    it.Height = 0;
    it.HardwareWidth = 0;
    it.HardwareHeight = 0;
    _plugin = new FFGLPluginSource(pluginFileName, w, h, it);

    // no exceptions raised, continue with the plugin
    // try to update
    _plugin->update();

    // this source behaves like a normal source, except the texture index
    // comes from the plugin's FBO
    textureIndex = _plugin->FBOTextureStruct().Handle;

}

FFGLSource::~FFGLSource()
{
    if (_plugin)
        delete _plugin;
}

int FFGLSource::getFrameWidth() const {

    return _plugin->FBOTextureStruct().Width;
}

int FFGLSource::getFrameHeight() const {

    return _plugin->FBOTextureStruct().Height;
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

FFGLPluginSourceStack FFGLSource::getFreeframeGLPluginStack() {

    FFGLPluginSourceStack tmp(_ffgl_plugins);
    tmp.push_front(_plugin);
    return  tmp;
}

bool FFGLSource::hasFreeframeGLPlugin() {

    return true;
}
