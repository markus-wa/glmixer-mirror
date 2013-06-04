#include <QDebug>
#include <QElapsedTimer>

#include "FFGLPluginSource.h"
#include "Timer.h"
#include "FFGLFBO.h"
#include "FFGLPluginInstance.h"


#include <QGLFramebufferObject>

#include <GL/glu.h>


FFGLPluginSource::FFGLPluginSource(QString filename, unsigned int textureIndex, int w, int h)
    : _filename(filename), _initialized(false)
{
    //load plugin (does not instantiate!)
    _plugin = FFGLPluginInstance::New();
    CHECK_PTR_EXCEPTION(_plugin)

    if (_plugin->Load(filename.toUtf8())==FF_FAIL)
        FFGLPluginException().raise();

    _inputTexture.Handle = (GLuint) textureIndex;
    _inputTexture.Width = w;
    _inputTexture.Height = h;
    _inputTexture.HardwareWidth = w;
    _inputTexture.HardwareHeight = h;

}

FFGLPluginSource::~FFGLPluginSource()
{
    // FFGL exit sequence
    _plugin->DeInstantiateGL();
    _plugin->Unload();

    // deletes
    delete _plugin;
    delete _fbo;
}

void FFGLPluginSource::update()
{
    if (!initialize()) {
        qDebug()<< "FFGL plugin cannot initialize (" << _filename <<")";
        return;
    }

    if (_fbo->bind())
    {
        glPushAttrib(GL_COLOR_BUFFER_BIT | GL_VIEWPORT_BIT);
//        glPushAttrib(GL_ALL_ATTRIB_BITS);

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
        _plugin->SetTime(timer->GetElapsedTime());

        _plugin->SetFloatParameter(0, 0.5);

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
        if ( _plugin->CallProcessOpenGL(processStruct) != FF_SUCCESS )
            qDebug()<< "FFGL plugin call failed (" << _filename <<")";

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
    }
}


void FFGLPluginSource::bind() const
{
    glActiveTexture(GL_TEXTURE0);
    // bind the FBO texture (instead of the source texture)
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
                timer = Timer::New();

                _initialized = true;
                qDebug()<< "FFGL plugin initialized (" << _filename <<")";
                qDebug()<< "FFGLPlugin is " << _fbo->width()<< _fbo->height();

            }
            else
                qDebug()<< "FFGL plugin cannot instanciate GL (" << _filename <<")";

        }
        else
            qDebug()<< "FFGL plugin cannot create FBO (" << _filename <<")";

    }

    return _initialized;
}

void FFDebugMessage(const char *msg)
{
    qDebug()<<msg;
}
