#include <FFGL.h>
#include <FFGLLib.h>

#include <stdio.h>

#include "FreeFrameDelay.h"

#define FFPARAM_DELAY (0)


////////////////////////////////////////////////////////////////////////////////////////////////////
//  Plugin information
////////////////////////////////////////////////////////////////////////////////////////////////////

static CFFGLPluginInfo PluginInfo ( 
        FreeFrameDelay::CreateInstance,	// Create method
        "GLDLY",								// Plugin unique ID
        "FFGLDelay",			// Plugin name
        1,						   			// API major version number
        500,								  // API minor version number
        1,										// Plugin major version number
        000,									// Plugin minor version number
        FF_EFFECT,						// Plugin type
        "Delay (in second) the display of the provided input",	 // Plugin description
        "by Bruno Herbelin"  // About
        );


////////////////////////////////////////////////////////////////////////////////////////////////////
//  Constructor and destructor
////////////////////////////////////////////////////////////////////////////////////////////////////

FreeFrameDelay::FreeFrameDelay()
    : CFreeFrameGLPlugin()
{
    // Input properties
    SetMinInputs(1);
    SetMaxInputs(1);
    SetTimeSupported(true);

    // Parameters
    SetParamInfo(FFPARAM_DELAY, "Delay", FF_TYPE_STANDARD, 0.5f);
    delay = 0.5f;

    // init
    writeIndex = 0;
    readIndex = 0;
    m_curTime = 0.0;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
//  Methods
////////////////////////////////////////////////////////////////////////////////////////////////////
#ifdef FF_FAIL
// FFGL 1.5
DWORD   FreeFrameDelay::InitGL(const FFGLViewportStruct *vp)
#else
// FFGL 1.6
FFResult FreeFrameDelay::InitGL(const FFGLViewportStruct *vp)
#endif
{
    viewport.x = 0;
    viewport.y = 0;
    viewport.width = vp->width;
    viewport.height = vp->height;

    //init gl extensions
    glExtensions.Initialize();
    if (glExtensions.EXT_framebuffer_object==0)
        return FF_FAIL;

    // generate a buffer of FBOs with render buffers (& associated buffer of times)
    for (int n = 0; n < MAX_NUM_FRAMES; ++n) {
        if (! fbo[n].Create( viewport.width, viewport.height, glExtensions) )
            return FF_FAIL;
        times[n] = 0.0;
    }

    return FF_SUCCESS;
}


#ifdef FF_FAIL
// FFGL 1.5
DWORD   FreeFrameDelay::DeInitGL()
#else
// FFGL 1.6
FFResult FreeFrameDelay::DeInitGL()
#endif
{

    for (int n = 0; n < MAX_NUM_FRAMES; ++n)
        fbo[n].FreeResources( glExtensions);


    return FF_SUCCESS;
}

#ifdef FF_FAIL
// FFGL 1.5
DWORD   FreeFrameDelay::SetTime(double time)
#else
// FFGL 1.6
FFResult FreeFrameDelay::SetTime(double time)
#endif
{
    m_curTime = time;
    return FF_SUCCESS;
}

void drawQuad( FFGLViewportStruct vp, FFGLTextureStruct texture)
{
    // use the texture coordinates provided
    FFGLTexCoords maxCoords = GetMaxGLTexCoords(texture);

    // bind the texture to apply
    glBindTexture(GL_TEXTURE_2D, texture.Handle);

    //modulate texture colors with white (just show
    //the texture colors as they are)
    glColor4f(1.f, 1.f, 1.f, 1.f);

    // setup display
    glViewport(vp.x,vp.y,vp.width,vp.height);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    glBegin(GL_QUADS);
    //lower left
    glTexCoord2d(0.0, 0.0);
    glVertex2f(-1,-1);

    //upper left
    glTexCoord2d(0.0, maxCoords.t);
    glVertex2f(-1,1);

    //upper right
    glTexCoord2d(maxCoords.s, maxCoords.t);
    glVertex2f(1,1);

    //lower right
    glTexCoord2d(maxCoords.s, 0.0);
    glVertex2f(1,-1);
    glEnd();


}

#ifdef FF_FAIL
// FFGL 1.5
DWORD	FreeFrameDelay::ProcessOpenGL(ProcessOpenGLStruct* pGL)
#else
// FFGL 1.6
FFResult FreeFrameDelay::ProcessOpenGL(ProcessOpenGLStruct *pGL)
#endif
{
    if (!pGL)
        return FF_FAIL;

    if (pGL->numInputTextures<1)
        return FF_FAIL;

    if (pGL->inputTextures[0]==NULL)
        return FF_FAIL;

    FFGLTextureStruct &Texture = *(pGL->inputTextures[0]);

    //enable texturemapping
    glEnable(GL_TEXTURE_2D);

    // no depth test
    glDisable(GL_DEPTH_TEST);

    // recals the time of the writing of this frame into the fbo buffer
    times[writeIndex] = m_curTime;
    if (!fbo[writeIndex].BindAsRenderTarget(glExtensions))
        return FF_FAIL;

    // draw the input texture
    drawQuad( viewport, Texture);

    // find the read index where the time difference is just above the delay requested.
    if ( delay > 0.0) {
        // loop over the whole buffer
        for (int i = (readIndex + 1)%MAX_NUM_FRAMES; i != readIndex; i = (i+1)%MAX_NUM_FRAMES) {
            // break if found times corresponding to the delay
            if ( m_curTime - times[(i+1)%MAX_NUM_FRAMES] < delay && m_curTime - times[i] > delay ) {
                readIndex = i;
                break;
            }
        }
    } else
        readIndex = writeIndex;

    // (re)activate the HOST fbo as render target
    glExtensions.glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, pGL->HostFBO);

    // draw this index
    drawQuad( viewport, fbo[readIndex].GetTextureInfo());

    // next frame
    writeIndex = (writeIndex + 1) % MAX_NUM_FRAMES;

    //unbind the texture
    glBindTexture(GL_TEXTURE_2D, 0);

    //disable texturemapping
    glDisable(GL_TEXTURE_2D);


    // TODO : blit instead of draw ?
    //  glBindFramebuffer(GL_READ_FRAMEBUFFER, fboId);
    //  glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
    //  glBlitFramebuffer(0, 0, TEXTURE_WIDTH, TEXTURE_HEIGHT,
    //                    0, 0, screenWidth, screenHeight,
    //                    GL_COLOR_BUFFER_BIT,
    //                    GL_LINEAR);
    //  glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);

    return FF_SUCCESS;
}


#ifdef FF_FAIL
// FFGL 1.5
DWORD FreeFrameDelay::SetParameter(const SetParameterStruct* pParam)
{
    if (pParam != NULL && pParam->ParameterNumber == FFPARAM_DELAY) {
        delay = *((float *)(unsigned)&(pParam->NewParameterValue));
        return FF_SUCCESS;
    }

    return FF_FAIL;
}

DWORD FreeFrameDelay::GetParameter(DWORD index)
{
    DWORD dwRet;
    *((float *)(unsigned)&dwRet) = delay;

    if (index == FFPARAM_DELAY)
        return dwRet;
    else
        return FF_FAIL;
}

#else
// FFGL 1.6
FFResult FreeFrameDelay::SetFloatParameter(unsigned int index, float value)
{
    if (index == FFPARAM_DELAY) {
        delay = value;
        return FF_SUCCESS;
    }

    return FF_FAIL;
}

float FreeFrameDelay::GetFloatParameter(unsigned int index)
{
    if (index == FFPARAM_DELAY)
        return delay;

    return 0.0;
}
#endif


// find the index where the time difference is just above the delay requested.
//  int i = (writeIndex - 1) < 0 ? MAX_NUM_FRAMES - 1 : writeIndex - 1;
//  while (i != writeIndex )  {
//      if ( m_curTime - times[i] > delay ) {
//          readIndex = (i + 1)%MAX_NUM_FRAMES;
//          break;
//      }
//      N++;
//      i =  (i - 1) < 0 ? MAX_NUM_FRAMES - 1 : i - 1;
//  }

