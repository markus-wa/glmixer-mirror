#ifndef TAGSMANAGER_H
#define TAGSMANAGER_H

#include <QWidget>
#include <QHash>

#include "Tag.h"
#include "Source.h"
#include "SourceSet.h"


namespace Ui {
class TagsManager;
}

class TagsManager : public QWidget
{
    Q_OBJECT

public:
    explicit TagsManager(QWidget *parent = 0);
    ~TagsManager();

    QListWidgetItem *getTagItem(Tag *t);
    bool useTag(Tag *t);
    void selectTag(Tag *t);

public slots:

    // connected to List of tags
    void on_tagsListWidget_itemClicked(QListWidgetItem *i);

    // contex menu
    void ctxMenu(const QPoint &pos);
    void clearAllTags();

    // Source to operate
    void connectSource(SourceSet::iterator);

protected slots:
    void deselectItem();

private:
    Ui::TagsManager *ui;

    Source *currentSource;
    QHash<Tag *, QListWidgetItem *> tagsMap;
};

#endif // TAGSMANAGER_H
