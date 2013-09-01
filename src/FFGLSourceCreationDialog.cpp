#include "FFGLSourceCreationDialog.moc"
#include "ui_FFGLSourceCreationDialog.h"

#include <QFileDialog>
#include <QFileInfo>

#include "SourceDisplayWidget.h"
#include "FFGLPluginBrowser.h"
#include "FFGLSource.h"

FFGLSourceCreationDialog::FFGLSourceCreationDialog(QWidget *parent) :
    QDialog(parent), s(0),
    ui(new Ui::FFGLSourceCreationDialog)
{
    ui->setupUi(this);

    preview = new SourceDisplayWidget(this);
    ui->FFGLSourceCreationLayout->insertWidget(0, preview);
    preview->setMinimumHeight(120);
    preview->setSizePolicy(QSizePolicy::Preferred,QSizePolicy::Expanding);

    // Setup the FFGL plugin property browser
    pluginBrowser = new FFGLPluginBrowser(this);
    ui->PropertiesLayout->insertWidget( ui->PropertiesLayout->count(),  pluginBrowser);
    pluginBrowser->setSizePolicy(QSizePolicy::Expanding,QSizePolicy::Expanding);
}


FFGLSourceCreationDialog::~FFGLSourceCreationDialog()
{
    delete preview;
    delete pluginBrowser;
    delete ui;
}


void FFGLSourceCreationDialog::showEvent(QShowEvent *e){

    updateSourcePreview();

    QWidget::showEvent(e);
}

void FFGLSourceCreationDialog::done(int r){

    if (preview)
        preview->setSource(0);
    if (s) {
        delete s;
        s = 0;
    }
    QDialog::done(r);
}



void FFGLSourceCreationDialog::updateSourcePreview(){

    if(s) {
        // remove source from preview: this deletes the texture in the preview
        preview->setSource(0);
        // delete the source:
        delete s;
        s = 0;
    }


    if (QFileInfo(_filename).isFile()) {

        GLuint tex = preview->getNewTextureIndex();
        try {
            // create a new source with a new texture index and the new parameters
            s = new FFGLSource(_filename, 0, ui->widthSpinBox->value(), ui->heightSpinBox->value());

            pluginBrowser->showProperties( s->getFreeframeGLPluginStack() );

        }
        catch (...)  {
            // free the OpenGL texture
            glDeleteTextures(1, &tex);
            // return an invalid pointer
            s = 0;
        }

    }

    // apply the source to the preview
    preview->setSource(s);
    preview->playSource(true);
}

void FFGLSourceCreationDialog::updatePlugin(QString filename) {

    _filename = filename;

    updateSourcePreview();

    pluginBrowser->expandAll();
}

void FFGLSourceCreationDialog::browse() {

    // browse for a plugin file
    QString fileName = QFileDialog::getOpenFileName(this, tr("Choose FFGL Plugin file"), QDir::currentPath(), tr("Freeframe GL Plugin (*.so *.dll *.bundle)"));
    // if a file was selected
    if (!fileName.isEmpty()) {
        // add the filename to the pluginFileList
        ui->pluginFileList->insertItem(0, fileName);
        ui->pluginFileList->setCurrentIndex(0);
    }

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
        ui->heightSpinBox->setEnabled(true);
        ui->widthSpinBox->setEnabled(true);
        ui->h->setEnabled(true);
        ui->w->setEnabled(true);
    } else {
        ui->heightSpinBox->setEnabled(false);
        ui->widthSpinBox->setEnabled(false);
        ui->h->setEnabled(false);
        ui->w->setEnabled(false);

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
    }

}
