#include "FFGLPluginSource.h"
#include "FFGLPluginSourceStack.h"


FFGLPluginSourceStack::FFGLPluginSourceStack( FFGLPluginSource *ffgl_plugin )
{
    this->push(ffgl_plugin);
}

void FFGLPluginSourceStack::pushNewPlugin(QString filename, int width, int height, unsigned int inputTexture){

    if (filename.isEmpty() || !QFileInfo(filename).isFile()) {
        qCritical()<< filename << QChar(124).toLatin1()<< QObject::tr("FreeFrameGL plugin given an invalid file name");
        return;
    }

    FFGLTextureStruct it;

    // descriptor for the source texture, used also to store size
    if ( ! isEmpty() )
        // chaining the plugins means providing the texture index of the previous
        it = top()->getOutputTextureStruct();
    else {
        it.Handle = (GLuint) inputTexture;
        it.Width = width;
        it.Height = height;
        it.HardwareWidth = width;
        it.HardwareHeight = height;
    }

    // create the plugin itself
    try {
        FFGLPluginSource *ffgl_plugin = NULL;

        // create new plugin with this file
        ffgl_plugin = new FFGLPluginSource(filename, width, height, it);

        // in case of success, add it to the stack
        this->push(ffgl_plugin);
    }
    catch (FFGLPluginException &e)  {
        qCritical() << filename << QChar(124).toLatin1()<< e.message();
    }
    catch (...)  {
        qCritical() << filename << QChar(124).toLatin1()<< QObject::tr("Unknown error in FreeframeGL plugin");
    }

}

void FFGLPluginSourceStack::removePlugin(FFGLPluginSource *p)
{
    // finds the index of the plugin in the list
    int id = indexOf(p);

    // if the plugin *p is in the stack
    if (id > -1) {

        // if there is a plugin after *p in the stack
        // then we have to update the input texture of the one after *p
        if (id+1 < count())  {
            // if we remove the first in the stack
            if ( id == 0 )
                // set input texture of the next to be the one of the current first
                at(1)->setInputTextureStruct(p->getInputTextureStruct());
            else
                // chain the plugin before with the plugin after
                at(id+1)->setInputTextureStruct(at(id-1)->getOutputTextureStruct());
        }

        // now we can remove the plugin from the stack, and delete it
        remove(id);
        delete p;
    }

}

void FFGLPluginSourceStack::clear(){
    while (!isEmpty())
        delete pop();
    QStack<FFGLPluginSource *>::clear();
}

void FFGLPluginSourceStack::bind() const{
    if (!isEmpty())
        top()->bind();
}

void FFGLPluginSourceStack::update(){

    // update in the staking order, from original to top
    for (FFGLPluginSourceStack::iterator it = begin(); it != end(); ) {
        try {
            (*it)->update();
            ++it;
        }
        catch (FFGLPluginException &e) {
            qCritical() << (*it)->fileName() << QChar(124).toLatin1() <<  e.message() << QObject::tr("\nThe plugin was removed after a crash");
            it = erase(it);
        }
    }

}


QStringList FFGLPluginSourceStack::namesList()
{
    QStringList pluginlist;
    for (FFGLPluginSourceStack::iterator it = begin(); it != end(); ++it )
        pluginlist.append( QFileInfo((*it)->fileName()).baseName() );

    return pluginlist;
}
