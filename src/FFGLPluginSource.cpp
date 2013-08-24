#include <QDebug>

#include "FFGLPluginSource.h"
#include "FFGLPluginInstance.h"

#include <QGLFramebufferObject>


class FFGLPluginSourceInstance : public FFGLPluginInstance {

public:

    FFGLPluginSourceInstance() : FFGLPluginInstance() {}

    bool hasProcessOpenGLCapability() {

        FFMixed arg;
        arg.UIntValue = FF_CAP_PROCESSOPENGL;
        FFUInt32 returned = m_ffPluginMain(FF_GETPLUGINCAPS,arg,0).UIntValue;

        return returned == FF_SUPPORTED;
    }


    QVariantHash getInfo() {
        QVariantHash mapinfo;
        FFMixed arg;
        void *result = m_ffPluginMain(FF_GETINFO, arg, 0).PointerValue;
        if (result!=NULL) {
            PluginInfoStructTag *plugininfo = (PluginInfoStructTag*) result;
            mapinfo.insert("Name", plugininfo->PluginName);
            mapinfo.insert("Type", plugininfo->PluginType);
        }
        return mapinfo;
    }

    QVariantHash getExtendedInfo() {
        QVariantHash mapinfo;
        FFMixed arg;
        void *result = m_ffPluginMain(FF_GETEXTENDEDINFO, arg, 0).PointerValue;
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
        FFMixed arg;
        FFUInt32 returned;
        for (i=0; i<MAX_PARAMETERS && i<m_numParameters; i++)
        {
            arg.UIntValue = i;
            FFUInt32 ffParameterType = m_ffPluginMain(FF_GETPARAMETERTYPE,arg,0).UIntValue;
            QVariant value;
            switch ( ffParameterType ) {
                case FF_TYPE_BOOLEAN:
                    returned = m_ffPluginMain(FF_GETPARAMETERDEFAULT,arg,0).UIntValue;
                    value.setValue(  *((bool *)&returned)  );
                break;

                case FF_TYPE_TEXT:
                    value.setValue( QString( (const char*) m_ffPluginMain(FF_GETPARAMETERDEFAULT,arg,0).PointerValue ) );
                break;

                default:
                case FF_TYPE_STANDARD:
                    returned = m_ffPluginMain(FF_GETPARAMETERDEFAULT,arg,0).UIntValue;
                    value.setValue( *((float *)&returned) );
                break;
            }

            // add it to the list
            params.insert( QString(GetParameterName(i)), value);
        }

        return params;
    }

    QVariantHash getParameters()
    {
        QVariantHash params;

        if (m_ffInstanceID==INVALIDINSTANCE)
            return getParametersDefaults();

        // fill in parameter map with default values
        unsigned int i;
        FFMixed arg;
        FFUInt32 returned;
        for (i=0; i<MAX_PARAMETERS && i<m_numParameters; i++)
        {
            arg.UIntValue = i;
            FFUInt32 ffParameterType = m_ffPluginMain(FF_GETPARAMETERTYPE,arg,0).UIntValue;
            QVariant value;
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

            // add it to the list
            params.insert( QString(GetParameterName(i)), value);
        }

        qDebug() << params;
        return params;
    }

    unsigned int getNumParameters() {
        return m_numParameters;
    }

    bool setParameter(unsigned int paramNum, QVariant value)
    {
        if (paramNum<0 || paramNum>=m_numParameters ||
            m_ffInstanceID==INVALIDINSTANCE || m_ffPluginMain==NULL)
            return false;

        FFMixed arg;
        arg.UIntValue = paramNum;
        FFUInt32 ffParameterType = m_ffPluginMain(FF_GETPARAMETERTYPE,arg,0).UIntValue;

        SetParameterStruct ArgStruct;
        ArgStruct.ParameterNumber = paramNum;
        arg.PointerValue = &ArgStruct;

        // depending on type assign value in data structure
        if ( ffParameterType == FF_TYPE_STANDARD && value.canConvert(QVariant::Double) )
        {
            // Cast to pack our float into FFUInt32
            float v = value.toFloat();
            ArgStruct.NewParameterValue.UIntValue = *(FFUInt32 *)&v;
            m_ffPluginMain(FF_SETPARAMETER, arg, m_ffInstanceID);
        }
        else if ( ffParameterType == FF_TYPE_TEXT && value.canConvert(QVariant::String) )
        {
            // Cast to string
            ArgStruct.NewParameterValue.PointerValue = value.toString().toAscii().data();
            m_ffPluginMain(FF_SETPARAMETER, arg, m_ffInstanceID);
        }
        else if ( ffParameterType == FF_TYPE_BOOLEAN && value.canConvert(QVariant::UInt) )
        {
            // Cast to unsigned int
            ArgStruct.NewParameterValue.UIntValue = value.toUInt();
            m_ffPluginMain(FF_SETPARAMETER, arg, m_ffInstanceID);
        }
        else
            return false;

        return true;
    }

};


FFGLPluginSource::FFGLPluginSource(QString filename, int w, int h, FFGLTextureStruct inputTexture)
    : _filename(filename), _initialized(false), _elapsedtime(0), _pause(false), _fboSize(w,h)
{
    // create plugin object (does not instanciate dll)
    _plugin = (FFGLPluginSourceInstance *) FFGLPluginInstance::New();
    if (!_plugin){
        qWarning()<< _filename << "| " << QObject::tr("FreeframeGL plugin could not be instanciated");
        FFGLPluginException().raise();
    }

    // load dll plugin
    if (_plugin->Load(filename.toUtf8())==FF_FAIL){
        qWarning()<< _filename << "| " << QObject::tr("FreeframeGL plugin could not be loaded");
        FFGLPluginException().raise();
    }

    // ensure functionnalities plugin
    if ( !_plugin->hasProcessOpenGLCapability()){
        qWarning()<< _filename << "| " << QObject::tr("Invalid FreeframeGL plugin: does not have OpenGL support.");
        FFGLPluginException().raise();
    }

    // fill in the information about this plugin
    info = _plugin->getInfo();
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

    qDebug() << _filename << "| " << QObject::tr("FreeframeGL plugin created") << " ("<< info["Name"].toString() <<", "<< _inputTexture.Width << _inputTexture.Height <<")";
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

FFGLTextureStruct FFGLPluginSource::FBOTextureStruct(){

    FFGLTextureStruct it;
    it.Handle = _fbo->texture();
    it.Width = _fbo->width();
    it.Height = _fbo->height();
    it.HardwareWidth = _fbo->width();
    it.HardwareHeight = _fbo->height();

    return it;
}

void FFGLPluginSource::update()
{
    if (initialize() && _fbo->bind())
    {
        // Safer to push all attribs because who knows what is done in the puglin!!
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
            qWarning()<< _filename << "| " << QObject::tr("FreeframeGL plugin could not process OpenGL. Probably missing an input texture.");
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
                qDebug()<< _filename << "| " << QObject::tr("FreeframeGL plugin initialized");

            }
            else {
                qWarning()<< _filename << "| " << QObject::tr("FreeframeGL plugin could not be initialized");
                FFGLPluginException().raise();
            }
        }
        else{
            qWarning()<< _filename << "| " << QObject::tr("FreeframeGL plugin could not create FBO");
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
        qWarning()<< _filename << "| " << QObject::tr("Parameter could not be set.");

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

// for getting debug messages from FFGL code
void FFDebugMessage(const char *msg)
{
    qDebug()<<msg;
}
