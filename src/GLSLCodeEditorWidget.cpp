#include "GLSLCodeEditorWidget.moc"
#include "ui_GLSLCodeEditorWidget.h"
#include "FFGLPluginSourceShadertoy.h"
//#include "FreeFrameQtGLSL.h"

#include <QDebug>


GLSLCodeEditorWidget::GLSLCodeEditorWidget(QWidget *parent) :
    QWidget(parent), ui(new Ui::GLSLCodeEditorWidget), _currentplugin(NULL)
{
    ui->setupUi(this);

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

    // change the header with the plugin code
    ui->headerText->append( _currentplugin->getHeaders() );

    // change the text editor to the plugin code
    ui->codeTextEdit->append( _currentplugin->getCode() );

    ui->codeTextEdit->setShiftLineNumber( ui->headerText->lineCount() );

    ui->nameEdit->setText( _currentplugin->getName() );
}


void GLSLCodeEditorWidget::unlinkPlugin()
{
    // unset current plugin
    _currentplugin = NULL;

    // clear all text
    ui->codeTextEdit->clear();
    ui->logText->clear();
}


void GLSLCodeEditorWidget::apply()
{
    _currentplugin->setCode(ui->codeTextEdit->toPlainText());

    QTimer::singleShot(200, this, SLOT(showLogs()));
}


void GLSLCodeEditorWidget::showLogs()
{
    ui->logText->clear();
    ui->logText->appendPlainText(_currentplugin->getLogs());
}
