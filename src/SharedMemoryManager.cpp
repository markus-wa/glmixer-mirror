/*
 * SharedMemoryManager.cpp
 *
 *  Created on: Aug 6, 2011
 *      Author: bh
 */

#include "SharedMemoryManager.h"

#define MAX_NUM_SHM 100

#include <QtDebug>
#include <QDataStream>
#include <QBuffer>
#include <QMap>

SharedMemoryManager *SharedMemoryManager::_instance = 0;

SharedMemoryManager::SharedMemoryManager() {

    //
    // Create the shared memory map
    //
	glmixerShmMap = new QSharedMemory("glmixerSharedMemoryMap");

	// dummy map to know the size of MAX_NUM_SHM data structures
    QBuffer buffer;
    buffer.open(QIODevice::WriteOnly);
    QDataStream bufstream(&buffer);
    QMap<qint64, QVariantMap> glmixerMap;
    QVariantMap processInformation;
    processInformation["program"] = "dummy";
    processInformation["width"] = 100000;
    processInformation["height"] = 100000;
    processInformation["format"] = 0;
    for (qint64 i = 0; i < MAX_NUM_SHM; ++i)
    	glmixerMap[i] = processInformation;
    bufstream << glmixerMap;
    buffer.close();

    // make sure we detach
    while (glmixerShmMap->isAttached()) {
        qDebug() << "SharedMemoryManager|" << QObject::tr("Detaching shared memory map");
        glmixerShmMap->detach();
    }
    // try to create or attach
    if (glmixerShmMap->create(buffer.size()))
        qDebug() << "SharedMemoryManager|" << "Shared map created (" <<  glmixerShmMap->size() << "bytes).";
    else {
    	glmixerShmMap->attach(QSharedMemory::ReadWrite);
    	qWarning() << "SharedMemoryManager|" << "Shared map existed (" <<  glmixerShmMap->size() << "bytes).";
    }

}

SharedMemoryManager::~SharedMemoryManager() {

	// this also detaches the QSharedMemory
	delete glmixerShmMap;
}

SharedMemoryManager *SharedMemoryManager::getInstance() {

	if (_instance == 0)
		_instance = new SharedMemoryManager();

	return _instance;
}

void SharedMemoryManager::deleteInstance(){

	if (_instance)
		delete _instance;

	_instance = 0;
}


QMap<qint64, QVariantMap> SharedMemoryManager::getSharedMap(){

    QBuffer buffer;
    QDataStream bufstream(&buffer);
    QMap<qint64, QVariantMap> glmixerMap;

    // read the current map
    glmixerShmMap->lock();
    buffer.setData((char*)glmixerShmMap->constData(), glmixerShmMap->size());
    buffer.open(QBuffer::ReadOnly);
    bufstream >> glmixerMap;

//    qDebug() << "read " << glmixerMap;

    glmixerShmMap->unlock();
    buffer.close();

    return glmixerMap;

}

