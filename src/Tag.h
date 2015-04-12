#ifndef TAG_H
#define TAG_H

#include <QColor>

#include <Source.h>


class Tag
{
    QString _label;
    QColor _color;

    static Tag *_defaultTag;

public:
    Tag(QString l, QColor c);

    inline QString label() {return _label;}
    inline QColor color() {return _color;}

    static Tag *getDefault();


public:

    void setLabel(QString l);
    void setColor(QColor c);

};

#endif // TAG_H
