#ifndef CODECMANAGER_H
#define CODECMANAGER_H


extern "C"
{
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavfilter/avfilter.h>
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
    static AVFormatContext *openFormatContext(QString streamToOpen);
    /**
     *  Open a Codec
     *
     */
    static QString openCodec(AVCodecContext *codeccontext);
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

private:

    CodecManager(QObject *parent = 0);
    static CodecManager *_instance;

};

#endif // CODECMANAGER_H
