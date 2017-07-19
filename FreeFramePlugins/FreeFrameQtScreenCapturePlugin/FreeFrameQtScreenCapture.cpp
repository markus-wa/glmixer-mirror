
#include <GL/glew.h>
#include <QApplication>
#include <QDesktopWidget>
#include <QPixmap>
#include <QImage>

#include "FreeFrameQtScreenCapture.h"


#define FFPARAM_PORTION (0)

////////////////////////////////////////////////////////////////////////////////////////////////////
//  Plugin information
////////////////////////////////////////////////////////////////////////////////////////////////////

static CFFGLPluginInfo PluginInfo (
        FreeFrameQtScreenCapture::CreateInstance,	// Create method
        "GLSCQT",        					        // Plugin unique ID
        "ScreenCapture",			                // Plugin name
        1,						   			        // API major version number
        500,								        // API minor version number
        1,										    // Plugin major version number
        000,									    // Plugin minor version number
        FF_SOURCE,						            // Plugin type
        "Shows the content of the screen",	        // Plugin description
        "by Bruno Herbelin"                         // About
        );


////////////////////////////////////////////////////////////////////////////////////////////////////
//  Constructor and destructor
////////////////////////////////////////////////////////////////////////////////////////////////////

FreeFrameQtScreenCapture::FreeFrameQtScreenCapture()
    : CFreeFrameGLPlugin()
{
    // Input properties
    SetMinInputs(0);
    SetMaxInputs(0);
    SetTimeSupported(false);

    displayList = 0;
    textureIndex = 0;

    // Parameters
    SetParamInfo(FFPARAM_PORTION, "Portion", FF_TYPE_STANDARD, 1.0f);
    portion = 1.f;

    param_changed = true;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
//  Methods
////////////////////////////////////////////////////////////////////////////////////////////////////

#ifdef FF_FAIL
// FFGL 1.5
DWORD   FreeFrameQtScreenCapture::InitGL(const FFGLViewportStruct *vp)
#else
// FFGL 1.6
FFResult FreeFrameQtScreenCapture::InitGL(const FFGLViewportStruct *vp)
#endif
{
    glEnable(GL_TEXTURE);
    glGenTextures(1, &textureIndex);
    glBindTexture(GL_TEXTURE_2D, textureIndex);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

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

    return FF_SUCCESS;
}

#ifdef FF_FAIL
// FFGL 1.5
DWORD   FreeFrameQtScreenCapture::DeInitGL()
#else
// FFGL 1.6
FFResult FreeFrameQtScreenCapture::DeInitGL()
#endif
{

    if (textureIndex) glDeleteTextures(1, &textureIndex);
    if (displayList) glDeleteLists(displayList, 1);

    return FF_SUCCESS;
}

#ifdef FF_FAIL
// FFGL 1.5
DWORD	FreeFrameQtScreenCapture::ProcessOpenGL(ProcessOpenGLStruct* pGL)
#else
// FFGL 1.6
FFResult FreeFrameQtScreenCapture::ProcessOpenGL(ProcessOpenGLStruct *pGL)
#endif
{
    if (!pGL)
        return FF_FAIL;

    //enable texturemapping
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, textureIndex);

    // grab desktop (primary screen)
    QWidget *desktopWidget = QApplication::desktop()->screen();
    QRect desktopRect = QApplication::desktop()->availableGeometry();
    QImage image = QPixmap::grabWindow(desktopWidget->winId(), desktopRect.x(), desktopRect.y(), (int) ( (float) desktopRect.width() * portion), (int) ( (float) desktopRect.height() * portion)).toImage();

    // new value of the portion parameter
    if(param_changed) {
        // define texture according to new image size
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, image.width(), image.height(), 0,
                     GL_BGRA, GL_UNSIGNED_INT_8_8_8_8_REV,(GLvoid*) image.constBits());


        param_changed = false;
    }
    // otherwise fast call to texsubimage
    else
        glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, image.width(), image.height(),
                        GL_BGRA, GL_UNSIGNED_INT_8_8_8_8_REV,(GLvoid*) image.constBits());

    glCallList(displayList);

    //unbind the texture
    glBindTexture(GL_TEXTURE_2D, 0);

    //disable texturemapping
    glDisable(GL_TEXTURE_2D);

    return FF_SUCCESS;
}

#ifdef FF_FAIL
// FFGL 1.5
DWORD FreeFrameQtScreenCapture::SetParameter(const SetParameterStruct* pParam)
{
    if (pParam != NULL) {
        if (pParam->ParameterNumber == FFPARAM_PORTION) {
            portion = *((float *)(unsigned)&(pParam->NewParameterValue));
            param_changed = true;
            return FF_SUCCESS;
        }
    }
    return FF_FAIL;
}

DWORD FreeFrameQtScreenCapture::GetParameter(DWORD index)
{
    DWORD dwRet = 0;

    if (index == FFPARAM_PORTION) {
        *((float *)(unsigned)&dwRet) = portion;
        return dwRet;
    } else
        return FF_FAIL;
}

#else
// FFGL 1.6
FFResult FreeFrameQtScreenCapture::SetFloatParameter(unsigned int index, float value)
{
    if (index == FFPARAM_PORTION) {
        portion = value;
        param_changed = true;
        return FF_SUCCESS;
    }

    return FF_FAIL;
}

float FreeFrameQtScreenCapture::GetFloatParameter(unsigned int index)
{
    if (index == FFPARAM_PORTION)
        return portion;

    return 0.0;
}
#endif
