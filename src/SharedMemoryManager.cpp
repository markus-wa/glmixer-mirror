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
#include <QPixmap>
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
    processInformation["opengl"] = true;
    processInformation["info"] = "dummy information string";
	QVariant variant = QPixmap(QString::fromUtf8(":/glmixer/icons/gear.png"));
	processInformation["icon"] = variant;

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
			// test it !
			QSharedMemory *m_sharedMemory = new QSharedMemory(i.value()["key"].toString());
			if( !m_sharedMemory->attach() ) {
					qDebug() << "Deleted invalid shared memory map:" << i.value()["key"].toString();
					glmixerMap.remove(i.key());
			} else
				found = true;

			delete m_sharedMemory;
        }
    }

    return found;
}
