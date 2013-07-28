#ifndef FFGLMirror_H
#define FFGLMirror_H

#include <FFGLPluginSDK.h>

class FreeFrameTest : public CFreeFrameGLPlugin
{
public:
    FreeFrameTest();
    virtual ~FreeFrameTest() {}

	///////////////////////////////////////////////////
	// FreeFrame plugin methods
	///////////////////////////////////////////////////
	
	FFResult	ProcessOpenGL(ProcessOpenGLStruct* pGL);
    FFResult    SetTime(double time);

    FFResult    InitGL(const FFGLViewportStruct *vp);
    FFResult    DeInitGL();

	///////////////////////////////////////////////////
	// Factory method
	///////////////////////////////////////////////////

	static FFResult __stdcall CreateInstance(CFreeFrameGLPlugin **ppOutInstance)
    {
        *ppOutInstance = new FreeFrameTest();
        if (*ppOutInstance != NULL)
            return FF_SUCCESS;
        return FF_FAIL;
    }

protected:
    // Time
    double m_curTime;
};


#endif
