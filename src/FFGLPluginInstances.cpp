#include "FFGLPluginInstances.h"

#include <QDebug>

#ifdef Q_OS_WIN

#include <Win32/WinPluginInstance.cpp>
#define CLASSPLUGININSTANCE WinPluginInstance

#else
#ifdef Q_OS_MAC

#include <OSX/OSXPluginInstance.cpp>
#define CLASSPLUGININSTANCE OSXPluginInstance

#else

#include <Linux/LinuxPluginInstance.cpp>
#define CLASSPLUGININSTANCE LinuxPluginInstance

#endif
#endif


// Platform dependent class with specific implementation of the
//  FFGLPluginInstanceFreeframe functions
class FFGLPluginInstanceFreeframePlatform : public CLASSPLUGININSTANCE, public FFGLPluginInstanceFreeframe  {

public:
    FFGLPluginInstanceFreeframePlatform() {}

    virtual bool hasProcessOpenGLCapability();

    QVariantHash getInfo();
    QVariantHash getExtendedInfo();
    QVariantHash getParametersDefaults();
    QVariantHash getParameters();

    unsigned int getNumParameters();
    bool setParameter(QString paramName, QVariant value);
    bool setParameter(unsigned int paramNum, QVariant value);

};

FFGLPluginInstance *FFGLPluginInstanceFreeframe::New()
{
  return new FFGLPluginInstanceFreeframePlatform();
}



bool FFGLPluginInstanceFreeframePlatform::hasProcessOpenGLCapability() {

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


QVariantHash FFGLPluginInstanceFreeframePlatform::getInfo() {
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
        mapinfo.insert("Type", plugininfo->PluginType == 1 ? QString("Source") : QString("Effect") );
    }
    return mapinfo;
}

QVariantHash FFGLPluginInstanceFreeframePlatform::getExtendedInfo() {
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
        mapinfo.insert("Version", QString("%1.%2").arg(plugininfo->PluginMajorVersion).arg(plugininfo->PluginMinorVersion));
        mapinfo.insert("Description", plugininfo->Description);
        mapinfo.insert("About", plugininfo->About);
    }
    return mapinfo;
}

QVariantHash FFGLPluginInstanceFreeframePlatform::getParametersDefaults() {
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

    return params;
}

QVariantHash FFGLPluginInstanceFreeframePlatform::getParameters()
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

    return params;
}

unsigned int FFGLPluginInstanceFreeframePlatform::getNumParameters() {
    return m_numParameters;
}

bool FFGLPluginInstanceFreeframePlatform::setParameter(QString paramName, QVariant value)
{
    // find the parameter with that name
    for (unsigned int i=0; i < (unsigned int) m_numParameters; ++i){
        if ( paramName.compare(GetParameterName(i), Qt::CaseInsensitive) == 0 )
            return setParameter(i, value);
    }

    return false;
}

bool FFGLPluginInstanceFreeframePlatform::setParameter(unsigned int paramNum, QVariant value)
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



#ifdef Q_OS_WIN
typedef __declspec(dllimport) void (__stdcall *_FuncPtrSetCode)(const char *, FFInstanceID);
typedef __declspec(dllimport) (char *) (__stdcall *_FuncPtrGetCode)(FFInstanceID);
#else
typedef void (*_FuncPtrSetCode)(const char *, FFInstanceID);
typedef char *(*_FuncPtrGetCode)(FFInstanceID);
#endif


class FFGLPluginInstanceShadertoyPlaftorm : public FFGLPluginInstanceFreeframePlatform, public FFGLPluginInstanceShadertoy {

public:

    FFGLPluginInstanceShadertoyPlaftorm() : m_ffPluginFunctionSetCode(NULL), m_ffPluginFunctionGetCode(NULL) {}

    bool declareShadertoyFunctions();
    void setCode(const char *code);
    char *getCode();

    _FuncPtrSetCode m_ffPluginFunctionSetCode;
    _FuncPtrGetCode m_ffPluginFunctionGetCode;
};

FFGLPluginInstance *FFGLPluginInstanceShadertoy::New()
{
  return new FFGLPluginInstanceShadertoyPlaftorm();
}


void FFGLPluginInstanceShadertoyPlaftorm::setCode(const char *code)
{
    if (m_ffPluginFunctionGetCode!=NULL && m_ffInstanceID!=INVALIDINSTANCE)
        m_ffPluginFunctionSetCode(code, m_ffInstanceID);

}

char *FFGLPluginInstanceShadertoyPlaftorm::getCode()
{
    if (m_ffPluginFunctionGetCode==NULL || m_ffInstanceID==INVALIDINSTANCE)
        return 0;

    return m_ffPluginFunctionGetCode(m_ffInstanceID);
}

// Platform dependent implementation of the DLL function pointer assignment

#ifdef Q_OS_WIN

void FFGLPluginInstanceShadertoyPlaftorm::declareShadertoyFunctions()
{



}

#else
#ifdef Q_OS_MAC

void FFGLPluginInstanceShadertoyPlaftorm::declareShadertoyFunctions()
{



}

#else

bool FFGLPluginInstanceShadertoyPlaftorm::declareShadertoyFunctions()
{
    if (plugin_handle == NULL)
        return false;

    m_ffPluginFunctionSetCode = (_FuncPtrSetCode) dlsym(plugin_handle, "setFragmentProgramCode");
    m_ffPluginFunctionGetCode = (_FuncPtrGetCode) dlsym(plugin_handle, "getFragmentProgramCode");

    return ( m_ffPluginFunctionSetCode != NULL && m_ffPluginFunctionGetCode != NULL);
}

#endif
#endif
