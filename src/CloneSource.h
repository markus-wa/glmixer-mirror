/*
 * CloneSource.h
 *
 *  Created on: Feb 27, 2010
 *      Author: bh
 *
 *  This file is part of GLMixer.
 *
 *   GLMixer is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation, either version 3 of the License, or
 *   (at your option) any later version.
 *
 *   GLMixer is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with GLMixer.  If not, see <http://www.gnu.org/licenses/>.
 *
 *   Copyright 2009, 2012 Bruno Herbelin
 *
 */

#ifndef CLONESOURCE_H_
#define CLONESOURCE_H_

#include "SourceSet.h"


class SourceCloneException : public AllocationException {
public:
    virtual QString message() { return "Invalid original source given."; }
    void raise() const { throw *this; }
    Exception *clone() const { return new SourceCloneException(*this); }
};

class CloneSource: public Source {

    friend class RenderingManager;
#ifdef GLM_FFGL
    friend class FFGLEffectSelectionDialog;
#endif

public:

    static RTTI type;
    RTTI rtti() const { return type; }
    QString getInfo() const;

    inline QString getOriginalName() const { return original->getName(); }
    inline GLuint getOriginalId() { return original->getId(); }

    GLuint getTextureIndex() const { return original->getTextureIndex(); }
    int getFrameWidth() const { return original->getFrameWidth(); }
    int getFrameHeight() const { return original->getFrameHeight(); }
    double getFrameRate() const { return original->getFrameRate(); }
    double getAspectRatio() const { return original->getAspectRatio(); }

    QDomElement getConfiguration(QDomDocument &doc, QDir current);

    void setOriginal(Source *s);
    void setOriginal(SourceSet::iterator sit);

    // only RenderingManager & FFGLEffectSelectionDialog can create a source
protected:
    CloneSource(SourceSet::iterator sit,  double d);
    ~CloneSource();

private:
    Source *original;

};

#endif /* CLONESOURCE_H_ */

