/*
 *   FFGLSourceCreationDialog
 *
 *   This file is part of GLMixer.
 *
 *   GLMixer is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation, either version 3 of the License, or
 *   (at your option) any later version.
 *
 *   GLMixer is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with GLMixer.  If not, see <http://www.gnu.org/licenses/>.
 *
 *   Copyright 2009, 2012 Bruno Herbelin
 *
 */

#include "FFGLSourceCreationDialog.moc"
#include "ui_FFGLSourceCreationDialog.h"

#include <QFileInfo>

#include "SourceDisplayWidget.h"
#include "FFGLPluginSource.h"
#include "FFGLPluginBrowser.h"
#include "FFGLSource.h"
#include "glmixer.h"


FFGLSourceCreationDialog::FFGLSourceCreationDialog(QWidget *parent, QSettings *settings) :
    QDialog(parent), ui(new Ui::FFGLSourceCreationDialog), pluginBrowserStack(NULL), s(NULL), appSettings(settings)
{
    // setup the user interface
    ui->setupUi(this);
    ui->freeframeLabelWarning->setVisible(false);
    ui->shadertoyLabelWarning->setVisible(false);
    ui->sizeGrid->setVisible(false);

    // Setup the FFGL plugin property browser
//    pluginBrowser = new FFGLPluginBrowser(this, false);
//    ui->PropertiesLayout->insertWidget( ui->PropertiesLayout->count(),  pluginBrowser);
//    pluginBrowser->setSizePolicy(QSizePolicy::Expanding,QSizePolicy::Expanding);
//    pluginBrowser->switchToGroupView();

    // restore settings
    ui->freeframeFileList->addItem("");
    if (appSettings) {
        if (appSettings->contains("recentFFGLPluginsList")) {
            QStringListIterator it(appSettings->value("recentFFGLPluginsList").toStringList());
            while (it.hasNext()){
                QFileInfo pluginfile(it.next());
                ui->freeframeFileList->addItem(pluginfile.baseName(), pluginfile.absoluteFilePath());
            }
        }
//        if (appSettings->contains("FFGLDialogLayout"))
//            ui->splitter->restoreState(appSettings->value("FFGLDialogLayout").toByteArray());
    }
}


FFGLSourceCreationDialog::~FFGLSourceCreationDialog()
{
//    delete preview;
//    delete pluginBrowser;
    delete ui;

}


void FFGLSourceCreationDialog::showEvent(QShowEvent *e){

    ui->freeframeFileList->setCurrentIndex(0);
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

    if (appSettings) {
        QStringList l;
        for ( int i = 1; i < ui->freeframeFileList->count(); ++i )
            l.append(ui->freeframeFileList->itemData(i).toString());
        appSettings->setValue("recentFFGLPluginsList", l);

//        appSettings->setValue("FFGLDialogLayout", ui->splitter->saveState());
    }

    QDialog::done(r);
}



void FFGLSourceCreationDialog::updateSourcePreview(QDomElement config){

    if(s) {
        ui->freeframeLabelWarning->setVisible(false);
        // remove source from preview: this deletes the texture in the preview
        ui->preview->setSource(0);
        // delete the source:
        delete s;
    }

    if (pluginBrowserStack) {
        // clear plugin browser
//        pluginBrowser->clear();
        delete pluginBrowserStack;
    }

    s = NULL;
    pluginBrowserStack = NULL;

    if (QFileInfo(_filename).isFile()) {

        GLuint tex = ui->preview->getNewTextureIndex();
        try {
            try {
                // create a new source with a new texture index and the new parameters
                s = new FFGLSource(_filename, tex, 0, ui->widthSpinBox->value(), ui->heightSpinBox->value());

                if (!config.isNull())
                    s->freeframeGLPlugin()->setConfiguration(config);

                // create a plugin stack
                pluginBrowserStack = new FFGLPluginSourceStack;
                pluginBrowserStack->push(s->freeframeGLPlugin());

                // show the plugin stack
//                pluginBrowser->showProperties( pluginBrowserStack );

                // show warning if selected plugin is not of type 'Source'
                ui->freeframeLabelWarning->setVisible( !s->freeframeGLPlugin()->isSourceType() );

            }
            catch (FFGLPluginException &e)  {
                qCritical() << tr("Freeframe error; ") << e.message();
                throw;
            }
        }
        catch (...)  {
            // free the OpenGL texture
            glDeleteTextures(1, &tex);
            // return an invalid pointer
            s = NULL;

            qCritical() << _filename << QChar(124).toLatin1() << tr("Could no create plugin source.");
        }

    }

    // apply the source to the preview (null pointer is ok to reset preview)
    ui->preview->setSource(s);
    ui->preview->playSource(true);
}


void FFGLSourceCreationDialog::pluginTypeChanged(int tab)
{
    qDebug() << "pluginTypeChanged(int tab)" << tab;

    if ( tab == 0 ) {

        setFreeframePlugin( ui->freeframeFileList->currentIndex() );

    } else {

        setShadertoyPlugin(0);
    }

}

void FFGLSourceCreationDialog::setFreeframePlugin(int index) {

    _filename = ui->freeframeFileList->itemData(index).toString();

    updateSourcePreview();

//    pluginBrowser->expandAll();
}

void FFGLSourceCreationDialog::browseFreeframePlugin() {

    #ifdef Q_OS_MAC
    QString ext = " (*.bundle *.so)";
    #else
    #ifdef Q_OS_WIN
    QString ext = " (*.dll)";
    #else
    QString ext = " (*.so)";
    #endif
    #endif
    // browse for a plugin file
    QString fileName = GLMixer::getInstance()->getFileName(tr("Open FFGL Plugin file"), tr("Freeframe GL Plugin") + ext);

    QFileInfo pluginfile(fileName);
#ifdef Q_OS_MAC
    if (pluginfile.isBundle())
        pluginfile.setFile( pluginfile.absoluteFilePath() + "/Contents/MacOS/" + pluginfile.baseName() );
#endif
    // if a file was selected
    if (pluginfile.isFile()) {
        // try to find & remove the file in the recent plugins list
        ui->freeframeFileList->removeItem( ui->freeframeFileList->findData(pluginfile.absoluteFilePath()) );

        // add the filename to the pluginFileList
        ui->freeframeFileList->insertItem(1, pluginfile.baseName(), pluginfile.absoluteFilePath());
        ui->freeframeFileList->setCurrentIndex(1);
    }

}


void FFGLSourceCreationDialog::setShadertoyPlugin(int index) {


    updateSourcePreview();

}

void FFGLSourceCreationDialog::browseShadertoyPlugin() {

}

QString  FFGLSourceCreationDialog::getPluginInfo(){

    return _filename;
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
        case 15:
            ui->widthSpinBox->setValue(1280);
            ui->heightSpinBox->setValue(720);
            break;
        case 16:
            ui->widthSpinBox->setValue(1600);
            ui->heightSpinBox->setValue(1200);
            break;
        case 17:
            ui->widthSpinBox->setValue(1920);
            ui->heightSpinBox->setValue(1080);
            break;
        }

        if (s)
            updateSourcePreview(s->freeframeGLPlugin()->getConfiguration());
        else
            updateSourcePreview();
    }

}
