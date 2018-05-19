/*
 * RenderingEncoder.h
 *
 *  Created on: Mar 13, 2011
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

#ifndef RENDERINGENCODER_H_
#define RENDERINGENCODER_H_

#include <QObject>
#include <QTime>
#include <QString>

/**
 * Minimum and Maximum size of the recording buffer
 * Expressed in bytes
 */
// 100 MB
#define MIN_RECORDING_BUFFER_SIZE 104857600
// 4 GB
#define MAX_RECORDING_BUFFER_SIZE 4294967296
// 5% of range, approx 200 MB
#define DEFAULT_RECORDING_BUFFER_SIZE 209505485
// use glReadPixel or glGetTextImage ?
#define RECORDING_READ_PIXEL 1

extern "C" {
//#include "video_rec.h"
#include <libavutil/frame.h>
}

#include "VideoRecorder.h"
#include "RenderingManager.h"

class EncodingThread: public QThread {

    Q_OBJECT

public:

    EncodingThread();
    ~EncodingThread();

    void initialize(VideoRecorder *rec, int width, int height, unsigned long bufSize);
    void clear();
    void stop();

    void releaseAndPushFrame(int64_t timestamp);
    AVBufferRef *lockFrameAndGetBuffer();
    bool frameq_full();

    int getFrameWidth() const { return framewidth; }
    int getFrameHeight() const { return frameheight; }
    int getFrameQueueSize() const { return pictq_max_count; }

signals:
    void encodingFinished(bool);

protected:

    // the update function
    void run();

    // ref to the recorder
    VideoRecorder *recorder;

    // execution management
    bool _quit;
    QMutex *pictq_mutex;
    QWaitCondition *pictq_cond;

    // picture queue management
    int pictq_max_count, pictq_size_count, pictq_rindex, pictq_windex;
    AVFrame **frameq;
    int framewidth, frameheight;
};

class RenderingEncoder: public QObject {

    Q_OBJECT

    friend class EncodingThread;

public:

    RenderingEncoder(QObject * parent = 0);
    ~RenderingEncoder();

    void addFrame(uint8_t *data = 0);

    // preferences encoding
    void setEncodingFormat(encodingformat f);
    encodingformat encodingFormat() { return format; }
    void setUpdatePeriod(uint ms) { update=ms; }
    uint updatePeriod() { return update; }
    void setEncodingQuality(encodingquality q);
    encodingquality encodingQuality() { return quality; }

    // preferences saving mode
    void setAutomaticSavingMode(bool on);
    bool automaticSavingMode() { return automaticSaving;}
    void setAutomaticSavingFolder(QString d);
    QDir automaticSavingFolder() { return savingFolder; }

    // status
    bool isActive() { return started; }
    int getRecodingTime();
    bool isRecording() { return started && !paused ; }

    // utility
    static unsigned long computeBufferSize(int percent);
    static int computeBufferPercent(unsigned long bytes);

public slots:
    void setActive(bool on);
    void setPaused(bool on);
    void saveFile(QString suffix, QString filename = QString::null);
    void saveFileAs(QString suffix, QString description);
    void close(bool success);
    void kill();

    void setBufferSize(unsigned long bytes);
    unsigned long getBufferSize();

signals:
    void activated(bool);
    void processing(bool);
    void status(const QString &, int);
    void timing(const QString &);
    void selectAspectRatio(const standardAspectRatio );

protected:
    bool start();

private:
    // files location
    QString temporaryFileName;
    QDir savingFolder, temporaryFolder;
    bool automaticSaving;

    // state machine
    bool started, paused;
    QTime timer;
    int elapsed_time;
    int skipframecount;
    QString errormessage;

    // encoder & recorder
    EncodingThread *encoder;
    VideoRecorder *recorder;

    uint update, displayupdate;
    encodingformat format;
    encodingquality quality;
    unsigned long bufferSize;
};

#endif /* RENDERINGENCODER_H_ */
