#include "GLSLCodeEditorWidget.moc"
#include "ui_GLSLCodeEditorWidget.h"
#include "FFGLPluginSourceShadertoy.h"

#include <QDebug>


GLSLCodeEditorWidget::GLSLCodeEditorWidget(QWidget *parent) :
    QWidget(parent), ui(new Ui::GLSLCodeEditorWidget), _currentplugin(NULL)
{
    ui->setupUi(this);

    // connect buttons
    connect(ui->helpButton, SIGNAL(clicked()), this, SLOT(showHelp()));
    connect(ui->pasteButton, SIGNAL(clicked()), this, SLOT(pasteCode()));
    connect(ui->loadButton, SIGNAL(clicked()), this, SLOT(loadCode()));
    connect(ui->restoreButton, SIGNAL(clicked()), this, SLOT(restoreCode()));

    // header area is read only
    ui->headerText->setReadOnly(true);

    // collapse logs by default
    ui->splitter->setCollapsible(1, true);
    QList<int> sizes;
    sizes << (ui->splitter->sizes()[0] + ui->splitter->sizes()[1]) << 0;
    ui->splitter->setSizes( sizes );
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
        ui->headerText->setCode( _currentplugin->getHeaders() );

        // change the text editor to the plugin code
        ui->codeTextEdit->setShiftLineNumber( ui->headerText->lineCount() );
        ui->codeTextEdit->setCode( _currentplugin->getCode() );

        // set name field
        QVariantHash plugininfo = _currentplugin->getInfo();
        ui->nameEdit->setText( plugininfo["Name"].toString() );
        QString about = "By ";
#ifdef Q_OS_WIN
    about.append(getenv("USERNAME"));
#else
    about.append(getenv("USER"));
#endif
        ui->aboutEdit->setText( about );
        ui->descriptionEdit->setText( plugininfo["Description"].toString() );

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
    _currentplugin->setCode(ui->codeTextEdit->code());
    _currentplugin->setName(ui->nameEdit->text());
    _currentplugin->setAbout(ui->aboutEdit->text());
    _currentplugin->setDescription(ui->descriptionEdit->text());

    emit ( applied() );

    // because the plugin needs 1 frame to compile the GLSL
    // we shall call the display of the logs later
    QTimer::singleShot(100, this, SLOT(showLogs()));
}


void GLSLCodeEditorWidget::showLogs()
{
    // clear and replace the logs
    ui->logText->clear();
    QString logs = _currentplugin->getLogs();
    ui->logText->appendPlainText(logs);

    // if log contains something
    if (!logs.isEmpty()) {

        // if there is an error, signal it
        if (logs.contains("error"))
            emit ( error(_currentplugin) );

        // if log area is collapsed, show it
        if ( ui->splitter->sizes()[1] == 0) {
            QList<int> sizes;
            sizes << (ui->splitter->sizes()[0] * 3 / 4) << (ui->splitter->sizes()[0] / 4);
            ui->splitter->setSizes( sizes );
        }
    }
}


void GLSLCodeEditorWidget::showHelp()
{
    QDesktopServices::openUrl(QUrl("https://www.shadertoy.com", QUrl::TolerantMode));
}

void GLSLCodeEditorWidget::pasteCode()
{
    ui->codeTextEdit->setCode( QApplication::clipboard()->text() );
}

void GLSLCodeEditorWidget::restoreCode()
{
    ui->codeTextEdit->setCode( _currentplugin->getDefaultCode() );
}

void GLSLCodeEditorWidget::loadCode()
{
    static QDir dir(QDir::home());
    QString fileName = QFileDialog::getOpenFileName(this, tr("Open ShaderToy GLSL fragment shader code"), dir.absolutePath(), tr("GLSL code (*.glsl);;Text file (*.txt);;Any file (*.*)") );

    // check validity of file
    QFileInfo fileInfo(fileName);
    if (fileInfo.isFile() && fileInfo.isReadable()) {
        // open file
        QFile fileContent( fileInfo.absoluteFilePath());
        if (!fileContent.open(QIODevice::ReadOnly | QIODevice::Text))
            return;
        QTextStream tx(&fileContent);
        ui->codeTextEdit->setCode( QTextStream(&fileContent).readAll() );
    }

}
