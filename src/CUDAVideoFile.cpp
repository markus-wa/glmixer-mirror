#include "CUDAVideoFile.moc"

// NVIdia libCUDADecoder
#include "cudaModuleMgr.h"
#include "helper_cuda_drvapi.h"

// GLMixer
#include "defines.h"

// utility
#include <QApplication>


bool CUDAVideoFile::cudaregistered = false;
CUdevice CUDAVideoFile::g_oDevice = 0;

CUDAVideoFile::CUDAVideoFile(QObject *parent, bool generatePowerOfTwo,
                             int swsConversionAlgorithm, int destinationWidth,
                             int destinationHeight) :
    VideoFile(parent, generatePowerOfTwo, swsConversionAlgorithm, destinationWidth, destinationHeight), apVideoSource(0), apFrameQueue(0), apVideoDecoder(0), apVideoParser(0), apImageGL(0)
{

    AllocationException().raise();

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

    // initialize fields
    g_oContext = 0;
    g_pCudaModule = 0;
    g_kernelNV12toARGB = 0;
    g_kernelPassThru = 0;
    g_CtxLock = NULL;

    // Create CUDA context
    checkCudaErrors(cuCtxCreate(&g_oContext, CU_CTX_BLOCKING_SYNC, g_oDevice));

    // Create CUDA Module Manager : Open PTX pre-compiled CUDA assembly
    // TODO : NV Runtime compilation of the PTX
    try {

        g_pCudaModule = new CUmoduleManager("NV12ToARGB_drvapi64.ptx", qPrintable(QApplication::applicationDirPath()), 2, 2, 2);
    }
    catch (char const *p_file)
    {
        // If the CUmoduleManager constructor fails to load the PTX file, it will throw an exception
        qWarning("\n>> CUmoduleManager::Exception!  %s not found!\n", p_file);
        qWarning(">> Please rebuild NV12ToARGB_drvapi.cu or re-install this sample.\n");
        AllocationException().raise();
    }

    // Get functions from CUDA Module
    g_pCudaModule->GetCudaFunction("NV12ToARGB_drvapi",   &g_kernelNV12toARGB);
    g_pCudaModule->GetCudaFunction("Passthru_drvapi",     &g_kernelPassThru);

    // create the CUDA resources and the CUDA decoder context

    // bind the context lock to the CUDA context
    CUresult result = cuvidCtxLockCreate(&g_CtxLock, g_oContext);
    if (result != CUDA_SUCCESS)
    {
        qWarning("cuvidCtxLockCreate failed: %d\n", result);
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
    return apVideoSource != NULL;
}

bool CUDAVideoFile::open(QString file, double markIn, double markOut, bool ignoreAlphaChannel)
{
//    return VideoFile::open(file, markIn, markOut, ignoreAlphaChannel);

    apVideoSource = new cuda::VideoManager(file.toStdString(), apFrameQueue);


    // retrieve the video source (width,height)
    apVideoSource->getSourceDimensions(targetWidth, targetHeight);

    g_eVideoCreateFlags = cudaVideoCreate_PreferCUVID;

    apVideoDecoder = new cuda::VideoDecoder(apVideoSource->format(), g_oContext, g_eVideoCreateFlags, g_CtxLock);
    if (!apVideoDecoder)
        return false;

    apVideoParser = new cuda::VideoParser(apVideoDecoder, apFrameQueue, &g_oContext);
    if (!apVideoParser)
        return false;

    apVideoSource->setParser(*apVideoParser);

    apImageGL = new cuda::ImageGL(targetWidth, targetHeight, targetWidth, targetHeight);
    if (!apImageGL)
        return false;

    // initialize image
    apImageGL->clear(0x80);
    apImageGL->setCUDAcontext(g_oContext);
    apImageGL->setCUDAdevice(g_oDevice);

    CUcontext cuCurrent = NULL;
    CUresult result = cuCtxPopCurrent(&cuCurrent);
    if (result != CUDA_SUCCESS)
    {
        printf("cuCtxPopCurrent: %d\n", result);
        return false;
    }

    return true;
}

void CUDAVideoFile::start()
{
    apVideoSource->start();
}

void CUDAVideoFile::stop()
{
    apFrameQueue->endDecode();

    apVideoSource->stop();
}

void CUDAVideoFile::requestSeek(double time, bool lock)
{

}


class CUDADecodingThread: public videoFileThread
{
public:
    CUDADecodingThread(CUDAVideoFile *video) : videoFileThread(video)
    {

    }
    ~CUDADecodingThread()
    {

    }

    void run();

private:

};

void CUDADecodingThread::run()
{
    bool bFramesDecoded = false;

    _working = true;
    while (is && !is->quit && !_forceQuit)
    {

//        bFramesDecoded = copyDecodedFrameToTexture(nRepeatFrame, bUseInterop, &bIsProgressive);


    } // end while

    _working = false;


    // if normal exit through break (couldn't get any more packet)
    if (is) {

    }

}

