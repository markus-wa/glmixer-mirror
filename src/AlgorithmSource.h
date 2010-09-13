/*
 * AlgorithmSource.h
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
 *   Copyright 2009, 2010 Bruno Herbelin
 *
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

	RTTI rtti() const { return type; }
	bool isPlayable() const { return playable; }
	bool isPlaying() const;

	typedef enum {FLAT = 0, BW_NOISE, COLOR_NOISE, PERLIN_BW_NOISE, PERLIN_COLOR_NOISE, TURBULENCE} algorithmType;
	static QString getAlgorithmDescription(int t);

    inline algorithmType getAlgorithmType() const { return algotype; }
	inline double getVariability() const { return variability; }
	inline unsigned long getPeriodicity() const { return period; }
	inline double getFrameRate() const { return framerate; }
	int getFrameWidth() const;
	int getFrameHeight() const;

public Q_SLOTS:
	void play(bool on);
	void setPeriodicity(unsigned long u_seconds) {period = u_seconds;}
	void setVariability(double v) { variability = CLAMP(v, 0.0, 1.0); }

    // only RenderingManager can create a source
protected:
	AlgorithmSource(int type, GLuint texture, double d, int w = 256, int h = 256, double v = 1.0, unsigned long p= 16666);
	~AlgorithmSource();

	static RTTI type;
	static bool playable;
	void update();

	void initBuffer();
	void setStandby(bool on);

	algorithmType algotype;
	unsigned char *buffer;
	int width, height;
	unsigned long period;
	double framerate;
    double vertical, horizontal;
    double variability;

    AlgorithmThread *thread;
    QMutex *mutex;
    QWaitCondition *cond;

};

#endif /* ALGORITHMSOURCE_H_ */
