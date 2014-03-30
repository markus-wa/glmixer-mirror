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
    QString getDefaultCode();
    void setCode(QString code);

    // get logs of GLSL Shadertoy execution
    QString getLogs();
    QString getHeaders();

    QString getName();
    void setName(QString);

//    QString getAbout();
//    void setAbout();

private:

    QString _name, _about, _description;

    static QString libraryFileName();
    static RTTI type;
};

#endif // FFGLPLUGINSOURCEFACTORY_H
