
extern "C"
{
#include <libavcodec/avcodec.h>
#include <libavutil/pixdesc.h>
#include <libavutil/imgutils.h>
#include <libavfilter/buffersink.h>
}

#include "VideoPicture.h"

#include <QObject>
#include <QDebug>

#ifdef Q_OS_UNIX
#include <sys/mman.h>
#ifndef MAP_ANONYMOUS
#define MAP_ANONYMOUS MAP_ANON
#endif
#define PICTURE_MAP
#endif


// memory map
QList<VideoPicture::PictureMap*> VideoPicture::_pictureMaps;
QMutex VideoPicture::VideoPictureMapLock;
long int VideoPicture::PictureMap::_totalmemory = 0;
int VideoPicture::count = 0;

VideoPicture::PictureMap::PictureMap(int pageSize) : _pageSize(pageSize), _isFull(false)
{

#ifdef Q_OS_UNIX
    _map = (uint8_t *) mmap(0, PICTUREMAP_SIZE * _pageSize, PROT_READ|PROT_WRITE, MAP_PRIVATE|MAP_ANONYMOUS, -1, 0);

    _totalmemory += PICTUREMAP_SIZE * _pageSize;

#ifdef VIDEOPICTURE_DEBUG
    fprintf(stderr, "\n+ Video File Picture Maps size = %.2f Mb.", (float)_totalmemory / (float) MEGABYTE);
#endif

#endif

    int j=0;
    for (j=0; j<PICTUREMAP_SIZE;++j){
        _picture[j] = 0;
    }
}

VideoPicture::PictureMap::~PictureMap()
{
#ifdef Q_OS_UNIX
    munmap(_map, PICTUREMAP_SIZE * _pageSize);
    _totalmemory -= PICTUREMAP_SIZE * _pageSize;

#ifdef VIDEOPICTURE_DEBUG
    fprintf(stderr, "\n- Video File Picture Maps size = %.2f Mb.", (float)_totalmemory / (float) MEGABYTE);
#endif

#endif
}

void VideoPicture::PictureMap::freePictureMemory(uint8_t *p)
{
    int j=0;
    for (j=0;j<PICTUREMAP_SIZE;++j){
        if (_picture[j] == p) {
            _picture[j] = NULL;
            break;
        }
    }
}

bool VideoPicture::PictureMap::isEmpty()
{
    int j=0;
    for (j=0;j<PICTUREMAP_SIZE;++j){
        if (_picture[j] != NULL)
           return false;
    }
    return true;
}

bool VideoPicture::PictureMap::isFull()
{
    int j=0;
    for (j=0;j<PICTUREMAP_SIZE;++j){
        if (_picture[j] == NULL)
           return false;
    }
    return true;
}

uint8_t *VideoPicture::PictureMap::getAvailablePictureMemory()
{

    int j=0;
    for (j=0;j<PICTUREMAP_SIZE;++j){
        if (_picture[j] == NULL) {
            _picture[j] = _map + j * _pageSize;
            return _picture[j];
        }
    }

    return NULL;
}

VideoPicture::PictureMap *VideoPicture::getAvailablePictureMap(int w, int h, enum AVPixelFormat format) {
    PictureMap *m = NULL;
    int pageSize = 0;
    if(format==AV_PIX_FMT_RGB24)
        pageSize = w * h * 3 * sizeof(uint8_t);
    else
        pageSize = w * h * 4 * sizeof(uint8_t);

    QList<PictureMap*>::iterator it = _pictureMaps.begin();
    while (it != _pictureMaps.end() ) {
        if ( (*it)->getPageSize() == pageSize && !(*it)->isFull() ) {
            m = *it;
            break;
        }
        it++;
    }

    // nothing found
    if (m == NULL) {
        m = new PictureMap(pageSize);
        VideoPicture::_pictureMaps.append(m);
#ifdef VIDEOPICTURE_DEBUG
        fprintf(stderr, "+ Video File Picture Maps size = %d.", _pictureMaps.size());
#endif
    }

    return m;
}


void VideoPicture::clearPictureMaps()
{
    while (!_pictureMaps.isEmpty())
         delete _pictureMaps.takeFirst();

#ifdef VIDEOPICTURE_DEBUG
    fprintf(stderr, "\n- Video File Picture Maps cleared : %d.", _pictureMaps.size());
#endif
}

void VideoPicture::freePictureMap(PictureMap *pmap)
{
    int i = _pictureMaps.indexOf(pmap);
    if (i != -1 && _pictureMaps.at(i)->isEmpty() ) {
        _pictureMaps.removeAt(i);
        delete pmap;
#ifdef VIDEOPICTURE_DEBUG
        fprintf(stderr, "\n- Video File Picture Maps size = %d.", _pictureMaps.size());
#endif
    }
}

VideoPicture::VideoPicture() :
    pixel_format(AV_PIX_FMT_RGB24),
    data(NULL),
    pts(0),
    width(0),
    height(0),
    rowlength(0),
    action(0),
    frame(NULL),
    _pictureMap(NULL)
{
#ifdef VIDEOPICTURE_DEBUG
    VideoPicture::count++;
#endif
}

VideoPicture::VideoPicture(int w, int h, double Pts) :
    pixel_format(AV_PIX_FMT_RGB24),
    data(NULL),
    pts(Pts),
    width(w),
    height(h),
    rowlength(w),
    action(0),
    frame(NULL),
    _pictureMap(NULL)
{
    if (width==0 && height==0)
        VideoPictureException().raise();

#ifdef PICTURE_MAP
    VideoPicture::VideoPictureMapLock.lock();
    do {
        _pictureMap = VideoPicture::getAvailablePictureMap(width, height, pixel_format);
        data = _pictureMap->getAvailablePictureMemory();
    } while (data == NULL);
    VideoPicture::VideoPictureMapLock.unlock();
#else
    data = (uint8_t *) malloc( sizeof(uint8_t) * getBufferSize());
#endif

    // initialize buffer with zeros
    memset((void *) data, 0,  getBufferSize());
#ifdef VIDEOPICTURE_DEBUG
    VideoPicture::count++;
#endif
}

VideoPicture::VideoPicture(AVFilterContext *sink, double Pts):
    pts(Pts),
    data(NULL),
    action(0),
    _pictureMap(NULL)
{
    frame = av_frame_alloc();
    if (!frame || av_buffersink_get_frame(sink, frame) < 0 )
        VideoPictureException().raise();

    // copy properties
    width = frame->width;
    height = frame->height;
    pixel_format = (AVPixelFormat) frame->format;
    if (pixel_format!=AV_PIX_FMT_RGB24 && pixel_format!=AV_PIX_FMT_RGBA)
        VideoPictureException().raise();

    // row lenght is given by frame linesize
    rowlength = frame->linesize[0] / (pixel_format == AV_PIX_FMT_RGB24 ? 3 : 4);

    // do not need to copy data
#ifdef VIDEOPICTURE_DEBUG
    VideoPicture::count++;
#endif
}


VideoPicture::VideoPicture(AVFrame *f, double Pts):
    pts(Pts),
    rowlength(0),
    action(0),
    frame(NULL),
    _pictureMap(NULL)
{
    // copy properties
    width = f->width;
    height = f->height;
    pixel_format = (AVPixelFormat) f->format;
    if (pixel_format!= AV_PIX_FMT_RGB24 && pixel_format!=AV_PIX_FMT_RGBA)
        VideoPictureException().raise();

    // row lenght is given by frame linesize
    rowlength = f->linesize[0] / (pixel_format == AV_PIX_FMT_RGB24 ? 3 : 4);

    // allocate buffer
#ifdef PICTURE_MAP
    VideoPicture::VideoPictureMapLock.lock();
    do {
        _pictureMap = VideoPicture::getAvailablePictureMap(rowlength, height, pixel_format);
        data = _pictureMap->getAvailablePictureMemory();
    } while (data == NULL);
    VideoPicture::VideoPictureMapLock.unlock();
#else
    data = (uint8_t *) malloc( sizeof(uint8_t) * getBufferSize());
#endif

    // copy memory of data
    memmove((void *) (data), f->data[0], getBufferSize() );
#ifdef VIDEOPICTURE_DEBUG
    VideoPicture::count++;
#endif
}




VideoPicture::~VideoPicture()
{
#ifdef VIDEOPICTURE_DEBUG
        VideoPicture::count--;
#endif

    if (frame) {
        av_frame_free(&frame);
    }

    if (data) {
#ifdef PICTURE_MAP
        VideoPicture::VideoPictureMapLock.lock();
        _pictureMap->freePictureMemory(data);
        VideoPicture::freePictureMap(_pictureMap);
        VideoPicture::VideoPictureMapLock.unlock();
#else
        free(data);
#endif
    }

#ifdef GLM_CUDA
    if (g_pInteropFrame)
    {
        cuMemFree(g_pInteropFrame);
    }
#endif

}

void VideoPicture::saveToPPM(QString filename) const
{
    if (rowlength > 0)
    {
        FILE *pFile;
        int y;

        // Open file
        pFile = fopen(qPrintable(filename), "wb");
        if (pFile == NULL)
            return;

        // Write header
        fprintf(pFile, "P6\n%d %d\n255\n", width, height);

        for (int j = 0; j < height; ++j)
        {
          for (int i = 0; i < width; ++i)
          {
            (void) fwrite(data + j * rowlength + i * (pixel_format == AV_PIX_FMT_RGBA ? 4 : 3), 1, 3, pFile);
          }
        }

        // Close file
        fclose(pFile);

    }
}

#ifdef GLM_CUDA

void VideoPicture::fill(CUdeviceptr  pInteropFrame, double Pts)
{
    g_pInteropFrame = pInteropFrame;

    // remember pts
    pts = Pts;

    pixel_format = AV_PIX_FMT_RGBA;

    // no AVFrame
    data = 0;
}

#endif

int VideoPicture::getBufferSize() const
{
    return height * MAXI(rowlength, width) * (pixel_format == AV_PIX_FMT_RGB24 ? 3 : 4);
}

char *VideoPicture::getBuffer() const
{
    if (frame)
        return (char *) frame->data[0];
    else
        return (char*) data;
}
