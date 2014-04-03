#ifndef FFGLPLUGINSOURCESHADERTOY_H
#define FFGLPLUGINSOURCESHADERTOY_H

#include "FFGLPluginSource.h"

class FFGLPluginSourceShadertoy : public FFGLPluginSource
{
    Q_OBJECT

public:
    FFGLPluginSourceShadertoy(int w, int h, FFGLTextureStruct inputTexture);
    ~FFGLPluginSourceShadertoy();

    // Run-Time Type Information
    RTTI rtti() const { return FFGLPluginSourceShadertoy::type; }

    // FFGLPluginSource XML config
    QDomElement getConfiguration(QDir current = QDir());
    void setConfiguration(QDomElement xml);

    // get and set of GLSL Shadertoy code
    QString getCode();
    QString getDefaultCode();
    void setCode(QString code = QString::null);

    // get logs of GLSL Shadertoy execution
    QString getLogs();

    // get the header of the plugin shadertoy
    QString getHeaders();

public Q_SLOTS:
    // set information fields
    void setName(QString);
    void setAbout(QString);
    void setDescription(QString);

Q_SIGNALS:
    void dying();

private:

    static QString libraryFileName();
    static RTTI type;
};

#endif // FFGLPLUGINSOURCEFACTORY_H
