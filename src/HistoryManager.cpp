#include <QtGlobal>
#include <QDebug>

#include "Source.h"
#include "RenderingManager.h"
#include "ViewRenderWidget.h"

#include "HistoryManager.moc"


//#define DEBUG_HISTORY

HistoryManager::HistoryManager(QObject *parent) : QObject(parent)
{

}

void HistoryManager::clear()
{

#ifdef DEBUG_HISTORY
    qDebug() << "HistoryManager clear";
#endif
}

// append history
void HistoryManager::addHistory(History history)
{

}


