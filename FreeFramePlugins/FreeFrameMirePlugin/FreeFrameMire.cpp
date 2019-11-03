#include <GL/glew.h>
#include "FreeFrameMire.h"

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

// (c) Anton Platonov <platosha@gmail.com>
// from https://www.shadertoy.com/view/lltGRl

const GLchar *fragmentShaderCode =
        "#version 330 core \n"
        "uniform vec3      iResolution;\n"
        "out vec4          FragmentColor;\n"       
        "const float TAU = 6.283185307179586;"
        "const float aRatio = 4.0 / 3.0;"
        "const vec2 cNumber = vec2(26., 20.);"
        "const vec2 cSize = vec2(15., 15.);"
        "vec3 colorBars( float x ) {"
        "    return step(.5, fract(vec3(1. - x) * vec3(2., 1., 4.)));"
        "}"
        "vec3 checkerboard( vec2 p ) {"
        "    return vec3(mod((p.x + p.y), 2.));"
        "}"
        "vec3 cellFrame( vec2 p, vec3 bg ){"
        "    return (cSize.x - p.x) <= 1. || (cSize.y - p.y) <= 1. ? vec3(.9) : bg;"
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
        "vec3 comb( float freq, float t ) {"
        "    return vec3((sin(freq * t * TAU) + 1.) * .45);"
        "}"
        "bool circle( vec2 pos, float r ) {"
        "    return length(pos) <= r;"
        "}"
        "vec3 ueit( vec2 uv ) {"
        "    uv = (uv - vec2(.5, .5)) * (576./600.) + vec2(.5, .5);"
        "    if (abs(uv.x - .5) > .5 || abs(uv.y - .5) > .5) return vec3(0.);"
        "    vec2 pcc = uv * cNumber;"
        "    vec2 ppc = pcc * cSize;"
        "    vec2 pc = floor(ppc);"
        "    float ht = uv.x * .8333 * 0.000064;"
        "    vec2 pcpc = mod(ppc, cSize);"
        "    vec2 cpc = mod(pc, cSize);"
        "    vec2 cc = floor(pcc);"
        "    vec2 iuv = (pcc - 1.) / (cNumber - 2.);"
        "    bool bc = circle(uv * cNumber - cNumber * .5, 8. - .5/15.);"
        "    bool sctl = circle(pcc - vec2( 4.,  3.), 2. - .5/15.);"
        "    bool sctr = circle(pcc - vec2(22.,  3.), 2. - .5/15.);"
        "    bool scbl = circle(pcc - vec2( 4., 17.), 2. - .5/15.);"
        "    bool scbr = circle(pcc - vec2(22., 17.), 2. - .5/15.);"
        "    return rectangle(cc, cNumber) ? ("
        "                rectangle(pc - cSize + 1., cSize * (cNumber - 2.0)) ? .9 * vec3(.9) :"
        "                checkerboard(cc)"
        "            ) :  .9 * ("
        "            sctl ? ("
        "                rectangle(cc - vec2(3., 2.), vec2(2.)) ? ("
        "                    cc.y == 1. && cpc.y == 14. ? comb(3e6, ht) :"
        "                    cc.y == 2. && cpc.y <= 8. ? comb(3e6, ht) :"
        "                    cc.y == 3. && cpc.y == 14. ? vec3(.9) :"
        "                    cc.y == 3. && cpc.y >= 5. ? comb(4e6, ht) :"
        "                    cellFrame(cpc, vec3(.5))"
        "                ) :"
        "                vec3(.9)"
        "            ) :   sctr ? ("
        "                rectangle(cc - vec2(21., 2.), vec2(2.)) ? ("
        "                    cc.y == 1. && cpc.y == 14. ? comb(3e6, ht) :"
        "                    cc.y == 2. && cpc.y <= 8. ? comb(3e6, ht) :"
        "                    cc.y == 3. && cpc.y == 14. ? vec3(.9) :"
        "                    cc.y == 3. && cpc.y >= 5. ? comb(4e6, ht) :"
        "                    cellFrame(cpc, vec3(.5))"
        "                ) :"
        "                vec3(.9)"
        "            ) :"
        "            scbl ? ("
        "                rectangle(cc - vec2(3., 16.), vec2(2.)) ? ("
        "                    cc.y == 15. && cpc.y == 14. ? comb(4e6, ht) :"
        "                    cc.y == 16. && cpc.y <= 8. ? comb(4e6, ht) :"
        "                    cc.y == 17. && cpc.y == 14. ? vec3(.9) :"
        "                    cc.y == 17. && cpc.y >= 5. ? comb(3e6, ht) :"
        "                    cellFrame(cpc, vec3(.5))"
        "                ) :"
        "                vec3(.9)"
        "            ) :"
        "            scbr ? ("
        "                rectangle(cc - vec2(21., 16.), vec2(2.)) ? ("
        "                    cc.y == 15. && cpc.y == 14. ? comb(4e6, ht) :"
        "                    cc.y == 16. && cpc.y <= 8. ? comb(4e6, ht) :"
        "                    cc.y == 17. && cpc.y == 14. ? vec3(.9) :"
        "                    cc.y == 17. && cpc.y >= 5. ? comb(3e6, ht) :"
        "                    cellFrame(cpc, vec3(.5))"
        "                ) :"
        "                vec3(.9)"
        "            ) : ("
        "                ((cc.y == 2. || cc.y == 16.) && cpc.y >= 7.) ||"
        "                ((cc.y == 3. || cc.y == 17.) && cpc.y <= 6.)"
        "            ) && abs(cc.x - 12.5) <= 2. ? cellFrame(cpc, vec3(.5)) :"
        "            cc.y == 5. || cc.y == 6. ? .5 * colorBars(iuv.x) + .4 :"
        "            cc.y == 7. ?"
        "            	(cc.x >= 2. && cc.x <= 21. ? vec3(floor(1. + (cc.x - 2.) / 2.) / 10.) : vec3(0.)) :"
        "    		cc.y == 12. ? ("
        "                ("
        "                    pc.x >= 21. * cSize.x + 7. ||"
        "                	cc.x <= 4. && pc.x >= 1. * cSize.x + 6. "
        "                ) ? comb(2e6, ht) :"
        "                ("
        "                    cc.x <= 20. && pc.x >= 18. * cSize.x + 6. ||"
        "                	cc.x <= 7. && pc.x >= 5. * cSize.x + 6. "
        "                ) ? comb(3e6, ht) :"
        "                ("
        "                    cc.x <= 17. && pc.x >= 15. * cSize.x + 6. ||"
        "                	cc.x <= 10. && pc.x >= 8. * cSize.x + 6. "
        "                ) ? comb(4e6, ht) :"
        "                cc.x <= 14. && pc.x >= 11. * cSize.x + 6. ?"
        "                	comb(5e6, ht) :"
        "                vec3(.5)"
        "            ) :"
        "            cc.y == 13. || cc.y == 14. ? .9 * colorBars(iuv.x) :"
        "            cc.y == 15. ? ("
        "                cpc.y == 0. || cpc.y == 14. ? vec3(.9) :"
        "                (cc.x <= 6. || cc.x >= 19.) ? ("
        "                	abs(cc.x - 3.5) <= 2. || abs(cc.x - 21.5) <= 2. ? vec3(.0) :"
        "                	vec3(.9)"
        "                ) :"
        "                bc ? vec3(.9 * mod(cc.x, 2.)) :"
        "                vec3(.9 * mod(1. + cc.x, 2.))"
        "            ) :"
        "    		bc ? ("
        "                cc.y == 8. ? ("
        "                    cpc.y == 14. ? vec3(.9) :"
        "                    cc.x <= 9. ? (vec3(.4, .9, .4) + step(7.5, pcpc.x) * vec3(.5, -.5, .5)) :"
        "                    cc.x <= 15. ? (vec3(.4, .4, .9) + step(7.5, pcpc.x) * vec3(.5, .5, -.5)) :"
        "                    (vec3(.4, .9, .9) + step(7.5, pcpc.x) * vec3(.5, -.5, -.5))"
        "                ) :"
        "                cc.y == 9. ? ("
        "                    cc.x == 5. && cpc.x == 8. ? vec3(.0) :"
        "                    cpc.y == 14. && cpc.x == 14. && mod(cc.x - 5., 2.) == 1. ? vec3(.9) :"
        "                    cc.x <= 9. ? (cpc.y == 14. ? vec3(.0) : vec3(.9)) :"
        "                    cc.x >= 16. ? ("
        "                        cpc.y < 14. && (abs(pcpc.y - 14.0 + (pcc.x - 16.5) * (15./4.5)) < .25) ? vec3(.9) :"
        "                        vec3(.0)"
        "                    ) :"
        "                    cc.x == 11. && cpc.x == 14. ? vec3(.9) :"
        "                    cc.x >= 12. && cc.x <= 13. ? cellFrame(cpc, vec3(.5)) :"
        "                	vec3(.5)"
        "                ) :"
        "                cc.y == 10. ? ("
        "                    cc.x == 5. && cpc.x == 8. ? vec3(.9) :"
        "                    cpc.y == 14. ? vec3(.9) :"
        "                    cc.x <= 9. ? (cpc.y == 14. ? vec3(.9) : ("
        "                        cpc.y < 14. && (abs(pcpc.y - 14.0 + (pcc.x - 5.75) * (15./4.5)) < .25) ? vec3(.9) :"
        "                        vec3(.0)"
        "                    )) :"
        "                    cc.x >= 16. ? vec3(.9) :"
        "                    cc.x == 11. && cpc.x == 14. ? vec3(.9) :"
        "                    cc.x >= 12. && cc.x <= 13. ? cellFrame(cpc, vec3(.5)) :"
        "                    vec3(.5)"
        "                ) :"
        "                cc.y == 11. ? mix(vec3(0., 1., 0.), vec3(1., 0., 1.), (pcc.x - 5.) / 16.) : "
        "                vec3(.9)"
        "            ) :"
        "            cellFrame(cpc, vec3(.5))"
        "        );"
        "}"
        "void main(void)"
        "{"
        "    float scale = min(iResolution.x, iResolution.y * aRatio);"
        "    vec2 uv = vec2(.5, .5) + (gl_FragCoord.xy - iResolution.xy * .5) * vec2(1., aRatio) / scale;"
        "    uv.y = 1. - uv.y;"
        "	 FragmentColor = vec4( ueit(uv), 1. );"
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
DWORD   FreeFrameTest::InitGL(const FFGLViewportStruct *vp)
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
    uniform_time = glGetUniformLocation(shaderProgram, "time");

    glUseProgram(shaderProgram);
    glUniform3f(uniform_viewportsize, vp->width, vp->height, 0.0);
    glUniform1f(uniform_time, 100.0);
    glUseProgram(0);

    return FF_SUCCESS;
}


#ifdef FF_FAIL
// FFGL 1.5
DWORD   FreeFrameTest::DeInitGL()
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
DWORD   FreeFrameTest::SetTime(double time)
#else
// FFGL 1.6
FFResult FreeFrameMire::SetTime(double time)
#endif
{
  m_curTime = time;
  return FF_SUCCESS;
}

#ifdef FF_FAIL
// FFGL 1.5
DWORD	FreeFrameTest::ProcessOpenGL(ProcessOpenGLStruct* pGL)
#else
// FFGL 1.6
FFResult FreeFrameMire::ProcessOpenGL(ProcessOpenGLStruct *pGL)
#endif
{

  glClearColor(0.f, 0.f, 0.f, 0.f);
  glClear(GL_COLOR_BUFFER_BIT);

  // use the blurring shader program
  glUseProgram(shaderProgram);

  glUniform1f(uniform_time, m_curTime);

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
