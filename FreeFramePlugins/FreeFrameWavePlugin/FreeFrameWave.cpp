#include <FFGL.h>
#include <FFGLLib.h>
#include "FreeFrameWave.h"

#include <cmath>

////////////////////////////////////////////////////////////////////////////////////////////////////
//  Plugin information
////////////////////////////////////////////////////////////////////////////////////////////////////

static CFFGLPluginInfo PluginInfo ( 
        FreeFrameWave::CreateInstance,	// Create method
        "GLwAVE",           // Plugin unique ID
        "FreeFrameWave",    // Plugin name
        1,                  // API major version number
        500,                // API minor version number
        1,                  // Plugin major version number
        000,                // Plugin minor version number
        FF_EFFECT,          // Plugin type
        "Wavy plugin",      // Plugin description
        "by Bruno Herbelin" // About
        );


////////////////////////////////////////////////////////////////////////////////////////////////////
//  Constructor and destructor
////////////////////////////////////////////////////////////////////////////////////////////////////

FreeFrameWave::FreeFrameWave()
    : CFreeFrameGLPlugin()
{
    // Input properties
    SetMinInputs(1);
    SetMaxInputs(1);

}

////////////////////////////////////////////////////////////////////////////////////////////////////
//  Methods
////////////////////////////////////////////////////////////////////////////////////////////////////
#ifdef FF_FAIL
// FFGL 1.5
DWORD   FreeFrameWave::InitGL(const FFGLViewportStruct *vp)
#else
// FFGL 1.6
FFResult FreeFrameWave::InitGL(const FFGLViewportStruct *vp)
#endif
{

    width = vp->width;
    height = vp->height;

    return FF_SUCCESS;
}


#ifdef FF_FAIL
// FFGL 1.5
DWORD   FreeFrameWave::DeInitGL()
#else
// FFGL 1.6
FFResult FreeFrameWave::DeInitGL()
#endif
{

    return FF_SUCCESS;
}

#ifdef FF_FAIL
// FFGL 1.5
DWORD   FreeFrameWave::SetTime(double time)
#else
// FFGL 1.6
FFResult FreeFrameWave::SetTime(double time)
#endif
{
    m_curTime = time;
    return FF_SUCCESS;
}

#ifdef FF_FAIL
// FFGL 1.5
DWORD	FreeFrameWave::ProcessOpenGL(ProcessOpenGLStruct* pGL)
#else
// FFGL 1.6
FFResult FreeFrameWave::ProcessOpenGL(ProcessOpenGLStruct *pGL)
#endif
{
    if (!pGL)
        return FF_FAIL;

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
    // FFGLTexCoords maxCoords = GetMaxGLTexCoords(Texture);

    //modulate texture colors with white (just show
    //the texture colors as they are)
    glColor4f(1.f, 1.f, 1.f, 1.f);

    unsigned long tick = (unsigned long) (m_curTime * 1000);
    int i, j;

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    int wmargin = width / 32;
    int hmargin = height / 32;
    glOrtho(wmargin, width-wmargin, hmargin, height-hmargin, -1, 1);

    glMatrixMode(GL_MODELVIEW);

    // Generate a 16x16 grid, and perturb the UV coordinates
    glBegin(GL_TRIANGLE_STRIP);
    for (j = 0; j < 16; j++)
    {
        for (i = 0; i < 17; i++)
        {
            float u, v;
            u = i * (1 / 16.0f);
            v = j * (1 / 16.0f);
            u += (float)((sin((tick + i*100) * 0.012387) * 0.04) * sin(tick * 0.000514382));
            v += (float)((cos((tick + j*100) * 0.012387) * 0.04) * sin(tick * 0.000714282));
            glTexCoord2f(u, v);
            glVertex2f(i * (width / 16.0f), j * (height / 16.0f));

            u = i * (1 / 16.0f);
            v = (j + 1) * (1 / 16.0f);
            u += (float)((sin((tick +       i*100) * 0.012387) * 0.04) * sin(tick * 0.000514382));
            v += (float)((cos((tick + (j + 1)*100) * 0.012387) * 0.04) * sin(tick * 0.000714282));
            glTexCoord2f(u, v);
            glVertex2f(i * (width / 16.0f), (j + 1) * (height / 16.0f));
        }
        // create some degenerate surfaces to "turn" the strip after each horizontal span
        glVertex2f(width, (j + 1) * (height / 16.0f));
        glVertex2f(0, (j + 2) * (height / 16.0f));
    }
    glEnd();

    //unbind the texture
    glBindTexture(GL_TEXTURE_2D, 0);

    //disable texturemapping
    glDisable(GL_TEXTURE_2D);

    return FF_SUCCESS;
}
