#ifndef FFGLPLUGINSOURCE_H
#define FFGLPLUGINSOURCE_H

#include "FFGL.h"

#include <QtCore>
#include <QString>
#include <QElapsedTimer>

class FFGLPluginException : public QtConcurrent::Exception {
public:
    virtual QString message() { return "FreeframeGL plugin crashed."; }
    void raise() const { throw *this; }
    Exception *clone() const { return new FFGLPluginException(*this); }
};

class FFGLPluginSource {
public:

    FFGLPluginSource(QString filename, int w, int h, FFGLTextureStruct inputTexture);
    ~FFGLPluginSource();

    bool initialize();
    void update();
    void bind() const;
    inline QString fileName() { return _filename; }

    FFGLTextureStruct FBOTextureStruct();
    void setPaused(bool pause);

private:
    // self management
    QString _filename;
    bool _initialized;

    //this represents the texture (on the GPU) that we feed to the plugins
    FFGLTextureStruct _inputTexture;

    // FFGL specialized objects for plugin
    class FFGLPluginInstance *_plugin;

    // timer
    QElapsedTimer timer;
    qint64 _elapsedtime;
    bool _pause;

    // Frame buffer objet
    class QGLFramebufferObject *_fbo;
    QSize _fboSize;
};




#endif // FFGLPLUGINSOURCE_H
