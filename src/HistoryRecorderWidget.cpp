#include "HistoryRecorder.h"
#include "ui_HistoryRecorder.h"

HistoryRecorder::HistoryRecorder(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::HistoryRecorder)
{
    ui->setupUi(this);
}

HistoryRecorder::~HistoryRecorder()
{
    delete ui;
}
