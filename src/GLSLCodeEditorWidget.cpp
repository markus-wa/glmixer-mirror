#include "GLSLCodeEditorWidget.moc"
#include "ui_GLSLCodeEditorWidget.h"
//#include "GLSLCodeEditor.h"
//#include "FreeFrameQtGLSL.h"

#include <QDebug>


GLSLCodeEditorWidget::GLSLCodeEditorWidget(QWidget *parent) :
    QWidget(parent), ui(new Ui::GLSLCodeEditorWidget) //, _program(program)
{
    ui->setupUi(this);

//    ui->codeTextEdit->setFontFamily("monospace");
//    ui->headerText->setFontFamily("monospace");
//    ui->codeTextEdit->setTabStopWidth(30);
//    GlslSyntaxHighlighter *highlighter = new GlslSyntaxHighlighter(ui->codeTextEdit->document());
//    highlighter = new GlslSyntaxHighlighter(ui->headerText->document());

}

GLSLCodeEditorWidget::~GLSLCodeEditorWidget()
{
    delete ui;
}

void GLSLCodeEditorWidget::applyCode()
{
    ui->logText->clear();
//    _program->setFragmentProgramCode( ui->codeTextEdit->toPlainText().toLatin1().data() );
}


void GLSLCodeEditorWidget::setCode(const char *code)
{
    ui->codeTextEdit->clear();
    ui->codeTextEdit->append(QString::fromLatin1(code));
}


void GLSLCodeEditorWidget::setHeader(const char *code)
{
    ui->headerText->clear();
    ui->headerText->append(QString::fromLatin1(code));
}

void GLSLCodeEditorWidget::showLogs(const char *logstring)
{
    ui->logText->appendPlainText(QString::fromLatin1(logstring));
}
