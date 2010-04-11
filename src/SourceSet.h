/*
 * SourceSet.h
 *
 *  Created on: Dec 15, 2008
 *      Author: bh
 */

#ifndef SOURCESET_H_
#define SOURCESET_H_

#include <set>
#include <deque>

#include "Source.h"

typedef std::deque<SourceList> SourceListArray;


struct Source_distance_comp
{
    inline bool operator () (Source *a, Source *b) const
    {
        //Sort Furthest to Closest
        return (a->getDepth() < b->getDepth());
    }
};
typedef std::set<Source*, Source_distance_comp> SourceSet;

struct Source_distance_reverse_comp
{
    inline bool operator () (Source *a, Source *b) const
    {
        //Sort Closest to Furthest
        return (a->getDepth() > b->getDepth());
    }
};
typedef std::set<Source*, Source_distance_reverse_comp> reverseSourceSet;

struct hasId: public std::unary_function<Source*, bool>
{
    inline bool operator()(const Source* elem) const
    {
       return elem->getId() == _id;
    }

    hasId(GLuint id) : _id(id) { }


private:
    GLuint _id;

};


struct hasName: public std::unary_function<Source*, bool>
{
    inline bool operator()(const Source* elem) const
    {
       return elem->getName() == _n;
    }

    hasName(QString n) : _n(n) { }


private:
    QString _n;

};

struct isCloseTo: public std::unary_function<Source*, bool>
{
    inline bool operator()(const Source* elem) const
    {
       return ( ABS(elem->getDepth() - _d) < DEPTH_EPSILON );
    }

    isCloseTo(GLdouble d) : _d(d) { }


private:
    GLdouble _d;

};


#endif /* SOURCESET_H_ */
