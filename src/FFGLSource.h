/*
 * FFGLSource.h
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

#ifndef FFGLSOURCE_H
#define FFGLSOURCE_H

#include <QObject>

#include "Source.h"

class FFGLSource : public QObject, public Source
{
    Q_OBJECT

    friend class RenderingManager;
    friend class OutputRenderWidget;
    friend class FFGLSourceCreationDialog;

protected:
    // only friends can create a source
    FFGLSource(QString pluginFileName, GLuint texture, double d, int w = 256, int h = 256);
    virtual ~FFGLSource();
    void update();

public:

    FFGLPluginSourceStack getFreeframeGLPluginStack();
    bool hasFreeframeGLPlugin();

    RTTI rtti() const { return type; }
    bool isPlayable() const { return playable; }
    bool isPlaying() const;

    int getFrameWidth() const;
    int getFrameHeight() const;

public Q_SLOTS:
    void play(bool on);

private:
    static RTTI type;
    static bool playable;

    class FFGLPluginSource *_plugin;
    bool _playing;
    unsigned char *_buffer;
    GLuint _sourceTextureIndex;
};

#endif // FFGLSOURCE_H
