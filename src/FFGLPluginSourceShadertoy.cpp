#include "FFGLPluginSourceShadertoy.h"
#include "FFGLPluginInstances.h"

#include <QDesktopServices>


FFGLPluginSource::RTTI FFGLPluginSourceShadertoy::type = FFGLPluginSource::SHADERTOY_PLUGIN;


FFGLPluginSourceShadertoy::FFGLPluginSourceShadertoy(int w, int h, FFGLTextureStruct inputTexture) : FFGLPluginSource(w, h, inputTexture)
{
    // free the FFGLPluginInstanceFreeframe instance
    if (_plugin) free(_plugin);

    // instanciate a FFGLPluginInstanceFreeframe plugin
    _plugin =  FFGLPluginInstanceShadertoy::New();

    // check validity of plugin
    if (!_plugin){
        qWarning()<< "Shadertoy" << QChar(124).toLatin1() << QObject::tr("Plugin could not be instanciated");
        FFGLPluginException().raise();
    }

    // automatically load the resource embeded DLL
    load(libraryFileName());

    _name = getInfo()["Name"].toString();
    _about = getInfo()["About"].toString();
    _description = getInfo()["Description"].toString();

    // perform declaration of extra functions for Shadertoy
    FFGLPluginInstanceShadertoy *p = dynamic_cast<FFGLPluginInstanceShadertoy *>(_plugin);
    if ( !p || !p->declareShadertoyFunctions() ){
        qWarning()<< libraryFileName() << QChar(124).toLatin1() << QObject::tr("This library is not a valid GLMixer Shadertoy plugin.");
        FFGLPluginException().raise();
    }
}


QString FFGLPluginSourceShadertoy::getCode()
{
    QString c = "invalid";

    // access the functions for Shadertoy plugin
    FFGLPluginInstanceShadertoy *p = dynamic_cast<FFGLPluginInstanceShadertoy *>(_plugin);
    if ( p ) {
        char *string = p->getString(FFGLPluginInstanceShadertoy::CODE_SHADERTOY);
        if ( string ) {
            // Convert char array into QString
            c = QString::fromLatin1(string);
        }
    }

    return c;
}

QString FFGLPluginSourceShadertoy::getDefaultCode()
{
    QString c = "invalid";

    // access the functions for Shadertoy plugin
    FFGLPluginInstanceShadertoy *p = dynamic_cast<FFGLPluginInstanceShadertoy *>(_plugin);
    if ( p ) {
        char *string = p->getString(FFGLPluginInstanceShadertoy::DEFAULTCODE_SHADERTOY);
        if ( string ) {
            // Convert char array into QString
            c = QString::fromLatin1(string);
        }
    }

    return c;
}

QString FFGLPluginSourceShadertoy::getLogs()
{
    QString c = "";

    // access the functions for Shadertoy plugin
    FFGLPluginInstanceShadertoy *p = dynamic_cast<FFGLPluginInstanceShadertoy *>(_plugin);
    if ( p ) {
        char *string = p->getString(FFGLPluginInstanceShadertoy::LOG_SHADERTOY);
        if ( string ) {
            // Convert char array into QString
            c = QString::fromLatin1(string);
        }
    }

    return c;
}

QString FFGLPluginSourceShadertoy::getHeaders()
{
    QString c = "";

    // access the functions for Shadertoy plugin
    FFGLPluginInstanceShadertoy *p = dynamic_cast<FFGLPluginInstanceShadertoy *>(_plugin);
    if ( p ) {
        char *string = p->getString(FFGLPluginInstanceShadertoy::HEADER_SHADERTOY);
        if ( string ) {
            // Convert char array into QString
            c = QString::fromLatin1(string);
        }
    }

    return c;
}

void FFGLPluginSourceShadertoy::setCode(QString code)
{
    // access the functions for Shadertoy plugin
    FFGLPluginInstanceShadertoy *p = dynamic_cast<FFGLPluginInstanceShadertoy *>(_plugin);
    if ( p ) {
        // convert QString to char array
        p->setString(FFGLPluginInstanceShadertoy::CODE_SHADERTOY, code.toLatin1().data() );
    }
}

QString FFGLPluginSourceShadertoy::getName()
{
    return _name;
}

void FFGLPluginSourceShadertoy::setName(QString name)
{
    _name = name;
}


// Static function to extract the resource containing the DLL
// for the plugin Freeframe implementing the features of
// shadertoy GLSL rendering.
QString FFGLPluginSourceShadertoy::libraryFileName()
{
    static bool first_launch = true;

    // TODO support OSX and WIN32
    QFileInfo plugindll( QString(QDesktopServices::storageLocation(QDesktopServices::TempLocation)).append("/libShadertoy.so") );

    // replace the plugin file in temporary location
    // (make sure we do it each new run of GLMixer (might have been updated)
    // and in case the file does not exist (might have been deleted by sytem) )
    if (first_launch || !plugindll.exists()) {
        QFile::remove(plugindll.absoluteFilePath());
        if ( QFile::copy(":/ffgl/Shadertoy", plugindll.absoluteFilePath()) )
            first_launch = false;
        else
            qCritical() << QObject::tr("Error creating Shadertoy plugin.");
    }

    return plugindll.absoluteFilePath();
}
