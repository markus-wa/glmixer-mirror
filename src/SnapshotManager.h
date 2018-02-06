#ifndef SNAPSHOTMANAGER_H
#define SNAPSHOTMANAGER_H

#include <QObject>
#include <QPixmap>
#include <QMap>
#include <QDomDocument>
#include <QVector>

#define ICON_SIZE 128

class Source;

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
    QPixmap getSnapshotPixmap(QString id);
    QString getSnapshotLabel(QString id);
    void setSnapshotLabel(QString id, QString label);
    void removeSnapshot(QString id);

    /**
     * Access source lists
     * */
    QMap<Source *, QVector <double> > getSnapshot(QString id);

    /**
     * get and set configuration
     */
    QDomElement getConfiguration(QDomDocument &doc);
    void addConfiguration(QDomElement xmlconfig);
    void clearConfiguration();

signals:
    void clear();
    void snap();
    void newSnapshot(QString id);
    void deleteSnapshot(QString id);

public slots:
    void addSnapshot();
    void restoreSnapshot(QString id);

protected:
    void appendSnapshotDescrition(QString id, QString label, QDomElement config);

private:
    QDomDocument _snapshotsDescription;
    QMap< QString, QImage> _snapshotsList;
};

#endif // SNAPSHOTMANAGER_H
