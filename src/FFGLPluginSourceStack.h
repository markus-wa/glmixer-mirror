#ifndef FFGLPLUGINSOURCESTACK_H
#define FFGLPLUGINSOURCESTACK_H

#include <QStack>

class FFGLPluginSource;

class FFGLPluginSourceStack: public QStack<FFGLPluginSource *>
{

public:
    FFGLPluginSourceStack() {}
    FFGLPluginSourceStack(FFGLPluginSource *);

    QStringList namesList();

    FFGLPluginSource *pushNewPlugin(QString filename, int widht, int height, unsigned int inputTexture);
    FFGLPluginSource *pushNewPlugin(int widht, int height, unsigned int inputTexture);
    void removePlugin(FFGLPluginSource *p);

    void update();
    void bind() const;
    void clear();
    void play(bool on);

private:

};

#endif // FFGLPLUGINSOURCESTACK_H
