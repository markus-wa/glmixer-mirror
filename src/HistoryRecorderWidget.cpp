#include "HistoryRecorderWidget.moc"
#include "ui_HistoryRecorderWidget.h"


HistoryRecorderWidget::HistoryRecorderWidget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::HistoryRecorderWidget),
//    _editor(NULL),
    _recorder(NULL)
{
    ui->setupUi(this);

}

HistoryRecorderWidget::~HistoryRecorderWidget()
{
//    if (_recorder)
//        delete _recorder;
    delete ui;
}


void HistoryRecorderWidget::on_recordButton_toggled(bool on)
{
    static  QTreeWidgetItem * recordItem = NULL;

    // begin recording
    if (on) {
        ui->editorTab->setEnabled(false);
        ui->playButtonsFrame->setEnabled(false);
        ui->recordingsTable->setEnabled(false);

        // create item
        recordItem = new QTreeWidgetItem( ui->recordingsTable);
        recordItem->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEditable | Qt::ItemIsEnabled);

        // set a default name
        int i = 0;
        QString label;
        do {
            label = QString("rec_%1").arg(++i);
        } while ( !ui->recordingsTable->findItems(label, Qt::MatchExactly).isEmpty());
        recordItem->setText(0, label);

        // new recorder
        if (_recorder)
            delete _recorder;
        _recorder = new HistoryRecorder;

    }
    // end recording
    else {

        ui->editorTab->setEnabled(true);
        ui->playButtonsFrame->setEnabled(true);
        ui->recordingsTable->setEnabled(true);

        // append recorded events to manager
        HistoryManager *manager = new HistoryManager;
//        manager->setHistory(_recorder->stop());

        // store manager in item
//        recordItem->setData(0, Qt::UserRole, QVariant::fromValue(manager) );
//        recordItem->setText(1, QString::number(manager->duration()));
        ui->recordingsTable->setCurrentItem(recordItem);

        // done with recorder
        delete _recorder;
        _recorder = NULL;
    }

}

void HistoryRecorderWidget::on_recordingsTable_currentItemChanged ( QTreeWidgetItem * current, QTreeWidgetItem * previous )
{

//    if (_editor)
//        delete _editor;
//    _editor = NULL;

    // disconnect previous manager
    if (previous) {
        QVariant data = previous->data(0, Qt::UserRole);
//        HistoryManager *manager = data.value<HistoryManagerStar>();
//        manager->disconnect();
        ui->playButton->disconnect();
        ui->loopButton->disconnect();
        ui->reverseButton->disconnect();
        ui->rewindButton->disconnect();
    }

    // connect manager of current item selected
    if (current) {

        // get manager associated to item
        QVariant data = current->data(0, Qt::UserRole);
//        HistoryManager *manager = data.value<HistoryManagerStar>();

//        // create editor for this manager
//        _editor = new HistoryManagerWidget(manager, this);
//        ui->editorFrameContentLayout->addWidget(_editor);

//        // connect manager to control buttons
//        ui->playButton->setChecked(manager->isPlaying());
//        ui->loopButton->setChecked(manager->isLoop());
//        ui->reverseButton->setChecked(manager->isReverse());
//        connect(ui->playButton, SIGNAL(toggled(bool)), manager, SLOT(play(bool)) );
//        connect(manager, SIGNAL(playing(bool)), ui->playButton, SLOT(setChecked(bool)) );
//        connect(ui->loopButton, SIGNAL(toggled(bool)), manager, SLOT(loop(bool)) );
//        connect(ui->reverseButton, SIGNAL(toggled(bool)), manager, SLOT(reverse(bool)) );
//        connect(ui->rewindButton, SIGNAL(clicked()), manager, SLOT(rewind()) );
    }

}


