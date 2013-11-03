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

    QString code;
    code.append("varying vec2 texc;\nuniform vec2 texturesize;\nuniform sampler2D texture;\nvoid main(void){\n");
    code.append(ui->codeTextEdit->toPlainText());
    code.append("\n\n}");

    _program->setFragmentProgramCode( qPrintable(code));

}


void GLSLCodeEditorWidget::showLogs(char *logstring)
{
    ui->logText->appendPlainText(QString(logstring));
}
