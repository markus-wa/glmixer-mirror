#ifndef TAGSMANAGER_H
#define TAGSMANAGER_H

#include <QWidget>

#include "Tag.h"


namespace Ui {
class TagsManager;
}

class TagsManager : public QWidget
{
    Q_OBJECT

public:
    explicit TagsManager(QWidget *parent = 0);
    ~TagsManager();

    QListWidgetItem *addTag(Tag *t);

public Q_SLOTS:

    void createTag();
    void deleteTag();
    void useTag(QListWidgetItem *i);

private:
    Ui::TagsManager *ui;
    class QtColorPicker *colorpick;

    QListWidgetItem *currentItem;
};

#endif // TAGSMANAGER_H
