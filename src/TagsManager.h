#ifndef TAGSMANAGER_H
#define TAGSMANAGER_H

#include <QWidget>
#include <QMenu>
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

    QMenu *getTagsMenu() const {return tagmenu;}

    bool applyTag(Tag *t);
    void selectTag(Tag *t);

public slots:
    // connected to List of tags
    void on_tagsListWidget_itemClicked(QListWidgetItem *i);

    // Source to operate
    void connectSource(SourceSet::iterator);
    void applyTagtoCurrentSource();

protected slots:
    void deselectItem();
    void clearAllTags();

private:
    Ui::TagsManager *ui;

    QMenu *tagmenu;
    Source *currentSource;
    QHash<Tag *, QListWidgetItem *> tagsMap;
    QListWidgetItem *itemAll, *itemNone;
};

#endif // TAGSMANAGER_H
