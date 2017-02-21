#ifndef OPENSOUNDCONTROLTRANSLATOR_H
#define OPENSOUNDCONTROLTRANSLATOR_H

#include <QWidget>

namespace Ui {
class OpenSoundControlTranslator;
}

class OpenSoundControlTranslator : public QWidget
{
    Q_OBJECT

public:
    explicit OpenSoundControlTranslator(QWidget *parent = 0);
    ~OpenSoundControlTranslator();

private:
    Ui::OpenSoundControlTranslator *ui;
};

#endif // OPENSOUNDCONTROLTRANSLATOR_H
