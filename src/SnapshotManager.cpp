#include "RenderingManager.h"
#include "ViewRenderWidget.h"
#include "UndoManager.h"

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

        // ignore temporary node
        if (child.attribute("label", TEMP_SNAPSHOT_NAME) != TEMP_SNAPSHOT_NAME) {

            // copy config
            QDomNode newnode = config.appendChild( child.cloneNode() );

            // restore name and remove id for every source in copy
            QDomElement renderConfig = newnode.firstChildElement("SourceList");
            if ( !renderConfig.isNull()) {
                // browse the list of source in history
                QDomElement source = renderConfig.firstChildElement("Source");
                while (!source.isNull()) {
                    // to access sources even after renaming
                    GLuint id = source.attribute("id", "0").toUInt();
                    SourceSet::iterator sit = RenderingManager::getInstance()->getById(id);
                    if ( RenderingManager::getInstance()->isValid(sit) )  {
                        // set name attribute
                        source.setAttribute("name", (*sit)->getName() );
                        // remove id attribute
                        source.removeAttribute("id");
                    }
                    // read next source
                    source = source.nextSiblingElement();
                }
            }

            // get icon of snapshot
            QImage pix = _snapshotsList[newnode.nodeName()];

            // Add JPEG of icon in XML
            if (!pix.isNull()) {
                QByteArray ba;
                QBuffer buffer(&ba);
                buffer.open(QIODevice::WriteOnly);
                if (!pix.save(&buffer, "png", 70) )
                    qWarning() << "SnapshotManager"  << QChar(124).toLatin1() << tr("Could not save icon.");
                buffer.close();
                ba = ba.toBase64();
                QDomElement f = doc.createElement("Image");
                QDomText img = doc.createTextNode( QString::fromLatin1(ba.constData(), ba.size()) );
                f.appendChild(img);

                newnode.appendChild(f);
            }
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
        QByteArray data =  QByteArray::fromBase64( img.text().toLatin1() );
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

QImage SnapshotManager::getSnapshotImage(QString id)
{
    if (_snapshotsList.contains(id))
        return _snapshotsList[id];
    else
        return QImage();
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
    if (!_snapshotsList.contains(id))
        return;

    _snapshotsList.remove(id);

    QDomElement root = _snapshotsDescription.firstChildElement(id);
    if (!root.isNull())
        _snapshotsDescription.removeChild(root);

    emit deleteSnapshot(id);
}

QImage SnapshotManager::generateSnapshotIcon(QImage image)
{
    // convert image to ICON_SIZE
    image = image.scaled(SNAPSHOT_ICON_SIZE, SNAPSHOT_ICON_SIZE, Qt::KeepAspectRatioByExpanding);

    // copy centered image
    QRect area(0,0,SNAPSHOT_ICON_SIZE,SNAPSHOT_ICON_SIZE);
    area.moveCenter(image.rect().center());
    image = image.copy(area);

    // apply mask
    QRegion r(QRect(0,0,SNAPSHOT_ICON_SIZE,SNAPSHOT_ICON_SIZE), QRegion::Ellipse);
    QImage target(SNAPSHOT_ICON_SIZE,  SNAPSHOT_ICON_SIZE, QImage::Format_ARGB32);
    target.fill(Qt::transparent);
    QPainter painter(&target);
    painter.setClipRegion(r);
    painter.drawImage(0,0,image);

    return target;
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

    // add element in list
    _snapshotsList[id] = SnapshotManager::generateSnapshotIcon(capture);

    // create node
    appendSnapshotDescrition(id, id, RenderingManager::getInstance()->getConfiguration(_snapshotsDescription));

    // inform GUI
    emit snap();
    emit newSnapshot(id);
    emit status(tr("Snapshot %1 created.").arg(id), 2000);
}

void SnapshotManager::restoreSnapshot(QString id)
{
    if ( id != TEMP_SNAPSHOT_NAME && !_snapshotsList.contains(id)) {
        qWarning() << "Snapshot" << QChar(124).toLatin1() << "Snapshot " << id << tr("not found.");
        return;
    }

#ifdef GLM_UNDO
    // inform undo manager
    UndoManager::getInstance()->store();
#endif

    QString label;
    // get status of sources at given id snaphot
    QDomElement root = _snapshotsDescription.firstChildElement(id);
    if ( !root.isNull()) {

        label = root.attribute("label");

        // get the list of sources at index
        QDomElement renderConfig = root.firstChildElement("SourceList");
        if ( !renderConfig.isNull()) {

            // browse the list of source in history
            QDomElement child = renderConfig.firstChildElement("Source");
            while (!child.isNull()) {

                // access source configuration by id (not by name)
                // to access sources even after renaming
                GLuint sid = child.attribute("id", "0").toUInt();
                SourceSet::iterator sit = RenderingManager::getInstance()->getById(sid);
                if ( RenderingManager::getInstance()->isValid(sit) )  {

                    // apply configuration
                    if ( ! (*sit)->setConfiguration(child) )
                        qWarning() << "Snapshot" << QChar(124).toLatin1() << tr("failed to set configuration.");

                    // as we apply changes to alpha, source migh be standly or not
                    (*sit)->setStandby(false);

                    // apply change of depth
                    double depth = child.firstChildElement("Depth").attribute("Z", "0").toDouble();
                    RenderingManager::getInstance()->changeDepth(sit, depth);

                }
                else
                    qWarning() << "Snapshot" << QChar(124).toLatin1() << tr("Silently ignoring configuration of a non-existing source.");

                // read next source
                child = child.nextSiblingElement();
            }
        }
    }

    emit snapshotRestored();
    emit status(tr("Snapshot %1 restored.").arg(label), 2000);
}



void SnapshotManager::appendSnapshotDescrition(QString id, QString label, QDomElement config)
{
    if (!_snapshotsList.contains(id))
        return;

    // create description
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
            if ( RenderingManager::getInstance()->isValid(sit) ) {
                child.setAttribute("id", (*sit)->getId() );

                // discard properties that we do not want to be stored
                child.removeAttribute("name");
                child.removeAttribute("stanbyMode");
                child.removeAttribute("workspace");
#ifdef GLM_TAG
                child.removeAttribute("tag");
#endif
            }
            // read next source
            child = child.nextSiblingElement();
        }
    }

    // add snapshot to description list
    _snapshotsDescription.appendChild(root);
}

void SnapshotManager::storeTemporarySnapshotDescription()
{
    QDomElement config = RenderingManager::getInstance()->getConfiguration(_snapshotsDescription);

    // get and clear the previous temporary element
    QDomElement root = _snapshotsDescription.firstChildElement(TEMP_SNAPSHOT_NAME);
    if ( root.isNull()) {
        // create clean element
        root = _snapshotsDescription.createElement(TEMP_SNAPSHOT_NAME);
        root.setAttribute("label", TEMP_SNAPSHOT_NAME);
        // copy content of the config
        root.appendChild( config );
    }
    else
        root.replaceChild( root.firstChildElement("SourceList"), config );

    // post process list to add identifiers to each source
    QDomElement renderConfig = root.firstChildElement("SourceList");
    if ( !renderConfig.isNull()) {

        // browse the list of source in history
        QDomElement child = renderConfig.firstChildElement("Source");
        while (!child.isNull()) {
            // add an attribute with id to follow source after renaming
            QString sourcename = child.attribute("name");
            SourceSet::iterator sit = RenderingManager::getInstance()->getByName(sourcename);
            if ( RenderingManager::getInstance()->isValid(sit) ) {
                child.setAttribute("id", (*sit)->getId() );

                // discard properties that we do not want to be stored
                child.removeAttribute("name");
                child.removeAttribute("stanbyMode");
                child.removeAttribute("workspace");
#ifdef GLM_TAG
                child.removeAttribute("tag");
#endif
            }
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

    if (!_snapshotsList.contains(id))
        return list;

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
