
#include "ui_GLSLCodeEditorWidget.h"
#include "GLSLCodeEditorWidget.h"

GLSLCodeEditorWidget::GLSLCodeEditorWidget(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::GLSLCodeEditorWidget)
{
    ui->setupUi(this);
}

GLSLCodeEditorWidget::~GLSLCodeEditorWidget()
{
    delete ui;
}
