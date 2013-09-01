#ifndef FFGLPLUGINBROWSER_H
#define FFGLPLUGINBROWSER_H


#include "PropertyBrowser.h"
#include "FFGLPluginSourceStack.h"

class FFGLPluginBrowser : public PropertyBrowser
{
    Q_OBJECT

public:
    FFGLPluginBrowser(QWidget *parent = 0);

public slots:

    void clear();
    void showProperties(FFGLPluginSourceStack plugins);

    // Update the plugin when an action is performed on a property in the browser
    // This concerns every properties editable in the browser
    void valueChanged(QtProperty *property, bool value);
    void valueChanged(QtProperty *property, int value);
    void valueChanged(QtProperty *property, double value);
    void valueChanged(QtProperty *property, const QString &value);

    void resetAll();
    void defaultValue();

//Q_SIGNALS:
   // void changed(FFGLPluginSource *plugin);

private:

    // implementation methods
    bool canChange();

    // the link with plugin
    FFGLPluginSourceStack currentStack;
    QMap<QtProperty *, QPair<FFGLPluginSource *, int> > propertyToPluginParameter;

    QtProperty *createPluginPropertyTree(FFGLPluginSource *plugin);

};

#endif // FFGLPLUGINBROWSER_H
