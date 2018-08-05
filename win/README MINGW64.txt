
To get CUDA
-----------

Install cuda from https://developer.nvidia.com/ffmpeg 
NB: Custom installation into directory C:\msys64\opt\cuda

Install ffnvcodec
------------------

see https://superuser.com/questions/1299064/error-cuvid-requested-but-not-all-dependencies-are-satisfied-cuda-ffnvcodec

# compile package
$ cd mingw-w64-ffnvcodec-headers
$ MINGW_INSTALLS=mingw64 makepkg-mingw -sLf --nocheck

# install package
$ pacman -U mingw-w64-x86_64-ffnvcodec-headers-2~1.20180804-1-any.pkg.tar.xz


To compile ffmpeg package
--------------------------

# will be needed..
$ pacman -S diffutils make 

# make sure all dependencies are installed
$ pacman -S mingw-w64-x86_64-ffmpeg
$ pacman -R mingw-w64-x86_64-ffmpeg

# get approval PGP 
$ gpg --keyserver pgp.mit.edu --recv-keys FCF986EA15E6E293A5644F10B4322F04D67658D8

# compile package
$ cd mingw-w64-ffmpeg
$ MINGW_INSTALLS=mingw64 makepkg-mingw -sLf --nocheck

# install package
$ pacman -U mingw-w64-x86_64-ffmpeg-4.0.2-1-any.pkg.tar.xz

