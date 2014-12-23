/*
 * VideoSource.cpp
 *
 *  Created on: 3 nov. 2009
 *      Author: herbelin
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

#include "VideoSource.moc"

#include "VideoFile.h"
#include "RenderingManager.h"

#include <QGLFramebufferObject>

Source::RTTI VideoSource::type = Source::VIDEO_SOURCE;

VideoSource::VideoSource(VideoFile *f, GLuint texture, double d) :
    QObject(), Source(texture, d), is(f), vp(NULL) //bufferIndex(-1)
{
	if (!is)
		SourceConstructorException().raise();

    QObject::connect(is, SIGNAL(frameReady(VideoPicture *)), this, SLOT(updateFrame(VideoPicture *)));
	aspectratio = is->getStreamAspectRatio();

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, textureIndex);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

    // fills in the first frame
    const VideoPicture *_vp = is->getResetPicture();
    if (_vp)
	{
		// fill in the texture
        if (_vp->getFormat() == PIX_FMT_RGBA)
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, _vp->getWidth(),
                    _vp->getHeight(), 0, GL_RGBA, GL_UNSIGNED_BYTE,
                    _vp->getBuffer());
		else
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, _vp->getWidth(),
                    _vp->getHeight(), 0, GL_RGB, GL_UNSIGNED_BYTE,
                    _vp->getBuffer());
	}

}

VideoSource::~VideoSource()
{
    if (is)
        delete is;

    if (vp)
        delete vp;

}

bool VideoSource::isPlayable() const
{
    return (is && is->getNumFrames() > 1);
}

bool VideoSource::isPlaying() const
{
    return ( isPlayable() && is->isRunning() );
}

void VideoSource::play(bool on)
{
    Source::play(on);

    if ( isPlaying() == on )
        return;

    // cancel updated frame
    updateFrame(NULL);

    // transfer the order to the videoFile
    is->play(on);

}

bool VideoSource::isPaused() const
{
    return ( isPlaying() && is->isPaused() );
}

void VideoSource::pause(bool on)
{
	if (on != isPaused())
		is->pause(on);
}


// only Rendering Manager can call this
void VideoSource::update()
{
    // update texture if given a new vp
    if ( vp && vp->getBuffer() != NULL )
	{
        glBindTexture(GL_TEXTURE_2D, textureIndex);
        // use it for OpenGL
        if (vp->getFormat() == PIX_FMT_RGBA)
            glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, vp->getWidth(),
                    vp->getHeight(), GL_RGBA, GL_UNSIGNED_BYTE,
                    vp->getBuffer());
        else
            glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, vp->getWidth(),
                    vp->getHeight(), GL_RGB, GL_UNSIGNED_BYTE,
                    vp->getBuffer());

        // done! Cancel updated frame
        updateFrame(NULL);
    }

    Source::update();
}

void VideoSource::updateFrame(VideoPicture *p)
{
    // free the video picture if tagged as to be deleted.
    if (vp && vp->hasAction(VideoPicture::ACTION_DELETE))
        delete vp;

    // set new vp
    vp = p;
}


