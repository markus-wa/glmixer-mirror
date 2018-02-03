#include "RenderingManager.h"
#include "ViewRenderWidget.h"

#include "SnapshotManager.moc"

// static members
SnapshotManager *SnapshotManager::_instance = 0;

SnapshotManager::SnapshotManager(QObject *parent) : QObject(parent)
{

}

SnapshotManager *SnapshotManager::getInstance() {

    if (_instance == 0) {
        // ok to instanciate SnapshotManager
        _instance = new SnapshotManager;
        Q_CHECK_PTR(_instance);
    }

    return _instance;
}


QDomElement SnapshotManager::getConfiguration(QDomDocument &doc)
{
    QDomElement config = doc.createElement("SnapshotList");

    QDomElement child = _snapshotsDescription.firstChildElement();
    while (!child.isNull()) {

        // copy config
        QDomNode newnode = config.appendChild( child.cloneNode() );

        // get icon of snapshot
        QImage pix = _snapshotsList[newnode.nodeName()];

        // Add JPEG of icon in XML
        if (!pix.isNull()) {
            QByteArray ba;
            QBuffer buffer(&ba);
            buffer.open(QIODevice::WriteOnly);
            if (!pix.save(&buffer, "jpeg") )
                qWarning() << "SnapshotManager"  << QChar(124).toLatin1() << tr("Could not save icon.");
            buffer.close();
            QDomElement f = doc.createElement("Image");
            QDomText img = doc.createTextNode( QString::fromLatin1(ba.constData(), ba.size()) );
            f.appendChild(img);

            newnode.appendChild(f);
        }

        child = child.nextSiblingElement();
    }

    return config;
}

void SnapshotManager::addConfiguration(QDomElement xmlconfig)
{
    QDomElement child = xmlconfig.firstChildElement();
    while (!child.isNull()) {

        QString id = child.nodeName();

        // load icon
        QDomElement img = child.firstChildElement("Image");
        QImage image;
        QByteArray data =  img.text().toLatin1();
        if ( image.loadFromData( reinterpret_cast<const uchar *>(data.data()), data.size()) )
            _snapshotsList[id] = image;
        else
            _snapshotsList[id] = QImage(QString::fromUtf8(":/glmixer/icons/snapshot.png"));

        // create new entry
        QDomElement childnew = _snapshotsDescription.createElement(id);
        childnew.setAttribute("label", child.attribute("label"));
        // copy content of the snapshot
        childnew.appendChild( child.firstChildElement("SourceList") );
        // add snapshot to description list
        _snapshotsDescription.appendChild(childnew);

        // inform GUI
        emit newSnapshot(id);

        // loop
        child = child.nextSiblingElement();
    }

}

void SnapshotManager::clearConfiguration()
{
    _snapshotsList.clear();
    _snapshotsDescription = QDomDocument();

    emit clear();
}

QStringList SnapshotManager::getSnapshotIdentifiers()
{
    return QStringList( _snapshotsList.keys() );
}

QPixmap SnapshotManager::getSnapshotPixmap(QString id)
{
    return QPixmap::fromImage(_snapshotsList[id] );
}

void SnapshotManager::setSnapshotLabel(QString id, QString label)
{
    QDomElement root = _snapshotsDescription.firstChildElement(id);
    if ( !root.isNull())
        root.setAttribute("label", label);
}

QString SnapshotManager::getSnapshotLabel(QString id)
{
    QString label = id;

    QDomElement root = _snapshotsDescription.firstChildElement(id);
    if ( !root.isNull())
        label = root.attribute("label", id);

    return label;
}

void SnapshotManager::removeSnapshot(QString id)
{
    if (_snapshotsList.contains(id)) {

        _snapshotsList.remove(id);

        QDomElement root = _snapshotsDescription.firstChildElement(id);
        if (!root.isNull())
            _snapshotsDescription.removeChild(root);

        emit deleteSnapshot(id);
    }
}

void SnapshotManager::addSnapshot()
{
    // create a unique id from date & time
    QString id = QDateTime::currentDateTime().toString("yyyyMMddhhmmsszzz");
    id.prepend("s");

    // capture screenshot
    QImage capture = RenderingManager::getInstance()->captureFrameBuffer();
    if (capture.isNull())
        capture = QImage(QString::fromUtf8(":/glmixer/icons/snapshot.png"));

    // convert image to ICON_SIZE
    capture = capture.width() < capture.height() ? capture.scaledToWidth(ICON_SIZE, Qt::SmoothTransformation) : capture.scaledToHeight(ICON_SIZE, Qt::SmoothTransformation);

    // add element in list
    _snapshotsList[id] = capture;

    // create node
    QDomElement root = _snapshotsDescription.createElement( id );

    // Default Label
    root.setAttribute("label", id);

    // add the configuration
    QDomElement renderConfig = RenderingManager::getInstance()->getConfiguration(_snapshotsDescription);
    root.appendChild(renderConfig);

    // add node
    _snapshotsDescription.appendChild(root);

    emit snap();
    emit newSnapshot(id);
}

void SnapshotManager::restoreSnapshot(QString id)
{
    // get status of sources at given id snaphot
    QDomElement root = _snapshotsDescription.firstChildElement(id);
    if ( !root.isNull()) {
        // get the list of sources at index
        QDomElement renderConfig = root.firstChildElement("SourceList");
        if ( !renderConfig.isNull()) {

            // browse the list of source in history
            QDomElement child = renderConfig.firstChildElement("Source");
            while (!child.isNull()) {

                QString sourcename = child.attribute("name");
                SourceSet::iterator sit = RenderingManager::getInstance()->getByName(sourcename);
                if ( RenderingManager::getInstance()->isValid(sit) )  {

                    // apply configuration
                    if ( ! (*sit)->setConfiguration(child) )
                        qDebug() << "restoreSnapshot" << QChar(124).toLatin1() << "failed to set configuration" << sourcename;

                    // apply change of depth
                    double depth = child.firstChildElement("Depth").attribute("Z", "0").toDouble();
                    RenderingManager::getInstance()->changeDepth(sit, depth);

                }
                else
                    qDebug() << "restoreSnapshot" << QChar(124).toLatin1() << "no such  configuration" << sourcename;

                // read next source
                child = child.nextSiblingElement();
            }
        }
    }
}


