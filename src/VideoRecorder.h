#ifndef VIDEORECORDER_H
#define VIDEORECORDER_H

extern "C" {
#include <libavformat/avformat.h>
#include <libavutil/frame.h>
#include <libavfilter/avfilter.h>
}

#include "defines.h"
#include <QString>

class VideoRecorderException : public AllocationException {
    QString text;
public:
    VideoRecorderException(QString t) : AllocationException(), text(t) {}
    virtual ~VideoRecorderException() throw() {}
    QString message() { return QString("Error Recording video : %1").arg(text); }
    void raise() const { throw *this; }
    Exception *clone() const { return new VideoRecorderException(*this); }
};

typedef enum {
    FORMAT_MP4_H264 = 0,
    FORMAT_WEB_WEBM,
    FORMAT_MOV_PRORES,
    FORMAT_MP4_MPEG4,
    FORMAT_MPG_MPEG2,
    FORMAT_MPG_MPEG1,
    FORMAT_WMV_WMV2,
    FORMAT_FLV_FLV1,
    FORMAT_AVI_FFV3,
    FORMAT_AVI_RAW
} encodingformat;

typedef enum {
    QUALITY_AUTO = 0,
    QUALITY_LOW,
    QUALITY_MEDIUM,
    QUALITY_HIGH,
} encodingquality;


class VideoRecorder
{

public:

    static VideoRecorder *getRecorder(encodingformat f, QString filename, int w, int h, int fps, encodingquality quality);
    virtual ~VideoRecorder();

    QString getFileSuffix() const { return suffix; }
    QString getFileDescription() const { return description; }
    int getFrameRate() const { return frameRate; }
    QString getFilename() const { return fileName; }

    // Open the encoder and file for recording
    // Return true on success
    void open();

    // Closes the encoder and file
    // Return number of frames recorded
    int close();

    // Record one frame
    void addFrame(AVFrame *frame);


protected:
    VideoRecorder(QString filename, int w, int h, int fps );

    int estimateGroupOfPictureSize();
    void setupContext(QStringList codecnames, QString formatname, enum AVPixelFormat pixelformat);
    void setupFiltering();

    // properties
    QString fileName;
    int width;
    int height;
    int frameRate;
    int framenum;

    // format specific
    QString suffix;
    QString description;

    // encoding structures
    AVFormatContext *format_context;
    AVCodecContext *codec_context;
    AVStream *video_stream;
    AVCodec *codec;
    AVFrame *frame;
    AVDictionary *opts;

    // frame conversion
    AVFilterContext *in_video_filter;
    AVFilterContext *out_video_filter;
    AVFilterGraph *graph;

};

class VideoRecorderMP4 : public VideoRecorder
{
public:
    VideoRecorderMP4(QString filename, int w, int h, int fps, encodingquality quality);
};

class VideoRecorderH264 : public VideoRecorder
{
public:
    VideoRecorderH264(QString filename, int w, int h, int fps, encodingquality quality);
};

class VideoRecorderWebM : public VideoRecorder
{
public:
    VideoRecorderWebM(QString filename, int w, int h, int fps, encodingquality quality);
};

class VideoRecorderProRes : public VideoRecorder
{
public:
    VideoRecorderProRes(QString filename, int w, int h, int fps, encodingquality quality);
};

class VideoRecorderMPEG1 : public VideoRecorder
{
public:
    VideoRecorderMPEG1(QString filename, int w, int h, int fps);
};

class VideoRecorderMPEG2 : public VideoRecorder
{
public:
    VideoRecorderMPEG2(QString filename, int w, int h, int fps);
};

class VideoRecorderWMV : public VideoRecorder
{
public:
    VideoRecorderWMV(QString filename, int w, int h, int fps);
};

class VideoRecorderFLV : public VideoRecorder
{
public:
    VideoRecorderFLV(QString filename, int w, int h, int fps);
};

class VideoRecorderFFV : public VideoRecorder
{
public:
    VideoRecorderFFV(QString filename, int w, int h, int fps);
};

class VideoRecorderRAW : public VideoRecorder
{
public:
    VideoRecorderRAW(QString filename, int w, int h, int fps);
};

#endif // VIDEORECORDER_H
