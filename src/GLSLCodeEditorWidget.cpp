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

    // update
    updateFields();
}

void GLSLCodeEditorWidget::updateFields()
{
    if ( _currentplugin )
    {
        // because the plugin takes 1 frame to initialize
        // we should recall this function later
        if ( !_currentplugin->isinitialized() )
            QTimer::singleShot(50, this, SLOT(updateFields()));

        // change the header with the plugin code
        ui->headerText->setText( _currentplugin->getHeaders() );

        // change the text editor to the plugin code
        ui->codeTextEdit->setText( _currentplugin->getCode() );
        ui->codeTextEdit->setShiftLineNumber( ui->headerText->lineCount() );

        // set name field
        ui->nameEdit->setText( _currentplugin->getName() );

    } else
    {
        // clear all text
        ui->codeTextEdit->clear();
        ui->nameEdit->clear();
        ui->logText->clear();
    }

    update();
}


void GLSLCodeEditorWidget::unlinkPlugin()
{
    // unset current plugin
    _currentplugin = NULL;

    // update
    updateFields();
}


void GLSLCodeEditorWidget::apply()
{
    _currentplugin->setCode(ui->codeTextEdit->toPlainText());
    _currentplugin->setName(ui->nameEdit->text());

    // because the plugin needs 1 frame to compile the GLSL
    // we shall call the display of the logs later
    QTimer::singleShot(100, this, SLOT(showLogs()));
}


void GLSLCodeEditorWidget::showLogs()
{
    ui->logText->clear();
    ui->logText->appendPlainText(_currentplugin->getLogs());
}
