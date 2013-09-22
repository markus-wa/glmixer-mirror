#ifndef FFGLPLUGINSOURCESTACK_H
#define FFGLPLUGINSOURCESTACK_H

#include <QStack>
#include "FFGLPluginSource.h"

class FFGLPluginSourceStack: public QStack<FFGLPluginSource *>
{

public:
    FFGLPluginSourceStack() {}
    FFGLPluginSourceStack(FFGLPluginSource *);

    QStringList namesList();

    void pushNewPlugin(QString filename, int widht, int height, FFGLTextureStruct inputTexture);
    void removePlugin(FFGLPluginSource *p);

    void update();

private:

};

#endif // FFGLPLUGINSOURCESTACK_H
