#ifndef BASKETSELECTIONDIALOG_H
#define BASKETSELECTIONDIALOG_H

#include <QDialog>
#include <QSettings>
#include <QListWidget>
#include <QLabel>

namespace Ui {
class BasketSelectionDialog;
}

class ImageFilesList : public QListWidget
{
    Q_OBJECT

    friend class BasketSelectionDialog;

public:
    explicit ImageFilesList(QWidget *parent = 0);

    QStringList getFilesList();

signals:
    void countChanged(int);

public slots:
    void deleteSelectedItem();

protected:
    void dragEnterEvent(QDragEnterEvent *event);
    void dragMoveEvent(QDragMoveEvent *event);
    void dragLeaveEvent(QDragLeaveEvent *event);
    void dropEvent(QDropEvent * event);

    QListWidgetItem *dropHintItem;
};

class BasketSelectionDialog : public QDialog
{
    Q_OBJECT

public:
    explicit BasketSelectionDialog(QWidget *parent = 0, QSettings *settings = 0);
    ~BasketSelectionDialog();

    int getSelectedWidth();
    int getSelectedHeight();
    int getSelectedPeriod();
    QStringList getSelectedFiles();

public slots:

    void done(int r);

    void on_frequencySlider_valueChanged(int v);
    void displayCount(int v);

private:
    Ui::BasketSelectionDialog *ui;

    class BasketSource *s;
    QSettings *appSettings;
};

#endif // BASKETSELECTIONDIALOG_H
