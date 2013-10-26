
#include "ui_CodeEditorWidget.h"
#include "GLSLCodeEditorWidget.moc"

GLSLCodeEditorWidget::GLSLCodeEditorWidget(QWidget *parent) :
    QWidget(parent), ui(new Ui::CodeEditorWidget)
{
    ui->setupUi(this);
}

GLSLCodeEditorWidget::~GLSLCodeEditorWidget()
{
    delete ui;
}
