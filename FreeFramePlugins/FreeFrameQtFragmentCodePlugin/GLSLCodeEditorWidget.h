#ifndef GLSLCodeEditorWidget_H
#define GLSLCodeEditorWidget_H

#include <QDialog>

namespace Ui {
class GLSLCodeEditorWidget;
}

class GLSLCodeEditorWidget : public QDialog
{
    Q_OBJECT
    
public:
    explicit GLSLCodeEditorWidget(QWidget *parent = 0);
    ~GLSLCodeEditorWidget();
    
private:
    Ui::GLSLCodeEditorWidget *ui;
};

#endif // GLSLCodeEditorWidget_H
