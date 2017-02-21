#include "OpenSoundControlTranslator.moc"
#include "ui_OpenSoundControlTranslator.h"

OpenSoundControlTranslator::OpenSoundControlTranslator(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::OpenSoundControlTranslator)
{
    ui->setupUi(this);
}

OpenSoundControlTranslator::~OpenSoundControlTranslator()
{
    delete ui;
}
