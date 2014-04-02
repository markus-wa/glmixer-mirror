#ifndef GLSLCodeEditorWidget_H
#define GLSLCodeEditorWidget_H

#include <QtGui>

namespace Ui {
class GLSLCodeEditorWidget;
}

class FFGLPluginSource;
class FFGLPluginSourceShadertoy;

class GLSLCodeEditorWidget : public QWidget
{
    Q_OBJECT

public:
    GLSLCodeEditorWidget(QWidget *parent = 0);
    ~GLSLCodeEditorWidget();

    // associate / dissociate plugin to the GUI
    void linkPlugin(FFGLPluginSourceShadertoy *plugin);
    void unlinkPlugin();

public Q_SLOTS:

    // management of code
    void apply();
    void showLogs();
    void updateFields();

    // actions from buttons
    void showHelp();
    void pasteCode();
    void loadCode();
    void restoreCode();

Q_SIGNALS:
    // emited when code is applied
    void applied();
    // emited when error detected in compilation of plugin
    void error(FFGLPluginSource *plugin);

private:
    Ui::GLSLCodeEditorWidget *ui;
    FFGLPluginSourceShadertoy *_currentplugin;
};



#endif // GLSLCodeEditorWidget_H
