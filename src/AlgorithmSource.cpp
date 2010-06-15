/*
 * AlgorithmSource.cpp
 *
 *  Created on: Feb 27, 2010
 *      Author: bh
 */

#include "AlgorithmSource.moc"

Source::RTTI AlgorithmSource::type = Source::ALGORITHM_SOURCE;

#define PERLIN_WIDTH 128
#define PERLIN_HEIGHT 128

#include <limits>
#include <iostream>
//#include <ctime>

#include <QMutex>
#include <QWaitCondition>
#include <QThread>
#include <QTime>

/**
 * PERLIN NOISE
 */
#include <cmath>

static int p[512];
static int permutation[] = { 151,160,137,91,90,15,
131,13,201,95,96,53,194,233,7,225,140,36,103,30,69,142,8,99,37,240,
21,10,23,190,6,148,247,120,234,75,0,26,197,62,94,252,219,203,117,
35,11,32,57,177,33,88,237,149,56,87,174,20,125,136,171,168,68,175,
74,165,71,134,139,48,27,166,77,146,158,231,83,111,229,122,60,211,133,
230,220,105,92,41,55,46,245,40,244,102,143,54,65,25,63,161,1,216,
80,73,209,76,132,187,208,89,18,169,200,196,135,130,116,188,159,86,
164,100,109,198,173,186,3,64,52,217,226,250,124,123,5,202,38,147,
118,126,255,82,85,212,207,206,59,227,47,16,58,17,182,189,28,42,223,
183,170,213,119,248,152,2,44,154,163,70,221,153,101,155,167,43,
172,9,129,22,39,253,19,98,108,110,79,113,224,232,178,185,112,104,
218,246,97,228,251,34,242,193,238,210,144,12,191,179,162,241,81,51,
145,235,249,14,239,107,49,192,214,31,181,199,106,157,184,84,204,176,
115,121,50,45,127,4,150,254,138,236,205,93,222,114,67,29,24,72,243,
141,128,195,78,66,215,61,156,180
};

/* Function declarations */
double   fade(double t);
double   lerp(double t, double a, double b);
double   grad(int hash, double x, double y, double z);
void     init_pnoise();
double   pnoise(double x, double y, double z);

void init_pnoise()
{
int i;
for(i = 0; i < 256 ; i++)
    p[256+i] = p[i] = permutation[i];
}

double pnoise(double x, double y, double z)
{
int   X = (int)floor(x) & 255,             /* FIND UNIT CUBE THAT */
      Y = (int)floor(y) & 255,             /* CONTAINS POINT.     */
      Z = (int)floor(z) & 255;
	  x -= floor(x);                       /* FIND RELATIVE X,Y,Z */
	  y -= floor(y);                       /* OF POINT IN CUBE.   */
	  z -= floor(z);
double  u = fade(x),                       /* COMPUTE FADE CURVES */
        v = fade(y),                       /* FOR EACH OF X,Y,Z.  */
        w = fade(z);
int  A = p[X]+Y,
     AA = p[A]+Z,
     AB = p[A+1]+Z, /* HASH COORDINATES OF */
     B = p[X+1]+Y,
     BA = p[B]+Z,
     BB = p[B+1]+Z; /* THE 8 CUBE CORNERS, */

return lerp(w,lerp(v,lerp(u, grad(p[AA  ], x, y, z),   /* AND ADD */
                     grad(p[BA  ], x-1, y, z)),        /* BLENDED */
             lerp(u, grad(p[AB  ], x, y-1, z),         /* RESULTS */
                     grad(p[BB  ], x-1, y-1, z))),     /* FROM  8 */
             lerp(v, lerp(u, grad(p[AA+1], x, y, z-1 ),/* CORNERS */
                     grad(p[BA+1], x-1, y, z-1)),      /* OF CUBE */
             lerp(u, grad(p[AB+1], x, y-1, z-1),
                     grad(p[BB+1], x-1, y-1, z-1))));
}

double fade(double t){ return t * t * t * (t * (t * 6 - 15) + 10); }
double lerp(double t, double a, double b){ return a + t * (b - a); }
double grad(int hash, double x, double y, double z)
{
int     h = hash & 15;       /* CONVERT LO 4 BITS OF HASH CODE */
double  u = h < 8 ? x : y,   /* INTO 12 GRADIENT DIRECTIONS.   */
        v = h < 4 ? y : h==12||h==14 ? x : z;
return ((h&1) == 0 ? u : -u) + ((h&2) == 0 ? v : -v);
}

double turb(double x, double y, double z, double minFreq, double maxFreq){
	double r = 0.0;
	x += 123.456;
	for (double freq = minFreq; freq < maxFreq; freq *= 2.0 * freq ){
		r += ABS( pnoise (x, y, z) ) / freq;
		x *= 2.0;
		y *= 2.0;
		z *= 2.0;
	}
	return r - 0.3;
}
/**
 *  Thread class to update the texture
 */

class AlgorithmThread: public QThread {
public:
	AlgorithmThread(AlgorithmSource *source) :
        QThread(), as(source), end(false) {
    }
    ~AlgorithmThread() {
    }

    void run();

    AlgorithmSource* as;
    bool end;

};

void AlgorithmThread::run(){

	QTime t;
	int f = 0;

	t.start();
	while (!end) {

		as->mutex->lock();
		if (!as->frameChanged) {
			// compute new frame

			// Immediately discard the FLAT 'algo' ; it is the "do nothing" algorithm :)
			if( as->algotype != AlgorithmSource::FLAT )
			{
				srand( t.elapsed() );
				if ( as->algotype == AlgorithmSource::BW_NOISE ){
					for (int i = 0; i < (as->width * as->height); ++i) {
						as->buffer[i * 4 + 0] =  (unsigned char) ( as->variability * double(rand() % std::numeric_limits<unsigned char>::max()) + (1.0 - as->variability) * double(as->buffer[i * 4 + 0]) )  ;
						as->buffer[i * 4 + 1] = as->buffer[i * 4];
						as->buffer[i * 4 + 2] = as->buffer[i * 4];
						as->buffer[i * 4 + 3] = as->buffer[i * 4];
					}

				} else
				if ( as->algotype == AlgorithmSource::COLOR_NOISE ){
					for (int i = 0; i < (as->width * as->height * 4); ++i)
//						as->buffer[i] = (unsigned char) ((  rand() % 2 ) * std::numeric_limits<unsigned char>::max());
						as->buffer[i] = (unsigned char) ( as->variability * double(rand() % std::numeric_limits<unsigned char>::max()) + (1.0 - as->variability) * double(as->buffer[i]) )  ;

				} else
				if ( as->algotype == AlgorithmSource::PERLIN_BW_NOISE ){

					static double i = 0.0, di = 0.5;
					i += di * as->variability; // / RenderingManager::getRenderingWidget()->getFPS();
					if (i > 100000.0 || i < 0.0)   di = -di;
					for (int x = 0; x < as->width; ++x)
						for (int y = 0; y < as->height; ++y) {
							double v = pnoise( double(x) * as->horizontal , double(y) * as->vertical , i );
							as->buffer[(y * as->width + x) * 4 + 0 ] = (unsigned char) (128.0 * v  + 128);
							as->buffer[(y * as->width + x) * 4 + 1 ] = (unsigned char) (128.0 * v  + 128);
							as->buffer[(y * as->width + x) * 4 + 2 ] = (unsigned char) (128.0 * v  + 128);
							as->buffer[(y * as->width + x) * 4 + 3 ] = (unsigned char) (128.0 * v  + 128);
						}
				} else
					if ( as->algotype == AlgorithmSource::PERLIN_COLOR_NOISE ){

						static double i = 0.0, j = 0.0, k = 0.0, l = 0.0;
						static double di = 0.3, dj = 0.4, dk = 0.5, dl = 0.7;
						i += as->variability * di;; // / RenderingManager::getRenderingWidget()->getFPS();
						j += as->variability * dj;; // / RenderingManager::getRenderingWidget()->getFPS();
						k += as->variability * dk;; // / RenderingManager::getRenderingWidget()->getFPS();
						l += as->variability * dl;; // / RenderingManager::getRenderingWidget()->getFPS();
						if (i > 100000.0 || i < 0.0)   di = -di;
						for (int x = 0; x < as->width; ++x)
							for (int y = 0; y < as->height; ++y) {
								double v = pnoise( double(x) * as->horizontal , double(y) * as->vertical , i );
								as->buffer[(y * as->width + x) * 4 + 0 ] = (unsigned char) (128.0 * v  + 128);
								v = pnoise( double(x) * as->horizontal , double(y) * as->vertical , j );
								as->buffer[(y * as->width + x) * 4 + 1 ] = (unsigned char) (128.0 * v  + 128);
								v = pnoise( double(x) * as->horizontal , double(y) * as->vertical , k );
								as->buffer[(y * as->width + x) * 4 + 2 ] = (unsigned char) (128.0 * v  + 128);
								v = pnoise( double(x) * as->horizontal , double(y) * as->vertical , l );
								as->buffer[(y * as->width + x) * 4 + 3 ] = (unsigned char) 255;
							}
					} else
						if ( as->algotype == AlgorithmSource::TURBULENCE ){
							static double i = 0.0, di = 0.5;
							i += as->variability * di; // / RenderingManager::getRenderingWidget()->getFPS();
							if (i > 100000.0 || i < 0.0)   di = -di;
							for (int x = 0; x < as->width; ++x)
								for (int y = 0; y < as->height; ++y) {
									double v = turb( double(x) * as->horizontal , double(y) * as->vertical , i , 1.0, 16.0 );
									as->buffer[(y * as->width + x) * 4 + 0 ] = (unsigned char) (128.0 * v  + 128);
									as->buffer[(y * as->width + x) * 4 + 1 ] = (unsigned char) (128.0 * v  + 128);
									as->buffer[(y * as->width + x) * 4 + 2 ] = (unsigned char) (128.0 * v  + 128);
									as->buffer[(y * as->width + x) * 4 + 3 ] = (unsigned char) (128.0 * v  + 128);
								}
						}

			}
			as->frameChanged = true;
			as->cond->wait(as->mutex);
		}
		as->mutex->unlock();

		// wait for the period duration before updating next frame
		usleep(as->period);

		if ( ++f == 100 ) { // hundred frames to average the frame rate {
			as->framerate = 100000.0 / (double) t.elapsed();
			t.restart();
			f = 0;
		}
	}
}


AlgorithmSource::AlgorithmSource(int type, GLuint texture, double d, int w, int h, double v, unsigned long  p) : Source(texture, d),
		width(w), height(h), period(p), framerate(0), vertical(1.0), horizontal(1.0), variability(v) {

	algotype = CLAMP(AlgorithmSource::algorithmType(type), AlgorithmSource::FLAT, AlgorithmSource::TURBULENCE);

	aspectratio = double(w) / double(h);
	name.prepend("algo");

	// allocate and initialize the buffer
	initBuffer();

	// apply the texture
	glBindTexture(GL_TEXTURE_2D, textureIndex);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height,0, GL_BGRA, GL_UNSIGNED_BYTE, (unsigned char*) buffer);

	// if no period given, set to default 60Hz
	if (period <= 0)
		period = 17000;

	// create thread
	mutex = new QMutex;
    Q_CHECK_PTR(mutex);
    cond = new QWaitCondition;
    Q_CHECK_PTR(cond);
	thread = new AlgorithmThread(this);
    Q_CHECK_PTR(thread);
	thread->start();
}

AlgorithmSource::~AlgorithmSource() {

	// end the update thread
	thread->end = true;
	mutex->lock();
	cond->wakeAll();
	mutex->unlock();
    thread->wait(500);
	delete thread;
	delete cond;
	delete mutex;

	// delete picture buffer
	if (buffer)
		delete [] buffer;

	// free the OpenGL texture
	glDeleteTextures(1, &textureIndex);
}


void AlgorithmSource::play(bool on){

	if ( on ) { // start play
		if (! isRunning() ) {
			thread->end = false;
			thread->start();
		}
	} else { // stop play
		if ( isRunning() ) {
			thread->end = true;
			mutex->lock();
			cond->wakeAll();
		    frameChanged = false;
			mutex->unlock();
		    thread->wait(500);
		}
	}
}


bool AlgorithmSource::isRunning(){

	return !thread->end;

}

void AlgorithmSource::initBuffer(){

	QString description;
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
		case FLAT:
		case BW_NOISE:
		case COLOR_NOISE:
		default:
			break;
	}

	buffer = new unsigned char [width * height * 4];
	// CLEAR the buffer to white
	for (int i = 0; i < (width * height * 4); ++i)
		buffer[i] = std::numeric_limits<unsigned char>::max();

}


void AlgorithmSource::update(){

	Source::update();

	if( frameChanged )
	{
		mutex->lock();
		frameChanged = false;
		glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, width, height, GL_BGRA, GL_UNSIGNED_BYTE, (unsigned char*) buffer);
		cond->wakeAll();
		mutex->unlock();
	}

}


QString AlgorithmSource::getAlgorithmDescription(int t) {

	QString description;
	switch (t) {
	case FLAT:
		description = QString("Flat color");
		break;
	case BW_NOISE:
		description = QString("Greyscale noise");
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
	}

	return description;
}

