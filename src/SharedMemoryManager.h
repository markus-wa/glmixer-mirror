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

	QMap<qint64, QVariantMap> getSharedMap();

private:
	SharedMemoryManager();
	virtual ~SharedMemoryManager();

	static SharedMemoryManager *_instance;

    QSharedMemory *glmixerShmMap;
};

#endif /* SHAREDMEMORYMANAGER_H_ */
