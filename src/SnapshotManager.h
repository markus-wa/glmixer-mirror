#ifndef SNAPSHOTMANAGER_H
#define SNAPSHOTMANAGER_H

#include <QObject>
#include <QPixmap>
#include <QMap>
#include <QDomDocument>

#define ICON_SIZE 128

class SnapshotManager : public QObject
{
    Q_OBJECT
    explicit SnapshotManager(QObject *parent = nullptr);
    static SnapshotManager *_instance;

public:
    static SnapshotManager *getInstance();

    /**
     * Access items
     * */
    QStringList getSnapshotIdentifiers();
    QPixmap getSnapshotPixmap(QString id) { return _snapshotsList[id]; }

    void setSnapshotLabel(QString id, QString label);
    QString getSnapshotLabel(QString id);

    void removeSnapshot(QString id);

    /**
     * get and set configuration
     */
    QDomElement getConfiguration(QDomDocument &doc);
    void addConfiguration(QDomElement xmlconfig);
    void clearConfiguration();

signals:
    void clear();
    void newSnapshot(QString id);
    void deleteSnapshot(QString id);

public slots:
    void addSnapshot();
    void restoreSnapshot(QString id);

private:
    QDomDocument _snapshotsDescription;
    QMap< QString, QPixmap> _snapshotsList;
};

#endif // SNAPSHOTMANAGER_H
