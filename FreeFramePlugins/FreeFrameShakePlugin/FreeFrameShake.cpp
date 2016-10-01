#include "FreeFrameShake.h"

#include <cmath>

////////////////////////////////////////////////////////////////////////////////////////////////////
//  Plugin information
////////////////////////////////////////////////////////////////////////////////////////////////////

#define FFPARAM_DISTORT (0)

static CFFGLPluginInfo PluginInfo ( 
    FreeFrameShake::CreateInstance,	// Create method
    "GLSHAK",           // Plugin unique ID
    "FreeFrameShake",   // Plugin name
    1,                  // API major version number
    500,                // API minor version number
    1,                  // Plugin major version number
    000,                // Plugin minor version number
    FF_EFFECT,          // Plugin type
    "Shaking plugin",	 // Plugin description
    "by Bruno Herbelin"  // About
);


////////////////////////////////////////////////////////////////////////////////////////////////////
//  Constructor and destructor
////////////////////////////////////////////////////////////////////////////////////////////////////

FreeFrameShake::FreeFrameShake()
: CFreeFrameGLPlugin()
{
    // Input properties
    SetMinInputs(1);
    SetMaxInputs(1);

    SetTimeSupported(true);

    // Parameters
    SetParamInfo(FFPARAM_DISTORT, "Shaking", FF_TYPE_STANDARD, 0.7f);
    distort = 0.7;
    param_changed = true;
    deltaTime = 0.01;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
//  Methods
////////////////////////////////////////////////////////////////////////////////////////////////////
#ifdef FF_FAIL
// FFGL 1.5
DWORD   FreeFrameTest::InitGL(const FFGLViewportStruct *vp)
#else
// FFGL 1.6
FFResult FreeFrameShake::InitGL(const FFGLViewportStruct *vp)
#endif
{

    return FF_SUCCESS;
}


#ifdef FF_FAIL
// FFGL 1.5
DWORD   FreeFrameTest::DeInitGL()
#else
// FFGL 1.6
FFResult FreeFrameShake::DeInitGL()
#endif
{

  return FF_SUCCESS;
}

#ifdef FF_FAIL
// FFGL 1.5
DWORD   FreeFrameTest::SetTime(double time)
#else
// FFGL 1.6
FFResult FreeFrameShake::SetTime(double time)
#endif
{
  deltaTime = time - m_curTime;
  m_curTime = time;
  return FF_SUCCESS;
}

#ifdef FF_FAIL
// FFGL 1.5
DWORD	FreeFrameTest::ProcessOpenGL(ProcessOpenGLStruct* pGL)
#else
// FFGL 1.6
FFResult FreeFrameShake::ProcessOpenGL(ProcessOpenGLStruct *pGL)
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

  glColor4f(1.f, 1.f, 1.f, 1.f);

  double a = distort * 0.1;
  tx = 0.6 * ( a * (double) rand() / RAND_MAX ) + 0.4 * tx;
  ty = 0.6 * ( a * (double) rand() / RAND_MAX ) + 0.4 * ty;

  glBegin(GL_QUADS);

  //lower left
  glTexCoord2d(a-tx, a-ty);
  glVertex2f(-1,-1);

  //upper left
  glTexCoord2d(a-tx, 1.0-a-ty);
  glVertex2f(-1,1);

  //upper right
  glTexCoord2d(1.0-a-tx, 1.0-a-ty);
  glVertex2f(1,1);

  //lower right
  glTexCoord2d(1.0-a-tx, a-ty);
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
DWORD FreeFrameBlur::SetParameter(const SetParameterStruct* pParam)
{
    if (pParam != NULL && pParam->ParameterNumber == FFPARAM_DISTORT) {
        distort = *((float *)(unsigned)&(pParam->NewParameterValue));
        param_changed = true;
        return FF_SUCCESS;
    }

    return FF_FAIL;
}

DWORD FreeFrameBlur::GetParameter(DWORD index)
{
    DWORD dwRet = 0;
    *((float *)(unsigned)&dwRet) = distort;

    if (index == FFPARAM_DISTORT)
        return dwRet;
    else
        return FF_FAIL;
}

#else
// FFGL 1.6
FFResult FreeFrameShake::SetFloatParameter(unsigned int index, float value)
{
    if (index == FFPARAM_DISTORT) {
        distort = value;
        param_changed = true;
        return FF_SUCCESS;
    }

    return FF_FAIL;
}

float FreeFrameShake::GetFloatParameter(unsigned int index)
{
    if (index == FFPARAM_DISTORT)
        return distort;

    return 0.0;
}
#endif

