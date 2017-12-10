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

#include "glmixer.h"
#include "FFGLPluginSourceShadertoy.moc"
#include "FFGLPluginInstances.h"

FFGLPluginSource::RTTI FFGLPluginSourceShadertoy::type = FFGLPluginSource::SHADERTOY_PLUGIN;


FFGLPluginSourceShadertoy::FFGLPluginSourceShadertoy(bool plugintype, int w, int h, FFGLTextureStruct inputTexture) : FFGLPluginSource(w, h, inputTexture)
{
    // instanciate a FFGLPluginInstanceShadertoy plugin instead
    _plugin =  FFGLPluginInstanceShadertoy::New();

    // check validity of plugin
    if (!_plugin){
        qWarning()<< "Shadertoy" << QChar(124).toLatin1() << QObject::tr("GPU Plugin could not be instanciated");
        FFGLPluginException().raise();
    }

    // automatically load the resource-embeded DLL
    if ( plugintype == FF_EFFECT )
        load(libraryFileName("ShadertoyEffect"));
    else
        load(libraryFileName("ShadertoySource"));

    // perform declaration of extra functions for Shadertoy
    FFGLPluginInstanceShadertoy *p = dynamic_cast<FFGLPluginInstanceShadertoy *>(_plugin);
    if ( !p ){
        qWarning()<< "Shadertoy" << QChar(124).toLatin1() << QObject::tr("Cannot create GPU plugin.");
        FFGLPluginException().raise();
    }
    if ( !p->declareShadertoyFunctions() ){
        qWarning()<< "Shadertoy" << QChar(124).toLatin1() << QObject::tr("Invalid GPU plugin.");
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

    // connect for keyboard events
    connect(GLMixer::getInstance(), SIGNAL(keyPressed(int,bool)), this, SLOT(setKey(int,bool)));
}

FFGLPluginSourceShadertoy::~FFGLPluginSourceShadertoy()
{
    disconnect(GLMixer::getInstance(), SIGNAL(keyPressed(int,bool)), this, SLOT(setKey(int,bool)));
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
            delete string;
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
            delete string;
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
            delete string;
        }
    }

    c.append(warnings);

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
            delete string;
        }
    }

    return c;
}

void FFGLPluginSourceShadertoy::setCode(QString code)
{
    //
    // Pre-analyse the code to check for potential sources of errors
    // and generate internal warnings
    //
    warnings.clear(); // cleanup warnings
    int h = getHeaders().count('\n') + 2; // number of lines before code

    // WARNING on the presence of 'texture2D' call
    int i = code.indexOf("texture2D");
    if (i > 0)
        warnings.append( QString("\n(%1) : warning : texture2D deprecated, use 'texture' instead.").arg( h + code.left(i).count('\n')) );

    // WARNING on the presence of 'iMouse' variable
    i = code.indexOf("iMouse");
    if (i > 0)
        warnings.append( QString("\n(%1) : error : iMouse (mouse coordinates in ShaderToy) not implemented.").arg( h + code.left(i).count('\n')) );

    // WARNING on the presence of 'iGlobalTime' variable
    i = code.indexOf("iGlobalTime");
    if (i > 0) {
        warnings.append( QString("\n(%1) : warning : iGlobalTime deprecated, use 'iTime' instead.").arg( h + code.left(i).count('\n')) );
        code.replace("iGlobalTime", "iTime");
    }

    // access the functions for Shadertoy plugin
    FFGLPluginInstanceShadertoy *p = dynamic_cast<FFGLPluginInstanceShadertoy *>(_plugin);
    if ( p ) {
        p->setString(FFGLPluginInstanceShadertoy::CODE_SHADERTOY, code.trimmed().toLatin1().data() );

    }

}

void FFGLPluginSourceShadertoy::setKey(int key, bool status)
{
    // access the functions for Shadertoy plugin
    FFGLPluginInstanceShadertoy *p = dynamic_cast<FFGLPluginInstanceShadertoy *>(_plugin);
    if ( p ) {
        p->setKeyboard(key, status);
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
    QDomElement p = root.createElement("FreeFramePlugin");
    // parameter zero is the speed
    p.setAttribute("Speed", QString::number(_plugin->GetFloatParameter(0),'f',PROPERTY_DECIMALS) );

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
    // make sure its initialized
    if (initialize()) {

        // parameter zero is the speed
        setParameter( 0, QVariant(xml.attribute("Speed", "1.0").toDouble()) );

        // set specific elements
        setName( xml.firstChildElement("Name").text() );
        setAbout( xml.firstChildElement("About").text() );
        setDescription( xml.firstChildElement("Description").text() );
        setCode( xml.firstChildElement("Code").text() );
    }
}
