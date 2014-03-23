#ifndef FFGLQTGLSL_H
#define FFGLQTGLSL_H

#include <FFGLPluginSDK.h>
#include <FFGLFBO.h>


class FreeFrameQtGLSL : public CFreeFrameGLPlugin
{
public:
    FreeFrameQtGLSL();
    virtual ~FreeFrameQtGLSL() {}

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
        *ppOutInstance = new FreeFrameQtGLSL();
        if (*ppOutInstance != NULL)
            return FF_SUCCESS;
        return FF_FAIL;
    }

    void setFragmentProgramCode(const char *code);
    char *getFragmentProgramCode();

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

    // Time
    double m_curTime;
};



extern "C" {

#ifdef _WIN32

//BOOL APIENTRY DllMain(HANDLE hModule, DWORD ul_reason_for_call, LPVOID lpReserved);

//__declspec(dllexport) FFMixed __stdcall plugMain(FFUInt32 functionCode, FFMixed inputValue, FFInstanceID instanceID);
//typedef __declspec(dllimport) FFMixed (__stdcall *FF_Main_FuncPtr)(FFUInt32, FFMixed, FFInstanceID);

#else


void setFragmentProgramCode(const char *code, FFInstanceID *instanceID){

    // declare pPlugObj (pointer to this instance)
    // & typecast instanceid into pointer to a CFreeFrameGLPlugin
    FreeFrameQtGLSL* pPlugObj = (FreeFrameQtGLSL*) instanceID;

    if (pPlugObj)
        pPlugObj->setFragmentProgramCode(code);
}


char *getFragmentProgramCode(FFInstanceID instanceID){

    // declare pPlugObj (pointer to this instance)
    // & typecast instanceid into pointer to a CFreeFrameGLPlugin
    FreeFrameQtGLSL* pPlugObj = (FreeFrameQtGLSL*) instanceID;

    if (pPlugObj)
        return pPlugObj->getFragmentProgramCode();
    else
        return 0;
}


#endif

}

#endif
