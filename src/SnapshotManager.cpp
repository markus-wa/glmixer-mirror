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
        appendSnapshotDescrition(id, child.attribute("label"), child.firstChildElement("SourceList"));

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
    appendSnapshotDescrition(id, id, RenderingManager::getInstance()->getConfiguration(_snapshotsDescription));

    // inform GUI
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
                // access source configuration by id (not by name)
                // to access sources even after renaming
                GLuint id = child.attribute("id", "0").toUInt();
                SourceSet::iterator sit = RenderingManager::getInstance()->getById(id);
                if ( RenderingManager::getInstance()->isValid(sit) )  {

                    // apply configuration
                    if ( ! (*sit)->setConfiguration(child) )
                        qDebug() << "restoreSnapshot" << QChar(124).toLatin1() << "failed to set configuration" << sourcename;

                    // as we apply changes to alpha, source migh be standly or not
                    (*sit)->setStandby(false);

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



void SnapshotManager::appendSnapshotDescrition(QString id, QString label, QDomElement config)
{
    QDomElement root = _snapshotsDescription.createElement( id );

    // set Label
    root.setAttribute("label", label);
    // copy content of the config
    root.appendChild( config );

    // post process list to add identifiers to each source
    QDomElement renderConfig = root.firstChildElement("SourceList");
    if ( !renderConfig.isNull()) {

        // browse the list of source in history
        QDomElement child = renderConfig.firstChildElement("Source");
        while (!child.isNull()) {
            // add an attribute with id to follow source after renaming
            QString sourcename = child.attribute("name");
            SourceSet::iterator sit = RenderingManager::getInstance()->getByName(sourcename);
            if ( RenderingManager::getInstance()->isValid(sit) )
                child.setAttribute("id", (*sit)->getId() );

            // discard properties that we do not want to be stored
            child.removeAttribute("stanbyMode");
            child.removeAttribute("workspace");
#ifdef GLM_TAG
            child.removeAttribute("tag");
#endif
            // read next source
            child = child.nextSiblingElement();
        }
    }
    // add snapshot to description list
    _snapshotsDescription.appendChild(root);
}


QMap<Source *, QVector <double> > SnapshotManager::getSnapshot(QString id)
{
    QMap<Source *, QVector <double>  > list;

    QDomElement root = _snapshotsDescription.firstChildElement(id);
    if ( !root.isNull()) {

        // read source list
        QDomElement renderConfig = root.firstChildElement("SourceList");
        if ( !renderConfig.isNull()) {
            // browse the list of source in history
            QDomElement child = renderConfig.firstChildElement("Source");
            while (!child.isNull()) {

                GLuint sid = child.attribute("id", "0").toUInt();
                SourceSet::iterator sit = RenderingManager::getInstance()->getById(sid);
                if ( RenderingManager::getInstance()->isValid(sit) )  {

                    QVector <double>  snap;
                    snap << child.firstChildElement("Alpha").attribute("X", "0").toDouble(); // index 0
                    snap << child.firstChildElement("Alpha").attribute("Y", "0").toDouble(); // index 1
                    snap << child.firstChildElement("Position").attribute("X", "0").toDouble(); // index 2
                    snap << child.firstChildElement("Position").attribute("Y", "0").toDouble(); // index 3
                    snap << child.firstChildElement("Scale").attribute("X", "1").toDouble(); // index 4
                    snap << child.firstChildElement("Scale").attribute("Y", "1").toDouble(); // index 5
                    snap << child.firstChildElement("Angle").attribute("A", "0").toDouble(); // index 6
                    snap << child.firstChildElement("Crop").attribute("X", "0").toDouble(); // index 7
                    snap << child.firstChildElement("Crop").attribute("Y", "0").toDouble(); // index 8
                    snap << child.firstChildElement("Crop").attribute("W", "0").toDouble(); // index 9
                    snap << child.firstChildElement("Crop").attribute("H", "0").toDouble(); // index 10
                    snap << child.firstChildElement("Depth").attribute("Z", "0").toDouble(); // index 11

                    list[*sit] = snap;
                }
                // read next source
                child = child.nextSiblingElement();
            }
        }

    }


    return list;
}