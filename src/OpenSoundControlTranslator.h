#ifndef OPENSOUNDCONTROLTRANSLATOR_H
#define OPENSOUNDCONTROLTRANSLATOR_H

#include <QWidget>
#include <QSettings>

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

    void on_OSCHelp_pressed();

private:
    Ui::OpenSoundControlTranslator *ui;

    QSettings *appSettings;
};

#endif // OPENSOUNDCONTROLTRANSLATOR_H
