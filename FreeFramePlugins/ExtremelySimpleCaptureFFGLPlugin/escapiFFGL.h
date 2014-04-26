#ifndef FFGLMirror_H
#define FFGLMirror_H

#include <FFGLPluginSDK.h>

#define FFPARAM_DEVICE 0

class escapiFreeFrameGL : public CFreeFrameGLPlugin
{
public:
    escapiFreeFrameGL();
    virtual ~escapiFreeFrameGL() {}

	///////////////////////////////////////////////////
	// FreeFrame plugin methods
	///////////////////////////////////////////////////

    // FFGL 1.5
    DWORD	ProcessOpenGL(ProcessOpenGLStruct* pGL);
    DWORD   SetTime(double time);
    DWORD   InitGL(const FFGLViewportStruct *vp);
    DWORD   DeInitGL();


	///////////////////////////////////////////////////
	// Factory method
	///////////////////////////////////////////////////

    // FFGL 1.5
    static DWORD __stdcall CreateInstance(CFreeFrameGLPlugin **ppOutInstance)
    {
        *ppOutInstance = new escapiFreeFrameGL();
        if (*ppOutInstance != NULL)
            return FF_SUCCESS;
        return FF_FAIL;
    }

protected:

    class escapiFreeFrameGLData *data;
};


#endif
