#ifndef DIRECTSHOW_H
#define DIRECTSHOW_H

#pragma once

#include <QHash>
#include <QString>

#ifndef Q_OS_WIN
#error "This file is only meant to be compiled for Windows targets"
#endif

namespace directshow {
QHash<QString, QString> getDeviceList();
}

#endif // DIRECTSHOW_H