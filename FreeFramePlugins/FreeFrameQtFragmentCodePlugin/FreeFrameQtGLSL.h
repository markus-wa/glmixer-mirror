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

//#ifdef FF_FAIL
//    // FFGL 1.5
//    DWORD __stdcall setCode(char *code)
//#else
//    // FFGL 1.6
//    FFResult __stdcall setCode(char *code)
//#endif
//    {
//            return FF_SUCCESS;
//    }

    void setFragmentProgramCode(const char *code);

protected:
//    class GLSLCodeEditorWidget *w;

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

BOOL APIENTRY DllMain(HANDLE hModule, DWORD ul_reason_for_call, LPVOID lpReserved);

__declspec(dllexport) FFMixed __stdcall plugMain(FFUInt32 functionCode, FFMixed inputValue, FFInstanceID instanceID);
typedef __declspec(dllimport) FFMixed (__stdcall *FF_Main_FuncPtr)(FFUInt32, FFMixed, FFInstanceID);

#else

//linux and Mac OSX share these
//FFMixed plugMain(FFUInt32 functionCode, FFMixed inputValue, FFInstanceID instanceID);
//typedef FFMixed (*FF_Main_FuncPtr)(FFUInt32 funcCode, FFMixed inputVal, FFInstanceID instanceID);

void  plugTest(char *code, FreeFrameQtGLSL *instance){

    instance->setFragmentProgramCode(code);
}
//typedef void  (*FF_Test_FuncPtr)(void);


#endif

}

#endif
