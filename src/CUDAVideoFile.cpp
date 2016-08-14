#include "CUDAVideoFile.moc"

#include "defines.h"

#include "helper_cuda_drvapi.h"

bool CUDAVideoFile::cudaregistered = false;
CUdevice CUDAVideoFile::g_oDevice = 0;

CUDAVideoFile::CUDAVideoFile(QObject *parent, bool generatePowerOfTwo,
                             int swsConversionAlgorithm, int destinationWidth,
                             int destinationHeight) :
    VideoFile(parent, generatePowerOfTwo, swsConversionAlgorithm, destinationWidth, destinationHeight), apVideoManager(0), apFrameQueue(0)
{
    if (!cudaregistered) {
        cuInit(0);


        CUdevice cuda_device;
        cuda_device = findCudaDeviceDRV();

        checkCudaErrors(cuDeviceGet(&g_oDevice, cuda_device));

        // get compute capabilities and the devicename
        int major, minor;
        size_t totalGlobalMem;

        char deviceName[256];
        checkCudaErrors(cuDeviceComputeCapability(&major, &minor, g_oDevice));
        checkCudaErrors(cuDeviceGetName(deviceName, 256, g_oDevice));
        printf("> Using GPU Device: %s has SM %d.%d compute capability\n", deviceName, major, minor);

        checkCudaErrors(cuDeviceTotalMem(&totalGlobalMem, g_oDevice));
        printf("  Total amount of global memory:     %4.4f MB\n", (float)totalGlobalMem/(1024*1024));


        cudaregistered = true;
    }


    apFrameQueue = new cuda::FrameQueue();

    if (!apFrameQueue)
        AllocationException().raise();

}

CUDAVideoFile::~CUDAVideoFile()
{

    delete apFrameQueue;
}

bool CUDAVideoFile::isOpen() const
{
    return apVideoManager != NULL;
}

bool CUDAVideoFile::open(QString file, double markIn, double markOut, bool ignoreAlphaChannel)
{
//    return VideoFile::open(file, markIn, markOut, ignoreAlphaChannel);

    apVideoManager = new cuda::VideoManager(file.toStdString(), apFrameQueue);


    // retrieve the video source (width,height)
    apVideoManager->getSourceDimensions(targetWidth, targetHeight);

    g_eVideoCreateFlags = cudaVideoCreate_PreferCUVID;

    return true;
}

void CUDAVideoFile::start()
{

}

void CUDAVideoFile::stop()
{

}

void CUDAVideoFile::requestSeek(double time, bool lock)
{

}
