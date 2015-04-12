#include "Tag.h"

Tag *Tag::_defaultTag = 0;

Tag::Tag(QString l, QColor c): _label(l), _color(c)
{

}


void Tag::setLabel(QString l)
{
    _label = l;
}

void Tag::setColor(QColor c)
{
    _color = c;
}

Tag *Tag::getDefault()
{
    if (!Tag::_defaultTag)
        Tag::_defaultTag = new Tag("Default", QColor(255,255,60));
    return Tag::_defaultTag;
}
