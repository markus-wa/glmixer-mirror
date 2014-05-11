#ifndef FFGLQTGLSL_H
#define FFGLQTGLSL_H

#include <FFGLPluginSDK.h>
#include <FFGLFBO.h>

class FreeFrameShadertoy : public CFreeFrameGLPlugin
{
public:
    FreeFrameShadertoy();
    virtual ~FreeFrameShadertoy() {}

    ///////////////////////////////////////////////////
    // FreeFrame plugin methods
    ///////////////////////////////////////////////////

#ifdef FF_FAIL
    // FFGL 1.5
    DWORD	ProcessOpenGL(ProcessOpenGLStruct* pGL);
    DWORD   SetTime(double time);
    DWORD   InitGL(const FFGLViewportStruct *vp);
    DWORD   DeInitGL();
#else
    // FFGL 1.6
    FFResult    ProcessOpenGL(ProcessOpenGLStruct* pGL);
    FFResult    SetTime(double time);
    FFResult    InitGL(const FFGLViewportStruct *vp);
    FFResult    DeInitGL();
#endif

    ///////////////////////////////////////////////////
    // Factory method
    ///////////////////////////////////////////////////
#ifdef FF_FAIL
    // FFGL 1.5
    static DWORD __stdcall CreateInstance(CFreeFrameGLPlugin **ppOutInstance)
#else
    // FFGL 1.6
    static FFResult __stdcall CreateInstance(CFreeFrameGLPlugin **ppOutInstance)
#endif
    {
        *ppOutInstance = new FreeFrameShadertoy();
        if (*ppOutInstance != NULL)
            return FF_SUCCESS;
        return FF_FAIL;
    }

    void setFragmentProgramCode(const char *code);
    char *getFragmentProgramCode();
    char *getFragmentProgramLogs();

protected:

    FFGLViewportStruct viewport;
    FFGLExtensions glExtensions;
    FFGLFBO frameBufferObject;
    GLuint shaderProgram;
    GLuint fragmentShader;
    GLuint uniform_texturesize;
    GLuint uniform_viewportsize;
    GLuint uniform_time;
    GLuint uniform_channeltime;
    GLuint uniform_date;

    bool code_changed;
    char *fragmentShaderCode;

    // logging
    int infologLength;
    char infoLog[4096];

    // Time
    double m_curTime;
};



extern "C" {


#ifdef FF_FAIL  // FFGL 1.5

#ifdef _WIN32

__declspec(dllexport) bool __stdcall setString(unsigned int t, const char *string, DWORD instanceID);
__declspec(dllexport) char * __stdcall getString(unsigned int t, DWORD instanceID);


#else

bool setString(unsigned int t, const char *string, DWORD *instanceID);
char *getString(unsigned int t, DWORD instanceID);

#endif

#else

bool setString(unsigned int t, const char *string, FFInstanceID *instanceID);
char *getString(unsigned int t, FFInstanceID instanceID);


#endif


}

#endif
