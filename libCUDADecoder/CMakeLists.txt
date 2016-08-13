
cmake_minimum_required (VERSION 2.8)
project (LIBCUDADECODER)


set(LIBCUDADECODER_SRC
    ImageGL.cpp  
    FrameQueue.cpp    
    VideoParser.cpp
    VideoDecoder.cpp   
    VideoSource.cpp
    cudaProcessFrame.cpp
	cudaModuleMgr.cpp   
)

#nvcc
set(CUDA_NVCC_FLAGS ${CUDA_NVCC_FLAGS};-gencode arch=compute_20,code=sm_20)

cuda_compile(NV12ToARGB_O NV12ToARGB_drvapi.cu)

add_library (CUDADecoder ${LIBCUDADECODER_SRC})  
