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

    QStringList getFiles();
    QList<int> getPlayList();

    void appendImageFiles(QList<QUrl> urlList);
    QList<QListWidgetItem*> selectedImages();

signals:
    void changed(int);

public slots:
    void duplicateSelectedImages();
    void deleteSelectedImages();
    void deleteAllImages();
    void sortAlphabetically();
    void moveSelectionUp();
    void moveSelectionDown();

protected:
    void dragEnterEvent(QDragEnterEvent *event);
    void dragMoveEvent(QDragMoveEvent *event);
    void dragLeaveEvent(QDragLeaveEvent *event);
    void dropEvent(QDropEvent * event);

    QList<QListWidgetItem *> findItemsData(QString filename);

    QListWidgetItem *dropHintItem;
    QStringList _fileNames;
};

class BasketSelectionDialog : public QDialog
{
    Q_OBJECT

public:
    explicit BasketSelectionDialog(QWidget *parent = 0, QSettings *settings = 0);
    ~BasketSelectionDialog();

    // give parameters generated
    int getSelectedWidth();
    int getSelectedHeight();
    int getSelectedPeriod();
    QStringList getSelectedFiles();
    QStringList getSelectedPlayList();
    bool getSelectedBidirectional();
    bool getSelectedShuffle();

public slots:

    void done(int r);

    // user input on parameters
    void on_frequencySlider_valueChanged(int v);
    void on_bidirectional_toggled(bool on);
    void on_shuffle_toggled(bool on);
    // actions on items
    void on_addImages_pressed();
    void on_moveUp_pressed();
    void on_moveDown_pressed();
    // updates display
    void displayCount(int v);
    void updateSourcePreview();
    void updateActions();

protected:
    void showEvent(QShowEvent *);

private:
    Ui::BasketSelectionDialog *ui;
    ImageFilesList *basket;

    class BasketSource *s;
    QSettings *appSettings;
};

#endif // BASKETSELECTIONDIALOG_H
