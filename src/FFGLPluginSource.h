#ifndef FFGLPLUGINSOURCE_H
#define FFGLPLUGINSOURCE_H

#include "FFGL.h"

#include <QtCore>
#include <QString>
#include <QElapsedTimer>
#include <QDomElement>

class FFGLPluginException : public QtConcurrent::Exception {
public:
    virtual QString message() { return QObject::tr("FreeframeGL plugin crashed."); }
    void raise() const { throw *this; }
    Exception *clone() const { return new FFGLPluginException(*this); }
};



class FFGLPluginSource {
public:

    FFGLPluginSource(QString filename, int w, int h, FFGLTextureStruct inputTexture);
    virtual ~FFGLPluginSource();

    bool initialize();
    void update();
    void bind() const;
    inline QString fileName() const { return _filename; }
    inline bool isSourceType() const { return _isFreeframeTypeSource; }
    inline int width() const { return _fboSize.width(); }
    inline int height() const { return _fboSize.height(); }

    FFGLTextureStruct getOutputTextureStruct();
    FFGLTextureStruct getInputTextureStruct();
    void setInputTextureStruct(FFGLTextureStruct inputTexture);

    // pause update
    void setPaused(bool pause);
    bool isPaused() { return _pause;}

    // parameters
    QVariantHash getParameters();
    QVariantHash getParametersDefaults() { return parametersDefaults; }
    QVariantHash getInfo() { return info; }
    void setParameter(int parameterNum, QVariant value);

    // reset
    void restoreDefaults();

    // XML config
    virtual QDomElement getConfiguration(QDir current = QDir());
    virtual void setConfiguration(QDomElement xml);

private:
    // self management
    QVariantHash parametersDefaults;
    QVariantHash info;
    QString _filename;
    bool _initialized, _isFreeframeTypeSource;

    //this represents the texture (on the GPU) that we feed to the plugins
    FFGLTextureStruct _inputTexture;

    // FFGL specialized objects for plugin
    class FFGLPluginSourceInstance *_plugin;

    // timer
    QElapsedTimer timer;
    qint64 _elapsedtime;
    bool _pause;

    // Frame buffer objet
    class QGLFramebufferObject *_fbo;
    QSize _fboSize;
};




#endif // FFGLPLUGINSOURCE_H
