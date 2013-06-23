#ifndef FFGLPLUGINSOURCESTACK_H
#define FFGLPLUGINSOURCESTACK_H

#include <QStack>
#include "FFGLPluginSource.h"

class FFGLPluginSourceStack: public QStack<FFGLPluginSource *>
{

public:
    FFGLPluginSourceStack();
    ~FFGLPluginSourceStack();


    void pushNewPlugin(QString filename, int widht, int height, FFGLTextureStruct inputTexture);

    void update();

private:

};

#endif // FFGLPLUGINSOURCESTACK_H
