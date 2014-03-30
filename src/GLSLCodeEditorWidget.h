#ifndef GLSLCodeEditorWidget_H
#define GLSLCodeEditorWidget_H

#include <QtGui>

namespace Ui {
class GLSLCodeEditorWidget;
}

class FFGLPluginSourceShadertoy;

class GLSLCodeEditorWidget : public QWidget
{
    Q_OBJECT

public:
    GLSLCodeEditorWidget(QWidget *parent = 0);
    ~GLSLCodeEditorWidget();

    void linkPlugin(FFGLPluginSourceShadertoy *plugin);
    void unlinkPlugin();

public Q_SLOTS:
    void apply();
    void showLogs();
    void updateFields();

    // actions from buttons
    void showHelp();
    void pasteCode();
    void loadCode();
    void restoreCode();

private:
    Ui::GLSLCodeEditorWidget *ui;
    FFGLPluginSourceShadertoy *_currentplugin;
};



#endif // GLSLCodeEditorWidget_H
