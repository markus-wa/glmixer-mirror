#include <GL/glew.h>
#define APIENTRY
#include <FFGL.h>
#include <FFGLLib.h>
#include <cstdio>
#include <ctime>

#include "FreeFrameFragmentCodePlugin.h"


const char *fragmentShaderHeader =  "uniform vec3      iResolution;           // viewport resolution (in pixels)\n"
                                    "uniform vec3      iChannelResolution[1]; // input channel resolution (in pixels)\n"
                                    "uniform float     iGlobalTime;           // shader playback time (in seconds)\n"
                                    "uniform float     iChannelTime[1];       // channel playback time (in seconds)\n"
                                    "uniform sampler2D iChannel0;             // input channel (texture id).\n"
                                    "uniform vec4      iDate;                 // (year, month, day, time in seconds)\0";

const char *fragmentShaderDefaultCode = "void main(void){\n"
        "\tvec2 uv = gl_FragCoord.xy / iChannelResolution[0].xy;\n"
        "\tgl_FragColor = texture2D(iChannel0, uv);\n"
        "}\0";

////////////////////////////////////////////////////////////////////////////////////////////////////
//  Plugin information
////////////////////////////////////////////////////////////////////////////////////////////////////

static CFFGLPluginInfo PluginInfo (
        FreeFrameShadertoy::CreateInstance,	// Create method
        "STFFGLGLSLE",                       // Plugin unique ID
        "Shadertoy",                        // Plugin name
        1,						   			// API major version number
        500,								// API minor version number
        1,									// Plugin major version number
        000,								// Plugin minor version number
        FF_EFFECT,                          // Plugin type
        "Shadertoy GLSL Effect",            // Plugin description
        "by Bruno Herbelin"                 // About
        );




////////////////////////////////////////////////////////////////////////////////////////////////////
//  Constructor and destructor
////////////////////////////////////////////////////////////////////////////////////////////////////

FreeFrameShadertoy::FreeFrameShadertoy()
    : CFreeFrameGLPlugin()
{
    // Input properties
    SetMinInputs(1);
    SetMaxInputs(1);
    SetTimeSupported(true);

    // No Parameters
    code_changed = true;
    shaderProgram = 0;
    fragmentShader = 0;
    fragmentShaderCode = NULL;
    setFragmentProgramCode(fragmentShaderDefaultCode);

}



#include "FreeFrameFragmentCodePlugin.cpp"
