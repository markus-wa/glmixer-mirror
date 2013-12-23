#include <QDebug>
#include "ui_CodeEditorWidget.h"
#include "GLSLCodeEditorWidget.moc"

#include "FreeFrameQtGLSL.h"

GLSLCodeEditorWidget::GLSLCodeEditorWidget(FreeFrameQtGLSL *program, QWidget *parent) :
    QWidget(parent), ui(new Ui::CodeEditorWidget), _program(program)
{
    ui->setupUi(this);



}

GLSLCodeEditorWidget::~GLSLCodeEditorWidget()
{
    delete ui;
}

void GLSLCodeEditorWidget::applyCode()
{
    ui->logText->clear();
    _program->setFragmentProgramCode( ui->codeTextEdit->toPlainText().toLatin1().data() );
}


void GLSLCodeEditorWidget::setCode(const char *code)
{
    ui->codeTextEdit->clear();
    ui->codeTextEdit->setFontFamily("courier");
    ui->codeTextEdit->append(QString::fromLatin1(code));
}

void GLSLCodeEditorWidget::showLogs(const char *logstring)
{
    ui->logText->appendPlainText(QString::fromLatin1(logstring));
}
