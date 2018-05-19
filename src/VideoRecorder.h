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
    virtual QString message() { return QString("Error Recording video : %1").arg(text); }
    void raise() const { throw *this; }
    Exception *clone() const { return new VideoRecorderException(*this); }
};

typedef enum {
    FORMAT_AVI_RAW = 0,
    FORMAT_AVI_FFVHUFF,
    FORMAT_MPG_MPEG1,
    FORMAT_MPG_MPEG2,
    FORMAT_MP4_MPEG4,
    FORMAT_WMV_WMV2,
    FORMAT_FLV_FLV1
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

    QString getSuffix() const { return suffix; }
    QString getDescription() const { return description; }
    int getFrameRate() const { return frameRate; }
    QString getFilename() const { return fileName; }

    // Open the encoder and file for recording
    // Return true on success
    bool open();

    // Closes the encoder and file
    // Return number of frames recorded
    int close();

    // Record one frame
    void addFrame(AVFrame *frame);


protected:
    VideoRecorder(QString filename, int w, int h, int fps );

    int estimateGroupOfPictureSize();
    void setupContext(QString formatname);
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
    enum AVPixelFormat targetFormat;
    enum AVCodecID codecId;

    // encoding structures
    AVFormatContext *format_context;
    AVCodecContext *codec_context;
    AVStream *video_stream;
    AVCodec *codec;
    AVFrame *frame;

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

class VideoRecorderFFVHUFF : public VideoRecorder
{
public:
    VideoRecorderFFVHUFF(QString filename, int w, int h, int fps);
};

class VideoRecorderRAW : public VideoRecorder
{
public:
    VideoRecorderRAW(QString filename, int w, int h, int fps);
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

#endif // VIDEORECORDER_H
