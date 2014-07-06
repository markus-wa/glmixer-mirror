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

    // make sure we detach
    glmixerShmMap->detach();

    if (!attach())
        qDebug() << "SharedMemoryManager|" << QObject::tr("Could not attach to GLMixer shared memory map");

}

SharedMemoryManager::~SharedMemoryManager() {

	// this also detaches the QSharedMemory
	delete glmixerShmMap;
}

bool SharedMemoryManager::attach() {

    if (!glmixerShmMap->isAttached()) {
        if (!glmixerShmMap->attach(QSharedMemory::ReadWrite))
            return false;
        else
            return true;
    } else
        return true;
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


QMap<qint64, QVariantMap>  SharedMemoryManager::readMap(){

    QBuffer buffer;
    QDataStream in(&buffer);
    QMap<qint64, QVariantMap> glmixerMap;

    glmixerShmMap->lock();

    // read the current map
    buffer.setData((char*)glmixerShmMap->constData(), glmixerShmMap->size());
    buffer.open(QBuffer::ReadOnly);
    in >> glmixerMap;
    buffer.close();

    glmixerShmMap->unlock();

    return glmixerMap;
}

void SharedMemoryManager::writeMap(QMap<qint64, QVariantMap> glmixerMap){

    QBuffer bufferwrite;
    QDataStream out(&bufferwrite);


    qDebug() << glmixerMap;


    glmixerShmMap->lock();

    // write the new map
    bufferwrite.open(QBuffer::ReadWrite);
    out << glmixerMap;
    bufferwrite.close();
    char *to = (char*)glmixerShmMap->data();
    const char *from = bufferwrite.data().data();
    memcpy(to, from, qMin(glmixerShmMap->size(), (int) bufferwrite.size()));

    glmixerShmMap->unlock();
}

//

QVariantMap SharedMemoryManager::getItemSharedMap(qint64 pid){

    // read the current map
    QMap<qint64, QVariantMap> glmixerMap = readMap();
    // find the wanted item
    QMap<qint64, QVariantMap>::iterator item = glmixerMap.find(pid);

    if (item != glmixerMap.end())
    	return item.value();
    else
    	return QVariantMap();
}

QMap<qint64, QVariantMap> SharedMemoryManager::getSharedMap(){

    // read the current map
    QMap<qint64, QVariantMap> glmixerMap = readMap();

    // test the viability of each entry, and remove the bad ones
	QMapIterator<qint64, QVariantMap> i(glmixerMap);
	while (i.hasNext()) {
		i.next();

		QSharedMemory *m_sharedMemory = new QSharedMemory(i.value()["key"].toString());

		if( !m_sharedMemory->attach() ) {
			qDebug() << "Deleted invalid shared memory map:" << i.value()["key"].toString();
		    glmixerMap.remove(i.key());
		}
		delete m_sharedMemory;
	}

    // write the new map
	writeMap(glmixerMap);

    return glmixerMap;
}


void SharedMemoryManager::removeItemSharedMap(qint64 pid){

    // read the current map
    QMap<qint64, QVariantMap> glmixerMap = readMap();

    // remove the element
    glmixerMap.remove(pid);

    // write the new map
	writeMap(glmixerMap);
}

void SharedMemoryManager::addItemSharedMap(qint64 pid, QVariantMap descriptormap){

    // read the current map
    QMap<qint64, QVariantMap> glmixerMap = readMap();

    // add the element
    glmixerMap[pid] = descriptormap;

    // write the new map
	writeMap(glmixerMap);
}


qint64 SharedMemoryManager::findItemSharedMap(QString key){

    // read the current map
    QMap<qint64, QVariantMap> glmixerMap = readMap();

    // test the viability of each entry, and remove the bad ones
	QMapIterator<qint64, QVariantMap> i(glmixerMap);
	while (i.hasNext()) {
		i.next();

		// found it ?
		if (i.value()["key"] == key) {
			bool found = true;
			// test it !
			QSharedMemory *m_sharedMemory = new QSharedMemory(i.value()["key"].toString());
			if( !m_sharedMemory->attach() ) {
				qDebug() << "Deleted invalid shared memory map:" << i.value()["key"].toString();
				glmixerMap.remove(i.key());
				found = false;
			}
			delete m_sharedMemory;
			// if it passed the test, return it
			if (found)
				return i.key();
		}
	}

	return 0;
}


bool SharedMemoryManager::hasProgramSharedMap(QString program){

    bool found = false;

    // read the current map
    QMap<qint64, QVariantMap> glmixerMap = readMap();

    // test the viability of each entry, and remove the bad ones
    QMapIterator<qint64, QVariantMap> i(glmixerMap);
    while (i.hasNext()) {
        i.next();

        // found it ?
        if (i.value()["program"] == program) {
                found = true;
                // test it !
                QSharedMemory *m_sharedMemory = new QSharedMemory(i.value()["key"].toString());
                if( !m_sharedMemory->attach() ) {
                        qDebug() << "Deleted invalid shared memory map:" << i.value()["key"].toString();
                        glmixerMap.remove(i.key());
                        found = false;
                }
                delete m_sharedMemory;
        }
    }

    return found;
}


