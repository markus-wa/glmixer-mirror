#ifndef CUDAVIDEOFILE_H
#define CUDAVIDEOFILE_H

#include "VideoFile.h"
#include "VideoManager.h"

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

    cuda::VideoManager *cv;

};

#endif // CUDAVIDEOFILE_H
