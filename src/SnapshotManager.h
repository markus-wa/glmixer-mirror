#ifndef SNAPSHOTMANAGER_H
#define SNAPSHOTMANAGER_H

#include <QObject>
#include <QPixmap>
#include <QMap>
#include <QDomDocument>
#include <QVector>

#define SNAPSHOT_ICON_SIZE 128
#define TEMP_SNAPSHOT_NAME "temp"

class Source;

class SnapshotManager : public QObject
{
    Q_OBJECT

    explicit SnapshotManager(QObject *parent = nullptr);
    static SnapshotManager *_instance;

public:
    static SnapshotManager *getInstance();
    static QImage generateSnapshotIcon(QImage image);

    /**
     * Access snapshot list
     * */
    QStringList getSnapshotIdentifiers();
    QImage getSnapshotImage(QString id);
    QString getSnapshotLabel(QString id);
    void setSnapshotLabel(QString id, QString label);
    void removeSnapshot(QString id);

    /**
     * Access content of snapshots
     * */
    QMap<Source *, QVector <double> > getSnapshot(QString id);
    void restoreSnapshot(QString id = TEMP_SNAPSHOT_NAME);

    /**
     * get and set configuration
     */
    QDomElement getConfiguration(QDomDocument &doc);
    void addConfiguration(QDomElement xmlconfig);
    void clearConfiguration();

    /**
     * store the temporary snapshot
     */
    void storeTemporarySnapshotDescription();

signals:
    void clear();
    void snap();
    void newSnapshot(QString id);
    void deleteSnapshot(QString id);
    void snapshotRestored();
    void status(QString, int);

public slots:
    void addSnapshot();

protected:
    void appendSnapshotDescrition(QString id, QString label, QDomElement config);

private:
    QDomDocument _snapshotsDescription;
    QMap< QString, QImage> _snapshotsList;
};

#endif // SNAPSHOTMANAGER_H
