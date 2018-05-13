#ifndef VIDEORECORDER_H
#define VIDEORECORDER_H

extern "C" {
#include <libavformat/avformat.h>
#include <libavutil/frame.h>
#include <libavfilter/avfilter.h>
}

#include <QObject>


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


class VideoRecorder : public QObject
{
    Q_OBJECT
public:
    explicit VideoRecorder(QObject *parent = nullptr);

    // get writable buffer reference (access data and size for writing in)
    AVBufferRef *getTopFrameBuffer();
    void pushFrame( int64_t pts );

signals:

public slots:

private:
    int width;
    int height;
    int fps;
    int framenum;
    long int previous_dts;

    // file formats
    char suffix[6];
    char description[64];
    encodingformat format;
    encodingquality quality;

    // encoding structures
    AVFormatContext *pFormatCtx;
    AVCodecContext *video_enc;
    AVStream *video_stream;

    // frame conversion
    enum AVPixelFormat targetFormat;
    AVFilterContext *in_video_filter;
    AVFilterContext *out_video_filter;
    AVFilterGraph *graph;

};

#endif // VIDEORECORDER_H
