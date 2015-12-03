#include <GL/glew.h>
#include <cstdio>
#include <ctime>

#include "FreeFrameFragmentCodePlugin.h"

const char *fragmentShaderHeader =  "uniform vec3      iResolution;           // viewport resolution (in pixels)\n"
                                    "uniform float     iGlobalTime;           // shader playback time (in seconds)\n"
                                    "uniform float     iChannelTime[1];       // channel playback time (in seconds)\n"
                                    "uniform vec4      iDate;                 // (year, month, day, time in seconds)\n"
                                    "uniform bool      key[10];               // numpad key pressed\0";

const char *fragmentShaderDefaultCode = "void mainImage( out vec4 fragColor, in vec2 fragCoord )\n{\n"
                                        "\tfragColor = vec4(1.0, 1.0, 1.0, 1.0);\n"
                                        "}\0";


////////////////////////////////////////////////////////////////////////////////////////////////////
//  Plugin information
////////////////////////////////////////////////////////////////////////////////////////////////////

static CFFGLPluginInfo PluginInfo (
        FreeFrameShadertoy::CreateInstance,	// Create method
        "STFFGLGLSLS",                       // Plugin unique ID
        "Shadertoy",                        // Plugin name
        1,						   			// API major version number
        500,								// API minor version number
        1,									// Plugin major version number
        000,								// Plugin minor version number
        FF_SOURCE,                          // Plugin type
        "Shadertoy GLSL Source",            // Plugin description
        "by Bruno Herbelin"                 // About
        );




////////////////////////////////////////////////////////////////////////////////////////////////////
//  Constructor and destructor
////////////////////////////////////////////////////////////////////////////////////////////////////

FreeFrameShadertoy::FreeFrameShadertoy()
    : CFreeFrameGLPlugin()
{
    // clean start
    textureFrameBufferObject.Handle = 0;
    frameBufferObject = 0;
    shaderProgram = 0;
    fragmentShader = 0;
    uniform_texturesize = 0;
    uniform_viewportsize = 0;
    uniform_time = 0;
    uniform_channeltime = 0;
    uniform_date = 0;
    uniform_keys = 0;
    m_curTime = 0.0;

    for (int k=0; k<10; ++k) keyboard[k] = 0;

    // Input properties
    SetMinInputs(0);
    SetMaxInputs(0);
    SetTimeSupported(true);

    // No Parameters
    code_changed = true;
    fragmentShaderCode = NULL;
    setFragmentProgramCode(fragmentShaderDefaultCode);

}


#include "FreeFrameFragmentCodePlugin.cpp"
