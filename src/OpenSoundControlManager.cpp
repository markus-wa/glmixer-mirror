#include <QDebug>

#include "OpenSoundControlManager.moc"

// static members
OpenSoundControlManager *OpenSoundControlManager::_instance = 0;




OpenSoundControlManager *OpenSoundControlManager::getInstance() {

    if (_instance == 0) {
        _instance = new OpenSoundControlManager;
        Q_CHECK_PTR(_instance);
    }

    return _instance;
}

OpenSoundControlManager::OpenSoundControlManager(qint16 port) : QObject()
{
    _udpSocket.bind(QHostAddress::LocalHost, port);

}


void OpenSoundControlManager::setEnabled(bool enabled)
{
    if (enabled)
        connect(&_udpSocket, SIGNAL(readyRead()),  this, SLOT(readPendingDatagrams()));
    else
        disconnect(&_udpSocket, SIGNAL(readyRead()),  this, SLOT(readPendingDatagrams()));

    qDebug() << "OpenSoundControlManager" << QChar(124).toLatin1() << "Network Open Sound Control " << (enabled ? "enabled" : "disabled");
}

void OpenSoundControlManager::ReadPendingDatagrams()
{
    while (_udpSocket.hasPendingDatagrams()) {
        QByteArray datagram;
        datagram.resize(_udpSocket.pendingDatagramSize());
        QHostAddress sender;
        quint16 senderPort;

        _udpSocket.readDatagram(datagram.data(), datagram.size(), &sender, &senderPort);

        // HERE PROCESS THE UDP Datagram

        qDebug() << "UDP received " << datagram.data();
    }

}
