#include <FFGL.h>
#include <FFGLLib.h>

#include "FreeFrameQtGLSL.h"
#include "GLSLCodeEditorWidget.h"

////////////////////////////////////////////////////////////////////////////////////////////////////
//  Plugin information
////////////////////////////////////////////////////////////////////////////////////////////////////

static CFFGLPluginInfo PluginInfo ( 
    FreeFrameQtGLSL::CreateInstance,	// Create method
    "GLQTGLSL",								// Plugin unique ID
    "FFGLQtGLSL",			// Plugin name
	1,						   			// API major version number 													
    500,								  // API minor version number
	1,										// Plugin major version number
	000,									// Plugin minor version number
	FF_EFFECT,						// Plugin type
    "Sample plugin",	 // Plugin description
    "by Bruno Herbelin"  // About
);


////////////////////////////////////////////////////////////////////////////////////////////////////
//  Constructor and destructor
////////////////////////////////////////////////////////////////////////////////////////////////////

FreeFrameQtGLSL::FreeFrameQtGLSL()
: CFreeFrameGLPlugin()
{
	// Input properties
    SetMinInputs(1);
    SetMaxInputs(1);

    // No Parameters




}

////////////////////////////////////////////////////////////////////////////////////////////////////
//  Methods
////////////////////////////////////////////////////////////////////////////////////////////////////

FFResult FreeFrameQtGLSL::InitGL(const FFGLViewportStruct *vp)
{
    w = new GLSLCodeEditorWidget();

    w->show();

    return FF_SUCCESS;
}


FFResult FreeFrameQtGLSL::DeInitGL()
{

    w->hide();
    delete w;

  return FF_SUCCESS;
}


FFResult FreeFrameQtGLSL::ProcessOpenGL(ProcessOpenGLStruct *pGL)
{
//    if(!t->isRunning())
//        t->start();



 /* if (pGL->numInputTextures<1)
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
  glTexCoord2d( fabs(sin(m_curTime)), fabs(cos(m_curTime)));
  glVertex2f(-1,-1);

  //upper left
  glTexCoord2d(fabs(sin(m_curTime)), 1.0);
  glVertex2f(-1,1);

  //upper right
  glTexCoord2d(1.0, 1.0);
  glVertex2f(1,1);

  //lower right
  glTexCoord2d(1.0, fabs(cos(m_curTime)));
  glVertex2f(1,-1);
  glEnd();

  //unbind the texture
  glBindTexture(GL_TEXTURE_2D, 0);

  //disable texturemapping
  glDisable(GL_TEXTURE_2D);*/

  return FF_SUCCESS;
}
