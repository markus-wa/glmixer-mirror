#ifndef HISTORYRECORDERWIDGET_H
#define HISTORYRECORDERWIDGET_H

#include <QWidget>
#include <QTreeWidget>

#include "HistoryPlayer.h"
#include "HistoryRecorder.h"
#include "HistoryManager.h"
#include "HistoryManagerWidget.h"

namespace Ui {
class HistoryRecorderWidget;
}

class HistoryRecorderWidget : public QWidget
{
    Q_OBJECT

public:
    explicit HistoryRecorderWidget(QWidget *parent = 0);
    ~HistoryRecorderWidget();

public slots:

    void on_recordingsTable_currentItemChanged (QTreeWidgetItem * current, QTreeWidgetItem *previous );
    void on_recordButton_toggled(bool on);

private:
    Ui::HistoryRecorderWidget *ui;

//    class HistoryManagerWidget *_editor;
    HistoryRecorder *_recorder;
    HistoryPlayer *_player;
};

#endif // HISTORYRECORDER_H
