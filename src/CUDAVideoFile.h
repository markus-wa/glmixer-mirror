#ifndef CUDAVIDEOFILE_H
#define CUDAVIDEOFILE_H

#include "VideoFile.h"
#include "VideoManager.h"
#include "FrameQueue.h"



class CUDAVideoFile : public VideoFile
{    
    Q_OBJECT

public:
    CUDAVideoFile(QObject *parent = 0,  bool generatePowerOfTwo = false,
                  int swsConversionQuality = 0, int destinationWidth = 0,
                  int destinationHeight = 0);

    virtual  ~CUDAVideoFile();

    bool open(QString file, double  markIn = -1.0, double  markOut = -1.0, bool ignoreAlphaChannel = false);
    bool isOpen() const;
    void start();
    void stop();

private:

    void requestSeek(double time, bool lock = false);

    // CUDA wrapper
    cuda::VideoManager *apVideoManager;
    cuda::FrameQueue *apFrameQueue;

    // CUDA internal
    cudaVideoCreateFlags g_eVideoCreateFlags;
    CUcontext          g_oContext;
    class CUmoduleManager   *g_pCudaModule;


    static bool cudaregistered;
    static CUdevice g_oDevice;

};

#endif // CUDAVIDEOFILE_H
