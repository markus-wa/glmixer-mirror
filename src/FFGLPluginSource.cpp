#include <QDebug>

#include "FFGLPluginSource.h"
#include "FFGLPluginInstance.h"

#include <QGLFramebufferObject>


FFGLPluginSource::FFGLPluginSource(QString filename, unsigned int textureIndex, int w, int h)
    : _filename(filename), _initialized(false)
{
    // create plugin object (does not instanciate dll)
    _plugin = FFGLPluginInstance::New();
    if (!_plugin){
        qWarning()<< "FreeframeGL plugin could be instanciated (" << _filename <<")";
        FFGLPluginException().raise();
    }

    // load dll plugin
    if (_plugin->Load(filename.toUtf8())==FF_FAIL){
        qWarning()<< "FreeframeGL plugin could not be loaded (" << _filename <<")";
        FFGLPluginException().raise();
    }

    // descriptor for the source texture, used also to store size
    _inputTexture.Handle = (GLuint) textureIndex;
    _inputTexture.Width = w;
    _inputTexture.Height = h;
    _inputTexture.HardwareWidth = w;
    _inputTexture.HardwareHeight = h;
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

        //tell plugin about the current time
        _plugin->SetTime(((double) timer.elapsed()) / 1000.0 );

        // TEMPORARY : initialize all parameters to 0.5
        for(int i = 0; !QString(_plugin->GetParameterName(i)).isEmpty(); ++i )
            _plugin->SetFloatParameter(i, 0.5);

        //create the array of OpenGLTextureStruct * to be passed
        //to the plugin
        FFGLTextureStruct *inputTextures[1];
        inputTextures[0] = &_inputTexture;

        //prepare the structure used to call
        //the plugin's ProcessOpenGL method
        ProcessOpenGLStructTag processStruct;

        //provide the 1 input texture we allocated above
        processStruct.numInputTextures = 1;
        processStruct.inputTextures = inputTextures;

        //specify our FBO's handle in the processOpenGLStruct
        processStruct.HostFBO = _fbo->handle();

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
            qWarning()<< "FreeframeGL plugin could not process OpenGL (" << _filename <<")";
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
        _fbo = new QGLFramebufferObject(QSize(_inputTexture.Width, _inputTexture.Height));
        if (_fbo) {

            FFGLViewportStruct _fboViewport;
            _fboViewport.x = 0;
            _fboViewport.y = 0;
            _fboViewport.width = _fbo->width();
            _fboViewport.height = _fbo->height();

            //instantiate the plugin passing a viewport that matches
            //the FBO (plugin is rendered into our FBO)
            if ( _plugin->InstantiateGL( &_fboViewport ) == FF_SUCCESS ) {

                // one timer per FFGLPluginSource
                timer.start();

                // remember successful initialization
                _initialized = true;
                qDebug()<< "FreeframeGL plugin initialized (" << _filename <<")";

            }
            else {
                qWarning()<< "FreeframeGL plugin could not be initialized (" << _filename <<")";
                FFGLPluginException().raise();
            }
        }
        else{
            qWarning()<< "FreeframeGL plugin could not create FBO (" << _filename <<")";
            FFGLPluginException().raise();
        }
    }

    return _initialized;
}

// for getting debug messages from FFGL code
void FFDebugMessage(const char *msg)
{
    qDebug()<<msg;
}
