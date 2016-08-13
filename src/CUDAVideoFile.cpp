#include "CUDAVideoFile.moc"

CUDAVideoFile::CUDAVideoFile(QObject *parent, bool generatePowerOfTwo,
                             int swsConversionAlgorithm, int destinationWidth,
                             int destinationHeight) :
    VideoFile(parent, generatePowerOfTwo, swsConversionAlgorithm, destinationWidth, destinationHeight)
{

}

CUDAVideoFile::~CUDAVideoFile()
{

}

bool CUDAVideoFile::isOpen() const {
    return VideoFile::isOpen();
}

bool CUDAVideoFile::open(QString file, double markIn, double markOut, bool ignoreAlphaChannel)
{
    return VideoFile::open(file, markIn, markOut, ignoreAlphaChannel);

}

void CUDAVideoFile::start() {

}

void CUDAVideoFile::stop() {

}
