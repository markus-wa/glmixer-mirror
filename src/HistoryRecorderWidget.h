#ifndef HISTORYRECORDER_H
#define HISTORYRECORDER_H

#include <QWidget>

namespace Ui {
class HistoryRecorder;
}

class HistoryRecorder : public QWidget
{
    Q_OBJECT

public:
    explicit HistoryRecorder(QWidget *parent = 0);
    ~HistoryRecorder();

private:
    Ui::HistoryRecorder *ui;
};

#endif // HISTORYRECORDER_H
