#ifndef OPENSOUNDCONTROLMANAGER_H
#define OPENSOUNDCONTROLMANAGER_H

#include <QObject>
#include <QUdpSocket>


class OpenSoundControlManager: public QObject
{    
    Q_OBJECT

public:
    static OpenSoundControlManager *getInstance();

public Q_SLOTS:
    void ReadPendingDatagrams();
    void setEnabled(bool);

private:
    OpenSoundControlManager(qint16 port=7000);
    static OpenSoundControlManager *_instance;

    QUdpSocket _udpSocket;
};

#endif // OPENSOUNDCONTROLMANAGER_H
