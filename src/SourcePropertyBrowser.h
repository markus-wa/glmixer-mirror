/*
 * SourcePropertyBrowser.h
 *
 *  Created on: Mar 14, 2010
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
 *   Copyright 2009, 2010 Bruno Herbelin
 *
 */

#ifndef SOURCEPROPERTYBROWSER_H_
#define SOURCEPROPERTYBROWSER_H_

#include <QWidget>
#include <QtCore/QMap>

#include "SourceSet.h"

class QtProperty;

class SourcePropertyBrowser  : public QWidget {

	Q_OBJECT

public:
	SourcePropertyBrowser(QWidget *parent = 0);

public slots:
	// Shows the properties in the browser for the given source (iterator in the Manager list)
	// This is called every time we want to get information on a source
	void showProperties(Source *source);
	void showProperties(SourceSet::iterator sourceIt);

	// Update the values of the property browser for the current source
    void updateMixingProperties();
    void updateGeometryProperties();
    void updateLayerProperties();
    void updateMarksProperties(bool showFrames);

    // Update the source when an action is performed on a property in the browser
    // This concerns every properties editable in the browser
    void valueChanged(QtProperty *property, const QColor &value);
    void valueChanged(QtProperty *property, const QPointF &value);
    void valueChanged(QtProperty *property, bool value);
    void valueChanged(QtProperty *property, int value);
    void enumChanged(QtProperty *property, int value);
	void valueChanged(QtProperty *property, double value);
	void valueChanged(QtProperty *property, const QString &value);
    void valueChanged(QtProperty *property, const QRectF &value);

    // force expanding or collapsing of the property tree items
    void setGlobalExpandState(bool expanded);
    // utility slot for expanding all tree items
    void expandAll() { setGlobalExpandState(true); }
    // utility slot for collapsing all tree items
    void collapseAll() { setGlobalExpandState(false); }
    // Context menu actions
    void ctxMenuGroup(const QPoint &);
    void ctxMenuTree(const QPoint &);
    void switchToTreeView();
    void switchToGroupView();

    void setPropertyEnabled(QString propertyName, bool enabled);

private:

	// property tree
    QtProperty *root;
	// utility lists of properties
    QMap<Source::RTTI, QtProperty *> rttiToProperty;
    QMap<QString, QtProperty *> idToProperty;
    QMap<QString, bool> idToExpanded;

    // the link with sources
    void createPropertyTree();
    void updatePropertyTree(Source *s);
    void addProperty(QtProperty *property);
    void updateExpandState();
    Source *currentItem;

	// the property browsers
	class QVBoxLayout *layout;
	class QScrollArea *propertyGroupArea;
	class QtTreePropertyBrowser *propertyTreeEditor;
	class QtGroupBoxPropertyBrowser *propertyGroupEditor;

    // managers for different data types
    class QtGroupPropertyManager *groupManager;
    class QtDoublePropertyManager *doubleManager;
    class QtIntPropertyManager *intManager;
    class QtStringPropertyManager *stringManager, *infoManager;
    class QtColorPropertyManager *colorManager;
    class QtPointFPropertyManager *pointManager;
    class QtSizePropertyManager *sizeManager;
    class QtEnumPropertyManager *enumManager;
    class QtBoolPropertyManager *boolManager;
    class QtRectFPropertyManager *rectManager;

};

#endif /* SOURCEPROPERTYBROWSER_H_ */
