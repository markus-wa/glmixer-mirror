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
	QObject(), Source(texture, d), is(f), bufferIndex(-1)
{
	if (!is)
		SourceConstructorException().raise();

	QObject::connect(is, SIGNAL(frameReady(int)), this, SLOT(updateFrame(int)));
	aspectratio = is->getStreamAspectRatio();

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, textureIndex);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

	// fills in the first frame
	const VideoPicture *vp = is->getPictureAtIndex(-1);
	if (vp && vp->isAllocated())
	{
		// fill in the texture
		if (vp->getFormat() == PIX_FMT_RGBA)
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, vp->getWidth(),
					vp->getHeight(), 0, GL_RGBA, GL_UNSIGNED_BYTE,
					vp->getBuffer());
		else
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, vp->getWidth(),
					vp->getHeight(), 0, GL_RGB, GL_UNSIGNED_BYTE,
					vp->getBuffer());
	}

}

VideoSource::~VideoSource()
{
	if (is)
		delete is;

	// free the OpenGL texture
	glDeleteTextures(1, &textureIndex);
}

bool VideoSource::isPlayable() const
{
	return (is->getEnd() - is->getBegin() > 1);
}

bool VideoSource::isPlaying() const
{
	return isPlayable() && is->isRunning();
}

void VideoSource::play(bool on)
{
	if (on != isPlaying())
		is->play(on);
}

bool VideoSource::isPaused() const
{
	return isPlaying() && is->isPaused();
}

void VideoSource::pause(bool on)
{
	if (on != isPaused())
		is->pause(on);
}


// only Rendering Manager can call this
void VideoSource::update()
{
	// update texture
	if (frameChanged && is)
	{

		const VideoPicture *vp = is->getPictureAtIndex(bufferIndex);

		// is the picture good ?
		if (vp && vp->isAllocated())
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
            // shouldn't update next time unless requested
            frameChanged = false;
#ifdef FFGL
            if (ffgl_plugin)
                ffgl_plugin->update();
#endif
        }
    }
}

void VideoSource::updateFrame(int i)
{
	frameChanged = true;
	bufferIndex = i;
}

void VideoSource::applyFilter()
{
	// if the video file is stopped or paused
	if (!is->isRunning() || is->isPaused())
	{
		// re-filter reset picture
		is->getPictureAtIndex(-1)->refilter();
		// re-filter whole buffer
		if (isPlayable())
		{
			for (int i = 0; i < VIDEO_PICTURE_QUEUE_SIZE; ++i)
				is->getPictureAtIndex(i)->refilter();
		}
	}
	// request to redraw the buffer
	frameChanged = true;
}

// Adjust brightness factor
void VideoSource::setBrightness(int b)
{
	Source::setBrightness(b);
	if (ViewRenderWidget::filteringEnabled())
		// use GLSL and disable VideoFile filtering
		is->setBrightness(0);
	else
	{
		// use VideoFile filter
		is->setBrightness(b);
		applyFilter();
	}
}

// Adjust contrast factor
void VideoSource::setContrast(int c)
{
	Source::setContrast(c);
	if (ViewRenderWidget::filteringEnabled())
		// use GLSL and disable VideoFile filtering
		is->setContrast(0);
	else
	{
		// use VideoFile filter
		is->setContrast(c);
		applyFilter();
	}
}

// Adjust saturation factor
void VideoSource::setSaturation(int s)
{
	Source::setSaturation(s);
	if (ViewRenderWidget::filteringEnabled())
		// use GLSL and disable VideoFile filtering
		is->setSaturation(0);
	else
	{
		// use VideoFile filter
		is->setSaturation(s);
		applyFilter();
	}
}

