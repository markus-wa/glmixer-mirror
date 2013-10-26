#ifndef GLSLCodeEditorWidget_H
#define GLSLCodeEditorWidget_H

#include <QWidget>

namespace Ui {
class CodeEditorWidget;
}

class GLSLCodeEditorWidget : public QWidget
{
    Q_OBJECT
    
public:
    GLSLCodeEditorWidget(QWidget *parent = 0);
    ~GLSLCodeEditorWidget();
    
private:
    Ui::CodeEditorWidget *ui;
};

#endif // GLSLCodeEditorWidget_H
