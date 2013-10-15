#include <QDebug>

#include "common.h"
#include "FFGLPluginSource.h"
#include "FFGLPluginInstance.h"

#include <QGLFramebufferObject>


class FFGLPluginSourceInstance : public FFGLPluginInstance {

public:

    FFGLPluginSourceInstance() : FFGLPluginInstance() {}

    bool hasProcessOpenGLCapability() {

#ifdef FF_FAIL
        // FFGL 1.5
        DWORD arg = FF_CAP_PROCESSOPENGL;
        DWORD returned = m_ffPluginMain(FF_GETPLUGINCAPS,arg,0).ivalue;
#else
        // FFGL 1.6
        FFMixed arg;
        arg.UIntValue = FF_CAP_PROCESSOPENGL;
        FFUInt32 returned = m_ffPluginMain(FF_GETPLUGINCAPS,arg,0).UIntValue;
#endif

        return returned == FF_SUPPORTED;
    }


    QVariantHash getInfo() {
        QVariantHash mapinfo;
#ifdef FF_FAIL
        // FFGL 1.5
        DWORD arg = 0;
        void *result = m_ffPluginMain(FF_GETINFO,arg,0).PISvalue;
#else
        // FFGL 1.6
        FFMixed arg;
        void *result = m_ffPluginMain(FF_GETINFO, arg, 0).PointerValue;
#endif
        if (result!=NULL) {
            PluginInfoStructTag *plugininfo = (PluginInfoStructTag*) result;
            mapinfo.insert("Name", (char *) plugininfo->PluginName);
            mapinfo.insert("Type", (uint) plugininfo->PluginType);
        }
        return mapinfo;
    }

    QVariantHash getExtendedInfo() {
        QVariantHash mapinfo;
#ifdef FF_FAIL
        // FFGL 1.5
        DWORD arg = 0;
        void *result = m_ffPluginMain(FF_GETEXTENDEDINFO,arg,0).ivalue;
#else
        // FFGL 1.6
        FFMixed arg;
        void *result = m_ffPluginMain(FF_GETEXTENDEDINFO, arg, 0).PointerValue;
#endif
        if (result!=NULL) {
            PluginExtendedInfoStructTag *plugininfo = (PluginExtendedInfoStructTag*) result;
            mapinfo.insert("Description", plugininfo->Description);
            mapinfo.insert("About", plugininfo->About);
            mapinfo.insert("Version", QString("%1.%2").arg(plugininfo->PluginMajorVersion).arg(plugininfo->PluginMinorVersion));
        }
        return mapinfo;
    }

    QVariantHash getParametersDefaults() {
        QVariantHash params;

        if (m_ffPluginMain==NULL)
            return params;

        // fill in parameter map with default values
        unsigned int i;
        QVariant value;
        for (i=0; i<MAX_PARAMETERS && i < (unsigned int) m_numParameters; i++)
        {

#ifdef FF_FAIL
            // FFGL 1.5;
            DWORD ffParameterType = m_ffPluginMain(FF_GETPARAMETERTYPE, (DWORD)i, 0).ivalue;
            plugMainUnion returned = m_ffPluginMain(FF_GETPARAMETERDEFAULT, (DWORD)i, 0);
            if (returned.ivalue != FF_FAIL) {
                switch ( ffParameterType ) {
                    case FF_TYPE_TEXT:
                        value.setValue( QString( (char*) returned.svalue ) );
                        break;
                    case FF_TYPE_BOOLEAN:
                        value.setValue( returned.fvalue > 0.0 );
                        break;
                    default:
                    case FF_TYPE_STANDARD:
                        value.setValue( returned.fvalue );
                        break;
                }
            }
#else
            // FFGL 1.6
            FFMixed arg;
            FFUInt32 returned;
            arg.UIntValue = i;
            FFUInt32 ffParameterType = m_ffPluginMain(FF_GETPARAMETERTYPE,arg,0).UIntValue;
            switch ( ffParameterType ) {
                case FF_TYPE_TEXT:
                    value.setValue( QString( (const char*) m_ffPluginMain(FF_GETPARAMETERDEFAULT,arg,0).PointerValue ) );
                    break;
                case FF_TYPE_BOOLEAN:
                    returned = m_ffPluginMain(FF_GETPARAMETERDEFAULT,arg,0).UIntValue;
                    value.setValue(  *((bool *)&returned)  );
                    break;
                default:
                case FF_TYPE_STANDARD:
                    returned = m_ffPluginMain(FF_GETPARAMETERDEFAULT,arg,0).UIntValue;
                    value.setValue( *((float *)&returned) );
                    break;
            }
#endif

            // add it to the list
            params.insert( QString(GetParameterName(i)), value);
        }

        qDebug() << "getParametersDefaults" << params;

        return params;
    }

    QVariantHash getParameters()
    {
        QVariantHash params;

        if (m_ffInstanceID==INVALIDINSTANCE)
            return getParametersDefaults();

        // fill in parameter map with default values
        unsigned int i;
        QVariant value;
        for (i=0; i<MAX_PARAMETERS && i < (unsigned int) m_numParameters; i++)
        {
#ifdef FF_FAIL
            // FFGL 1.5;
            DWORD ffParameterType = m_ffPluginMain(FF_GETPARAMETERTYPE, (DWORD)i, 0).ivalue;
            plugMainUnion returned = m_ffPluginMain(FF_GETPARAMETER, (DWORD)i, m_ffInstanceID);
            if (returned.ivalue != FF_FAIL) {
                switch ( ffParameterType ) {
                    case FF_TYPE_TEXT:
                        value.setValue( QString( (char*) returned.svalue ) );
                        break;
                    case FF_TYPE_BOOLEAN:
                        value.setValue( returned.fvalue > 0.0 );
                        break;
                    default:
                    case FF_TYPE_STANDARD:
                        value.setValue( returned.fvalue );
                        break;
                }
            }
#else
            // FFGL 1.6
            FFMixed arg;
            FFUInt32 returned;
            arg.UIntValue = i;
            FFUInt32 ffParameterType = m_ffPluginMain(FF_GETPARAMETERTYPE,arg,0).UIntValue;
            switch ( ffParameterType ) {
                case FF_TYPE_BOOLEAN:
                    returned = m_ffPluginMain(FF_GETPARAMETER, arg, m_ffInstanceID).UIntValue;
                    value.setValue(  *((bool *)&returned)  );
                break;

                case FF_TYPE_TEXT:
                    value.setValue( QString( (const char*) m_ffPluginMain(FF_GETPARAMETER, arg, m_ffInstanceID).PointerValue ) );
                break;

                default:
                case FF_TYPE_STANDARD:
                    returned = m_ffPluginMain(FF_GETPARAMETER, arg, m_ffInstanceID).UIntValue;
                    value.setValue( *((float *)&returned) );
                break;
            }
#endif
            // add it to the list
            params.insert( QString(GetParameterName(i)), value);
        }

        qDebug() << "getParameters" << params;
        return params;
    }

    unsigned int getNumParameters() {
        return m_numParameters;
    }

    bool setParameter(QString paramName, QVariant value)
    {
        // find the parameter with that name
        for (unsigned int i=0; i < (unsigned int) m_numParameters; ++i){
            if ( paramName.compare(GetParameterName(i), Qt::CaseInsensitive) == 0 )
                return setParameter(i, value);
        }

        return false;
    }

    bool setParameter(unsigned int paramNum, QVariant value)
    {
        if (paramNum<0 || paramNum >= (unsigned int) m_numParameters ||
            m_ffInstanceID==INVALIDINSTANCE || m_ffPluginMain==NULL)
            return false;

        SetParameterStruct ArgStruct;
        ArgStruct.ParameterNumber = paramNum;

#ifdef FF_FAIL
        // FFGL 1.5
        DWORD ffParameterType = m_ffPluginMain(FF_GETPARAMETERTYPE, (DWORD)paramNum, 0).ivalue;
#else
        // FFGL 1.6
        FFMixed arg;
        arg.UIntValue = paramNum;
        FFUInt32 ffParameterType = m_ffPluginMain(FF_GETPARAMETERTYPE,arg,0).UIntValue;

        arg.PointerValue = &ArgStruct;
#endif

        // depending on type assign value in data structure
        if ( ffParameterType == FF_TYPE_STANDARD && value.canConvert(QVariant::Double) )
        {
            // Cast to float
            float v = value.toFloat();

            qDebug()<< "setparameter FF_TYPE_STANDARD "<< ArgStruct.ParameterNumber <<" = "<<v;

#ifdef FF_FAIL
            *((float *)(unsigned)&ArgStruct.NewParameterValue) = v;
            m_ffPluginMain(FF_SETPARAMETER,(DWORD)(&ArgStruct), m_ffInstanceID);
#else
            // pack our float into FFUInt32
            ArgStruct.NewParameterValue.UIntValue = *(FFUInt32 *)&v;
            m_ffPluginMain(FF_SETPARAMETER, arg, m_ffInstanceID);
#endif
        }
        else if ( ffParameterType == FF_TYPE_TEXT && value.canConvert(QVariant::String) )
        {
            // Cast to string
#ifdef FF_FAIL
            ArgStruct.NewParameterValue = (char *) value.toString().toAscii().data();
            m_ffPluginMain(FF_SETPARAMETER, (DWORD)(&ArgStruct), m_ffInstanceID);
#else
            ArgStruct.NewParameterValue.PointerValue = value.toString().toAscii().data();
            m_ffPluginMain(FF_SETPARAMETER, arg, m_ffInstanceID);
#endif
        }
        else if ( ffParameterType == FF_TYPE_BOOLEAN && value.canConvert(QVariant::UInt) )
        {
            // Cast to unsigned int
#ifdef FF_FAIL
            ArgStruct.NewParameterValue = value.toUInt();
            m_ffPluginMain(FF_SETPARAMETER, (DWORD)(&ArgStruct), m_ffInstanceID);
#else
            ArgStruct.NewParameterValue.UIntValue = value.toUInt();
            m_ffPluginMain(FF_SETPARAMETER, arg, m_ffInstanceID);
#endif
        }
        else
            return false;

        return true;
    }

};


FFGLPluginSource::FFGLPluginSource(QString filename, int w, int h, FFGLTextureStruct inputTexture)
    : _filename(filename), _initialized(false), _isFreeframeTypeSource(false), _elapsedtime(0), _pause(false), _fboSize(w,h)
{
    // check the file exists
    QFileInfo pluginfile(filename);
    if (!pluginfile.isFile()){
        qWarning()<< _filename << "| " << QObject::tr("The file does not exist.");
        FFGLPluginException().raise();
    }

    // if the plugin file exists, it might be accompanied by other DLLs
    // and we should add the path for the system to find them
    addPathToSystemPath( pluginfile.absolutePath().toUtf8() );

    // create plugin object (does not instanciate dll)
    _plugin = (FFGLPluginSourceInstance *) FFGLPluginInstance::New();
    if (!_plugin){
        qWarning()<< _filename << "| " << QObject::tr("FreeframeGL plugin could not be instanciated");
        FFGLPluginException().raise();
    }

    // load dll plugin
    if (_plugin->Load(filename.toLatin1().data()) == FF_FAIL){
        qWarning()<< _filename << "| " << QObject::tr("FreeframeGL plugin could not be loaded");
        FFGLPluginException().raise();
    }

    // ensure functionnalities plugin
    if ( !_plugin->hasProcessOpenGLCapability()){
        qWarning()<< QFileInfo(_filename).baseName() << "| " << QObject::tr("Invalid FreeframeGL plugin: does not have OpenGL support.");
        FFGLPluginException().raise();
    }

    // fill in the information about this plugin
    info = _plugin->getInfo();
    _isFreeframeTypeSource = info["Type"].toUInt() > 0;
    info.unite(_plugin->getExtendedInfo());
//    qDebug() << info;

    // remember default values
    parametersDefaults = _plugin->getParametersDefaults();

    // descriptor for the source texture, used also to store size
    _inputTexture.Handle = inputTexture.Handle;
    _inputTexture.Width = inputTexture.Width;
    _inputTexture.Height = inputTexture.Height;
    _inputTexture.HardwareWidth = inputTexture.HardwareWidth;
    _inputTexture.HardwareHeight = inputTexture.HardwareHeight;

//    qDebug() << _filename << "| " << QObject::tr("FreeframeGL plugin created") << " ("<< info["Name"].toString() <<", "<< _inputTexture.Width << _inputTexture.Height <<")";
}

FFGLPluginSource::~FFGLPluginSource()
{
    _plugin->DeInstantiateGL();
    _plugin->Unload();

    // deletes
    delete _plugin;
    if (_fbo)
        delete _fbo;
}

FFGLTextureStruct FFGLPluginSource::getOutputTextureStruct(){

    FFGLTextureStruct it;

    if (initialize()) {
        it.Handle = _fbo->texture();
        it.Width = _fbo->width();
        it.Height = _fbo->height();
        it.HardwareWidth = _fbo->width();
        it.HardwareHeight = _fbo->height();
    }
    return it;
}

FFGLTextureStruct FFGLPluginSource::getInputTextureStruct(){

    FFGLTextureStruct it;

    it.Handle = _inputTexture.Handle;
    it.Width = _inputTexture.Width;
    it.Height = _inputTexture.Height;
    it.HardwareWidth = _inputTexture.HardwareWidth;
    it.HardwareHeight = _inputTexture.HardwareHeight;

    return it;
}


void FFGLPluginSource::setInputTextureStruct(FFGLTextureStruct inputTexture)
{
    // descriptor for the source texture, used also to store size
    _inputTexture.Handle = inputTexture.Handle;
    _inputTexture.Width = inputTexture.Width;
    _inputTexture.Height = inputTexture.Height;
    _inputTexture.HardwareWidth = inputTexture.HardwareWidth;
    _inputTexture.HardwareHeight = inputTexture.HardwareHeight;
}

void FFGLPluginSource::update()
{
    if (initialize() && _fbo->bind())
    {
        // Safer to push all attribs ; who knows what is done in the puglin ?!!
        // (but slower)
        glPushAttrib(GL_ALL_ATTRIB_BITS);

        // draw in the viewport area
        glViewport(0, 0, _fbo->width(), _fbo->height());

        //make sure all the matrices are reset
        glMatrixMode(GL_TEXTURE);
        glPushMatrix();
        glLoadIdentity();
        glMatrixMode(GL_PROJECTION);
        glPushMatrix();
        glLoadIdentity();
        glMatrixMode(GL_MODELVIEW);
        glPushMatrix();
        glLoadIdentity();

        //clear color buffers
        glClearColor(0.f, 0.f, 0.f, 1.f);
        glClear(GL_COLOR_BUFFER_BIT);

        // update time
        if (!_pause)
            _elapsedtime += timer.restart();

        //tell plugin about the current time
        _plugin->SetTime(((double) _elapsedtime) / 1000.0 );

        //prepare the structure used to call
        //the plugin's ProcessOpenGL method
        ProcessOpenGLStructTag processStruct;

        //specify our FBO's handle in the processOpenGLStruct
        processStruct.HostFBO = _fbo->handle();

        // if a texture handle was provided
        if (_inputTexture.Handle > 0) {
            //create the array of OpenGLTextureStruct * to be passed
            //to the plugin
            FFGLTextureStruct *inputTextures[1];
            inputTextures[0] = &_inputTexture;
            //provide the 1 input texture structure we allocated above
            processStruct.numInputTextures = 1;
            processStruct.inputTextures = inputTextures;
        } else {
            //provide no input texture
            processStruct.numInputTextures = 0;
            processStruct.inputTextures = NULL;
        }

        //call the plugin's ProcessOpenGL
#ifdef FF_FAIL
        // FFGL 1.5
        DWORD callresult = _plugin->CallProcessOpenGL(processStruct);
#else
        // FFGL 1.6
        FFResult callresult = _plugin->CallProcessOpenGL(processStruct);
#endif

        // make sure we restore state
        glPopAttrib();
        glMatrixMode(GL_TEXTURE);
        glPopMatrix();
        glMatrixMode(GL_PROJECTION);
        glPopMatrix();
        glMatrixMode(GL_MODELVIEW);
        glPopMatrix();

        //deactivate rendering to the fbo
        //(this re-activates rendering to the window)
        _fbo->release();

        // through exception once opengl has returned to normal
        if ( callresult != FF_SUCCESS ){
            qWarning()<< QFileInfo(_filename).baseName() << "| " << QObject::tr("FreeframeGL plugin could not process OpenGL. Probably missing an input texture.");
            FFGLPluginException().raise();
        }
    }
}


void FFGLPluginSource::bind() const
{
    if (_initialized)
        // bind the FBO texture
        glBindTexture(GL_TEXTURE_2D, _fbo->texture());
}

bool FFGLPluginSource::initialize()
{
    if ( !_initialized )
    {
        // create an fbo (with internal automatic first texture attachment)
        _fbo = new QGLFramebufferObject(_fboSize);
        if (_fbo) {

            FFGLViewportStruct _fboViewport;
            _fboViewport.x = 0;
            _fboViewport.y = 0;
            _fboViewport.width = _fbo->width();
            _fboViewport.height = _fbo->height();

            //instantiate the plugin passing a viewport that matches
            //the FBO (plugin is rendered into our FBO)
            if ( _plugin->InstantiateGL( &_fboViewport ) == FF_SUCCESS ) {

                timer.start();

                // remember successful initialization
                _initialized = true;
//                qDebug()<< QFileInfo(_filename).baseName() << "| " << QObject::tr("FreeframeGL plugin initialized");

            }
            else {
                qWarning()<< QFileInfo(_filename).baseName() << "| " << QObject::tr("FreeframeGL plugin could not be initialized");
                FFGLPluginException().raise();
            }
        }
        else{
            qWarning()<< QFileInfo(_filename).baseName() << "| " << QObject::tr("FreeframeGL plugin could not create FBO");
            FFGLPluginException().raise();
        }
    }

    return _initialized;
}


QVariantHash FFGLPluginSource::getParameters()
{
    return _plugin->getParameters();
}

void FFGLPluginSource::setParameter(int parameterNum, QVariant value)
{
    if( !_plugin->setParameter(parameterNum, value) )
        qWarning()<< QFileInfo(_filename).baseName() << "| " << QObject::tr("Parameter could not be set.");

}

void FFGLPluginSource::restoreDefaults()
{
    // iterate over the list of parameters
    QHashIterator<QString, QVariant> i(parametersDefaults);
    unsigned int paramNum = 0;
    while (i.hasNext()) {
        i.next();
        setParameter(paramNum, i.value());
        // increment paramNum
        paramNum++;
    }
}

void FFGLPluginSource::setPaused(bool pause) {

    _pause = pause;

    if (!_pause)
        timer.restart();

}


QDomElement FFGLPluginSource::getConfiguration( QDir current )
{
    QDomDocument root;
    QDomElement p = root.createElement("FreeFramePlugin");

    // save filename of the plugin
    QDomElement f = root.createElement("Filename");
    if (current.isReadable())
        f.setAttribute("Relative", current.relativeFilePath( fileName() ));
    QDomText filename = root.createTextNode( QDir::root().absoluteFilePath( fileName() ));
    f.appendChild(filename);
    p.appendChild(f);

    // iterate over the list of parameters
    QHashIterator<QString, QVariant> i( getParameters());
    while (i.hasNext()) {
        i.next();

        // save parameters as XML nodes
        // e.g. <Parameter name='amplitude' type='float'>0.1</param>
        QDomElement param = root.createElement("Parameter");
        param.setAttribute( "name", i.key() );
        param.setAttribute( "type", i.value().typeName() );
        QDomText value = root.createTextNode( i.value().toString() );
        param.appendChild(value);

        p.appendChild(param);
    }

    return p;
}

void FFGLPluginSource::setConfiguration(QDomElement xml)
{
    initialize();

    // start loop of parameters to read
    QDomElement p = xml.firstChildElement("Parameter");
    while (!p.isNull()) {

        // read and apply parameter, if applicable
        if ( !_plugin->setParameter( p.attribute("name"), QVariant(p.text()) ) )
            qWarning()<< QFileInfo(_filename).baseName() << "| " << p.attribute("name") << QObject::tr(": parameter could not be set.");

        p = p.nextSiblingElement("Parameter");
    }
}



// for getting debug messages from FFGL code
void FFDebugMessage(const char *msg)
{
    qDebug()<<msg;
}
