/*
 * AlgorithmSource.cpp
 *
 *  Created on: Feb 27, 2010
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

#include "AlgorithmSource.moc"

Source::RTTI AlgorithmSource::type = Source::ALGORITHM_SOURCE;
bool AlgorithmSource::playable = true;

#define PERLIN_WIDTH 128
#define PERLIN_HEIGHT 128

#include <limits>
#include <iostream>

#include "common.h"
#include "SourcePropertyBrowser.h"

#include <QMutex>
#include <QWaitCondition>
#include <QThread>
#include <QTime>
#include <QDateTime>
#include <cmath>


/**
 * HSV to RGB
 */

void HsvToRgb(unsigned char hsv[], unsigned char rgb[])
{
    rgb[3] = hsv[3];

    unsigned char region, remainder, p, q, t;

    if (hsv[1] == 0)
    {
        rgb[0] = hsv[2];
        rgb[1] = hsv[2];
        rgb[2] = hsv[2];
        return;
    }

    region = hsv[0] / 43;
    remainder = (hsv[0] - (region * 43)) * 6;

    p = (hsv[2] * (255 - hsv[1])) >> 8;
    q = (hsv[2] * (255 - ((hsv[1] * remainder) >> 8))) >> 8;
    t = (hsv[2] * (255 - ((hsv[1] * (255 - remainder)) >> 8))) >> 8;

    switch (region)
    {
        case 0:
            rgb[0] = hsv[2]; rgb[1] = t; rgb[2] = p;
            break;
        case 1:
            rgb[0] = q; rgb[1] = hsv[2]; rgb[2] = p;
            break;
        case 2:
            rgb[0] = p; rgb[1] = hsv[2]; rgb[2] = t;
            break;
        case 3:
            rgb[0] = p; rgb[1] = q; rgb[2] = hsv[2];
            break;
        case 4:
            rgb[0] = t; rgb[1] = p; rgb[2] = hsv[2];
            break;
        default:
            rgb[0] = hsv[2]; rgb[1] = p; rgb[2] = q;
            break;
    }

    return;
}


/**
 * PERLIN NOISE
 */

static int p[512];
static int permutation[] = { 151, 160, 137, 91, 90, 15, 131, 13, 201, 95, 96,
                             53, 194, 233, 7, 225, 140, 36, 103, 30, 69, 142, 8, 99, 37, 240, 21, 10,
                             23, 190, 6, 148, 247, 120, 234, 75, 0, 26, 197, 62, 94, 252, 219, 203,
                             117, 35, 11, 32, 57, 177, 33, 88, 237, 149, 56, 87, 174, 20, 125, 136,
                             171, 168, 68, 175, 74, 165, 71, 134, 139, 48, 27, 166, 77, 146, 158,
                             231, 83, 111, 229, 122, 60, 211, 133, 230, 220, 105, 92, 41, 55, 46,
                             245, 40, 244, 102, 143, 54, 65, 25, 63, 161, 1, 216, 80, 73, 209, 76,
                             132, 187, 208, 89, 18, 169, 200, 196, 135, 130, 116, 188, 159, 86, 164,
                             100, 109, 198, 173, 186, 3, 64, 52, 217, 226, 250, 124, 123, 5, 202, 38,
                             147, 118, 126, 255, 82, 85, 212, 207, 206, 59, 227, 47, 16, 58, 17, 182,
                             189, 28, 42, 223, 183, 170, 213, 119, 248, 152, 2, 44, 154, 163, 70,
                             221, 153, 101, 155, 167, 43, 172, 9, 129, 22, 39, 253, 19, 98, 108, 110,
                             79, 113, 224, 232, 178, 185, 112, 104, 218, 246, 97, 228, 251, 34, 242,
                             193, 238, 210, 144, 12, 191, 179, 162, 241, 81, 51, 145, 235, 249, 14,
                             239, 107, 49, 192, 214, 31, 181, 199, 106, 157, 184, 84, 204, 176, 115,
                             121, 50, 45, 127, 4, 150, 254, 138, 236, 205, 93, 222, 114, 67, 29, 24,
                             72, 243, 141, 128, 195, 78, 66, 215, 61, 156, 180 };

/* Function declarations */
double fade(double t);
double lerp(double t, double a, double b);
double grad(int hash, double x, double y, double z);
void init_pnoise();
double pnoise(double x, double y, double z);

void init_pnoise() {
    int i;
    for (i = 0; i < 256; i++)
        p[256 + i] = p[i] = permutation[i];
}

double pnoise(double x, double y, double z) {
    int X = (int) floor(x) & 255, /* FIND UNIT CUBE THAT */
            Y = (int) floor(y) & 255, /* CONTAINS POINT.     */
            Z = (int) floor(z) & 255;
    x -= floor(x); /* FIND RELATIVE X,Y,Z */
    y -= floor(y); /* OF POINT IN CUBE.   */
    z -= floor(z);
    double u = fade(x), /* COMPUTE FADE CURVES */
            v = fade(y), /* FOR EACH OF X,Y,Z.  */
            w = fade(z);
    int A = p[X] + Y, AA = p[A] + Z, AB = p[A + 1] + Z, /* HASH COORDINATES OF */
            B = p[X + 1] + Y, BA = p[B] + Z, BB = p[B + 1] + Z; /* THE 8 CUBE CORNERS, */

    return lerp(w, lerp(v, lerp(u, grad(p[AA], x, y, z), /* AND ADD */
                                grad(p[BA], x - 1, y, z)), /* BLENDED */
                        lerp(u, grad(p[AB], x, y - 1, z), /* RESULTS */
                             grad(p[BB], x - 1, y - 1, z))), /* FROM  8 */
                lerp(v, lerp(u, grad(p[AA + 1], x, y, z - 1),/* CORNERS */
                     grad(p[BA + 1], x - 1, y, z - 1)), /* OF CUBE */
            lerp(u, grad(p[AB + 1], x, y - 1, z - 1),
            grad(p[BB + 1], x - 1, y - 1, z - 1))));
}

double fade(double t) {
    return t * t * t * (t * (t * 6 - 15) + 10);
}
double lerp(double t, double a, double b) {
    return a + t * (b - a);
}
double grad(int hash, double x, double y, double z) {
    int h = hash & 15; /* CONVERT LO 4 BITS OF HASH CODE */
    double u = h < 8 ? x : y, /* INTO 12 GRADIENT DIRECTIONS.   */
            v = h < 4 ? y : h == 12 || h == 14 ? x : z;
    return ((h & 1) == 0 ? u : -u) + ((h & 2) == 0 ? v : -v);
}

double turb(double x, double y, double z, double minFreq, double maxFreq) {
    double r = 0.0;
    x += 123.456;
    for (double freq = minFreq; freq < maxFreq; freq *= 2.0 * freq) {
        r += ABS( pnoise (x, y, z) ) / freq;
        x *= 2.0;
        y *= 2.0;
        z *= 2.0;
    }
    return r - 0.3;
}

unsigned char randDisp(double disp) {

    return (unsigned char) (127.0 * disp);
    //    return (unsigned char) ( 127.0 * ( ((double)rand()) / ((double)RAND_MAX) ) *  disp);
}

/**
 *  Thread class to update the texture
 */

class AlgorithmThread: public QThread {
public:
    AlgorithmThread(AlgorithmSource *source) :
        QThread(), as(source), end(false), phase(1), i(0.0), j(0.0), k(0.0), l(0.0), di(0.5), dj(0.4), dk(0.3), dl(0.7) {

    }

    void run();
    void fill(double);

    AlgorithmSource* as;
    bool end;
    int phase;

private:

    double i, j, k, l, di, dj, dk, dl;
};

void AlgorithmThread::run() {

    QTime t;
    as->framerate = 30.0;
    unsigned long e = 0;

    t.start();
    do
    {
        as->_mutex->lock();
        if (!as->frameChanged) {

            // fill frame
            if (as->variability > EPSILON )
                fill(as->variability);

            as->frameChanged = true;
            as->_cond->wait(as->_mutex);
        }
        as->_mutex->unlock();

        // computing time
        e = CLAMP( (unsigned long) t.elapsed() * 1000, 1000, as->period - 500 ) ;

        // wait for the period duration minus time spent before updating next frame
        usleep( as->period - e);

        // exponential moving average to compute FPS
        as->framerate = 0.7 * 1000.0 / (double) t.restart() + 0.3 * as->framerate;

        // change random
        srand( (unsigned int) QDateTime::currentMSecsSinceEpoch() );
    }
    while (!end);

}

void AlgorithmThread::fill(double var) {

    if (!as->buffer)
        return;

    if (as->algotype == AlgorithmSource::FLAT) {

        // linear color change white <-> black
        int a = phase * int(var * var * 255.0);
        int c = (int) as->buffer[0] + a;
        phase = c > 255 ? -phase : c < 0 ? -phase : phase ;
        // fill image
        memset((void *) as->buffer, (unsigned char) CLAMP(c, 0, 255), as->width * as->height * 4);


//        double t = (double) (as->width * as->width + as->height * as->height);
//        for (int x = 0; x < as->width; ++x)
//            for (int y = 0; y < as->height; ++y)
////                memset((void *) (as->buffer + (y * as->width + x) * 4), (unsigned char) ( 256.0 * ( (double) (x * x + y * y) / (double) t ) ), 4 );
//                memset((void *) (as->buffer + (y * as->width + x) * 4), c * (unsigned char) ( 256.0 * ( (double) (x * x + y * y) / (double) t ) ), 4 );

    }
    else if (as->algotype == AlgorithmSource::FLAT_COLOR) {

        unsigned char hsv[4] = {0, 255, 255, 255};
        unsigned char rgb[4] = {0, 0, 0, 255};

        // vary Hue
        hsv[0] = (unsigned char) ( 255.0 + phase ) % (256);
        phase = (phase + int(var * 25.6)) % (256);
        // convert to RGB
        HsvToRgb(hsv, rgb);
        // fill 1 line
        for (int x = 0; x < as->width; ++x)
            memmove((void *) (as->buffer + x * 4), rgb, 4 );
        // duplicate line
        for (int y = 1; y < as->height ; ++y)
            memmove((void *) (as->buffer + y * as->width * 4), as->buffer, as->width * 4 );

    }
    else if (as->algotype == AlgorithmSource::BW_LINES) {

        // linear color change white <-> black
        int a = phase * int(var * var * 255.0);
        int c = (int) as->buffer[0] + a;
        phase = c > 255 ? -phase : c < 0 ? -phase : phase ;
        // alternate B&W
        unsigned char b = (unsigned char) CLAMP(c, 0, 255);
        unsigned char w = 255 - b;
        // fill lines
        for (int y = 0; y < as->height ; ++y)
            memset((void *) (as->buffer + y * as->width * 4), y%2 ? w : b, 4 * as->width );

    }
    else if (as->algotype == AlgorithmSource::COLOR_LINES) {

        unsigned char hsv[4] = {0, 255, 255, 255};

        // create first line
        for (int x = 0; x < as->width; ++x) {
            // vary Hue
            hsv[0] = (unsigned char) ( ( (double) x / (double) as->width ) * 255.0 + phase ) % (255);
            // convert to RGB
            HsvToRgb(hsv, as->buffer + x * 4);
        }

        // duplicate line per bloc
        for (int y = 1; y < as->height ; ++y)
            memmove((void *) (as->buffer + y * as->width * 4), as->buffer, as->width * 4 );

        phase = (phase + int(var * 25.6)) % 256;
    }
    else if (as->algotype == AlgorithmSource::BW_CHECKER) {

        // linear color change white <-> black
        int a = phase * int(var * var * 255.0);
        int c = (int) as->buffer[0] + a;
        phase = c > 255 ? -phase : c < 0 ? -phase : phase ;
        // alternate B&W
        unsigned char b = (unsigned char) CLAMP(c, 0, 255);
        unsigned char w = 255 - b;
        bool on = false;

        // fast checkerboard if even numbers
        if ( as->height > 2 && !(as->width % 2) && !(as->height%2) ) {
            int y = 0;
            // create two first lines
            for (; y < 2; ++y) {
                on = y%2;
                for (int x = 0; x < as->width; ++x, on = !on)
                    memset((void *) (as->buffer + (y * as->width + x) * 4),
                            on ? w : b , 4);
            }
            // duplicate lines per bloc
            for (; y < as->height ; y += 2)
                memmove((void *) (as->buffer + y * as->width  * 4), as->buffer, as->width * 4 * 2);
        }
        else {
            for (int x = 0; x < as->width; ++x) {
                on = x%2;
                for (int y = 0; y < as->height; ++y, on = !on)
                    memset((void *) (as->buffer + (y * as->width + x) * 4),
                            on ? w : b , 4);
            }
        }
    }
    else if (as->algotype == AlgorithmSource::BW_NOISE) {

        // initial frame
        if (phase > 0) {
            phase = 0;
            for (int i = 0; i < (as->width * as->height); ++i)
                memset((void *) (as->buffer + i * 4), (unsigned char) ( rand() % 256 ), 4);
        }
        // update incremental
        else {
            for (int i = 0; i < (as->width * as->height); ++i)
                memset((void *) (as->buffer + i * 4),
                    ( as->buffer[i*4] + (unsigned char) ( var * double( rand() % 256) ) ) % 256, 4);
        }
    }
    else if (as->algotype == AlgorithmSource::BW_COSBARS) {

        unsigned char c = 0;
        phase = (phase + int(var * 36.0)) % (360);

        for (int y = 0; y < as->height ; ++y) {
            c = (unsigned char) (cos(  double(phase) * M_PI / 180.0
                                       + double(y) * 2.0 * M_PI
                                       / double(as->height)) * 127.0
                                 + 128.0);
            memset((void *) (as->buffer + y * as->width * 4), c , 4 * as->width );
        }

    }
    else if (as->algotype == AlgorithmSource::BW_COSCHECKER) {

        unsigned char c = 0;
        phase = (phase + int(var * 36.0)) % (360);
        double p = double(phase) * M_PI / 180.0;

        for (int x = 0; x < as->width; ++x)
            for (int y = 0; y < as->height; ++y) {
                c = (unsigned char) (cos( p + double(x) * 2.0 * M_PI / double(as->width)) * 63.0
                                     + 64.0);
                c += (unsigned char) (cos( p+ double(y) * 2.0 * M_PI / double(as->height)) * 63.0
                                      + 64.0);
                memset( (void *) (as->buffer + (y * as->width + x) * 4), c, 4);
            }

    }
    else if (as->algotype == AlgorithmSource::COLOR_NOISE) {

        // initial frame
        if (phase > 0) {
            phase = 0;
            for (int i = 0; i < (as->width * as->height * 4); ++i)
                as->buffer[i] = (unsigned char) ( rand() % 256 );
        }
        // update incremental
        else {
            for (int i = 0; i < (as->width * as->height * 4); ++i)
                as->buffer[i] = ( as->buffer[i] + (unsigned char) ( var * double( rand() % 256) ) ) % 256;
        }
    }
    else if (as->algotype == AlgorithmSource::PERLIN_BW_NOISE) {

        i += di * var;
        if (i > 100000.0 || i < 0.0)
            di = -di;
        for (int x = 0; x < as->width; ++x)
            for (int y = 0; y < as->height; ++y) {
                double v = pnoise(double(x) * as->horizontal,
                                  double(y) * as->vertical, i);
                memset(  (void *) (as->buffer  + (y * as->width + x) * 4),
                         (unsigned char) (128.0 * v) + 128, 4);
            }
    }
    else if (as->algotype  == AlgorithmSource::PERLIN_COLOR_NOISE) {

        i += var * di;
        j += var * dj;
        k += var * dk;
        l += var * dl;
        if (i > 100000.0 || i < 0.0)
            di = -di;
        for (int x = 0; x < as->width; ++x)
            for (int y = 0; y < as->height; ++y) {
                double v = pnoise(double(x) * as->horizontal,
                                  double(y) * as->vertical, i);
                as->buffer[(y * as->width + x) * 4 + 0] =
                        (unsigned char) (128.0 * v + 128);
                v = pnoise(double(x) * as->horizontal,
                           double(y) * as->vertical, j);
                as->buffer[(y * as->width + x) * 4 + 1] =
                        (unsigned char) (128.0 * v + 128);
                v = pnoise(double(x) * as->horizontal,
                           double(y) * as->vertical, k);
                as->buffer[(y * as->width + x) * 4 + 2] =
                        (unsigned char) (128.0 * v + 128);
                v = pnoise(double(x) * as->horizontal,
                           double(y) * as->vertical, l);
                as->buffer[(y * as->width + x) * 4 + 3] =
                        (unsigned char) (128.0 * v + 128);
                //								as->buffer[(y * as->width + x) * 4 + 3 ] = (unsigned char) 255;
            }
    }
    else if (as->algotype == AlgorithmSource::TURBULENCE) {

        i += var * di;
        if (i > 100000.0 || i < 0.0)
            di = -di;
        for (int x = 0; x < as->width; ++x)
            for (int y = 0; y < as->height; ++y) {
                double v = turb(double(x) * as->horizontal,
                                double(y) * as->vertical, i, 1.0, 16.0);
                memset( (void *) (as->buffer  + (y * as->width + x) * 4),
                        (unsigned char) (128.0 * v) + 128, 4);
            }
    }

}

AlgorithmSource::AlgorithmSource(int type, GLuint texture, double d, int w,
                                 int h, double v, unsigned long p, bool ia) :
    Source(texture, d), buffer(0), pattern(0), width(w), height(h), period(p), framerate(0), vertical( 1.0), horizontal(1.0), ignoreAlpha(false), frameChanged(false), format(GL_RGBA)
{
    // no PBO by default
    pboIds = 0;
    setVariability(v);

    algotype = CLAMP(AlgorithmSource::algorithmType(type), AlgorithmSource::FLAT, AlgorithmSource::NONE);

    switch (algotype) {
    case PERLIN_BW_NOISE:
    case PERLIN_COLOR_NOISE:
    case TURBULENCE:
        horizontal = 0.001 * width;
        vertical = 0.001 * height;
        width = PERLIN_WIDTH;
        height = PERLIN_HEIGHT;
        init_pnoise();
        break;
    default:
        break;
    }

    // allocate and initialize the buffer
    initBuffer();

    // create the texture
    setIgnoreAlpha(ia);

    // if no period given, set to default 40Hz
    if (period <= 0)
        period = 25000;

    // create thread
    _mutex = new QMutex;
    CHECK_PTR_EXCEPTION(_mutex);
    _cond = new QWaitCondition;
    CHECK_PTR_EXCEPTION(_cond);
    _thread = new AlgorithmThread(this);
    CHECK_PTR_EXCEPTION(_thread);

    // fill up first frame
    _thread->end = true;
    _thread->start();
    _thread->setPriority(QThread::LowPriority);

    update();
}

AlgorithmSource::~AlgorithmSource() {

    // stop play
    _thread->end = true;
    _mutex->lock();
    _cond->wakeAll();
    _mutex->unlock();
    // wait for usleep pediod time + 100 ms buffer
    _thread->wait(100 + period / 1000);

    delete _thread;
    delete _cond;
    delete _mutex;

    // delete picture buffer
    if (pboIds)
        glDeleteBuffers(1, &pboIds);
    else if (buffer)
        delete[] buffer;

    if (pattern)
        delete[] pattern;
}

QString AlgorithmSource::getInfo() const {

    return Source::getInfo() + tr(" - Algorithm : ") + getAlgorithmDescription(algotype);
}

void AlgorithmSource::setVariability(double v) {

    variability = CLAMP(v, 0.001, 1.0);

}

void AlgorithmSource::play(bool on) {

    if (isPlaying() == on)
        return;

    if (on) {
        // start play
        _thread->end = false;
        _thread->start();
    }
    else {
        // stop play
        _thread->end = true;
        _mutex->lock();
        _cond->wakeAll();
        _mutex->unlock();
        // wait for usleep pediod time + 100 ms buffer
        _thread->wait(100 + period / 1000);
        // make sure last frame is displayed
        frameChanged = false;
    }

    Source::play(on);
}

bool AlgorithmSource::isPlaying() const {

    return !_thread->end;

}

void AlgorithmSource::initBuffer() {

    if (RenderingManager::usePboExtension()) {
        // create a pixel buffer object,
        glGenBuffers(1, &pboIds);
        glBindBuffer(GL_PIXEL_UNPACK_BUFFER, pboIds);
        glBufferData(GL_PIXEL_UNPACK_BUFFER, width * height * 4, 0, GL_STREAM_DRAW);
        buffer = (GLubyte*) glMapBuffer(GL_PIXEL_UNPACK_BUFFER, GL_WRITE_ONLY);
        CHECK_PTR_EXCEPTION(buffer);
        // CLEAR the buffer
        memset((void *) buffer, std::numeric_limits<unsigned char>::min(),  width * height * 4);
        glUnmapBuffer(GL_PIXEL_UNPACK_BUFFER); // release pointer to mapping buffer
        glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);

    } else {
        buffer = new unsigned char[width * height * 4];
        CHECK_PTR_EXCEPTION(buffer);
        // CLEAR the buffer
        memset((void *) buffer, std::numeric_limits<unsigned char>::min(),  width * height * 4);
    }

    pattern = 0;
}

void AlgorithmSource::update() {

    if (frameChanged) {

        // bind the texture
        glBindTexture(GL_TEXTURE_2D, textureIndex);

        if (pboIds) {

            // bind PBO
            glBindBuffer(GL_PIXEL_UNPACK_BUFFER, pboIds);

            // lock filling thread
            _mutex->lock();

            glBufferData(GL_PIXEL_UNPACK_BUFFER, width * height * 4, 0, GL_STREAM_DRAW);

            // map the buffer object into client's memory
            GLubyte* ptr = (GLubyte*)glMapBuffer(GL_PIXEL_UNPACK_BUFFER, GL_WRITE_ONLY);
            if (ptr) {
                // update data directly on the mapped buffer
                buffer = ptr;
                glUnmapBuffer(GL_PIXEL_UNPACK_BUFFER); // release pointer to mapping buffer
            }
            else
                buffer = 0;

            // unlock filling thread
            frameChanged = false;
            _cond->wakeAll();
            _mutex->unlock();

            // copy pixels from PBO to texture object
            glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, width, height, GL_RGBA, GL_UNSIGNED_BYTE, 0);

            // release PBOs with ID 0 after use.
            glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);

        } else {

            _mutex->lock();
            glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, width, height, GL_RGBA,
                            GL_UNSIGNED_BYTE, (unsigned char*) buffer);

            frameChanged = false;
            _cond->wakeAll();
            _mutex->unlock();
        }

    }

    Source::update();
}

int AlgorithmSource::getFrameWidth() const {

    if (algotype == TURBULENCE || algotype == PERLIN_BW_NOISE  || algotype == PERLIN_COLOR_NOISE)
        return (int) (horizontal * 1000.0);
    return width;
}

int AlgorithmSource::getFrameHeight() const {

    if (algotype == TURBULENCE || algotype == PERLIN_BW_NOISE || algotype == PERLIN_COLOR_NOISE)
        return (int) (vertical * 1000.0);
    return height;
}

QString AlgorithmSource::getAlgorithmDescription(int t) {

    QString description;
    switch (t) {
    case FLAT:
        description = QString("White uniform");
        break;
    case FLAT_COLOR:
        description = QString("Color uniform");
        break;
    case BW_CHECKER:
        description = QString("B&W Checkerboard");
        break;
    case BW_LINES:
        description = QString("B&W lines");
        break;
    case COLOR_LINES:
        description = QString("Color spectrum lines");
        break;
    case BW_NOISE:
        description = QString("Greyscale noise");
        break;
    case BW_COSBARS:
        description = QString("Greyscale cosine bars");
        break;
    case BW_COSCHECKER:
        description = QString("Greyscale cosine checkerboard");
        break;
    case COLOR_NOISE:
        description = QString("Color noise");
        break;
    case PERLIN_BW_NOISE:
        description = QString("Greyscale Perlin noise");
        break;
    case PERLIN_COLOR_NOISE:
        description = QString("Color Perlin noise");
        break;
    case TURBULENCE:
        description = QString("Greyscale Perlin turbulence");
        break;
    default:
        description = QString("Undefined");
        break;
    }

    return description;
}

void AlgorithmSource::setIgnoreAlpha(bool on) {

    ignoreAlpha = on;
    format = ignoreAlpha ? GL_RGB : GL_RGBA;

    glBindTexture(GL_TEXTURE_2D, textureIndex);
    glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, (unsigned char*) buffer);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
}


QDomElement AlgorithmSource::getConfiguration(QDomDocument &doc, QDir current)
{
    // get the config from proto source
    QDomElement sourceElem = Source::getConfiguration(doc, current);
    sourceElem.setAttribute("playing", isPlaying());

    QDomElement specific = doc.createElement("TypeSpecific");
    specific.setAttribute("type", rtti());

    QDomElement f = doc.createElement("Algorithm");
    QDomText algo = doc.createTextNode(QString::number(getAlgorithmType()));
    f.appendChild(algo);
    f.setAttribute("IgnoreAlpha", getIgnoreAlpha());
    specific.appendChild(f);

    // get size
    QDomElement s = doc.createElement("Frame");
    s.setAttribute("Width", getFrameWidth());
    s.setAttribute("Height", getFrameHeight());
    specific.appendChild(s);

    QDomElement x = doc.createElement("Update");
    x.setAttribute("Periodicity", QString::number(getPeriodicity()) );
    x.setAttribute("Variability", QString::number(getVariability(),'f',PROPERTY_DECIMALS) );
    specific.appendChild(x);

    sourceElem.appendChild(specific);
    return sourceElem;
}
