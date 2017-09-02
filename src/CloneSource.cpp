/*
 * CloneSource.cpp
 *
 *  Created on: Oct 24, 2016
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
 *   Copyright 2009, 2016 Bruno Herbelin
 *
 */

#include "CloneSource.h"

#ifdef GLM_FFGL
#include "FFGLPluginSource.h"
#endif

Source::RTTI CloneSource::type = Source::CLONE_SOURCE;


CloneSource::CloneSource(SourceSet::iterator sit,  double d): Source(0, d), original(NULL){

    // initialize
    setOriginal(*sit);
}

CloneSource::~CloneSource() {
    // remove myself from the list of clones or my original
    original->getClones()->erase((Source*) this);
}

void CloneSource::setOriginal(SourceSet::iterator sit) {

    setOriginal( (Source *)(*sit));
}

void CloneSource::setOriginal(Source *s) {

    if (!s)
        SourceCloneException().raise();

    // remove this clone from the list of previous original
    if (original) {
        original->getClones()->erase((Source*) this);
    }

    // set the original
    original = s;

    // when cloning a clone, get back to the original ;
    CloneSource *tmp = dynamic_cast<CloneSource *>(original);
    if (tmp)
        original = tmp->original;

    // add this clone to the list of clones into the original source
    std::pair<SourceList::iterator,bool> ret;
    ret = original->getClones()->insert((Source *) this);
    if (!ret.second)
        SourceCloneException().raise();

#ifdef GLM_FFGL
    // re-connect texture to freeframe gl plugin
    if (! _ffgl_plugins.isEmpty()) {
        FFGLTextureStruct it;
        it.Handle = (GLuint) getTextureIndex();
        it.Width = getFrameWidth();
        it.Height = getFrameHeight();
        it.HardwareWidth = getFrameWidth();
        it.HardwareHeight = getFrameHeight();
        _ffgl_plugins.top()->setInputTextureStruct(it);
    }
#endif
}


QDomElement CloneSource::getConfiguration(QDomDocument &doc, QDir current)
{
    // get the config from proto source
    QDomElement sourceElem = Source::getConfiguration(doc, current);
    sourceElem.setAttribute("playing", isPlaying());
    QDomElement specific = doc.createElement("TypeSpecific");
    specific.setAttribute("type", rtti());

    QDomElement f = doc.createElement("CloneOf");
    QDomText name = doc.createTextNode(getOriginalName());
    f.appendChild(name);
    specific.appendChild(f);

    sourceElem.appendChild(specific);
    return sourceElem;
}

