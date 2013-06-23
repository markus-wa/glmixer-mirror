#include <QDebug>

#include "FFGLPluginSource.h"
#include "FFGLPluginInstance.h"

#include <QGLFramebufferObject>


FFGLPluginSource::FFGLPluginSource(QString filename, int w, int h, FFGLTextureStruct inputTexture)
    : _filename(filename), _initialized(false), _elapsedtime(0), _pause(false), _fboSize(w,h)
{
    // create plugin object (does not instanciate dll)
    _plugin = FFGLPluginInstance::New();
    if (!_plugin){
        qWarning()<< _filename << "| FreeframeGL plugin could be instanciated.";
        FFGLPluginException().raise();
    }

    // load dll plugin
    if (_plugin->Load(filename.toUtf8())==FF_FAIL){
        qWarning()<< _filename << "| FreeframeGL plugin could not be loaded.";
        FFGLPluginException().raise();
    }

    // descriptor for the source texture, used also to store size
    _inputTexture.Handle = inputTexture.Handle;
    _inputTexture.Width = inputTexture.Width;
    _inputTexture.Height = inputTexture.Height;
    _inputTexture.HardwareWidth = inputTexture.HardwareWidth;
    _inputTexture.HardwareHeight = inputTexture.HardwareHeight;

   qDebug() << _filename << "| FreeframeGL plugin created ("<< _inputTexture.Handle <<", "<< _inputTexture.Width << _inputTexture.Height <<")";
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

        // TEMPORARY : initialize all parameters to 0.5
        for(int i = 0; !QString(_plugin->GetParameterName(i)).isEmpty(); ++i )
            _plugin->SetFloatParameter(i, 0.5);

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
            qWarning()<< _filename << "| FreeframeGL plugin could not process OpenGL.";
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

                // remember successful initialization
                _initialized = true;
                qDebug()<< _filename << "| FreeframeGL plugin initialized.";

            }
            else {
                qWarning()<< _filename << "| FreeframeGL plugin could not be initialized.";
                FFGLPluginException().raise();
            }
        }
        else{
            qWarning()<< _filename << "| FreeframeGL plugin could not create FBO.";
            FFGLPluginException().raise();
        }
    }

    return _initialized;
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
