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
	virtual ~SourcePropertyBrowser();

	void createProperty(Source *s);

public slots:
	void showProperties(SourceSet::iterator source);
    void updateProperties(SourceSet::iterator source, bool force = false);

//    void valueChanged(QtProperty *property, double value);
//    void valueChanged(QtProperty *property, const QString &value);
    void valueChanged(QtProperty *property, const QColor &value);
//    void valueChanged(QtProperty *property, const QFont &value);
    void valueChanged(QtProperty *property, const QPointF &value);
//    void valueChanged(QtProperty *property, const QSize &value);

    void setGlobalExpandState(bool expanded);
    void expandAll() { setGlobalExpandState(true); }
    void collapseAll() { setGlobalExpandState(false); }
    void ctxMenuGroup(const QPoint &);
    void ctxMenuTree(const QPoint &);
    void switchToTreeView();
    void switchToGroupView();

private:

    void addProperty(QtProperty *property);
    void updateExpandState();

	// the property browsers
	class QVBoxLayout *layout;
	class QtTreePropertyBrowser *propertyTreeEditor;
	class QtGroupBoxPropertyBrowser *propertyGroupEditor;

	// the temporary lists of properties
    QMap<QtProperty *, QString> propertyToId;
//    QMap<QString, QtProperty *> idToProperty;
    QMap<QString, bool> idToExpanded;

    // managers for different data types
    class QtGroupPropertyManager *groupManager;
    class QtDoublePropertyManager *doubleManager;
    class QtStringPropertyManager *stringManager;
    class QtColorPropertyManager *colorManager;
    class QtPointFPropertyManager *pointManager;
    class QtSizeFPropertyManager *sizeManager;
    class QtEnumPropertyManager *enumManager;
    class QtBoolPropertyManager *boolManager;
    class QtTimePropertyManager *timeManager;

};

#endif /* SOURCEPROPERTYBROWSER_H_ */
