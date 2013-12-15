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

    void setFragmentProgramCode(char *code);

protected:
    class GLSLCodeEditorWidget *w;


    bool code_changed;
    FFGLViewportStruct viewport;
    FFGLExtensions glExtensions;
    FFGLFBO frameBufferObject;
    GLuint shaderProgram;
    GLuint vertexShader;
    GLuint fragmentShader;
    GLuint uniform_texturesize;
    char *fragmentShaderCode;
    // Time
    double m_curTime;
};


#endif
