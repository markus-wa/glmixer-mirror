/*
 * SourcePropertyBrowser.h
 *
 *  Created on: Mar 14, 2010
 *      Author: bh
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
	void showProperties(SourceSet::iterator source);

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

    // force recursive expanding or collapsing of the property tree items
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

private:

	// property tree
    QtProperty *root;
	// utility lists of properties
    QMap<Source::RTTI, QtProperty *> rttiToProperty;
    QMap<QString, QtProperty *> idToProperty;
    QMap<QString, bool> idToExpanded;

    void createPropertyTree();
    void updatePropertyTree(Source *s);
    void addProperty(QtProperty *property);
    void updateExpandState();

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
    class QtTimePropertyManager *timeManager;

};

#endif /* SOURCEPROPERTYBROWSER_H_ */
