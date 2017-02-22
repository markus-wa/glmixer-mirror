#ifndef OPENSOUNDCONTROLTRANSLATOR_H
#define OPENSOUNDCONTROLTRANSLATOR_H

#include <QWidget>
#include <QSettings>
#include <QRegExpValidator>

namespace Ui {
class OpenSoundControlTranslator;
}

class OpenSoundControlTranslator : public QWidget
{
    Q_OBJECT

public:
    explicit OpenSoundControlTranslator(QSettings *settings, QWidget *parent = 0);
    ~OpenSoundControlTranslator();

public slots:

    void settingsChanged();
    void logMessage(QString m);
    void addTranslation(QString before, QString after);

    void on_OSCHelp_pressed();
    void on_addTranslation_pressed();
    void on_verboseLogs_toggled(bool);

private:
    Ui::OpenSoundControlTranslator *ui;

    QRegExpValidator validator;
    QSettings *appSettings;
};

#endif // OPENSOUNDCONTROLTRANSLATOR_H
