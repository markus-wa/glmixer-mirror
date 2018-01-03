

                       GLMixer

                 Graphic Live Mixer
  Real-time video mixing software for live performance

                 By Bruno Herbelin


       https://sourceforge.net/projects/glmixer/
                    GPL3 LICENCE

ABOUT

GLMixer performs in real time the graphical blending of several movie clips.
You would typically load a set of video samples by drag'n drop, and decide on the fly
how much they should be visible, arrange the images in the screen, and which effects
you apply to them.

The principle of interaction is to drop video files into a workspace, and to move
them in a circular area to change their opacity ; if you selects two videos, moving
them together makes a fading transition, and this principle is generalized to any
number of videos. Direct interaction with the video icons allows you to be fast and reactive,
and to move and deform videos on screen.

The output of your operations is shown in the main rendering window, which should
for example be displayed full-screen on an external monitor or a projector, or can
be saved as a video file.


HELP

Wiki
http://sourceforge.net/projects/glmixer/

Tutorial videos
http://vimeo.com/album/2401475

DEPENDENCIES

To compile GLMixer you need;
subversion
cmake
Qt Version 4 (not Qt 5)
Libav (or ffmpeg)
OpenCV (optionnal)


COMPILATION OVERVIEW 

  STEP 1. Install the dependencies

  STEP 2. Checkout GLMixer source files from https://sourceforge.net/p/glmixer/Source/HEAD/tree/.

        svn checkout svn://svn.code.sf.net/p/glmixer/Source/trunk glmixer-Source

     or download source from

        https://sourceforge.net/projects/glmixer/files/Linux/

  STEP 3. Create a folder for building (e.g. glmixer-Build)

  STEP 4. Compilation ; Run CMake and compile 

  STEP 5 - Packaging


STEP 4 - COMPILATION GENERIC INSTRUCTIONS

     Open a terminal

     Go to the building directory
        $ cd glmixer-Build

     Run cmake command line to choose ninja generator, for instance:
        $ cmake -G Ninja ../glmixer-Source

     Or, to be more specific, you might want to build a Release, ignore development warnings
     and use the optional features of OpenCV, Undo and FreeFrame (plugins):
        $ cmake -DCMAKE_BUILD_TYPE=RelWithDebInfo -Wno-dev -DUSE_OPENCV=True -DUSE_UNDO=True -D USE_FREEFRAMEGL=1.6 -G Ninja ../glmixer-Source

     If all goes well, it ends with :

        -- Configuring done
        -- Generating done
        -- Build files have been written to: XXX YOUR SOURCE PATH XXX

    Compile with ninja :
        $ ninja

     It should end with a message like (ignore warnings):

        [128/128] Linking CXX executable src/glmixer

     You can run the program directly :
        $ ./src/glmixer



LINUX COMPILATION

    STEP 1 - Install programs and libraries (Ubuntu)

        $ sudo apt-get install subversion cmake-qt-gui ninja-build libqt4-opengl-dev libglew-dev libavformat-dev libhighgui-dev libavfilter-dev libv4l-dev xsltproc

    (This installs all necessary packages such as xsltproc, libqtcore4, libqtgui4, libqt4-xml, libqt4-opengl, qt4-qmake, libqt4-dev, libqt4-opengl-dev, libavcodec-dev, libswscale-dev, libavformat-dev, libavutil-dev, libavfilter-dev, libglew-dev, libvorbis-dev, libx264-dev, libxvidcore-dev, libv4l-dev, libcv-dev, libcvaux-dev, libhighgui-dev, libv4l-dev).

    STEPS 2 to 4 - See above

    STEP 5 - Packaging to Deb package

    To install it in your system, run cmake (as above but) with the following options :

        $ cmake -D CMAKE_BUILD_TYPE=RelWithDebInfo -D USE_OPENCV=True -D USE_FREEFRAMEGL=1.6 -D CMAKE_INSTALL_PREFIX=/usr -G Ninja ../glmixer-Source/

    After compiling the program, build the debian package :

        $ cpack

    It hopefully ends with :

        CPack: - package: /home/[YOUR SOURCE PATH]/GLMixer_[version].deb generated.

    And you can now install it (use the filename generated above):

        $ sudo apt-get install /home/[YOUR SOURCE PATH]/GLMixer_[version].deb



OSX COMPILATION

    STEP 1 - Install programs and libraries (OSX)

    Install home-brew

        Follow instructions from http://brew.sh/

    Install programs and libraries (run the following in a terminal)

        brew install subversion
        brew install ninja
        brew install cmake
        brew install ffmpeg
        brew install qt4
        brew install glew
        brew install homebrew/science/opencv

    STEPS 2 to 3 - See above

    STEP 4 - Compile for OSX

    If you installed qt4 with brew, specify where qmake is :
        $ cmake -DCMAKE_BUILD_TYPE=Release -DUSE_OPENCV=True -DUSE_FREEFRAMEGL=1.6 -DUSE_UNDO=True -DCMAKE_OSX_DEPLOYMENT_TARGET=10.12 -DQT_QMAKE_EXECUTABLE=/usr/local/Cellar/qt/4.8.7_2/bin/qmake -G Ninja ../glmixer-Source/

    The rest is the same as in the generic instructions. To test your program;

        $ ./src/glmixer.app/Contents/MacOS/glmixer

    STEP 5 - Packaging to OSX Bundle package

    After compiling the program, build the package :

        $ sudo cpack
 
    This will generate the DragNDrop package (pkg) containing the Bundle (actual application) that can be dropped to the Application folder.




WINDOWS COMPILATION

    STEP 1 - Install programs and libraries (OSX)

    Install msys2

        Follow instructions from https://msys2.github.io/

    Install programs and x86_64 libraries (run the following in a terminal)

        pacman -S subversion
        pacman -S mingw-w64-x86_64-gcc
        pacman -S mingw-w64-x86_64-ninja
        pacman -S mingw-w64-x86_64-cmake
        pacman -S mingw-w64-x86_64-glew
        pacman -S mingw-w64-x86_64-qt4
        pacman -S mingw-w64-x86_64-opencv
        pacman -S mingw-w64-x86_64-ffmpeg

    STEPS 2 to 3 - See above

    STEP 4 - Compile for Windows

    To configure for Windows Release packaging:
        $ cmake -D CMAKE_BUILD_TYPE=RelWithDebInfo -D USE_OPENCV=True -DUSE_UNDO=True -D USE_FREEFRAMEGL=1.6 -G Ninja ../glmixer-Source/
    
    The rest is the same as in the generic instructions. To test your program;

        $ ./src/glmixer.exe

    STEP 5 - Packaging to Windows installer

    Install NSIS

        Follow instructions from http://nsis.sourceforge.net/Download

    After compiling the program, re-execute cmake before running cpack:

        $ cmake .
        $ cpack
 
    This will generate the installer for windows (file GLMixer_1.6-XXXX_Windows.exe)
