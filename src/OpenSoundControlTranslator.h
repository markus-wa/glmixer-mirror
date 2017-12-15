#ifndef OPENSOUNDCONTROLTRANSLATOR_H
#define OPENSOUNDCONTROLTRANSLATOR_H

#include <QWidget>
#include <QSettings>
#include <QRegExpValidator>
#include <QAbstractTableModel>
#include <QAbstractItemView>
#include <QItemDelegate>

namespace Ui {
class OpenSoundControlTranslator;
}

class TranslationDelegate : public QItemDelegate
{
    Q_OBJECT
public:
    explicit TranslationDelegate(QObject *parent = 0);

protected:
    QWidget* createEditor(QWidget *parent, const QStyleOptionViewItem &option, const QModelIndex &index) const;
    void setEditorData(QWidget * editor, const QModelIndex & index) const;
    void setModelData(QWidget * editor, QAbstractItemModel * model, const QModelIndex & index) const;
    void updateEditorGeometry(QWidget * editor, const QStyleOptionViewItem & option, const QModelIndex & index) const;

private:
    QRegExpValidator validator;
};

class TranslationTableModel : public QAbstractTableModel
{
    Q_OBJECT

public:
    TranslationTableModel(QObject *parent = 0);

    int rowCount(const QModelIndex &parent = QModelIndex()) const ;
    int columnCount(const QModelIndex &parent = QModelIndex()) const;
    QVariant headerData(int section, Qt::Orientation orientation, int role) const;
    Qt::ItemFlags flags(const QModelIndex & index) const ;

    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const;
    bool setData(const QModelIndex &index, const QVariant & value, int role = Qt::EditRole);
    bool insertRows(int position, int rows, const QModelIndex &index=QModelIndex());
    bool removeRows(int position, int rows, const QModelIndex &index=QModelIndex());

    bool contains(QString before, QString after = QString::null);

private:
    QList< QPair<QString, QString> >  *dictionnary;

};

class OpenSoundControlTranslator : public QWidget
{
    Q_OBJECT

public:
    explicit OpenSoundControlTranslator(QSettings *settings, QWidget *parent = 0);
    ~OpenSoundControlTranslator();

public slots:
    void updateManager();
    void contextMenu(QPoint);
    void removeSelection();

    void logMessage(QString m);
    void logError(QString m);
    void logStatus();

    void on_OSCHelp_pressed();
    void on_addTranslation_pressed();
    void on_verboseLogs_toggled(bool);
    void on_clearTranslation_pressed();
    void on_translationPresets_currentIndexChanged(int);

protected:
    void showEvent(QShowEvent *);
    void hideEvent(QHideEvent *);

private:

    void addTranslation(QString, QString);

    Ui::OpenSoundControlTranslator *ui;

    TranslationTableModel translations;
    QRegExpValidator validator;
    QSettings *appSettings;
};

#endif // OPENSOUNDCONTROLTRANSLATOR_H
