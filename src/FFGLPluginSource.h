#ifndef FFGLPLUGINSOURCE_H
#define FFGLPLUGINSOURCE_H

#include <FFGL.h>

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

    FFGLPluginSource(int w, int h, FFGLTextureStruct inputTexture);
    virtual ~FFGLPluginSource();

    // Run-Time Type Information
    typedef enum {
        FREEFRAME_PLUGIN = 0,
        SHADERTOY_PLUGIN
    } RTTI;
    virtual RTTI rtti() const { return type; }

    // loading of FFGL plugin
    void load(QString filename);

    // update texture (to be called at each update of source)
    void update();
    void bind() const;

    // general attributes
    inline QString fileName() const { return _filename; }
    inline bool isSourceType() const { return _isFreeframeTypeSource; }
    inline int width() const { return _fboSize.width(); }
    inline int height() const { return _fboSize.height(); }

    // texture attributes
    FFGLTextureStruct getOutputTextureStruct();
    FFGLTextureStruct getInputTextureStruct();
    void setInputTextureStruct(FFGLTextureStruct inputTexture);

    // pause update
    void setPaused(bool pause);
    bool isPaused() { return _pause;}
    bool isinitialized() { return _initialized;}

    // parameters
    QVariantHash getParameters();
    QVariantHash getParametersDefaults() { return parametersDefaults; }
    void setParameter(int parameterNum, QVariant value);
    void setParameter(QString parameterName, QVariant value);

    // information
    virtual QVariantHash getInfo() { return info; }

    // reset
    void restoreDefaults();

    // XML config
    virtual QDomElement getConfiguration(QDir current = QDir());
    virtual void setConfiguration(QDomElement xml);


protected:
    // FFGL specialized objects for plugin
    class FFGLPluginInstance *_plugin;

    // initialization should be called from inside
    bool initialize();

private:
    // self management
    QVariantHash parametersDefaults;
    QVariantHash info;
    QString _filename;
    bool _initialized, _isFreeframeTypeSource;

    //this represents the texture (on the GPU) that we feed to the plugins
    FFGLTextureStruct _inputTexture;

    // timer
    QElapsedTimer timer;
    qint64 _elapsedtime;
    bool _pause;

    // Frame buffer objet
    class QGLFramebufferObject *_fbo;
    QSize _fboSize;

    // RTTI
    static RTTI type;
};




#endif // FFGLPLUGINSOURCE_H
