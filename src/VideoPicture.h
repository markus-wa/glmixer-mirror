#ifndef VIDEOPICTURE_H
#define VIDEOPICTURE_H

extern "C" {
#include <libavformat/avformat.h>
#include <libavfilter/avfilter.h>
}

#include "defines.h"
#include <QList>
#include <QMutex>
#include <QString>

/**
 * Size of a Mega Byte
 */
#define MEGABYTE 1048576
/**
 * Number of Pictures in one page of Memory Map
 */
#define PICTUREMAP_SIZE 20
/**
 * uncomment to monitor execution with debug information
 */
//#ifndef NDEBUG
//#define VIDEOPICTURE_DEBUG
//#endif

class VideoPictureException : public AllocationException {
public:
    virtual QString message() { return "Could not allocate Video Picture"; }
    void raise() const { throw *this; }
    Exception *clone() const { return new VideoPictureException(*this); }
};

/**
 *
 */
class VideoPicture {

public:

    // default constructor
    VideoPicture();

    // create a black picture
    VideoPicture(int w, int h, double Pts = 0.0);

    // create a picture from the frame extrated of the filter sink
    // (av_buffersink_get_frame)
    VideoPicture(AVFilterContext *sink, double Pts = 0.0);

    // create a picture by copying the content of a frame
    VideoPicture(AVFrame *f, double Pts = 0.0);

    /**
     * Get a pointer to the buffer containing the frame.
     *
     * If the buffer is allocated in the PIX_FMT_RGB24 pixel format, it is
     * a Width x Height x 3 array of unsigned bytes (char or uint8_t) .
     * This buffer is directly applicable as a GL_RGB OpenGL texture.
     *
     * If the buffer is allocated in the PIX_FMT_RGBA pixel format, it is
     * a Width x Height x 4 array of unsigned bytes (char or uint8_t) .
     * This buffer is directly applicable as a GL_RGBA OpenGL texture.
     *
     * @return pointer to an array of unsigned bytes (char or uint8_t)
     */
    char *getBuffer() const;

    /**
      * Deletes the VideoPicture and frees the av picture
      */
    ~VideoPicture();
    /**
     * Get the Presentation timestamp
     *
     * @return Presentation Time of the picture in second.
     */
    inline double getPts() const {
        return pts;
    }
    /**
     * Get the width of the picture.
     *
     * @return Width of the picture in pixels.
     */
    inline int getWidth() const {
        return width;
    }
    /**
     * Get the length of a row of pixels in the picture data if different from width,
     * 0 otherwise.
     *
     *   glPixelStorei(GL_UNPACK_ROW_LENGTH, vp->getRowLenght());
     *
     * @return Row lenght of the picture in pixels.
     */
    inline int getRowLength() const {
        return rowlength;
    }
    /**
     * Get the height of the picture.
     *
     * @return Height of the picture in pixels.
     */
    inline int getHeight() const {
        return height;
    }
    /**
     * Get aspect ratio width / height
     *
     * @return aspect ratio as a double
     */
    inline double getAspectRatio() const {
        return (double)width / (double)height;
    }

    /**
     * Set Fading value [0.0 1.0]
     *
     */
    inline void setFading(double f) { fade = f; }

    /**
     * Get fading value
     *
     * @return fading
     */
    inline double getFading() { return fade; }
    /**
     * Creates and saves a .ppm image file with the current buffer (if full).
     */
    void saveToPPM(QString filename = "videoPicture.ppm") const;
    /**
     * Internal pixel format of the buffer.
     *
     * This is usually PIX_FMT_RGB24 or PIX_FMT_RGBA for OpenGL texturing, but depends
     * on the pixel format requested when instanciating the VideoFile.
     *
     * Pixel format should be tested before applying texture, e.g.:
     *    <pre>
     *     if ( vp->getFormat() == PIX_FMT_RGBA)
     *           glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA,  vp->getWidth(),
     *                      vp->getHeight(), 0, GL_RGBA, GL_UNSIGNED_BYTE,
     *                      vp->getBuffer() );
     *    </pre>
     *
     * @return PIX_FMT_RGB24 by default, PIX_FMT_RGBA if there is alpha channel
     */
    inline enum AVPixelFormat getFormat() const {
         return pixel_format;
    }

    int getBufferSize() const;

    /**
      * Actions to perform on the Video Picture
      */
    enum {
        ACTION_SHOW = 1,
        ACTION_STOP = 2,
        ACTION_RESET_PTS = 4,
        ACTION_DELETE = 8,
        ACTION_MARK = 16
    };
    typedef unsigned short Action;
    inline void resetAction() { action = 0; }
    inline void addAction(Action a) { action |= a; }
    inline void removeAction(Action a) { action ^= (action & a); }
    inline bool hasAction(Action a) const { return (action & a); }

private:

    enum AVPixelFormat pixel_format;
    uint8_t *data;
    double pts;
    int width, height, rowlength;
    Action action;
    AVFrame *frame;
    double fade;

    class PictureMap
    {
        uint8_t *_map;
        uint8_t *_picture[PICTUREMAP_SIZE];
        int _pageSize;
        bool _isFull;
        static long int _totalmemory;

    public:
        PictureMap(int pageSize);
        ~PictureMap();

        uint8_t *getAvailablePictureMemory();
        void freePictureMemory(uint8_t *p);
        bool isEmpty();
        bool isFull();
        int getPageSize() { return _pageSize; }
    };
    PictureMap *_pictureMap;

    static PictureMap *getAvailablePictureMap(int w, int h, enum AVPixelFormat format);
    static void freePictureMap(PictureMap *pmap);
    static QList<PictureMap*> _pictureMaps;
    static QMutex VideoPictureMapLock;

public:
    static void clearPictureMaps();
    static int count;

};



#endif // VIDEOPICTURE_H
