#ifndef GLSLCodeEditorWidget_H
#define GLSLCodeEditorWidget_H

#include <QWidget>
#include <QTextEdit>

namespace Ui {
class CodeEditorWidget;
}

class FreeFrameQtGLSL;

class GLSLCodeEditorWidget : public QWidget
{
    Q_OBJECT
    
public:
    GLSLCodeEditorWidget(FreeFrameQtGLSL *program, QWidget *parent = 0);
    ~GLSLCodeEditorWidget();

public Q_SLOTS:
    void applyCode();
    void showLogs(const char *logstring);

    void setHeader(const char *header);
    void setCode(const char *code);

private:
    Ui::CodeEditorWidget *ui;

    FreeFrameQtGLSL *_program;
};



#endif // GLSLCodeEditorWidget_H
