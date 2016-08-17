#include "CUDAVideoFile.moc"
#include "cudaModuleMgr.h"

#include "defines.h"

#include "helper_cuda_drvapi.h"


bool CUDAVideoFile::cudaregistered = false;
CUdevice CUDAVideoFile::g_oDevice = 0;

CUDAVideoFile::CUDAVideoFile(QObject *parent, bool generatePowerOfTwo,
                             int swsConversionAlgorithm, int destinationWidth,
                             int destinationHeight) :
    VideoFile(parent, generatePowerOfTwo, swsConversionAlgorithm, destinationWidth, destinationHeight), apVideoManager(0), apFrameQueue(0), g_oContext(0)
{
    if (!cudaregistered) {
        cuInit(0);


        CUdevice cuda_device;
        cuda_device = findCudaDeviceDRV();
        checkCudaErrors(cuDeviceGet(&g_oDevice, cuda_device));

        // get device name
        char deviceName[256];
        checkCudaErrors(cuDeviceGetName(deviceName, 256, g_oDevice));

        // check minimum capability
        int bSupported = checkCudaCapabilitiesDRV(1, 1, g_oDevice);
        if (!bSupported)
        {
            qWarning("  -> GPU: \"%s\" does not meet the minimum spec of SM 1.1\n", deviceName);
            qWarning("  -> A GPU with a minimum compute capability of SM 1.1 or higher is required.\n");
            AllocationException().raise();
        }

        // get compute capabilities & memory
        int major, minor;
        size_t totalGlobalMem;
        checkCudaErrors(cuDeviceComputeCapability(&major, &minor, g_oDevice));
        checkCudaErrors(cuDeviceTotalMem(&totalGlobalMem, g_oDevice));
        qDebug()<< "Found CUDA GPU Device" << deviceName << "(SM "<< major << '.' << minor << "compute capability, " << (float)totalGlobalMem/(1024*1024) << "MB memory)";

        cudaregistered = true;
    }

    checkCudaErrors(cuCtxCreate(&g_oContext, CU_CTX_BLOCKING_SYNC, g_oDevice));


    char exec_path[256];
    try {
        g_pCudaModule = new CUmoduleManager(":/cuda/libCUDADecoder/NV12ToARGB_drvapi64.ptx", exec_path, 2, 2, 2);
    }
    catch (char const *p_file)
    {
        // If the CUmoduleManager constructor fails to load the PTX file, it will throw an exception
        qWarning("\n>> CUmoduleManager::Exception!  %s not found!\n", p_file);
        qWarning(">> Please rebuild NV12ToARGB_drvapi.cu or re-install this sample.\n");
        AllocationException().raise();
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
