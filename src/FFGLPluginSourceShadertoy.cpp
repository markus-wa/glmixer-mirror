/*
 *   FFGLPluginSourceShadertoy
 *
 *   This file is part of GLMixer.
 *
 *   GLMixer is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation, either version 3 of the License, or
 *   (at your option) any later version.
 *
 *   GLMixer is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with GLMixer.  If not, see <http://www.gnu.org/licenses/>.
 *
 *   Copyright 2009, 2012 Bruno Herbelin
 *
 */

#include "FFGLPluginSourceShadertoy.moc"
#include "FFGLPluginInstances.h"

#include <QDesktopServices>


FFGLPluginSource::RTTI FFGLPluginSourceShadertoy::type = FFGLPluginSource::SHADERTOY_PLUGIN;


FFGLPluginSourceShadertoy::FFGLPluginSourceShadertoy(int w, int h, FFGLTextureStruct inputTexture) : FFGLPluginSource(w, h, inputTexture)
{
    // free the FFGLPluginInstanceFreeframe instance
    if (_plugin) free(_plugin);

    // instanciate a FFGLPluginInstanceShadertoy plugin instead
    _plugin =  FFGLPluginInstanceShadertoy::New();

    // check validity of plugin
    if (!_plugin){
        qWarning()<< "Shadertoy" << QChar(124).toLatin1() << QObject::tr("Plugin could not be instanciated");
        FFGLPluginException().raise();
    }

    // automatically load the resource-embeded DLL
    load(libraryFileName());

    // perform declaration of extra functions for Shadertoy
    FFGLPluginInstanceShadertoy *p = dynamic_cast<FFGLPluginInstanceShadertoy *>(_plugin);
    if ( !p || !p->declareShadertoyFunctions() ){
        qWarning()<< libraryFileName() << QChar(124).toLatin1() << QObject::tr("This library is not a valid GLMixer Shadertoy plugin.");
        FFGLPluginException().raise();
    }

    //  customize about string
    QString about = "By ";
#ifdef Q_OS_WIN
    about.append(getenv("USERNAME"));
#else
    about.append(getenv("USER"));
#endif
    _info["About"] = about;

}

FFGLPluginSourceShadertoy::~FFGLPluginSourceShadertoy()
{
    emit dying();
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
            c = QString::fromLatin1(string).trimmed();
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
            c = QString::fromLatin1(string).trimmed();
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
            c = QString::fromLatin1(string).trimmed();
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
            c = QString::fromLatin1(string).trimmed();
        }
    }

    return c;
}

void FFGLPluginSourceShadertoy::setCode(QString code)
{
    // access the functions for Shadertoy plugin
    FFGLPluginInstanceShadertoy *p = dynamic_cast<FFGLPluginInstanceShadertoy *>(_plugin);
    if ( p ) {

        // not initialized yet ?
        if ( initialize() )
            p->setString(FFGLPluginInstanceShadertoy::CODE_SHADERTOY, code.trimmed().toLatin1().data() );

    }
}


void FFGLPluginSourceShadertoy::setName(QString string)
{
    _info["Name"] = string;

    emit changed();
}
void FFGLPluginSourceShadertoy::setAbout(QString string)
{
    _info["About"] = string;

    emit changed();
}
void FFGLPluginSourceShadertoy::setDescription(QString string)
{
    _info["Description"] = string;

    emit changed();
}


QDomElement FFGLPluginSourceShadertoy::getConfiguration( QDir current )
{
    QDomDocument root;
    QDomElement p = root.createElement("ShadertoyPlugin");

    // save info as XML nodes
    QDomElement info = root.createElement("Name");
    QDomText value = root.createTextNode( _info["Name"].toString() );
    info.appendChild(value);
    p.appendChild(info);

    info = root.createElement("About");
    value = root.createTextNode( _info["About"].toString() );
    info.appendChild(value);
    p.appendChild(info);

    info = root.createElement("Description");
    value = root.createTextNode( _info["Description"].toString() );
    info.appendChild(value);
    p.appendChild(info);

    // save code of the plugin
    QDomElement c = root.createElement("Code");
    QDomText code = root.createTextNode( getCode().toAscii() );
    c.appendChild(code);
    p.appendChild(c);

    return p;
}

void FFGLPluginSourceShadertoy::setConfiguration(QDomElement xml)
{
    initialize();

    setName( xml.firstChildElement("Name").text() );
    setAbout( xml.firstChildElement("About").text() );
    setDescription( xml.firstChildElement("Description").text() );
    setCode( xml.firstChildElement("Code").text() );

}


// Static function to extract the resource containing the DLL
// for the plugin Freeframe implementing the features of
// shadertoy GLSL rendering.
QString FFGLPluginSourceShadertoy::libraryFileName()
{
    static bool first_launch = true;

#ifdef Q_OS_WIN
    QFileInfo plugindll( QString(QDesktopServices::storageLocation(QDesktopServices::TempLocation)).append("/libShadertoy.dll") );
#else
    QFileInfo plugindll( QString(QDesktopServices::storageLocation(QDesktopServices::TempLocation)).append("/libShadertoy.so") );
#endif

    // replace the plugin file in temporary location
    // (make sure we do it each new run of GLMixer (might have been updated)
    // and in case the file does not exist (might have been deleted by sytem) )
    if (first_launch || !plugindll.exists()) {
        QFile::remove(plugindll.absoluteFilePath());
        if ( QFile::copy(":/ffgl/Shadertoy", plugindll.absoluteFilePath()) ) {
            QFile::setPermissions(plugindll.absoluteFilePath(), QFile::ReadOwner | QFile::WriteOwner);
            first_launch = false;
        } else
            qCritical() << QObject::tr("Error creating Shadertoy plugin (%1).").arg(plugindll.absoluteFilePath());
    }

    return plugindll.absoluteFilePath();
}
