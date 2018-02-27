#include "FreeFrameShake.h"

#include <cstdio>
#include <cstdlib>
#include <random>
#include <cmath>
#include <ctime>
#define CLAMP(x, low, high)  (((x) > (high)) ? (high) : (((x) < (low)) ? (low) : (x)))

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
        "Emulate camera shaking",	 // Plugin description
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
    SetParamInfo(FFPARAM_DISTORT, "Amplitude", FF_TYPE_STANDARD, 0.7f);
    distort = 0.7;

    deltaTime = 0.01;
    m_curTime = 0.0;
    tx = ty = sx = sy = 0.0;
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
    if (!pGL)
        return FF_FAIL;

    if (pGL->numInputTextures<1)
        return FF_FAIL;

    if (pGL->inputTextures[0]==NULL)
        return FF_FAIL;

    FFGLTextureStruct &Texture = *(pGL->inputTextures[0]);

    glClearColor(0.f, 0.f, 0.f, 0.f);
    glClear(GL_COLOR_BUFFER_BIT);

    //bind the texture handle to its target
    glBindTexture(GL_TEXTURE_2D, Texture.Handle);

    //enable texturemapping
    glEnable(GL_TEXTURE_2D);

    glColor4f(1.f, 1.f, 1.f, 1.f);

    double amplitude = distort * 0.1;
    srand((unsigned int) (m_curTime * 1000));
    // first call to rand is often a bad pick
    rand();

    // attract to the center
    double attractor =  tx - (0.5 * amplitude);
    // horizontal shake is a random acceleration factor
    double shake =  distort * ( 2.0 * ((double) rand() / (double) (RAND_MAX)) - 1.0);
    // speed and position mecanics
    sx += deltaTime * (shake - attractor);
    tx += deltaTime * sx;
    // bounce with damping on border
    if (tx < 0.0 || tx > amplitude)
        sx = -sx * 0.1;

    // attract to the center
    attractor = ty - (0.5 * amplitude);
    // vertical shake is a random acceleration, but larger than horizontal
    shake =  2.0 * distort * ( 2.0 * ((double) rand() / (double) (RAND_MAX)) - 1.0);
    // speed and position mecanics
    sy += deltaTime * (shake - attractor);
    ty += deltaTime * sy;
    // bounce with damping on border
    if (ty < 0.0 || ty > amplitude)
        sy = -sy * 0.1;

// fprintf(stderr, "%f, %f, %f, %f %f \n", tx, sx, ty, sy, attractor );

    // ensure values are clamped in valid range
    tx = CLAMP( tx, 0.0, amplitude);
    ty = CLAMP( ty, 0.0, amplitude);

    glBegin(GL_QUADS); 

    //lower left
    glTexCoord2d(amplitude-tx, amplitude-ty);
    glVertex2f(-1,-1);

    //upper left
    glTexCoord2d(amplitude-tx, 1.0-amplitude-ty);
    glVertex2f(-1,1);

    //upper right
    glTexCoord2d(1.0-amplitude-tx, 1.0-amplitude-ty);
    glVertex2f(1,1);

    //lower right
    glTexCoord2d(1.0-amplitude-tx, amplitude-ty);
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
DWORD FreeFrameShake::SetParameter(const SetParameterStruct* pParam)
{
    if (pParam != NULL && pParam->ParameterNumber == FFPARAM_DISTORT) {
        distort = *((float *)(unsigned)&(pParam->NewParameterValue));
        return FF_SUCCESS;
    }

    return FF_FAIL;
}

DWORD FreeFrameShake::GetParameter(DWORD index)
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
        sx = sy = 0.0;
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

