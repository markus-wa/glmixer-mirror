#include "OpenSoundControlTranslator.moc"
#include "ui_OpenSoundControlTranslator.h"
#include "OpenSoundControlManager.h"

#include <QDesktopServices>

OpenSoundControlTranslator::OpenSoundControlTranslator(QSettings *settings, QWidget *parent) :
    QWidget(parent),
    ui(new Ui::OpenSoundControlTranslator),
    appSettings(settings)
{
    ui->setupUi(this);

    if (appSettings) {
        // restore table translator
    }

    // set GUI initial status from Manager
    ui->enableOSC->setChecked( OpenSoundControlManager::getInstance()->isEnabled());
    ui->OSCPort->setValue( OpenSoundControlManager::getInstance()->getPort() );

    // connect GUI to Manager
    connect(ui->enableOSC, SIGNAL(toggled(bool)), this, SLOT(settingsChanged()) );
    connect(ui->OSCPort, SIGNAL(valueChanged(int)), this, SLOT(settingsChanged()) );

    // connect logs
    connect(OpenSoundControlManager::getInstance(), SIGNAL(log(QString)), this, SLOT(logMessage(QString)) );
}

OpenSoundControlTranslator::~OpenSoundControlTranslator()
{
    delete ui;
}

void OpenSoundControlTranslator::on_OSCHelp_pressed()
{
    QDesktopServices::openUrl(QUrl("https://sourceforge.net/p/glmixer/wiki/GLMixer_OSC_Specs/", QUrl::TolerantMode));
}

void OpenSoundControlTranslator::settingsChanged()
{
    bool on = ui->enableOSC->isChecked();
    qint16 p = (qint16) ui->OSCPort->value();
    OpenSoundControlManager::getInstance()->setEnabled(on, p);
}

void OpenSoundControlTranslator::logMessage(QString m)
{
    ui->consoleOSC->append(m);
}
