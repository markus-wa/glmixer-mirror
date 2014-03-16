#ifndef FFGLPLUGINSOURCEFACTORY_H
#define FFGLPLUGINSOURCEFACTORY_H

#include "FFGLPluginSource.h"

class FFGLPluginSourceFactory : public FFGLPluginSource
{
public:
    FFGLPluginSourceFactory(QString filename, int w, int h, FFGLTextureStruct inputTexture);


};

#endif // FFGLPLUGINSOURCEFACTORY_H
