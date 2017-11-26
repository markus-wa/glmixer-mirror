#include <QDebug>
#include <QRegExp>
#include <QStringList>
#include <QNetworkInterface>

#include "OpenSoundControlManager.moc"

#include "OscReceivedElements.h"
#include "RenderingManager.h"
#include "glmixer.h"


namespace osc{

class MalformedAddressException : public Exception{
public:
    MalformedAddressException( const char *w="malformed address pattern" )
        : Exception( w ) {}
};

class BundleNotSupportedException : public Exception{
public:
    BundleNotSupportedException( const char *w="Bundle not supported" )
        : Exception( w ) {}
};


class InvalidAttributeException : public Exception{
public:
    InvalidAttributeException( const char *w="invalid attribute" )
        : Exception( w ) {}
};

class InvalidObjectException : public Exception{
public:
    InvalidObjectException( const char *w="invalid target" )
        : Exception( w ) {}
};

}

// static members
OpenSoundControlManager *OpenSoundControlManager::_instance = 0;

OpenSoundControlManager *OpenSoundControlManager::getInstance() {

    if (_instance == 0) {
        _instance = new OpenSoundControlManager();
        Q_CHECK_PTR(_instance);
    }

    return _instance;
}

OpenSoundControlManager::OpenSoundControlManager() : QObject(), _udpSocket(0), _port(7000), _verbose(false)
{
    _dictionnary = new QList< QPair<QString, QString> >();
}

qint16 OpenSoundControlManager::getPort()
{
    return _port;
}

bool OpenSoundControlManager::isEnabled()
{
    return ( _udpSocket != 0 );
}

void OpenSoundControlManager::setEnabled(bool enable, qint16 port)
{

    if ( _udpSocket ) {
        delete _udpSocket;
        _udpSocket = 0;
    }

    // set port
    _port = port;

    if (enable) {
        // bind socket and connect reading slot
        _udpSocket = new QUdpSocket(this);
        _udpSocket->bind(_port);
        connect(_udpSocket, SIGNAL(readyRead()),  this, SLOT(readPendingDatagrams()));

        // Provide informative log
        QStringList addresses;
        foreach( const QHostAddress &a, QNetworkInterface::allAddresses())
            if (a.protocol() == QAbstractSocket::IPv4Protocol)
                addresses << a.toString();
        qDebug() << addresses.join(", ") << QChar(124).toLatin1() << "UDP OSC Server enabled (port " << _port <<").";
    }
    else
        qDebug() << "OpenSoundControlManager" << QChar(124).toLatin1() << "UDP OSC Server disabled.";

}

void OpenSoundControlManager::readPendingDatagrams()
{
    // read all datagrams
    while (_udpSocket->hasPendingDatagrams())
    {
        // read data
        QHostAddress sender;
        quint16 senderPort;
        QByteArray datagram;
        datagram.resize(_udpSocket->pendingDatagramSize());
        _udpSocket->readDatagram(datagram.data(), datagram.size(), &sender, &senderPort);

        // initialize error message
        bool ok = false;
        QString logstring = QString("'%1' ").arg(datagram.data());

        // PROCESS THE UDP Datagram
        try {

            // read packet from datagram
            osc::ReceivedPacket p(datagram.constData(), datagram.size());

            // Treat OSC plain message (not bundle)
            if (p.IsMessage()) {

                // read message
                osc::ReceivedMessage message(p);
                if (_verbose)
                    emit log( logstring );

                // read arguments list
                QVariantList args;
                QString values;
                osc::ReceivedMessage::const_iterator arg = message.ArgumentsBegin();
                for( ;arg != message.ArgumentsEnd(); arg++) {

                    if ((arg)->IsInt32()) {
                        args.append( (int) (arg)->AsInt32() );
                        values.append(" " + QString::number((int)(arg)->AsInt32()));
                    } else if ((arg)->IsInt64()) {
                        args.append( (int) (arg)->AsInt64() );
                        values.append(" " + QString::number((int)(arg)->AsInt64()));
                    } else if ((arg)->IsFloat()) {
                        args.append( (double) (arg)->AsFloat() );
                        values.append(" " + QString::number((double)(arg)->AsFloat(), 'f', 4));
                    } else if ((arg)->IsDouble()) {
                        args.append( (double) (arg)->AsDouble() );
                        values.append(" " + QString::number((double)(arg)->AsDouble(), 'f', 4));
                    } else if ((arg)->IsBool()) {
                        args.append( (bool) (arg)->AsBool() );
                        values.append(" " + QString::number((int)(arg)->AsBool()));
                    } else if ((arg)->IsNil() || (arg)->IsInfinitum()) {
                        args.append( (double) std::numeric_limits<double>::max() );
                        values.append(" N");
                    } else
                        throw osc::WrongArgumentTypeException();
                }

                // read pattern
                QString pattern = message.AddressPattern();

                // List of all patterns to test, including original message
                QStringList patterns;
                patterns << pattern;

                // include all translations to patterns for testing
                QListIterator< QPair<QString, QString> > t(*(_dictionnary));
                while (t.hasNext()) {
                    QPair<QString, QString> p = t.next();
                    if (pattern.contains( p.first )){
                        QString translatedPattern = pattern;
                        translatedPattern.replace(p.first, p.second);
                        patterns << translatedPattern;
                    }
                }

                // try all patterns (after multiple translations)
                QStringList errors;
                foreach (const QString &pat, patterns) {

                    if (_verbose)
                        emit log(tr("\tTrying ") + pat + values);

                    try {
                        // execute this message
                        executeMessage(pat, args);

                        // verbose log for success if no exception
                        if (_verbose)
                            emit log(tr("\tExecuted."));

                        // at least one execution was successful
                        ok = true;
                    }
                    catch( osc::Exception& e ){
                        errors << pat + " " + e.what();
                        if (_verbose)
                            emit log(tr("\tFailed - ") + e.what() );
                    }
                }

                logstring += "Failed (" + errors.join(", ") + ")";

            }
            else
                throw osc::BundleNotSupportedException();

        }
        catch( osc::Exception& e ){
            // any parsing errors such as unexpected argument types, or
            // missing arguments get thrown as exceptions.
            ok = false;
            logstring += e.what();
        }

        if (!ok)
            emit error(logstring + " (from " + sender.toString() + ")");

    }

}



void OpenSoundControlManager::executeMessage(QString pattern, QVariantList args)
{
    // a valid address for OSC message is /glmixer/[property]/[attribute]
    QRegExp validOSCAddressPattern("^/glmixer(/[A-z0-9]+)(/[A-z0-9]+)");
    if ( !validOSCAddressPattern.exactMatch( pattern ) )
        throw osc::MalformedAddressException();

    QString object = validOSCAddressPattern.capturedTexts()[1];
    QString property = validOSCAddressPattern.capturedTexts()[2];

    // Regular expression and argument list are valid!
    // we can execute the command
    execute(object.remove(0,1), property.remove(0,1), args);
}

void OpenSoundControlManager::execute(QString object, QString property, QVariantList args)
{
    // Target OBJECT named "void" (debug)
    if ( object == "void" ) {
        // Target ATTRIBUTE for render : alpha (transparency)
        if ( property == "Log") {
            QString msg("/void/Log ");
            int i = 0;
            for (; i < args.size() ; ++i, msg += ' ' ) {
                msg += args[i].toString();
            }
            emit log(msg);
        }
        else if ( property == "Ignore") {}
        else
            throw osc::InvalidAttributeException();
    }
    // Target OBJECT named "render" (control rendering attributes)
    else if ( object == "render" ) {
        // Target ATTRIBUTE for render : alpha (transparency)
        if ( property == "Alpha") {
            if (args.size() > 0 && args[0].isValid()) {
                bool ok = false;
                double v = 1.0 - qBound(0.0, args[0].toDouble(&ok), 1.0);
                if (ok)
                    GLMixer::getInstance()->on_output_alpha_valueChanged( (int) (v * 100.0) );
                else
                    throw osc::WrongArgumentTypeException();
            }
            else
                throw osc::MissingArgumentException();
        }
        else if ( property == "Pause") {
            if (args.size() > 0 && args[0].isValid())
                RenderingManager::getInstance()->pause( args[0].toBool() );
            else
                throw osc::MissingArgumentException();
        }
        else
            throw osc::InvalidAttributeException();
    }
    // Target OBJECT : name of a source
    else {
        SourceSet::const_iterator sit = RenderingManager::getInstance()->getByName(object);
        // if the given source exists
        if ( RenderingManager::getInstance()->notAtEnd(sit)) {

            // create the string describing the slot
            // and build the list of arguments
            QString slot = "_set" + property + "(";
            QVector<SourceArgument> arguments;

            // fill the list with values
            int i = 0;
            for (; i < args.size() ; ++i, slot += ',' ) {
                arguments.append( SourceArgument(args[i]) );
                slot += arguments[i].typeName();
            }
            // finish the list with empty arguments
            for (; i < 7; ++i) {
                arguments.append( SourceArgument() );
            }

            if (slot.contains(','))
                slot.chop(1);
            slot += ')';

            // get the source on which to call the method
            Source *s = *sit;

            // Try to find the index of the given slot
            int methodIndex = s->metaObject()->indexOfMethod( qPrintable(slot) );
            if ( methodIndex > 0 ) {

//                qDebug() << "invoke " << slot << " on " << s->getName() << " " << arguments[0]<< arguments[1]<< arguments[2];

                // invoke the method with all arguments
                QMetaMethod method = s->metaObject()->method(methodIndex);
                method.invoke(s, Qt::QueuedConnection, arguments[0].argument(), arguments[1].argument(), arguments[2].argument(), arguments[3].argument(), arguments[4].argument(), arguments[5].argument(), arguments[6].argument() );


            }
            else
                throw osc::InvalidAttributeException();

        }
        else
            throw osc::InvalidObjectException();
    }

}

void OpenSoundControlManager::addTranslation(QString before, QString after)
{
    _dictionnary->append( QPair<QString, QString>(before, after) );
}

bool OpenSoundControlManager::hasTranslation(QString before, QString after)
{
    return _dictionnary->count( (QPair<QString, QString>(before, after) ) ) > 0;
}
