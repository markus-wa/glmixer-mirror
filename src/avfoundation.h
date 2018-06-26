#ifndef AVFOUNDATION_H
#define AVFOUNDATION_H

#include <QHash>
#include <QString>

#ifndef Q_OS_MAC
#error "This file is only meant to be compiled for Mac OS X targets"
#endif

namespace avfoundation {
QHash<QString, QString> getDeviceList();
QHash<QString, QString> getScreenList();
}

#endif // AVFOUNDATION_H