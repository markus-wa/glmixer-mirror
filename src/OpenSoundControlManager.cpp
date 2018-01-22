#include <QDebug>
#include <QRegExp>
#include <QStringList>
#include <QNetworkInterface>

#include "OpenSoundControlManager.moc"

#include "OscOutboundPacketStream.h"
#include "OscReceivedElements.h"
#include "RenderingManager.h"
#include "glmixer.h"
#include "VideoSource.h"


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

OpenSoundControlManager::OpenSoundControlManager() : QObject(), _udpReceive(0), _udpBroadcast(0), _portReceive(7000), _portBroadcast(3000), _verbose(false)
{
    _dictionnary = new QList< QPair<QString, QString> >();

}

qint16 OpenSoundControlManager::getPortReceive()
{
    return _portReceive;
}

qint16 OpenSoundControlManager::getPortBroadcast()
{
    return _portBroadcast;
}


bool OpenSoundControlManager::isEnabled()
{
    return ( _udpReceive != 0 );
}

void OpenSoundControlManager::setEnabled(bool enable, qint16 portreceive, qint16 portbroadcast)
{
    // reset receiving socket
    if ( _udpReceive ) {
        delete _udpReceive;
        _udpReceive = 0;
    }
    // reset broadcast socket
    if ( _udpBroadcast ) {
        delete _udpBroadcast;
        _udpBroadcast = 0;
        disconnect(RenderingManager::getInstance(), SIGNAL(countSourceChanged(int)), this, SLOT(broadcastSourceCount(int)));
    }

    // set ports
    _portReceive = portreceive;
    _portBroadcast = portbroadcast;

    if (enable) {
        // create broadcasting slot
        _udpBroadcast = new QUdpSocket(this);

        // listen to broadcasting messages
        connect(RenderingManager::getInstance(), SIGNAL(countSourceChanged(int)), this, SLOT(broadcastSourceCount(int)));

        // bind socket and connect reading slot
        _udpReceive = new QUdpSocket(this);
        _udpReceive->bind(_portReceive);
        connect(_udpReceive, SIGNAL(readyRead()), this, SLOT(readPendingDatagrams()));

        // Provide informative log
        QStringList addresses;
        foreach( const QHostAddress &a, QNetworkInterface::allAddresses())
            if (a.protocol() == QAbstractSocket::IPv4Protocol)
                addresses << a.toString();
        // log
        qDebug() << addresses.join(", ") << QChar(124).toLatin1() << "UDP OSC Server enabled (read port " << _portReceive <<").";

        // broadcast the information of connection
        QVariantList args;
        args.append(addresses.last());
        args.append(_portReceive);
        broadcastDatagram( OSC_REQUEST_CONNECT, args );
    }
    else
        qDebug() << "OpenSoundControlManager" << QChar(124).toLatin1() << "UDP OSC Server disabled.";

}


void OpenSoundControlManager::broadcastDatagram(QString property, QVariantList args)
{
    // ignore invalid
    if (!_udpBroadcast || property.isEmpty())
        return;

    // create an osc packet
    char buffer[1024];
    osc::OutboundPacketStream p( buffer, 1024 );

    // begin message
    p << osc::BeginMessage( QString("/glmixer/%1").arg(property).toLatin1().constData() );

    // fill argument list
    foreach (QVariant arg, args) {

        switch (arg.type()) {
        case QVariant::Int:
            p << (osc::int32) arg.toInt();
            break;
        case QVariant::UInt:
            p << (osc::int32) arg.toUInt();
            break;
        case QVariant::Double:
            p << (float) arg.toDouble();
            break;
        case QVariant::Bool:
            p << (bool) arg.toBool();
            break;
        case QVariant::String:
            p << (char *) qPrintable(arg.toString());
            break;
        default:
            break;
        }

    }
    // end message
    p << osc::EndMessage;

    // broadcast message
    _udpBroadcast->writeDatagram( p.Data(), p.Size(),
                                 QHostAddress::Broadcast, _portBroadcast);

}


void OpenSoundControlManager::readPendingDatagrams()
{
    if (!_udpReceive)
        return;

    // read all datagrams
    while (_udpReceive->hasPendingDatagrams())
    {
        // read data
        QHostAddress sender;
        quint16 senderPort;
        QByteArray datagram;
        datagram.resize(_udpReceive->pendingDatagramSize());
        _udpReceive->readDatagram(datagram.data(), datagram.size(), &sender, &senderPort);

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
                    }  else if ((arg)->IsString()) {
                        args.append( QString( (arg)->AsString() ) );
                        values.append(" " + QString( (arg)->AsString() ));
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


void invoke(Source *s, QString property, QVariantList args)
{
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
    // end the string with closing parenthesis instead of comas
    if (slot.contains(','))
        slot.chop(1);
    slot += ')';

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

void OpenSoundControlManager::executeSource(Source *s, QString property, QVariantList args)
{
    VideoFile *vf = NULL;

    // discard invalid arguments
    if (s && !property.isEmpty()) {

        // prepare for media sources
        if ( s->rtti() == Source::VIDEO_SOURCE ) {
            // get the pointer to the video file to control
            vf = ( dynamic_cast<VideoSource *>(s))->getVideoFile();
        }

        //  case of PRESET property
        if ( property.compare(OSC_SOURCE_PRESET, Qt::CaseInsensitive) == 0 ){
            if (args.size() > 0 && args[0].isValid())
                emit applyPreset(s->getName(), args[0].toString() );
            else
                throw osc::MissingArgumentException();
        }
        // Play / Stop source
        else if ( property.compare(OSC_SOURCE_PLAY, Qt::CaseInsensitive) == 0 ) {
            if (args.size() > 0 && args[0].isValid())
                s->play( args[0].toBool() );
            else
                throw osc::MissingArgumentException();
        }
        // pause video file
        else if ( property.compare(OSC_SOURCE_PAUSE, Qt::CaseInsensitive) == 0 && vf ) {
            if (args.size() > 0 && args[0].isValid())
                vf->pause( args[0].toBool() );
            else
                throw osc::MissingArgumentException();
        }
        // loop on / off
        else if ( property.compare(OSC_SOURCE_LOOP, Qt::CaseInsensitive) == 0 && vf ) {
            if (args.size() > 0 && args[0].isValid())
                vf->setLoop( args[0].toBool() );
            else
                throw osc::MissingArgumentException();
        }
        // fast forward of video file
        else if ( property.compare(OSC_SOURCE_FASTFORWARD, Qt::CaseInsensitive) == 0 && vf ) {
            if (args.size() > 0 && args[0].isValid())
                vf->setFastForward( args[0].toBool() );
            else
                throw osc::MissingArgumentException();
        }
        // source speed of video file
        else if ( property.compare(OSC_SOURCE_SPEED, Qt::CaseInsensitive) == 0 && vf ) {
            if (args.size() > 0 && args[0].isValid()) {
                bool ok = false;
                double v = args[0].toDouble(&ok);
                if (ok)
                    vf->setPlaySpeed( v );
                else
                    throw osc::WrongArgumentTypeException();
            }
            else
                throw osc::MissingArgumentException();
        }
        // source speed of video file
        else if ( property.compare(OSC_SOURCE_TIMING, Qt::CaseInsensitive) == 0 && vf ) {
            if (args.size() > 0 && args[0].isValid()) {
                bool ok = false;
                double v = qBound(0.0, args[0].toDouble(&ok), 1.0);
                if (ok) {
                    vf->seekToPosition( vf->getMarkIn() + v * (vf->getMarkOut()-vf->getMarkIn()) );
                } else
                    throw osc::WrongArgumentTypeException();
            }
            else
                throw osc::MissingArgumentException();
        }
        // general case : property name is Proto Source property
        else
            // invoke the call with given property and arguments on that source
            invoke( s, property, args);

    }
}

void OpenSoundControlManager::execute(QString object, QString property, QVariantList args)
{
    // Target OBJECT named "void" (debug)
    if ( object.compare(OSC_VOID, Qt::CaseInsensitive) == 0 ) {
        // Target ATTRIBUTE for render : alpha (transparency)
        if ( property.compare(OSC_VOID_LOG, Qt::CaseInsensitive) == 0 ) {
            QString msg("/void/Log ");
            int i = 0;
            for (; i < args.size() ; ++i, msg += ' ' ) {
                msg += args[i].toString();
            }
            emit log(msg);
        }
        else if ( property.compare(OSC_VOID_IGNORE, Qt::CaseInsensitive) == 0 )
        {}
        else
            throw osc::InvalidAttributeException();
    }
    // Target OBJECT named "render" (control rendering attributes)
    else if ( object.compare(OSC_RENDER, Qt::CaseInsensitive) == 0 ) {

        executeRender(property, args);
    }
    // Target OBJECT "request" (request for information)
    else if ( object.compare(OSC_REQUEST, Qt::CaseInsensitive) == 0 ) {

        executeRequest(property, args);
    }
    // Target OBJECT "select" (switch next or previous current source)
    else if ( object.compare(OSC_SELECT, Qt::CaseInsensitive) == 0 ) {
        // Target ATTRIBUTE next source
        if ( property.compare(OSC_SELECT_NEXT, Qt::CaseInsensitive) == 0 )
            RenderingManager::getInstance()->setCurrentNext();
        // Target ATTRIBUTE previous source
        else if ( property.compare(OSC_SELECT_PREVIOUS, Qt::CaseInsensitive) == 0 )
            RenderingManager::getInstance()->setCurrentPrevious();
        // Target ATTRIBUTE no source
        else if ( property.compare(OSC_SELECT_NONE, Qt::CaseInsensitive) == 0 )
            RenderingManager::getInstance()->unsetCurrentSource();
        else {
            SourceSet::const_iterator sit = RenderingManager::getInstance()->getByName(property);
            if ( RenderingManager::getInstance()->notAtEnd(sit))
                // set current by name
                RenderingManager::getInstance()->setCurrentSource(sit);
            else
                throw osc::InvalidAttributeException();
        }
    }
    // Target OBJECT named "current" (control current source attributes)
    else if ( object.compare(OSC_SOURCE_CURRENT, Qt::CaseInsensitive) == 0 ) {

        SourceSet::const_iterator sit = RenderingManager::getInstance()->getCurrentSource();
        // if the current source is valid
        if ( RenderingManager::getInstance()->notAtEnd(sit))
            executeSource( *sit, property, args);
        // NB: do not warn if no current source
    }
    // Target OBJECT : name of a source
    else {
        SourceSet::const_iterator sit = RenderingManager::getInstance()->getByName(object);
        // if the given source exists
        if ( RenderingManager::getInstance()->notAtEnd(sit))
            executeSource( *sit, property, args);
        else
            // inform that the source name is wrong
            throw osc::InvalidObjectException();
    }

}

void OpenSoundControlManager::executeRender(QString property, QVariantList args)
{
    // Target ATTRIBUTE for render : alpha (transparency)
    if ( property.compare(OSC_RENDER_ALPHA, Qt::CaseInsensitive) == 0 ) {
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
    else if ( property.compare(OSC_RENDER_TRANSPARENCY, Qt::CaseInsensitive) == 0 ) {
        if (args.size() > 0 && args[0].isValid()) {
            bool ok = false;
            double v = qBound(0.0, args[0].toDouble(&ok), 1.0);
            if (ok)
                GLMixer::getInstance()->on_output_alpha_valueChanged( (int) (v * 100.0) );
            else
                throw osc::WrongArgumentTypeException();
        }
        else
            throw osc::MissingArgumentException();
    }
    else if ( property.compare(OSC_RENDER_PAUSE, Qt::CaseInsensitive) == 0 ) {
        bool val = true;
        // optional argument
        if (args.size() > 0 && args[0].isValid())
            val = args[0].toBool();
        RenderingManager::getInstance()->pause( val );
    }
    else if ( property.compare(OSC_RENDER_UNPAUSE, Qt::CaseInsensitive) == 0 ) {
        bool val = false;
        // optional argument
        if (args.size() > 0 && args[0].isValid())
            val = ! args[0].toBool();
        RenderingManager::getInstance()->pause( val );
    }
    else if ( property.compare(OSC_RENDER_NEXT, Qt::CaseInsensitive) == 0 ) {
        // if argument is given, react only to TRUE value
        if (args.size() > 0 && args[0].isValid() && args[0].toBool() )
            GLMixer::getInstance()->openNextSession();
        else
            GLMixer::getInstance()->openNextSession();
    }
    else if ( property.compare(OSC_RENDER_PREVIOUS, Qt::CaseInsensitive) == 0 ) {
        // if argument is given, react only to TRUE value
        if (args.size() > 0 && args[0].isValid() && args[0].toBool() )
            GLMixer::getInstance()->openPreviousSession();
        else
            GLMixer::getInstance()->openPreviousSession();
    }
    else
        throw osc::InvalidAttributeException();
}


void OpenSoundControlManager::broadcastSourceCount(int count)
{
    // broadcast the count of source
    QVariantList args;
    args.append(count);
    broadcastDatagram( OSC_REQUEST_COUNT, args );
}

void OpenSoundControlManager::executeRequest(QString property, QVariantList args)
{
    // Target ATTRIBUTE for request : count (number of sources)
    if ( property.compare(OSC_REQUEST_COUNT, Qt::CaseInsensitive) == 0 ) {
        // get the list of source names
        QStringList list = RenderingManager::getInstance()->getSourceNameList();
        // reply to request with count of sources
        broadcastSourceCount(list.count());
        emit log(QString("Replied /glmixer/count i %1 ").arg(list.count()) );

    }
    // Target ATTRIBUTE for request : current (name of the current source)
    if ( property.compare(OSC_REQUEST_CURRENT, Qt::CaseInsensitive) == 0 ) {
        // if the current source is valid
        SourceSet::const_iterator sit = RenderingManager::getInstance()->getCurrentSource();
        if ( RenderingManager::getInstance()->notAtEnd(sit)) {
            // get the name of current source
            QString name = (*sit)->getName();
            // reply to request with name of source
            broadcastDatagram( name );
            emit log(QString("Replied /glmixer/%1").arg(name) );
        }

    }
    // Target ATTRIBUTE for request : name (of the source at given index)
    else if ( property.compare(OSC_REQUEST_NAME, Qt::CaseInsensitive) == 0 ) {
        // read the argument : index of source requested
        if (args.size() > 0 && args[0].isValid()) {
            bool ok = false;
            double n = args[0].toInt(&ok);
            if (ok) {
                // get the list of source names
                QStringList list = RenderingManager::getInstance()->getSourceNameList();

                if ( n < 0 || n > list.count()-1 )
                    emit error(QString("Invalid request : index %1 outside of range [0..%2].").arg(n).arg(list.count()-1) );
                else {
                    // reply to request with name of source
                    broadcastDatagram( list[n] );
                    emit log(QString("Replied /glmixer/%1").arg(list[n]) );
                }
            }
            else
                throw osc::WrongArgumentTypeException();
        }
        else
            throw osc::MissingArgumentException();
    }
    else
        throw osc::InvalidAttributeException();
}

void OpenSoundControlManager::addTranslation(QString before, QString after)
{
    _dictionnary->append( QPair<QString, QString>(before, after) );
}

bool OpenSoundControlManager::hasTranslation(QString before, QString after)
{
    return _dictionnary->count( (QPair<QString, QString>(before, after) ) ) > 0;
}
