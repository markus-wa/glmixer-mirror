#include "Tag.h"
#include "RenderingManager.h"

QList<Tag *> Tag::_tags;
QHash<Source *, Tag*> Tag::_sourceMap;

void Tag::initialize()
{
    if (Tag::_tags.isEmpty()) {
        // item 0 : default tag
        _tags.append( new Tag("Yellow", QColor(255,255,60)) );
        // other tags
        _tags.append( new Tag("Orange", QColor(255, 150, 0)) );
        _tags.append( new Tag("Red", QColor(255, 30, 30)) );
        _tags.append( new Tag("Purple", QColor(160, 100, 255)) );
        _tags.append( new Tag("Blue", QColor(30, 170, 255)) );
        _tags.append( new Tag("Cyan", QColor(0, 255, 220)) );
    }
}

Tag::Tag(QString l, QColor c): label(l), color(c)
{

}

QString Tag::getLabel() const
{
    return label;
}

QColor Tag::getColor() const
{
    return color;
}

int Tag::getIndex() const
{
    int i = Tag::_tags.size() - 1;
    while( i > 0 && Tag::_tags[i] != this )
        --i;
    return i;
}

SourceList Tag::getSources() const
{
    SourceList sl;

    // special case for default tag
    if ( this == Tag::getDefault() ) {

        for(SourceSet::iterator  its = RenderingManager::getInstance()->getBegin(); its != RenderingManager::getInstance()->getEnd(); its++) {
            if ( (*its) && !(Tag::_sourceMap.contains(*its)))
                sl.insert( *its );
        }

    }
    else {
        // get all sources associated to this Tag
        QHashIterator<Source *, Tag *> i(Tag::_sourceMap);
        while (i.hasNext()) {
            i.next();
            if ( i.value() == this )
                sl.insert( i.key() );
        }
    }

    return sl;
}

Tag *Tag::getDefault()
{
    Tag::initialize();
    return Tag::_tags[0];
}

Tag *Tag::get(int index)
{
    Tag::initialize();
    return Tag::_tags[ qBound(0, index, Tag::_tags.size()-1) ];
}

int Tag::getNumTags()
{
    Tag::initialize();
    return Tag::_tags.size();
}

Tag *Tag::get(Source *s)
{
    return Tag::_sourceMap.value(s, Tag::getDefault());
}

bool Tag::set(Source *s)
{
    if (!s)
        return false;

    // special case for default tag : do not add in map
    if ( this == Tag::getDefault() )
        // success if could remove from the list
        return Tag::_sourceMap.remove(s);

    if ( Tag::_sourceMap.value(s, Tag::getDefault()) == this )
        return ( false );

    // insert in hashmap replace previous element
    Tag::_sourceMap.insert(s, this);
    return true;
}

void Tag::remove(Source *s)
{
    if (!s)
        // if no source argument, clear all
        Tag::_sourceMap.clear();
    else
        // remove source from the map
        Tag::_sourceMap.remove(s);
}
