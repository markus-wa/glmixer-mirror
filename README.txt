

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
xsltproc
libqtcore4
libqtgui4
libqt4-xml
libqt4-opengl
qt4-qmake
libqt4-dev
libqt4-opengl-dev
libavcodec-dev
libswscale-dev
libavformat-dev
libavutil-dev
libavfilter-dev
libglew-dev

Dependencies from libav (depends on version):
libvorbis-dev
libx264-dev
libxvidcore-dev
libv4l-dev

For optional OpenCV:
libcv-dev
libcvaux-dev
libhighgui-dev
libv4l-dev

In short, under linux, the following command installs what is necessary :

	$ sudo apt-get install subversion cmake-qt-gui ninja-build libqt4-opengl-dev libglew-dev libavformat-dev libhighgui-dev libavfilter-dev libv4l-dev xsltproc



COMPILATION

  1. Install the dependencies

  2. Checkout GLMixer source files from https://sourceforge.net/p/glmixer/Source/HEAD/tree/.

        svn checkout svn://svn.code.sf.net/p/glmixer/Source/trunk glmixer-Source

     or download source from

        https://sourceforge.net/projects/glmixer/files/Linux/

  3. Create a folder for building (e.g. glmixer-Build)

  4. Run CMake GUI and select the GLMixer top directory as location of the source.
     Do **configure** (choose Ninja)
     Make sure there is no error and set '`CMAKE_BUILD_TYPE`' to '`Release`'
     Do **generate** with CMake.

  5. Compile : cd to the build folder, and run 'ninja' in a terminal to build the program (or use an IDE).


UNIX COMPILATION

  Alternatively for Linux, you can do all in a terminal :

     Go to the building directory

	$ cd glmixer-Build

     Run cmake command line and choose ninja generator.

        $ cmake -G Ninja ../glmixer-Source

     To run cmake for compiling with all options activated :

        $ cmake -D CMAKE_BUILD_TYPE=Release -D USE_OPENCV=True -D USE_FREEFRAMEGL=1.6 -G Ninja ../glmixer-Source

     To run cmake for compiling with all options and to generage a package :

	$ cmake -D CMAKE_BUILD_TYPE=RelWithDebInfo -D USE_OPENCV=True -D USE_FREEFRAMEGL=1.6 -D CMAKE_INSTALL_PREFIX=/usr -G Ninja ../glmixer-Source/
	
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

     To install it in your system, build the debian package :

        $ cpack

     It hopefully ends with :

        CPack: - package: /home/[YOUR SOURCE PATH]/GLMixer_[version]_amd64.deb generated.

     And you can install it :

        $ sudo apt-get install /home/[YOUR SOURCE PATH]/GLMixer_[version]_amd64.deb



OSX COMPILATION

Install home-brew

	Follow instructions from http://brew.sh/

Install programs and libraries
	
	brew install subversion
	brew install ninja



WINDOWS COMPILATION


Install msys2

    Follow instructions from https://msys2.github.io/

Install programs and x86_64 libraries

	pacman -S subversion
	pacman -S mingw-w64-x86_64-gcc
	pacman -S mingw-w64-x86_64-ninja
	pacman -S mingw-w64-x86_64-cmake
	pacman -S mingw-w64-x86_64-glew
	pacman -S mingw-w64-x86_64-qt4
	pacman -S mingw-w64-x86_64-opencv
	pacman -S mingw-w64-x86_64-ffmpeg

Now, you can open a mingw terminal and compile line under Linux (see above)


