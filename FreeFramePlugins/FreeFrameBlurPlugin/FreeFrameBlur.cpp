
#include <GL/glew.h>
#define APIENTRY
#include <FFGL.h>
#include <FFGLLib.h>
#include <stdio.h>

#include "FreeFrameBlur.h"

#define FFPARAM_BLUR (0)

// texture coords interpolation via varying texc
const GLchar *vertexShaderCode =    "varying vec2 texc;"
                            "attribute vec2 texcoord2d;"
                            "void main(void)"
                            "{"
                            "gl_Position = ftransform();"
                            "texc = gl_MultiTexCoord0.st;"
                            "}";

// gaussian linear sampling blur
// inspired from http://rastergrid.com/blog/2010/09/efficient-gaussian-blur-with-linear-sampling/
const GLchar *fragmentShaderCode =  "varying vec2 texc;"
                            "uniform vec2 textureoffset;"
                            "uniform sampler2D texture;"
                            "void main(void)"
                            "{"
                            "vec3 sum = vec3(0.0);"
                            "sum += 0.2270270270  * texture2D(texture, texc ).rgb ;"
                            "sum += 0.3162162162  * texture2D(texture, texc + 1.3846153846 * textureoffset ).rgb ;"
                            "sum += 0.3162162162  * texture2D(texture, texc  -1.3846153846 * textureoffset ).rgb ;"
                            "sum += 0.0702702703  * texture2D(texture, texc + 3.2307692308 * textureoffset ).rgb ;"
                            "sum += 0.0702702703  * texture2D(texture, texc -3.2307692308 * textureoffset ).rgb ;"
                            "gl_FragColor = vec4(sum, 1.0);"
                            "}";


////////////////////////////////////////////////////////////////////////////////////////////////////
//  Plugin information
////////////////////////////////////////////////////////////////////////////////////////////////////

static CFFGLPluginInfo PluginInfo ( 
    FreeFrameBlur::CreateInstance,	// Create method
    "GLBLR",								// Plugin unique ID
    "FFGLBlur",			// Plugin name
	1,						   			// API major version number 													
    500,								  // API minor version number
	1,										// Plugin major version number
	000,									// Plugin minor version number
	FF_EFFECT,						// Plugin type
    "Blurs the display of the provided input",	 // Plugin description
    "by Bruno Herbelin"  // About
);


////////////////////////////////////////////////////////////////////////////////////////////////////
//  Constructor and destructor
////////////////////////////////////////////////////////////////////////////////////////////////////

FreeFrameBlur::FreeFrameBlur()
: CFreeFrameGLPlugin()
{
    // Input properties
    SetMinInputs(1);
    SetMaxInputs(1);
    SetTimeSupported(false);

    // Parameters
    SetParamInfo(FFPARAM_BLUR, "Blur", FF_TYPE_STANDARD, 0.7f);
    blur = 0.7;
    param_changed = true;
}

void printLog(GLuint obj)
{
    int infologLength = 0;
    char infoLog[1024];

    if (glIsShader(obj))
        glGetShaderInfoLog(obj, 1024, &infologLength, infoLog);
    else
        glGetProgramInfoLog(obj, 1024, &infologLength, infoLog);

    if (infologLength > 0)
        fprintf(stderr, "GLSL :: %s\n", infoLog);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
//  Methods
////////////////////////////////////////////////////////////////////////////////////////////////////
#ifdef FF_FAIL
// FFGL 1.5
DWORD   FreeFrameBlur::InitGL(const FFGLViewportStruct *vp)
#else
// FFGL 1.6
FFResult FreeFrameBlur::InitGL(const FFGLViewportStruct *vp)
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

    glewInit();
    if (GLEW_VERSION_2_0)
        fprintf(stderr, "INFO: OpenGL 2.0 supported, proceeding\n");
    else
    {
        fprintf(stderr, "INFO: OpenGL 2.0 not supported. Exit\n");
        return FF_FAIL;
    }

    vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &vertexShaderCode, NULL);
    glCompileShader(vertexShader);
    printLog(vertexShader);

    fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &fragmentShaderCode, NULL);
    glCompileShader(fragmentShader);
    printLog(fragmentShader);

    shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    glLinkProgram(shaderProgram);
    printLog(shaderProgram);

    uniform_textureoffset = glGetUniformLocation(shaderProgram, "textureoffset");

    return FF_SUCCESS;
}


#ifdef FF_FAIL
// FFGL 1.5
DWORD   FreeFrameBlur::DeInitGL()
#else
// FFGL 1.6
FFResult FreeFrameBlur::DeInitGL()
#endif
{

    fbo1.FreeResources(glExtensions);
    fbo2.FreeResources(glExtensions);

    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);
    glDeleteProgram(shaderProgram);

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
DWORD	FreeFrameBlur::ProcessOpenGL(ProcessOpenGLStruct* pGL)
#else
// FFGL 1.6
FFResult FreeFrameBlur::ProcessOpenGL(ProcessOpenGLStruct *pGL)
#endif
{
  // get the input texture
  if (pGL->numInputTextures<1)
    return FF_FAIL;

  if (pGL->inputTextures[0]==NULL)
    return FF_FAIL;
  
  FFGLTextureStruct &Texture = *(pGL->inputTextures[0]);

  //enable texturemapping
  glEnable(GL_TEXTURE_2D);

  // new value of the blur parameter
  if(param_changed) {

      glExtensions.glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0);

      fboViewport.x = 0;
      fboViewport.y = 0;
      fboViewport.width = (int)((double)viewport.width * (1.0 - blur) );
      fboViewport.height = (int)((double)viewport.height * (1.0 - blur) );

      // sanity check for size
      fboViewport.width = fboViewport.width < 1 ? 1 : fboViewport.width;
      fboViewport.height = fboViewport.height < 1 ? 1 : fboViewport.height;

      fbo1.FreeResources(glExtensions);
      if (! fbo1.Create( fboViewport.width, fboViewport.height, glExtensions) )
          return FF_FAIL;

      fbo2.FreeResources(glExtensions);
      if (! fbo2.Create( fboViewport.width, fboViewport.height, glExtensions) )
          return FF_FAIL;

      param_changed = false;
  }

  // no depth test
  glDisable(GL_DEPTH_TEST);

  if (blur >0)
    {
      // activate the fbo2 as our render target
      if (!fbo2.BindAsRenderTarget(glExtensions))
        return FF_FAIL;

      //render the original texture on a quad in fbo2
      drawQuad( fboViewport, Texture);

      // use the blurring shader program
      glUseProgram(shaderProgram);

      // PASS 1:  horizontal filter
      glUniform2f(uniform_textureoffset, 1.f / (float) fbo2.GetTextureInfo().HardwareWidth,  0.f);

      // activate the fbo1 as our render target
      if (!fbo1.BindAsRenderTarget(glExtensions))
        return FF_FAIL;

      //render the fbo2 texture on a quad in fbo1
      drawQuad( fboViewport, fbo2.GetTextureInfo());

      // PASS 2 : vertical
      glUniform2f(uniform_textureoffset, 0.f,  1.f / (float) fbo1.GetTextureInfo().HardwareHeight);

      // activate the fbo2 as our render target
      if (!fbo2.BindAsRenderTarget(glExtensions))
        return FF_FAIL;

      // render the fbo1 texture on a quad in fbo2
      drawQuad( fboViewport, fbo1.GetTextureInfo());

      // disable shader program
      glUseProgram(0);

      // (re)activate the HOST fbo as render target
      glExtensions.glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, pGL->HostFBO);

      // render the fbo2 texture texture on a quad in the host fbo
      drawQuad( viewport, fbo2.GetTextureInfo() );
    }
  else
  {
      // render the fbo2 texture texture on a quad in the host fbo
      drawQuad( viewport, Texture );
  }

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
        if (pParam != NULL && pParam->ParameterNumber == FFPARAM_BLUR) {
            blur = *((float *)(unsigned)&(pParam->NewParameterValue));
            param_changed = true;
            return FF_SUCCESS;
        }

        return FF_FAIL;
    }

    DWORD FreeFrameBlur::GetParameter(DWORD index)
    {
        DWORD dwRet;
        *((float *)(unsigned)&dwRet) = blur;

        if (index == FFPARAM_BLUR)
            return dwRet;
        else
            return FF_FAIL;
    }

#else
    // FFGL 1.6
    FFResult FreeFrameBlur::SetFloatParameter(unsigned int index, float value)
    {
        if (index == FFPARAM_BLUR) {
            blur = value;
            param_changed = true;
            return FF_SUCCESS;
        }

        return FF_FAIL;
    }

    float FreeFrameBlur::GetFloatParameter(unsigned int index)
    {
        if (index == FFPARAM_BLUR)
            return blur;

        return 0.0;
    }
#endif




