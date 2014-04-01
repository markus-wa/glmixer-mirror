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
    void setCode(QString code = QString::null);

    // get logs of GLSL Shadertoy execution
    QString getLogs();
    QString getHeaders();

    // set information fields
    void setName(QString);
    void setAbout(QString);
    void setDescription(QString);

    // XML config
    QDomElement getConfiguration(QDir current = QDir());
    void setConfiguration(QDomElement xml);

private:
    QString _code;
    static QString libraryFileName();
    static RTTI type;
};

#endif // FFGLPLUGINSOURCEFACTORY_H
