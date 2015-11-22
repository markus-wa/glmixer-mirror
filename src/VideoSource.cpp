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

    // no PBO by default
    pboIds[0] = 0;
    pboIds[1] = 0;

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
        format = (_vp->getFormat() == AV_PIX_FMT_RGBA) ? GL_RGBA : GL_RGB;

        GLint preferedinternalformat = GL_RGB;

        if (glewIsSupported("GL_ARB_internalformat_query2"))
            glGetInternalformativ(GL_TEXTURE_2D, format, GL_INTERNALFORMAT_PREFERRED, 1, &preferedinternalformat);

        // create texture and fill-in with reset picture
        glTexImage2D(GL_TEXTURE_2D, 0, (GLenum) preferedinternalformat, _vp->getWidth(),
                     _vp->getHeight(), 0, format, GL_UNSIGNED_BYTE,
                     _vp->getBuffer());

        if (RenderingManager::usePboExtension()) {
            imgsize =  _vp->getWidth() * _vp->getHeight() * (format == GL_RGB ? 3 : 4);
            // create 2 pixel buffer objects,
            glGenBuffers(2, pboIds);
            // create first PBO
            glBindBuffer(GL_PIXEL_UNPACK_BUFFER, pboIds[0]);
            // glBufferDataARB with NULL pointer reserves only memory space.
            glBufferData(GL_PIXEL_UNPACK_BUFFER, imgsize, 0, GL_STREAM_DRAW);
            // fill in with reset picture
            GLubyte* ptr = (GLubyte*) glMapBuffer(GL_PIXEL_UNPACK_BUFFER, GL_WRITE_ONLY);
            if (ptr)  {
                // update data directly on the mapped buffer
                memmove(ptr, _vp->getBuffer(), imgsize);
                // release pointer to mapping buffer
                glUnmapBuffer(GL_PIXEL_UNPACK_BUFFER);
            }

            // idem with second PBO
            glBindBuffer(GL_PIXEL_UNPACK_BUFFER, pboIds[1]);
            glBufferData(GL_PIXEL_UNPACK_BUFFER, imgsize, 0, GL_STREAM_DRAW);
            ptr = (GLubyte*) glMapBuffer(GL_PIXEL_UNPACK_BUFFER, GL_WRITE_ONLY);
            if (ptr) {
                // update data directly on the mapped buffer
                memmove(ptr, _vp->getBuffer(), imgsize);
                // release pointer to mapping buffer
                glUnmapBuffer(GL_PIXEL_UNPACK_BUFFER);
            }
            glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);
            index = nextIndex = 0;
        }

    }

}

VideoSource::~VideoSource()
{
    if (is)
        delete is;

    if (vp)
        delete vp;

    // delete picture buffer
    if (pboIds[0] || pboIds[1])
        glDeleteBuffers(2, pboIds);
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

        if (pboIds[0] && pboIds[1]) {

            // In dual PBO mode, increment current index first then get the next index
            index = (index + 1) % 2;
            nextIndex = (index + 1) % 2;

            // bind PBO to read pixels
            glBindBuffer(GL_PIXEL_UNPACK_BUFFER, pboIds[index]);

            // copy pixels from PBO to texture object
            glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, vp->getWidth(), vp->getHeight(), format, GL_UNSIGNED_BYTE, 0);

            // bind PBO to update pixel values
            glBindBuffer(GL_PIXEL_UNPACK_BUFFER, pboIds[nextIndex]);

            glBufferData(GL_PIXEL_UNPACK_BUFFER, imgsize, 0, GL_STREAM_DRAW);

            // map the buffer object into client's memory
            GLubyte* ptr = (GLubyte*) glMapBuffer(GL_PIXEL_UNPACK_BUFFER, GL_WRITE_ONLY);
            if (ptr) {
                // update data directly on the mapped buffer
                memmove(ptr, vp->getBuffer(), imgsize);
                // release pointer to mapping buffer
                glUnmapBuffer(GL_PIXEL_UNPACK_BUFFER);
            }

            glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);

        } else {
            glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, vp->getWidth(),
                            vp->getHeight(), format, GL_UNSIGNED_BYTE, vp->getBuffer());
        }

        // done! Cancel (free) updated frame
        updateFrame(NULL);

    }

    Source::update();
}

void VideoSource::updateFrame(VideoPicture *p)
{
    // free the previous video picture if tagged as to be deleted.
    if (vp && vp->hasAction(VideoPicture::ACTION_DELETE))
        delete vp;

    // set new vp
    vp = p;

}


