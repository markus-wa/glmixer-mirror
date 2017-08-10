#ifndef BASKETSELECTIONDIALOG_H
#define BASKETSELECTIONDIALOG_H

#include <QDialog>
#include <QSettings>
#include <QListWidget>
#include <QMap>
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
    QList<int> getPlayList();

signals:
    void countChanged(int);

public slots:
    void deleteSelectedItems();
    void deleteAllItems();
    void sortAlphabetical();

protected:
    void dragEnterEvent(QDragEnterEvent *event);
    void dragMoveEvent(QDragMoveEvent *event);
    void dragLeaveEvent(QDragLeaveEvent *event);
    void dropEvent(QDropEvent * event);

    QListWidgetItem *dropHintItem;
    QStringList _fileNames;
    QList<QListWidgetItem *> _referenceItems;
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
    bool getSelectedBidirectional();
    bool getSelectedShuffle();

public slots:

    void done(int r);

    void on_frequencySlider_valueChanged(int v);
    void on_bidirectional_toggled(bool on);
    void on_shuffle_toggled(bool on);

    void displayCount(int v);
    void updateSourcePreview();

protected:
    void showEvent(QShowEvent *);

private:
    Ui::BasketSelectionDialog *ui;
    ImageFilesList *basket;

    class BasketSource *s;
    QSettings *appSettings;
};

#endif // BASKETSELECTIONDIALOG_H
