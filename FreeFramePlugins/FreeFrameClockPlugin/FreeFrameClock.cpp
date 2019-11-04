#include <GL/glew.h>
#include "FreeFrameClock.h"

#include <cmath>


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
        "#define PI  3.14159265359\n"
        "#define EPS 0.01\n"
        "uniform float     time;\n"
        "uniform int       fps;\n"
        "uniform vec3      iResolution;\n"
        "out vec4          FragmentColor;\n"
        "float df_disk(in vec2 p, in vec2 c, in float r) {"
        "    return clamp(length(p - c) - r, 0., 1.);"
        "}"
        "float df_circ(in vec2 p, in vec2 c, in float r) {"
        "    return abs(r - length(p - c));"
        "}"
        "float df_line(in vec2 p, in vec2 a, in vec2 b) {"
        "    vec2 pa = p - a, ba = b - a;"
        "    float h = clamp(dot(pa,ba) / dot(ba,ba), 0., 1.);"
        "    return length(pa - ba * h);"
        "}"
        "float sharpen(in float d, in float w) {"
        "    float e = 1. / min(iResolution.y , iResolution.x);"
        "    return 1. - smoothstep(-e, e, d - w);"
        "}"
        "vec2 rotate(in vec2 p, in float t) {"
        "    t = t * 2. * PI;"
        "    return vec2(p.x * cos(t) - p.y * sin(t),"
        "                p.y * cos(t) + p.x * sin(t));"
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
        "    float tmin = mod(time, 3600.) / 60.;"
        "    float tsec = mod(mod(time, 3600.), 60.);"
        "    float tms  = tsec - floor(tsec);"
        "    tsec = floor(tsec);"
        "    vec2 uv = gl_FragCoord.xy / iResolution.xy * 2.0 - 1.0;"
        "    uv.x *= iResolution.x / iResolution.y;"
        "    vec2 c = vec2(0), u = vec2(0,-1);"
        "    float c1 = sharpen(df_circ(uv, c, .90), EPS * 1.5);"
        "    float c2 = sharpen(df_circ(uv, c, .04), EPS * 0.5);"
        "    float d1 = sharpen(df_disk(uv, c, .01), EPS * 1.5);"
        "    float ls = sharpen(df_line(uv, c, rotate(u, tsec / 60.) * .78), EPS * 0.5);"
        "    float lm = sharpen(df_line(uv, c, rotate(u, tmin / 60.) * .65), EPS * 1.2);"
        "    float lms = sharpen(df_line(uv, rotate(u, tms) * .8, rotate(u, tms) * .85), EPS * 0.2);"
        "    vec3 col = vec3( max(lms, max(lm, max(ls, max(max(c1, c2), d1) ))) );"
        "    col += vec3( PrintInt( (uv + vec2(-0.6,0.8))*vec2(10.0, -10.0), fps ) );"
        "    \nFragmentColor = vec4(col, 1.0);"
        "}";


////////////////////////////////////////////////////////////////////////////////////////////////////
//  Plugin information
////////////////////////////////////////////////////////////////////////////////////////////////////

static CFFGLPluginInfo PluginInfo (
    FreeFrameClock::CreateInstance,	// Create method
    "GLCLOCK",             // Plugin unique ID
    "FreeFrameClock",    // Plugin name
    1,                  // API major version number
    500,                // API minor version number
    1,                  // Plugin major version number
    000,                // Plugin minor version number
    FF_SOURCE,          // Plugin type
    "Displays a clock",	 // Plugin description
    "by Bruno Herbelin"  // About
);


////////////////////////////////////////////////////////////////////////////////////////////////////
//  Constructor and destructor
////////////////////////////////////////////////////////////////////////////////////////////////////

FreeFrameClock::FreeFrameClock()
: CFreeFrameGLPlugin()
{
    shaderProgram = 0;
    fragmentShader = 0;
    m_fps = 10.0;
    m_curTime = 0.0;

    // Input properties
    SetMinInputs(0);
    SetMaxInputs(0);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
//  Methods
////////////////////////////////////////////////////////////////////////////////////////////////////
#ifdef FF_FAIL
// FFGL 1.5
DWORD   FreeFrameTest::InitGL(const FFGLViewportStruct *vp)
#else
// FFGL 1.6
FFResult FreeFrameClock::InitGL(const FFGLViewportStruct *vp)
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
    uniform_time = glGetUniformLocation(shaderProgram, "time");
    uniform_fps = glGetUniformLocation(shaderProgram, "fps");

    glUseProgram(shaderProgram);
    glUniform3f(uniform_viewportsize, vp->width, vp->height, 0.0);
    glUniform1f(uniform_time, 0.0);
    glUniform1i(uniform_fps, 0);
    glUseProgram(0);

    return FF_SUCCESS;
}


#ifdef FF_FAIL
// FFGL 1.5
DWORD   FreeFrameTest::DeInitGL()
#else
// FFGL 1.6
FFResult FreeFrameClock::DeInitGL()
#endif
{
    if (fragmentShader) glDeleteShader(fragmentShader);
    if (shaderProgram)  glDeleteProgram(shaderProgram);

    return FF_SUCCESS;
}

#ifdef FF_FAIL
// FFGL 1.5
DWORD   FreeFrameTest::SetTime(double time)
#else
// FFGL 1.6
FFResult FreeFrameClock::SetTime(double time)
#endif
{
    if (time > m_curTime)
        m_fps = 0.4 * (1.0 / (time - m_curTime)) + 0.6 * m_fps;
    m_curTime = time;
    return FF_SUCCESS;
}

#ifdef FF_FAIL
// FFGL 1.5
DWORD	FreeFrameTest::ProcessOpenGL(ProcessOpenGLStruct* pGL)
#else
// FFGL 1.6
FFResult FreeFrameClock::ProcessOpenGL(ProcessOpenGLStruct *pGL)
#endif
{

  glClearColor(0.f, 0.f, 0.f, 0.f);
  glClear(GL_COLOR_BUFFER_BIT);

  // use the shader program
  glUseProgram(shaderProgram);

  glUniform1f(uniform_time, m_curTime);
  glUniform1i(uniform_fps, int( m_fps ) );

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
