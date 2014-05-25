#include "NewSourceDialog.moc"
#include "ui_NewSourceDialog.h"

NewSourceDialog::NewSourceDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::NewSourceDialog)
{
    ui->setupUi(this);

#ifndef OPEN_CV
    QWidget *wcv = ui->SourceTypeToolBox->widget(1);
    ui->SourceTypeToolBox->removeItem(1);
    delete wcv;
#endif

#ifndef FFGL
    QWidget *wffgl = ui->SourceTypeToolBox->widget(6);
    ui->SourceTypeToolBox->removeItem(6);
    delete wffgl;
#endif

#ifndef SHM
    QWidget *wshm = ui->SourceTypeToolBox->widget(7);
    ui->SourceTypeToolBox->removeItem(7);
    delete wshm;
#endif
}

NewSourceDialog::~NewSourceDialog()
{
    delete ui;
}

Source::RTTI NewSourceDialog::selectedType()
{
    Source::RTTI t = Source::SIMPLE_SOURCE;

    QString text = ui->SourceTypeToolBox->itemText(ui->SourceTypeToolBox->currentIndex());

    if ( text.contains("Video"))
        t = Source::VIDEO_SOURCE;
    else if ( text.contains("Device"))
        t = Source::CAMERA_SOURCE;
    else if ( text.contains("Loop"))
        t = Source::RENDERING_SOURCE;
    else if ( text.contains("Capture"))
        t = Source::CAPTURE_SOURCE;
    else if ( text.contains("Algo"))
        t = Source::ALGORITHM_SOURCE;
    else if ( text.contains("Vector"))
        t = Source::SVG_SOURCE;
    else if ( text.contains("Plugin"))
        t = Source::FFGL_SOURCE;
    else if ( text.contains("Shared"))
        t = Source::SHM_SOURCE;


    return t;
}

void NewSourceDialog::changeEvent(QEvent *e)
{
    QDialog::changeEvent(e);
    switch (e->type()) {
    case QEvent::LanguageChange:
        ui->retranslateUi(this);
        break;
    default:
        break;
    }
}
