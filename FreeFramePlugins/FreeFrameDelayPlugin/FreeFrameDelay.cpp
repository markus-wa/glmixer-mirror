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
	000,								  // API minor version number	
	1,										// Plugin major version number
	000,									// Plugin minor version number
	FF_EFFECT,						// Plugin type
    "Delay the display of the provided input",	 // Plugin description
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
    SetParamInfo(FFPARAM_DELAY, "Delay", FF_TYPE_STANDARD, "0.5");
    delay = 0.5f;
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

    fprintf(stderr, "init Plugin %d x %d \n", vp->width, vp->height);

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

#ifdef FF_FAIL
    // FFGL 1.5
    DWORD	FreeFrameDelay::ProcessOpenGL(ProcessOpenGLStruct* pGL)
#else
    // FFGL 1.6
    FFResult FreeFrameDelay::ProcessOpenGL(ProcessOpenGLStruct *pGL)
#endif
{
  if (pGL->numInputTextures<1)
    return FF_FAIL;

  if (pGL->inputTextures[0]==NULL)
    return FF_FAIL;
  
  FFGLTextureStruct &Texture = *(pGL->inputTextures[0]);

  //bind the texture handle to its target
  glBindTexture(GL_TEXTURE_2D, Texture.Handle);

  //enable texturemapping
  glEnable(GL_TEXTURE_2D);

  //get the max s,t that correspond to the 
  //width,height of the used portion of the allocated texture space
  FFGLTexCoords maxCoords = GetMaxGLTexCoords(Texture);

  //modulate texture colors with white (just show
  //the texture colors as they are)
  glColor4f(1.f, 1.f, 1.f, 1.f);
  //(default texturemapping behavior of OpenGL is to
  //multiply texture colors by the current gl color)
  
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

  //unbind the texture
  glBindTexture(GL_TEXTURE_2D, 0);

  //disable texturemapping
  glDisable(GL_TEXTURE_2D);

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
        DWORD dwRet = FF_FAIL;

        if (index == FFPARAM_DELAY)
            *((float *)(unsigned)&dwRet) = delay;

        return dwRet;
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




