/*
 * SharedMemoryManager.h
 *
 *  Created on: Aug 6, 2011
 *      Author: bh
 */

#ifndef SHAREDMEMORYMANAGER_H_
#define SHAREDMEMORYMANAGER_H_

#include <QVariant>
#include <QSharedMemory>

class SharedMemoryManager {

public:
	static SharedMemoryManager *getInstance();
	static void deleteInstance();

	// get the full map of shm
	QMap<qint64, QVariantMap> getSharedMap();
	// get the item with the given ID
	QVariantMap getItemSharedMap(qint64 pid);
	// add / remove items
	void addItemSharedMap(qint64 pid, QVariantMap descriptormap);
	void removeItemSharedMap(qint64 pid);

	// find in the map if there is a match for this key
	qint64 findItemSharedMap(QString key);

    // find in the map if there is a match for a program of that name
	qint64 findProgramSharedMap(QString program);

private:
	SharedMemoryManager();
	virtual ~SharedMemoryManager();

	QMap<qint64, QVariantMap>  readMap();
	void writeMap(QMap<qint64, QVariantMap> glmixerMap);

	static SharedMemoryManager *_instance;

    QSharedMemory *glmixerShmMap;
};

#endif /* SHAREDMEMORYMANAGER_H_ */
