#include "GLSLCodeEditorWidget.moc"
#include "ui_GLSLCodeEditorWidget.h"
#include "FFGLPluginSourceShadertoy.h"
//#include "FreeFrameQtGLSL.h"

#include <QDebug>


GLSLCodeEditorWidget::GLSLCodeEditorWidget(QWidget *parent) :
    QWidget(parent), ui(new Ui::GLSLCodeEditorWidget), _currentplugin(NULL)
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

void GLSLCodeEditorWidget::linkPlugin(FFGLPluginSourceShadertoy *plugin)
{
    // unlink previous if already linked
    if (_currentplugin != plugin)
        unlinkPlugin();

    // remember which plugin is current
    _currentplugin = plugin;

    // change the text editor to the plugin code
    ui->codeTextEdit->append(_currentplugin->getCode());
}


void GLSLCodeEditorWidget::unlinkPlugin()
{
    // unset current plugin
    _currentplugin = NULL;

    // clear all text
    ui->codeTextEdit->clear();
    ui->logText->clear();
}


void GLSLCodeEditorWidget::applyCode()
{
    ui->logText->clear();

    _currentplugin->setCode(ui->codeTextEdit->toPlainText());

//    -

}


//void GLSLCodeEditorWidget::setCode(const char *code)
//{
//    ui->codeTextEdit->clear();
//    ui->codeTextEdit->append(QString::fromLatin1(code));
//}


//void GLSLCodeEditorWidget::setHeader(const char *code)
//{
//    ui->headerText->clear();
//    ui->headerText->append(QString::fromLatin1(code));
//}

//void GLSLCodeEditorWidget::showLogs(const char *logstring)
//{
//    ui->logText->appendPlainText(QString::fromLatin1(logstring));
//}
