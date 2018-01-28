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

}


void SnapshotManager::setConfiguration(QDomElement xmlconfig)
{

}

QStringList SnapshotManager::getSnapshotIdentifiers()
{
    return QStringList( _snapshotsList.keys() );
}

QString SnapshotManager::addSnapshot()
{
    // create a unique id from date & time
    QString id = QDateTime::currentDateTime().toString("yyyyMMddhhmmsszzz");

    // capture screenshot
    QPixmap s = QPixmap::fromImage(RenderingManager::getRenderingWidget()->grabFrameBuffer());
    if (s.isNull())
        s = QPixmap(QString::fromUtf8(":/glmixer/icons/glmixer_256x256.png"));

    // convert pixmap to ICON_SIZE x ICON_SIZE
    s = s.width() < s.height() ? s.scaledToWidth(ICON_SIZE, Qt::SmoothTransformation) : s.scaledToHeight(ICON_SIZE, Qt::SmoothTransformation);
    s = s.copy( (s.width()-ICON_SIZE)/2, (s.height()-ICON_SIZE)/2, ICON_SIZE, ICON_SIZE );

    // add element in list
    _snapshotsList[id] = s;

    // add the configuration
    QDomElement renderConfig = RenderingManager::getInstance()->getConfiguration(_snapshotsDescription);
    if (!renderConfig.isNull()) {
        // create node
        QDomElement root = _snapshotsDescription.createElement( id );

        // TODO add JPEG of Pixmap

        // add the whole config to the undo element
        root.appendChild(renderConfig);

        // add node
        _snapshotsDescription.appendChild(root);
    }

    emit snap();

    return id;
}

void SnapshotManager::restoreSnapshot(QString id)
{
    if (_snapshotsList.contains(id)) {
        // get status of sources at index in history
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
                    child = child.nextSiblingElement("Source");
                }
            }
        }
    }
}


