/*
 * VideoStream.h
 *
 *  Created on: Jul 10, 2009
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
 *   Copyright 2009, 2018 Bruno Herbelin
 *
 */

#ifndef VIDEOSTREAM_H
#define VIDEOSTREAM_H

#include <QObject>
#include <QQueue>
#include <QMutex>
#include <QWaitCondition>
#include <QThread>
#include <QTimer>
#include <QFile>
#include <QTextStream>


#include "VideoPicture.h"

class videoStreamThread;

class VideoStream : public QObject
{
    Q_OBJECT

    friend class StreamDecodingThread;
    friend class StreamOpeningThread;

public:
    VideoStream(QObject *parent = 0, int destinationWidth = 0, int destinationHeight = 0);
    virtual ~VideoStream();

    void open(QString urlname, QString format = "", QHash<QString, QString> options  = QHash<QString, QString>());
    void close();
    bool isOpen() const ;

    inline bool isActive() const {
        return active;
    }
    inline QString getUrl() const {
        return urlname;
    }
    inline QString getFormat() const {
        return formatname;
    }
    inline QHash<QString, QString> getFormatOptions() const {
        return formatoptions;
    }
    inline QString getCodecName() const {
        return codecname;
    }
    inline int getFrameWidth() const {
        return targetWidth;
    }
    inline int getFrameHeight() const {
        return targetHeight;
    }
    inline double getFrameRate() const {
        return frame_rate;
    }

signals:
    /**
     * Signal emmited when a new VideoPicture is ready;
     *
     * @param id the argument is the id of the VideoPicture to read.
     */
    void frameReady(VideoPicture *);
    /**
     * Signal emmited when started or stopped;
     *
     * @param run the argument is true for running, and false for not running.
     */
    void running(bool run);
    /**
     * Signal emited on reading or decoding failure.
     */
    void failed();
    /**
     * Signal emited after successful opening of the stream .
     */
    void openned();

public slots:

    void play(bool startorstop);


protected slots:
    /**
     * Slot called from an internal timer synchronized on the video time code.
     */
    void video_refresh_timer();

    bool openStream();
    void start();
    void stop();
    void onStop();

private:
    // Video and general information
    QString urlname;
    QString formatname;
    QHash<QString, QString> formatoptions;
    QString codecname;
    int targetWidth, targetHeight;
    double frame_rate;
    enum AVPixelFormat targetFormat;
    AVFormatContext *pFormatCtx;
    AVStream *video_st;
    AVCodecContext *video_dec;
    AVFilterContext *in_video_filter;   // the first filter in the video chain
    AVFilterContext *out_video_filter;  // the last filter in the video chain
    AVFilterGraph *graph;
    int videoStream;
    bool setupFiltering();

    // picture queue management
    int pictq_max_count;
    QQueue<VideoPicture*> pictq;
    QMutex *pictq_mutex;
    QWaitCondition *pictq_cond;
    void flush_picture_queue();
    void queue_picture(AVFrame *pFrame, double pts, VideoPicture::Action a);

    // Threads and execution manangement
    videoStreamThread *decod_tid;
    videoStreamThread *open_tid;
    bool quit, active;
    QTimer *ptimer;

};

class videoStreamThread: public QThread
{
    Q_OBJECT

public:
    videoStreamThread(VideoStream *video) :
        QThread(), is(video), _forceQuit(false)
    { }
    inline void forceQuit() {
        _forceQuit = true;
        emit failed();
    }

    virtual void run() = 0;

signals:
    void success();
    void failed();

protected:
    VideoStream *is;
    bool _forceQuit;
};


#endif // VIDEOSTREAM_H
