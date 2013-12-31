
#include <GL/glew.h>
#define APIENTRY
#include <FFGL.h>
#include <FFGLLib.h>
#include <stdio.h>
#include <ctime>

#include "FreeFrameQtGLSL.h"
#include "GLSLCodeEditorWidget.h"


const char *fragmentShaderHeader =  "uniform vec3      iResolution;           // viewport resolution (in pixels)\n"
                                    "uniform float     iGlobalTime;           // shader playback time (in seconds)\n"
                                    "uniform float     iChannelTime[1];       // channel playback time (in seconds)\n"
                                    "uniform vec3      iChannelResolution[1]; // channel resolution (in pixels)\n"
                                    "uniform sampler2D iChannel0;             // input channel.\n"
                                    "uniform vec4      iDate;                 // (year, month, day, time in seconds)\n";

const char *fragmentShaderDefaultCode = "void main(void){\n"
        "\tvec2 uv = gl_FragCoord.xy / iChannelResolution[0].xy;\n"
        "\tgl_FragColor = texture2D(iChannel0, uv);\n"
        "}\0";

// http://citeseerx.ist.psu.edu/viewdoc/download?doi=10.1.1.165.6050&rep=rep1&type=pdf
//vec3 c=texture2D(texture, texc, 0.0).rgb;
//vec3 a=texture2D(texture, texc, 0.5).rgb;
//c/=2.0*a;
//c = pow(c, vec3(0.4545) );
//gl_FragColor= vec4(a, 1.0);


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
    : CFreeFrameGLPlugin(), w(NULL)
{
    // Input properties
    SetMinInputs(1);
    SetMaxInputs(1);
    SetTimeSupported(false);

    // No Parameters
    shaderProgram = 0;
    fragmentShader = 0;
    fragmentShaderCode = NULL;
    setFragmentProgramCode(fragmentShaderDefaultCode);

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


void FreeFrameQtGLSL::setFragmentProgramCode(const char *code)
{
    // free  previous string
    if (fragmentShaderCode)
        free(fragmentShaderCode);

    // allocate, fill and terminate string
    fragmentShaderCode = (char *) malloc(sizeof(char)*(strlen(code)+1));
    strncpy(fragmentShaderCode, code, strlen(code));
    fragmentShaderCode[strlen(code)] = '\0';

    // inform update that code has changed
    code_changed = true;
}

#ifdef FF_FAIL
// FFGL 1.5
DWORD   FreeFrameQtGLSL::InitGL(const FFGLViewportStruct *vp)
#else
// FFGL 1.6
FFResult FreeFrameQtGLSL::InitGL(const FFGLViewportStruct *vp)
#endif
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


    //init gl extensions
    glExtensions.Initialize();
    if (glExtensions.EXT_framebuffer_object==0)
        return FF_FAIL;

    if (!frameBufferObject.Create( viewport.width, viewport.height, glExtensions) )
        return FF_FAIL;

    w = new GLSLCodeEditorWidget(this);
    w->setCode(fragmentShaderCode);
    w->setHeader(fragmentShaderHeader);
    w->show();

    return FF_SUCCESS;
}


#ifdef FF_FAIL
// FFGL 1.5
DWORD   FreeFrameQtGLSL::DeInitGL()
#else
// FFGL 1.6
FFResult FreeFrameQtGLSL::DeInitGL()
#endif
{

    frameBufferObject.FreeResources(glExtensions);

    if(fragmentShader) glDeleteShader(fragmentShader);
    if(shaderProgram)  glDeleteProgram(shaderProgram);

    if (w) delete w;

    return FF_SUCCESS;
}

#ifdef FF_FAIL
// FFGL 1.5
DWORD   FreeFrameQtGLSL::SetTime(double time)
#else
// FFGL 1.6
FFResult FreeFrameQtGLSL::SetTime(double time)
#endif
{
    m_curTime = time;
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
DWORD	FreeFrameQtGLSL::ProcessOpenGL(ProcessOpenGLStruct* pGL)
#else
// FFGL 1.6
FFResult FreeFrameQtGLSL::ProcessOpenGL(ProcessOpenGLStruct *pGL)
#endif
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

        // disable shader program
        glUseProgram(0);

        int infologLength = 0;
        char infoLog[2048];

        if (shaderProgram) glDeleteProgram(shaderProgram);
        if (fragmentShader) glDeleteShader(fragmentShader);

        char *fsc = (char *) malloc(sizeof(char)*(strlen(fragmentShaderHeader)+strlen(fragmentShaderCode)+1));
        strcpy(fsc, fragmentShaderHeader);
        strcat(fsc, fragmentShaderCode);
        fsc[strlen(fsc)] = '\0';

        fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
        glShaderSource(fragmentShader, 1, &fsc, NULL);
        glCompileShader(fragmentShader);
        glGetShaderInfoLog(fragmentShader, 2048, &infologLength, infoLog);

        shaderProgram = glCreateProgram();
        glAttachShader(shaderProgram, fragmentShader);
        glLinkProgram(shaderProgram);
        glGetProgramInfoLog(shaderProgram, 2048, &infologLength, infoLog);


        // use the shader program
        glUseProgram(shaderProgram);

        uniform_texturesize = glGetUniformLocation(shaderProgram, "iChannelResolution[0]");
        glUniform3f(uniform_texturesize, viewport.width, viewport.height, 0.0);
        uniform_viewportsize = glGetUniformLocation(shaderProgram, "iResolution");
        glUniform3f(uniform_viewportsize, viewport.width, viewport.height, 0.0);
        uniform_time = glGetUniformLocation(shaderProgram, "iGlobalTime");
        uniform_channeltime = glGetUniformLocation(shaderProgram, "iChannelTime[0]");
        uniform_date = glGetUniformLocation(shaderProgram, "iDate");


        if(w)
            w->showLogs(infoLog);

        code_changed = false;
    }


    // use the shader program
    glUseProgram(shaderProgram);

    // set time uniforms
    glUniform1f(uniform_time, m_curTime);
    glUniform1f(uniform_channeltime, m_curTime);
    std::time_t now = std::time(0);
    std::tm *local = std::localtime(&now);
    glUniform4f(uniform_date, local->tm_year, local->tm_mon, local->tm_mday, local->tm_hour*24.0+local->tm_min*60.0+local->tm_sec);

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
