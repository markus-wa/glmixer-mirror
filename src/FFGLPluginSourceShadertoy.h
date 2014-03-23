#ifndef FFGLPLUGINSOURCESHADERTOY_H
#define FFGLPLUGINSOURCESHADERTOY_H

#include "FFGLPluginSource.h"

class FFGLPluginSourceShadertoy : public FFGLPluginSource
{
public:
    FFGLPluginSourceShadertoy(int w, int h, FFGLTextureStruct inputTexture);

    // Run-Time Type Information
    RTTI rtti() const { return FFGLPluginSourceShadertoy::type; }

    // get and set of GLSL Shadertoy code
    QString getCode();
    void setCode(QString code);

private:

    static QString libraryFileName();
    static RTTI type;
};

#endif // FFGLPLUGINSOURCEFACTORY_H
