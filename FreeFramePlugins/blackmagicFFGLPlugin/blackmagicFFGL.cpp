#include "blackmagicFFGL.h"

#include "error_image.h"
#include "blackmagicPlayback.h"

#include <stdio.h>
#include <unistd.h>
#include <libkern/OSAtomic.h>


class blackmagicFreeFrameGLData {

public:

    // black magic
    IDeckLink*	deckLink;	
	PlaybackHelper*	helper;
    char			deviceName[64];

    // opengl
    GLuint textureIndex;
    GLuint errorTextureIndex;
    int width;
    int height;

    blackmagicFreeFrameGLData(){
        deckLink = NULL;
        helper = NULL;
        sprintf(deviceName, "None");
        
        textureIndex = 0;
        errorTextureIndex = 0;
        width = 0;
        height = 0;
    }

};


#define FFPARAM_DEVICE 0
GLuint displayList = 0;

////////////////////////////////////////////////////////////////////////////////////////////////////
//  Plugin information
////////////////////////////////////////////////////////////////////////////////////////////////////

static CFFGLPluginInfo PluginInfo (
        blackmagicFreeFrameGL::CreateInstance,	// Create method
        "FFGLBLACKMAGIC",								// Plugin unique ID
        "BlackMagic",                          // Plugin name
        1,                                      // API major version number
        600,                                    // API minor version number
        1,										// Plugin major version number
        000,									// Plugin minor version number
        FF_SOURCE,                              // Plugin type
        "Display BlackMagic input",            // Plugin description
        "by Bruno Herbelin"                     // About
        );


////////////////////////////////////////////////////////////////////////////////////////////////////
//  Constructor and destructor
////////////////////////////////////////////////////////////////////////////////////////////////////

blackmagicFreeFrameGL::blackmagicFreeFrameGL()
    : CFreeFrameGLPlugin()
{
    data = new blackmagicFreeFrameGLData;

    // Input properties
    SetMinInputs(0);
    SetMaxInputs(0);

    // Parameters
    // SetParamInfo(FFPARAM_DEVICE, "Device", FF_TYPE_TEXT, "auto");
}

blackmagicFreeFrameGL::~blackmagicFreeFrameGL()
{
    delete data;
}


////////////////////////////////////////////////////////////////////////////////////////////////////
//  Methods
////////////////////////////////////////////////////////////////////////////////////////////////////
#ifdef FF_FAIL
// FFGL 1.5
DWORD   blackmagicFreeFrameGL::InitGL(const FFGLViewportStruct *vp)
#else
// FFGL 1.6
FFResult blackmagicFreeFrameGL::InitGL(const FFGLViewportStruct *vp)
#endif
{

    glEnable(GL_TEXTURE);
    glActiveTexture(GL_TEXTURE0);

    glGenTextures(1, &(data->errorTextureIndex));
    glBindTexture(GL_TEXTURE_2D, data->errorTextureIndex);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, error_image.width, error_image.height, 0,
                 GL_RGBA, GL_UNSIGNED_BYTE,(GLvoid*) error_image.pixel_data);

    if (displayList == 0) {
        displayList = glGenLists(1);
        glNewList(displayList, GL_COMPILE);
            glColor4f(1.f, 1.f, 1.f, 1.f);
            glMatrixMode(GL_PROJECTION);
            glLoadIdentity();
            glMatrixMode(GL_MODELVIEW);
            glLoadIdentity();
            glBegin(GL_QUADS);
            //lower left
            glTexCoord2d(0.0, 0.0);
            glVertex2f(-1,-1);
            //upper left
            glTexCoord2d(0.0, 1.0);
            glVertex2f(-1,1);
            //upper right
            glTexCoord2d(1.0, 1.0);
            glVertex2f(1,1);
            //lower right
            glTexCoord2d(1.0, 0.0);
            glVertex2f(1,-1);
            glEnd();
        glEndList();
    }

	// Use the first decklink card
	data->deckLink = getFirstDeckLinkCard();	
    if (data->deckLink) {

        // Display device name
        CFStringRef		deviceNameCFString = NULL;
        data->deckLink->GetModelName(&deviceNameCFString);
        CFStringGetCString(deviceNameCFString, data->deviceName, sizeof(data->deviceName), kCFStringEncodingMacRoman);
        fprintf(stderr, "Found Blackmagic device: %s\n", data->deviceName);
        
        // Create playback helper
        data->helper = new PlaybackHelper(data->deckLink);
        if (data->helper->init()) {

            fprintf(stderr, "BlackMagic device %s initialized.\n", data->deviceName);

            if (data->helper->startPlayback())
            {
                fprintf(stderr, "BlackMagic device %s running.\n", data->deviceName);
            }

        }
        else {
            fprintf(stderr, "Could not initialize BlackMagic device %s.\n", data->deviceName);
            delete data->helper;
            data->helper = NULL;
        }

    }
    else 
        fprintf(stderr, "No BlackMagic device found.\n");

    return FF_SUCCESS;
}


#ifdef FF_FAIL
// FFGL 1.5
DWORD   blackmagicFreeFrameGL::DeInitGL()
#else
// FFGL 1.6
FFResult blackmagicFreeFrameGL::DeInitGL()
#endif
{
    // release playback
    if (data->helper) {

		data->helper->stopPlayback();

        data->helper->Release();
        
        fprintf(stderr, "BlackMagic capture from %s terminated.\n", data->deviceName);
    }
    // release device
    if (data->deckLink) {
        data->deckLink->Release();
        fprintf(stderr, "BlackMagic device %s closed.\n", data->deviceName);
    }

    return FF_SUCCESS;
}

#ifdef FF_FAIL
// FFGL 1.5
DWORD   blackmagicFreeFrameGL::SetTime(double time)
#else
// FFGL 1.6
FFResult blackmagicFreeFrameGL::SetTime(double time)
#endif
{
    m_curTime = time;
    return FF_SUCCESS;
}

#ifdef FF_FAIL
// FFGL 1.5
DWORD	blackmagicFreeFrameGL::ProcessOpenGL(ProcessOpenGLStruct* pGL)
#else
// FFGL 1.6
FFResult blackmagicFreeFrameGL::ProcessOpenGL(ProcessOpenGLStruct *pGL)
#endif
{
    if (!pGL)
        return FF_FAIL;

    glClear(GL_COLOR_BUFFER_BIT );

    glEnable(GL_TEXTURE_2D);

//     if (!data->stop) {

//         glBindTexture(GL_TEXTURE_2D, data->textureIndex);

//         pthread_mutex_lock( &(data->mutex) );
//         // get a new frame

//         glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, data->width, data->height, GL_RGB, GL_UNSIGNED_BYTE, data->_glbuffer[data->read_buffer]);
// //        fprintf(stderr, "%d %d image", data->width, data->height);

//         pthread_mutex_unlock( &(data->mutex) );

//     } else 
    {

        glBindTexture(GL_TEXTURE_2D, data->errorTextureIndex);

    }

    glCallList(displayList);

    //unbind the texture
    glBindTexture(GL_TEXTURE_2D, 0);

    //disable texturemapping
    glDisable(GL_TEXTURE_2D);

    return FF_SUCCESS;
}

#ifdef FF_FAIL
// FFGL 1.5
DWORD	blackmagicFreeFrameGL::SetTextParameter(unsigned int index, const char *value)
#else
// FFGL 1.6
FFResult blackmagicFreeFrameGL::SetTextParameter(unsigned int index, const char *value)
#endif
{

    return FF_FAIL;
}


char* blackmagicFreeFrameGL::GetTextParameter(unsigned int index)
{
    return (char *)FF_FAIL;
}


