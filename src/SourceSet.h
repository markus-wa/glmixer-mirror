/*
 * SourceSet.h
 *
 *  Created on: Dec 15, 2008
 *      Author: bh
 */

#ifndef SOURCESET_H_
#define SOURCESET_H_

#include <set>

#include "Source.h"


struct Source_distance_comp
{
    bool operator () (Source *a, Source *b) const
    {
        //Sort Furthest to Closest
        return (a->getDepth() < b->getDepth());
    }
};
typedef std::multiset<Source*, Source_distance_comp> SourceSet;

struct Source_distance_reverse_comp
{
    bool operator () (Source *a, Source *b) const
    {
        //Sort Furthest to Closest
        return (a->getDepth() > b->getDepth());
    }
};
typedef std::multiset<Source*, Source_distance_reverse_comp> reverseSourceSet;

struct hasName: public std::unary_function<Source*, bool>
{
    bool operator()(const Source* elem) const
    {
       return elem->getId() == _n;
    }

    hasName(GLuint n) : _n(n) { }


private:
    GLuint _n;

};

struct isCloseTo: public std::unary_function<Source*, bool>
{
    bool operator()(const Source* elem) const
    {
       return ( ABS(elem->getDepth() - _d) < DEPTH_EPSILON );
    }

    isCloseTo(GLdouble d) : _d(d) { }


private:
    GLdouble _d;

};


#endif /* SOURCESET_H_ */
