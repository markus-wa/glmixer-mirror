#include <GL/glew.h>
#include "FreeFrameMire.h"

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
        "vec3 colorBars( float x ) {"
        "    return step(.5, fract(vec3(1. - x) * vec3(2., 1., 4.)));"
        "}"
        "vec3 checkerboard( vec2 p ) {"
        "    return vec3(mod((p.x + p.y), 2.));"
        "}"
        "bool rectangle( vec2 p, vec2 size ) {"
        "    return 0. <= p.x &&"
        "        0. <= p.y &&"
        "        p.x < size.x &&"
        "        p.y < size.y && ("
        "            p.x < 1. ||"
        "            p.y < 1. ||"
        "            (size.x - 1.) <= p.x ||"
        "            (size.y - 1.) <= p.y"
        "        );"
        "}"
        "bool circle( vec2 pos, float r ) {"
        "    return length(pos) <= r;"
        "}"
        "float SampleDigit(const in float n, const in vec2 vUV) {	"
        "            if(vUV.x  < 0.0) return 0.0;"
        "            if(vUV.y  < 0.0) return 0.0;"
        "            if(vUV.x >= 1.0) return 0.0;"
        "            if(vUV.y >= 1.0) return 0.0;"
        "            float data = 0.0;"
        "                 if(n < 0.5) data = 7.0 + 5.0*16.0 + 5.0*256.0 + 5.0*4096.0 + 7.0*65536.0;"
        "            else if(n < 1.5) data = 2.0 + 2.0*16.0 + 2.0*256.0 + 2.0*4096.0 + 2.0*65536.0;"
        "            else if(n < 2.5) data = 7.0 + 1.0*16.0 + 7.0*256.0 + 4.0*4096.0 + 7.0*65536.0;"
        "            else if(n < 3.5) data = 7.0 + 4.0*16.0 + 7.0*256.0 + 4.0*4096.0 + 7.0*65536.0;"
        "            else if(n < 4.5) data = 4.0 + 7.0*16.0 + 5.0*256.0 + 1.0*4096.0 + 1.0*65536.0;"
        "            else if(n < 5.5) data = 7.0 + 4.0*16.0 + 7.0*256.0 + 1.0*4096.0 + 7.0*65536.0;"
        "            else if(n < 6.5) data = 7.0 + 5.0*16.0 + 7.0*256.0 + 1.0*4096.0 + 7.0*65536.0;"
        "            else if(n < 7.5) data = 4.0 + 4.0*16.0 + 4.0*256.0 + 4.0*4096.0 + 7.0*65536.0;"
        "            else if(n < 8.5) data = 7.0 + 5.0*16.0 + 7.0*256.0 + 5.0*4096.0 + 7.0*65536.0;"
        "            else if(n < 9.5) data = 7.0 + 4.0*16.0 + 7.0*256.0 + 5.0*4096.0 + 7.0*65536.0;"
        "            vec2 vPixel = floor(vUV * vec2(4.0, 5.0));"
        "            float fIndex = vPixel.x + (vPixel.y * 4.0);"
        "            return mod(floor(data / pow(2.0, fIndex)), 2.0);"
        "}"
        "float PrintInt(const in vec2 uv, const in float value ) {"
        "            float res = 0.0;"
        "            float maxDigits = 1.0+ceil(log2(value)/log2(10.0));"
        "            float digitID = floor(uv.x);"
        "            if( digitID>0.0 && digitID<maxDigits ) {"
        "                float digitVa = mod( floor( value/pow(10.0,maxDigits-1.0-digitID) ), 10.0 );"
        "                res = SampleDigit( digitVa, vec2(fract(uv.x), uv.y) );"
        "            }"
        "            return res;"
        "}"
        "void main(void)"
        "{"
        "        vec2 uv = gl_FragCoord.xy / iResolution.xy;"
        "        vec2 ar = vec2 ( 1.0, iResolution.y / iResolution.x); "
        "        vec2 Pos = floor(gl_FragCoord.xy / 2.0);"
        "        float greybars = floor(uv.x * 16.) * 17./255.;"
        "        bool bc = circle( (vec2(0.5, 0.5) - uv )* ar , 0.2);"
        "        vec3 color = bc ? vec3( mod(Pos.x + mod(Pos.y, 2.0), 2.0) ) : vec3( greybars);"
        "        color = abs(uv.y - 0.5) < 0.1 ? colorBars(-uv.x) : color;"
        "        float text = PrintInt( (uv-vec2(0.5,0.9))*vec2(10.0, -10.0), iResolution.y );"
        "        text += PrintInt( (uv-vec2(0.0,0.9))*vec2(10.0, -10.0), iResolution.x );"
        "        color += vec3(text);"
        "        float s = 10.0;"
        "        Pos = floor(gl_FragCoord.xy / 10.0);"
        "        float checker = mod(Pos.x + mod(Pos.y, 2.0), 2.0);   "
        "        FragmentColor = vec4( rectangle(Pos, iResolution.xy / s) ? vec3(checker) : color, 1.0);"
        "}";

////////////////////////////////////////////////////////////////////////////////////////////////////
//  Plugin information
////////////////////////////////////////////////////////////////////////////////////////////////////

static CFFGLPluginInfo PluginInfo (
    FreeFrameMire::CreateInstance,	// Create method
    "GLMIRE",             // Plugin unique ID
    "FreeFrameMire",    // Plugin name
    1,                  // API major version number
    500,                // API minor version number
    1,                  // Plugin major version number
    000,                // Plugin minor version number
    FF_SOURCE,          // Plugin type
    "Displays a test pattern",	 // Plugin description
    "by Bruno Herbelin"  // About
);


////////////////////////////////////////////////////////////////////////////////////////////////////
//  Constructor and destructor
////////////////////////////////////////////////////////////////////////////////////////////////////

FreeFrameMire::FreeFrameMire()
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
DWORD   FreeFrameMire::InitGL(const FFGLViewportStruct *vp)
#else
// FFGL 1.6
FFResult FreeFrameMire::InitGL(const FFGLViewportStruct *vp)
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
DWORD   FreeFrameMire::DeInitGL()
#else
// FFGL 1.6
FFResult FreeFrameMire::DeInitGL()
#endif
{
    if (fragmentShader) glDeleteShader(fragmentShader);
    if (shaderProgram)  glDeleteProgram(shaderProgram);

    return FF_SUCCESS;
}

#ifdef FF_FAIL
// FFGL 1.5
DWORD	FreeFrameMire::ProcessOpenGL(ProcessOpenGLStruct* pGL)
#else
// FFGL 1.6
FFResult FreeFrameMire::ProcessOpenGL(ProcessOpenGLStruct *pGL)
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
