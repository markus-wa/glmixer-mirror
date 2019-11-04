#include <GL/glew.h>
#include "FreeFrameGrid.h"

#ifdef DEBUG
#include <cstdio>
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
#endif

static const GLchar *fragmentShaderCode =
        "#version 330 core \n"
        "uniform vec3      iResolution;\n"
        "out vec4          FragmentColor;\n"
        "void main(void)"
        "{"
        "    float size1 = 50.0;"
        "    float size2 = 10.0;"
        "    float col = mod(gl_FragCoord.x, size1) > size1 - 2.0 || mod(gl_FragCoord.y, size1) > size1 - 2.0 ? 0.5 :"
        "                mod(gl_FragCoord.x, size2) > size2 - 2.0 || mod(gl_FragCoord.y, size2) > size2 - 2.0 ? 0.2 : 0.0;"
        "    FragmentColor = vec4(vec3(col),1.0);"
        "}";

////////////////////////////////////////////////////////////////////////////////////////////////////
//  Plugin information
////////////////////////////////////////////////////////////////////////////////////////////////////

static CFFGLPluginInfo PluginInfo (
    FreeFrameGrid::CreateInstance,	// Create method
    "GLGRID",           // Plugin unique ID
    "FreeFrameGrid",    // Plugin name
    1,                  // API major version number
    500,                // API minor version number
    1,                  // Plugin major version number
    000,                // Plugin minor version number
    FF_SOURCE,          // Plugin type
    "Displays a test grid",	 // Plugin description
    "by Bruno Herbelin"  // About
);


////////////////////////////////////////////////////////////////////////////////////////////////////
//  Constructor and destructor
////////////////////////////////////////////////////////////////////////////////////////////////////

FreeFrameGrid::FreeFrameGrid()
: CFreeFrameGLPlugin()
{
    shaderProgram = 0;
    fragmentShader = 0;

    // Input properties
    SetMinInputs(0);
    SetMaxInputs(0);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
//  Methods
////////////////////////////////////////////////////////////////////////////////////////////////////
#ifdef FF_FAIL
// FFGL 1.5
DWORD   FreeFrameGrid::InitGL(const FFGLViewportStruct *vp)
#else
// FFGL 1.6
FFResult FreeFrameGrid::InitGL(const FFGLViewportStruct *vp)
#endif
{

    glewInit();
    if (!GLEW_VERSION_2_0)
    {
#ifdef DEBUG
        fprintf(stderr, "OpenGL 2.0 not supported. Exiting freeframe plugin.\n");
#endif
        return FF_FAIL;
    }

    fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &fragmentShaderCode, NULL);
    glCompileShader(fragmentShader);
#ifdef DEBUG
    printLog(fragmentShader);
#endif
    shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, fragmentShader);
    glLinkProgram(shaderProgram);
#ifdef DEBUG
    printLog(shaderProgram);
#endif

    uniform_viewportsize = glGetUniformLocation(shaderProgram, "iResolution");

    glUseProgram(shaderProgram);
    glUniform3f(uniform_viewportsize, vp->width, vp->height, 0.0);
    glUseProgram(0);

    return FF_SUCCESS;
}


#ifdef FF_FAIL
// FFGL 1.5
DWORD   FreeFrameGrid::DeInitGL()
#else
// FFGL 1.6
FFResult FreeFrameGrid::DeInitGL()
#endif
{
    if (fragmentShader) glDeleteShader(fragmentShader);
    if (shaderProgram)  glDeleteProgram(shaderProgram);

    return FF_SUCCESS;
}

#ifdef FF_FAIL
// FFGL 1.5
DWORD	FreeFrameGrid::ProcessOpenGL(ProcessOpenGLStruct* pGL)
#else
// FFGL 1.6
FFResult FreeFrameGrid::ProcessOpenGL(ProcessOpenGLStruct *pGL)
#endif
{

  glClearColor(0.f, 0.f, 0.f, 0.f);
  glClear(GL_COLOR_BUFFER_BIT);

  // use the blurring shader program
  glUseProgram(shaderProgram);

  glBegin(GL_QUADS);
  glVertex2f(-1,-1);
  glVertex2f(-1,1);
  glVertex2f(1,1);
  glVertex2f(1,-1);
  glEnd();

  // disable shader program
  glUseProgram(0);

  return FF_SUCCESS;
}
