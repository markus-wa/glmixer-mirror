#ifndef CODECMANAGER_H
#define CODECMANAGER_H


extern "C"
{
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavfilter/avfilter.h>
#include <libavdevice/avdevice.h>
}

#include <QtGui>

class CodecManager : public QObject
{

    Q_OBJECT

public:

    /**
     *  Register all the libav formats, codecs and filters
     *  (Can be called multiple times)
     */
    static void registerAll();
    /**
     *  Open a video stream
     */
    static bool openFormatContext(AVFormatContext **_pFormatCtx, QString streamToOpen, QString streamFormat = QString::null, QHash<QString, QString> streamOptions = QHash<QString, QString>());
     /**
     *  Find duration of a stream
     *
     * @return duration of the stream in second (-1 on error)
     */
    static double getDurationStream(AVFormatContext *codeccontext, int stream);
    /**
     *  Find number of frames of a stream
     *  VERY SLOW : use only for special formats like GIF
     *
     * @return number of frames in stream
     */
    static int countFrames(AVFormatContext *codeccontext, int stream, AVCodecContext *decoder);
    /**
     *  Find frame rate of a stream
     *
     * @return frame rate of the stream in second (-1 on error)
     */
    static double getFrameRateStream(AVFormatContext *codeccontext, int stream);
    /**
     *  Get the name of the pixel format of the frames of the file opened.
     *
     *  @return FFmpeg name of the pixel format. String is empty if file was not opened.
     */
    static QString getPixelFormatName(AVPixelFormat pix_fmt);
    /**
     *  Indicates if the pixel format has an alpha channel (transparency)
     *
     *  @return true if the pixel format has an alpha channel
     */
    static bool pixelFormatHasAlphaChannel(AVPixelFormat pix_fmt);
    /**
     *  Try to find an hardware accelerated codec
     *
     */
#if LIBAVCODEC_VERSION_INT > AV_VERSION_INT(58,0,0)
    static const AVCodecHWConfig *getCodecHardwareAcceleration(AVCodec *codec);
    static AVBufferRef *applyCodecHardwareAcceleration(AVCodecContext *CodecContext, const AVCodecHWConfig *hwconfig);
#endif
    /**
     *  Test hardware decoding this file.
     *
     *  @return true if could decode file with HW acceleration
     */
    static bool supportsHardwareAcceleratedCodec(QString file);
    /**
     *  Decide to use hardware acceleration
     *
     */
    static bool useHardwareAcceleration();
    static void setHardwareAcceleration(bool use);
    /**
     * Displays a dialog window (QDialog) listing the formats and video codecs supported for reading.
     *
     * @param iconfile Name of the file to put as icon of the window
     */
    static void displayFormatsCodecsInformation(QString iconfile);
    /**
     * Changes the given dimensions (width and height) to the closest power of two values
     *
     * @param width Value for width to convert in power of two (will be changed)
     * @param height Value for height to convert in power of two (will be changed)
     */
    static void convertSizePowerOfTwo(int &width, int &height);
    /**
     *  qwarning with text error message from error code
     */
    static void printError(QString streamname, QString message, int err);
    /**
     *  Request the list of devices for a given format (e.g. v4l)
     *
     *  @return list of devices for a given format
     */
    static QHash<QString, QString> getDeviceList(QString formatname);
    /**
     *  Test ffmpeg format by name
     *
     *  @return true if the format is available
     */
    static bool hasFormat(QString formatname);

private:

    CodecManager(QObject *parent = 0);
    static CodecManager *_instance;

    bool _useHardwareAcceleration;

};

#endif // CODECMANAGER_H
