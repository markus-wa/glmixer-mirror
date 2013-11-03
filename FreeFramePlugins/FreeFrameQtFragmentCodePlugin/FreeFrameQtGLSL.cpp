
#include <GL/glew.h>
#define APIENTRY
#include <FFGL.h>
#include <FFGLLib.h>
#include <stdio.h>

#include "FreeFrameQtGLSL.h"
#include "GLSLCodeEditorWidget.h"


// texture coords interpolation via varying texc
const GLchar *vertexShaderCode =    "varying vec2 texc;"
                            "attribute vec2 texcoord2d;"
                            "void main(void)"
                            "{"
                            "gl_Position = ftransform();"
                            "texc = gl_MultiTexCoord0.st;"
                            "}";

char *fragmentShaderDefaultCode =  "varying vec2 texc;\n"
                            "uniform vec2 texturesize;\n"
                            "uniform sampler2D texture;\n"
                            "void main(void){\n"
                            "gl_FragColor = texture2D(texture, texc);\n"
                            "}";

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
    SetTimeSupported(false);

    // No Parameters
    shaderProgram = 0;
    vertexShader = 0;
    fragmentShader = 0;
    fragmentShaderCode = NULL;

    code_changed = true;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
//  Methods
////////////////////////////////////////////////////////////////////////////////////////////////////

void printLog(GLuint obj)
{
    int infologLength = 0;
    char infoLog[2048];

    if (glIsShader(obj))
        glGetShaderInfoLog(obj, 2048, &infologLength, infoLog);
    else
        glGetProgramInfoLog(obj, 2048, &infologLength, infoLog);

    if (infologLength > 0)
        fprintf(stderr, "GLSL :: %s\n", infoLog);
}


void FreeFrameQtGLSL::setFragmentProgramCode(char *code)
{

    code_changed = true;

    if (shaderProgram) glDeleteProgram(shaderProgram);
    if (vertexShader) glDeleteShader(vertexShader);
    if (fragmentShader) glDeleteShader(fragmentShader);

    shaderProgram = 0;
    vertexShader = 0;
    fragmentShader = 0;

    if (fragmentShaderCode)
        free(fragmentShaderCode);

    fragmentShaderCode = (char *) malloc(sizeof(char)*(strlen(code)+1));
    strncpy(fragmentShaderCode, code, strlen(code));

}

FFResult FreeFrameQtGLSL::InitGL(const FFGLViewportStruct *vp)
{
    viewport.x = 0;
    viewport.y = 0;
    viewport.width = vp->width;
    viewport.height = vp->height;


    glewInit();
    if (GLEW_VERSION_2_0)
        fprintf(stderr, "INFO: OpenGL 2.0 supported, proceeding\n");
    else
    {
        fprintf(stderr, "INFO: OpenGL 2.0 not supported. Exit\n");
        return FF_FAIL;
    }

    setFragmentProgramCode(fragmentShaderDefaultCode);

    if (shaderProgram) {
        uniform_texturesize = glGetUniformLocation(shaderProgram, "texturesize");
        glUniform2f(uniform_texturesize, viewport.width, viewport.height);
    }
    //init gl extensions
    glExtensions.Initialize();
    if (glExtensions.EXT_framebuffer_object==0)
      return FF_FAIL;

   if (!frameBufferObject.Create( viewport.width, viewport.height, glExtensions) )
       return FF_FAIL;

    w = new GLSLCodeEditorWidget(this);
    w->show();

    return FF_SUCCESS;
}


FFResult FreeFrameQtGLSL::DeInitGL()
{

    frameBufferObject.FreeResources(glExtensions);

    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);
    glDeleteProgram(shaderProgram);


    w->hide();
//    qApp->processEvents();
    delete w;


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


FFResult FreeFrameQtGLSL::ProcessOpenGL(ProcessOpenGLStruct *pGL)
{

  if (pGL->numInputTextures<1)
    return FF_FAIL;

  if (pGL->inputTextures[0]==NULL)
    return FF_FAIL;
  
  FFGLTextureStruct &Texture = *(pGL->inputTextures[0]);

  //enable texturemapping
  glEnable(GL_TEXTURE_2D);

  // no depth test
  glDisable(GL_DEPTH_TEST);

  // new code
  if(code_changed) {
      int infologLength = 0;
      char infoLog[2048];

      vertexShader = glCreateShader(GL_VERTEX_SHADER);
      glShaderSource(vertexShader, 1, &vertexShaderCode, NULL);
      glCompileShader(vertexShader);

      fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
      glShaderSource(fragmentShader, 1, &fragmentShaderCode, NULL);
      glCompileShader(fragmentShader);
      glGetShaderInfoLog(fragmentShader, 2048, &infologLength, infoLog);
      w->showLogs(infoLog);

      shaderProgram = glCreateProgram();
      glAttachShader(shaderProgram, vertexShader);
      glAttachShader(shaderProgram, fragmentShader);
      glLinkProgram(shaderProgram);
      glGetProgramInfoLog(shaderProgram, 2048, &infologLength, infoLog);
      w->showLogs(infoLog);

      code_changed = false;
  }


  // use the blurring shader program
  glUseProgram(shaderProgram);

  // activate the fbo2 as our render target
  if (!frameBufferObject.BindAsRenderTarget(glExtensions))
    return FF_FAIL;

  //render the original texture on a quad in fbo
  drawQuad( viewport, Texture);

  // disable shader program
  glUseProgram(0);

  // (re)activate the HOST fbo as render target
  glExtensions.glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, pGL->HostFBO);

  // render the fbo2 texture texture on a quad in the host fbo
  drawQuad( viewport, frameBufferObject.GetTextureInfo() );

  //unbind the texture
  glBindTexture(GL_TEXTURE_2D, 0);

  //disable texturemapping
  glDisable(GL_TEXTURE_2D);

  return FF_SUCCESS;
}
