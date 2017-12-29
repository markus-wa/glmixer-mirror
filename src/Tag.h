#ifndef TAG_H
#define TAG_H

#include "Source.h"

#include <QString>
#include <QColor>
#include <QHash>

class Tag
{
    QString label;
    QColor color;

    static QList<Tag *> _tags;
    static QHash<Source *, Tag*> _sourceMap;
    static void initialize();

    Tag(QString l, QColor c);

public:

    static void remove(Source *s = NULL);
    static Tag *get(Source *s);
    static Tag *get(int index = 0);
    static Tag *getDefault();
    static int getNumTags();

    void set(Source *s);

    QString getLabel() const;
    QColor getColor() const;
    int getIndex() const;
    SourceList getSources() const;
};


#endif // TAG_H
