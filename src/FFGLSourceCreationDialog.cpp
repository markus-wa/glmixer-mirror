#include "FFGLSourceCreationDialog.moc"
#include "ui_FFGLSourceCreationDialog.h"

#include <QFileInfo>

#include "SourceDisplayWidget.h"
#include "FFGLPluginSource.h"
#include "FFGLPluginBrowser.h"
#include "FFGLSource.h"
#include "glmixer.h"

FFGLSourceCreationDialog::FFGLSourceCreationDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::FFGLSourceCreationDialog), s(NULL)
{
    // create the preview with empty source
//    preview = new SourceDisplayWidget(this);

    // setup the user interface
    ui->setupUi(this);
    ui->labelWarninEffect->setVisible(false);

    // add the preview widget in the ui
    /*
    ui->FFGLSourceCreationLayout->insertWidget(0, preview);
    preview->setMinimumHeight(120);
    preview->setSizePolicy(QSizePolicy::Preferred,QSizePolicy::Preferred);*/

    // Setup the FFGL plugin property browser
    pluginBrowser = new FFGLPluginBrowser(this);
    ui->PropertiesLayout->insertWidget( ui->PropertiesLayout->count(),  pluginBrowser);
    pluginBrowser->setSizePolicy(QSizePolicy::Expanding,QSizePolicy::Expanding);
    pluginBrowser->switchToGroupView();

}


FFGLSourceCreationDialog::~FFGLSourceCreationDialog()
{
//    delete preview;
    delete pluginBrowser;
    delete ui;

}


void FFGLSourceCreationDialog::showEvent(QShowEvent *e){

    updateSourcePreview();

    QWidget::showEvent(e);
}

void FFGLSourceCreationDialog::done(int r){

    ui->preview->setSource(0);

    if (s) {
        // remember plugin config before deleting the source
        pluginConfiguration = s->freeframeGLPlugin()->getConfiguration();

        // delete source
        delete s;
        s = NULL;
    }
    else
        pluginConfiguration = QDomElement();

    QDialog::done(r);
}



void FFGLSourceCreationDialog::updateSourcePreview(QDomElement config){

    if(s) {
        ui->labelWarninEffect->setVisible(false);
        // remove source from preview: this deletes the texture in the preview
        ui->preview->setSource(0);
        // clear plugin browser
        pluginBrowser->clear();
        delete pluginBrowserStack;
        // delete the source:
        delete s;
    }

    s = NULL;

    if (QFileInfo(_filename).isFile()) {

        GLuint tex = ui->preview->getNewTextureIndex();
        try {
            // create a new source with a new texture index and the new parameters
            s = new FFGLSource(_filename, tex, 0, ui->widthSpinBox->value(), ui->heightSpinBox->value());

            if (!config.isNull())
                s->freeframeGLPlugin()->setConfiguration(config);

            // create a plugin stack
            pluginBrowserStack = new FFGLPluginSourceStack;
            pluginBrowserStack->push(s->freeframeGLPlugin());

            // show the plugin stack
            pluginBrowser->showProperties( pluginBrowserStack );

            // show warning if selected plugin is not of type 'Source'
            ui->labelWarninEffect->setVisible( !s->freeframeGLPlugin()->isSourceType() );

        }
        catch (...)  {
            // free the OpenGL texture
            glDeleteTextures(1, &tex);
            // return an invalid pointer
            s = NULL;

            qCritical() << _filename << QChar(124).toLatin1() << tr("Not a FreeframeGL plugin.");
        }

    }

    // apply the source to the preview (null pointer is ok to reset preview)
    ui->preview->setSource(s);
    ui->preview->playSource(true);
}

void FFGLSourceCreationDialog::updatePlugin(QString filename) {

    _filename = filename;

    updateSourcePreview();

    pluginBrowser->expandAll();
}

void FFGLSourceCreationDialog::browse() {

    #ifdef Q_OS_MAC
    QString ext = tr("Freeframe GL Plugin (*)");
    #else
    #ifdef Q_OS_WIN
    QString ext = tr("Freeframe GL Plugin (*.dll)");
    #else
    QString ext = tr("Freeframe GL Plugin (*.so)");
    #endif
    #endif
    // browse for a plugin file
    QString fileName = GLMixer::getInstance()->getFileName(tr("Open FFGL Plugin file"), ext);

    // if a file was selected
    if (!fileName.isEmpty()) {
        // add the filename to the pluginFileList
        ui->pluginFileList->insertItem(0, fileName);
        ui->pluginFileList->setCurrentIndex(0);
    }

}

QFileInfo  FFGLSourceCreationDialog::getFreeframePluginFileInfo(){

    return QFileInfo(_filename);
}

QDomElement FFGLSourceCreationDialog::getFreeframePluginConfiguration(){

    return pluginConfiguration;
}

int  FFGLSourceCreationDialog::getSelectedWidth(){

    return ui->widthSpinBox->value();
}


int  FFGLSourceCreationDialog::getSelectedHeight(){

    return ui->heightSpinBox->value();
}


void  FFGLSourceCreationDialog::selectSizePreset(int preset)
{
    if (preset == 0) {
        ui->sizeGrid->setVisible(true);

        QObject::connect(ui->heightSpinBox, SIGNAL(valueChanged(int)), this, SLOT(updateSourcePreview()));
        QObject::connect(ui->widthSpinBox, SIGNAL(valueChanged(int)), this, SLOT(updateSourcePreview()));
    }
    else {
        ui->sizeGrid->setVisible(false);

        QObject::disconnect(ui->heightSpinBox, SIGNAL(valueChanged(int)), this, SLOT(updateSourcePreview()));
        QObject::disconnect(ui->widthSpinBox, SIGNAL(valueChanged(int)), this, SLOT(updateSourcePreview()));

        switch (preset) {
        case 1:
            ui->heightSpinBox->setValue(2);
            ui->widthSpinBox->setValue(2);
            break;
        case 2:
            ui->heightSpinBox->setValue(8);
            ui->widthSpinBox->setValue(8);
            break;
        case 3:
            ui->heightSpinBox->setValue(16);
            ui->widthSpinBox->setValue(16);
            break;
        case 4:
            ui->heightSpinBox->setValue(32);
            ui->widthSpinBox->setValue(32);
            break;
        case 5:
            ui->heightSpinBox->setValue(64);
            ui->widthSpinBox->setValue(64);
            break;
        case 6:
            ui->heightSpinBox->setValue(128);
            ui->widthSpinBox->setValue(128);
            break;
        case 7:
            ui->heightSpinBox->setValue(256);
            ui->widthSpinBox->setValue(256);
            break;
        case 8:
            ui->widthSpinBox->setValue(160);
            ui->heightSpinBox->setValue(120);
            break;
        case 9:
            ui->widthSpinBox->setValue(320);
            ui->heightSpinBox->setValue(240);
            break;
        case 10:
            ui->widthSpinBox->setValue(640);
            ui->heightSpinBox->setValue(480);
            break;
        case 11:
            ui->widthSpinBox->setValue(720);
            ui->heightSpinBox->setValue(480);
            break;
        case 12:
            ui->widthSpinBox->setValue(768);
            ui->heightSpinBox->setValue(576);
            break;
        case 13:
            ui->widthSpinBox->setValue(800);
            ui->heightSpinBox->setValue(600);
            break;
        case 14:
            ui->widthSpinBox->setValue(1024);
            ui->heightSpinBox->setValue(768);
            break;
        }

        if (s) {

            updateSourcePreview(s->freeframeGLPlugin()->getConfiguration());

        }
        else
            updateSourcePreview();
    }

}
