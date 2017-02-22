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

    QRegExp validRegex("[/A-z0-9]+");
    validator.setRegExp(validRegex);
    ui->afterTranslation->setValidator(&validator);

    if (appSettings) {
        // restore table translator

    }

    // set GUI initial status from Manager
    ui->enableOSC->setChecked( OpenSoundControlManager::getInstance()->isEnabled());
    ui->OSCPort->setValue( OpenSoundControlManager::getInstance()->getPort() );
    ui->verboseLogs->setChecked( OpenSoundControlManager::getInstance()->isVerbose() );

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

void OpenSoundControlTranslator::addTranslation(QString before, QString after)
{
    if (before.isEmpty() || after.isEmpty())
        return;

    // add only if not already existing
    if (!OpenSoundControlManager::getInstance()->hasTranslation(before, after)) {
        // add item to the table
        QStringList l;
        l << before << after;
        QTreeWidgetItem *item = new QTreeWidgetItem(l);
        ui->tableTranslation->addTopLevelItem(item);

        // add translation to Manager
        OpenSoundControlManager::getInstance()->addTranslation(before, after);
    }
}

void OpenSoundControlTranslator::on_addTranslation_pressed()
{
    addTranslation(ui->beforeTranslation->text(), ui->afterTranslation->text());
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

void OpenSoundControlTranslator::on_OSCHelp_pressed()
{
    QDesktopServices::openUrl(QUrl("https://sourceforge.net/p/glmixer/wiki/GLMixer_OSC_Specs/", QUrl::TolerantMode));
}


void  OpenSoundControlTranslator::on_verboseLogs_toggled(bool on)
{
    OpenSoundControlManager::getInstance()->setVerbose(on);
    ui->consoleOSC->append(tr("Verbose logs %1").arg(on ? "on":"off"));
}
