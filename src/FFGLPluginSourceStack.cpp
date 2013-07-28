#include "FFGLPluginSourceStack.h"

FFGLPluginSourceStack::FFGLPluginSourceStack()
{
}

FFGLPluginSourceStack::~FFGLPluginSourceStack()
{
    // delete all plugins of the stack
    while (!isEmpty())
        delete pop();
}


void FFGLPluginSourceStack::pushNewPlugin(QString filename, int width, int height, FFGLTextureStruct inputTexture){

    if (filename.isEmpty() || !QFileInfo(filename).isFile()) {
        qCritical() << QObject::tr("FreeFrameGL plugin given an invalid file name") << " ('"<< filename <<"')";
        return;
    }

    // chaining the plugins means providing the texture index of the previous
    if ( ! isEmpty() )
        inputTexture = top()->FBOTextureStruct();

    // create the plugin itself
    try {
        FFGLPluginSource *ffgl_plugin = NULL;
        // create new plugin with this file
        ffgl_plugin = new FFGLPluginSource(filename, width, height, inputTexture);

        // in case of success, add it to the stack
        this->push(ffgl_plugin);
    }
    catch (FFGLPluginException &e)  {
        qCritical() << e.message();
    }
    catch (...)  {
        qCritical() << QObject::tr("Unknown error in FreeframeGL plugin");
    }

}


void FFGLPluginSourceStack::update(){

    // update in the staking order, from original to top
    for (QStack::iterator it = begin(); it != end(); ) {
        try {
            (*it)->update();
            ++it;
        }
        catch (FFGLPluginException &e) {
            erase(it);
            qCritical() <<  e.message() << QObject::tr("\nThe plugin was removed");
        }
    }

}
