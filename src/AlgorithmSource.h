/*
 * AlgorithmSource.h
 *
 *  Created on: Feb 27, 2010
 *      Author: bh
 */

#ifndef ALGORITHMSOURCE_H_
#define ALGORITHMSOURCE_H_

#include "Source.h"
#include "RenderingManager.h"

class AlgorithmThread;
class QMutex;
class QWaitCondition;

class AlgorithmSource: public QObject, public Source {

    Q_OBJECT

    friend class AlgorithmSelectionDialog;
	friend class RenderingManager;
    friend class AlgorithmThread;

public:

	static RTTI type;
	RTTI rtti() const { return type; }

	typedef enum {FLAT = 0, BW_NOISE, COLOR_NOISE, PERLIN_BW_NOISE, PERLIN_COLOR_NOISE, TURBULENCE} algorithmType;
	static QString getAlgorithmDescription(int t);

    inline algorithmType getAlgorithmType() const { return algotype; }
	inline int getFrameWidth() const { return width; }
	inline int getFrameHeight() const { return height; }
	inline double getFrameRate() const { return framerate; }
	bool isRunning();

public slots:
	void play(bool on);
	void setPeriodicity(unsigned long u_seconds) {period = u_seconds;}
	void setVariability(double v) { variability = v; }

    // only RenderingManager can create a source
protected:
	AlgorithmSource(int type, GLuint texture, double d, int w = 256, int h = 256, double v = 1.0, unsigned long p= 16666);
	~AlgorithmSource();

	void update();

	void initBuffer();

	algorithmType algotype;
	unsigned char *buffer;
	int width, height;
	unsigned long period;
	double framerate;
    bool frameChanged;
    double vertical, horizontal;
    double variability;

    AlgorithmThread *thread;
    QMutex *mutex;
    QWaitCondition *cond;

};

#endif /* ALGORITHMSOURCE_H_ */
