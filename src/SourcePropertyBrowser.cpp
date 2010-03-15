/*
 * SourcePropertyBrowser.cpp
 *
 *  Created on: Mar 14, 2010
 *      Author: bh
 */

#include <SourcePropertyBrowser.moc>

#include <QVBoxLayout>

#include <QtTreePropertyBrowser>
#include <QtButtonPropertyBrowser>
#include <QtGroupBoxPropertyBrowser>
#include <QtDoublePropertyManager>
#include <QtStringPropertyManager>
#include <QtColorPropertyManager>
#include <QtPointFPropertyManager>
#include <QtSizeFPropertyManager>
#include <QtEnumPropertyManager>
#include <QtBoolPropertyManager>
#include <QtTimePropertyManager>
#include <QtDoubleSpinBoxFactory>
#include <QtCheckBoxFactory>
#include <QtSpinBoxFactory>
#include <QtLineEditFactory>
#include <QtEnumEditorFactory>
#include <QtCheckBoxFactory>
#include <QtTimeEditFactory>
#include <QtColorEditorFactory>

#include "RenderingManager.h"

SourcePropertyBrowser::SourcePropertyBrowser(QWidget *parent) : QWidget (parent) {

	layout = new QVBoxLayout(this);
	layout->setObjectName(QString::fromUtf8("verticalLayout"));



	// property Group Box
	propertyGroupEditor = new QtGroupBoxPropertyBrowser(this);
	propertyGroupEditor->setObjectName(QString::fromUtf8("Property Groups"));
	propertyGroupEditor->setVisible(false);
	propertyGroupEditor->setContextMenuPolicy(Qt::CustomContextMenu);
	connect(propertyGroupEditor, SIGNAL(customContextMenuRequested(const QPoint &)), this, SLOT(ctxMenuGroup(const QPoint &)));

	// property TREE
    propertyTreeEditor = new QtTreePropertyBrowser(this);
    propertyTreeEditor->setObjectName(QString::fromUtf8("Property Tree"));
    propertyTreeEditor->setVisible(false);
    propertyTreeEditor->setContextMenuPolicy(Qt::CustomContextMenu);
	connect(propertyTreeEditor, SIGNAL(customContextMenuRequested(const QPoint &)), this, SLOT(ctxMenuTree(const QPoint &)));

	// TODO ; read default from application config
	propertyTreeEditor->setVisible(true);
    layout->addWidget(propertyTreeEditor);

    // create the property managers for every possible types
    groupManager = new QtGroupPropertyManager(this);
    doubleManager = new QtDoublePropertyManager(this);
    stringManager = new QtStringPropertyManager(this);
    colorManager = new QtColorPropertyManager(this);
    pointManager = new QtPointFPropertyManager(this);
    sizeManager = new QtSizeFPropertyManager(this);
    enumManager = new QtEnumPropertyManager(this);
    boolManager = new QtBoolPropertyManager(this);
    timeManager = new QtTimePropertyManager(this);

    // connect the managers to the corresponding value change
//    connect(doubleManager, SIGNAL(valueChanged(QtProperty *, double)),
//                this, SLOT(valueChanged(QtProperty *, double)));
//    connect(stringManager, SIGNAL(valueChanged(QtProperty *, const QString &)),
//                this, SLOT(valueChanged(QtProperty *, const QString &)));
    connect(colorManager, SIGNAL(valueChanged(QtProperty *, const QColor &)),
                this, SLOT(valueChanged(QtProperty *, const QColor &)));
    connect(pointManager, SIGNAL(valueChanged(QtProperty *, const QPointF &)),
                this, SLOT(valueChanged(QtProperty *, const QPointF &)));
//    connect(sizeManager, SIGNAL(valueChanged(QtProperty *, const QSizeF &)),
//                this, SLOT(valueChanged(QtProperty *, const QSizeF &)));

    // specify the factory for each of the property managers
    QtDoubleSpinBoxFactory *doubleSpinBoxFactory = new QtDoubleSpinBoxFactory(this);
    QtCheckBoxFactory *checkBoxFactory = new QtCheckBoxFactory(this);
    QtSpinBoxFactory *spinBoxFactory = new QtSpinBoxFactory(this);
    QtLineEditFactory *lineEditFactory = new QtLineEditFactory(this);
    QtEnumEditorFactory *comboBoxFactory = new QtEnumEditorFactory(this);
    QtTimeEditFactory *timeFactory = new QtTimeEditFactory(this);
    QtColorEditorFactory *colorFactory = new QtColorEditorFactory(this);

    propertyTreeEditor->setFactoryForManager(doubleManager, doubleSpinBoxFactory);
    propertyTreeEditor->setFactoryForManager(stringManager, lineEditFactory);
    propertyTreeEditor->setFactoryForManager(colorManager, colorFactory);
    propertyTreeEditor->setFactoryForManager(pointManager->subDoublePropertyManager(), doubleSpinBoxFactory);
    propertyTreeEditor->setFactoryForManager(sizeManager->subDoublePropertyManager(), doubleSpinBoxFactory);
    propertyTreeEditor->setFactoryForManager(enumManager, comboBoxFactory);
    propertyTreeEditor->setFactoryForManager(boolManager, checkBoxFactory);
    propertyTreeEditor->setFactoryForManager(timeManager, timeFactory);

    propertyGroupEditor->setFactoryForManager(doubleManager, doubleSpinBoxFactory);
    propertyGroupEditor->setFactoryForManager(stringManager, lineEditFactory);
    propertyGroupEditor->setFactoryForManager(colorManager, colorFactory);
    propertyGroupEditor->setFactoryForManager(pointManager->subDoublePropertyManager(), doubleSpinBoxFactory);
    propertyGroupEditor->setFactoryForManager(sizeManager->subDoublePropertyManager(), doubleSpinBoxFactory);
    propertyGroupEditor->setFactoryForManager(enumManager, comboBoxFactory);
    propertyGroupEditor->setFactoryForManager(boolManager, checkBoxFactory);
    propertyGroupEditor->setFactoryForManager(timeManager, timeFactory);

}

SourcePropertyBrowser::~SourcePropertyBrowser() {
	// TODO Auto-generated destructor stub
}


void SourcePropertyBrowser::createProperty(Source *s){

	if ( s != 0 ) {
		QtProperty *property;

		// the top property holding all the source sub-properties
		QtProperty *topItem = stringManager->addProperty( QLatin1String("Source ") +  QString::number(s->getId()));
		Q_CHECK_PTR(topItem);

		// Name
		property = stringManager->addProperty( QLatin1String("Name") );
		stringManager->setValue(property, QLatin1String("Source ") +  QString::number(s->getId()) );
		topItem->addSubProperty(property);
		// Position
		property = pointManager->addProperty("Position");
		pointManager->setValue(property, QPointF( s->getX(), s->getY()));
		Q_CHECK_PTR(property);
		topItem->addSubProperty(property);
		// Color
		property = colorManager->addProperty("Color");
		colorManager->setValue(property, QColor( s->getColor()));
		Q_CHECK_PTR(property);
		topItem->addSubProperty(property);

		// remember the top property into the source
		s->setProperty(topItem);
	}
}

void SourcePropertyBrowser::showProperties(SourceSet::iterator csi){

    updateExpandState();
    propertyTreeEditor->clear();
    propertyGroupEditor->clear();

	if ( RenderingManager::getInstance()->isValid(csi) && (*csi)->getProperty() != 0 ) {

		// ok, we got a valid top property; show all the subProperties into the browser:
	    QList<QtProperty *> list = (*csi)->getProperty()->subProperties();
	    QListIterator<QtProperty *> it(list);
	    while (it.hasNext()) {
			addProperty(it.next());
	    }

	} else {
		//qDebug("not showProperties");
	}
}



void SourcePropertyBrowser::addProperty(QtProperty *property)
{
    propertyToId[property] = property->propertyName();
    propertyGroupEditor->addProperty(property);
    QtBrowserItem *item = propertyTreeEditor->addProperty(property);
    if (idToExpanded.contains(property->propertyName()))
    	propertyTreeEditor->setExpanded(item, idToExpanded[property->propertyName()]);
    else
    	propertyTreeEditor->setExpanded(item, false);
}


void SourcePropertyBrowser::updateExpandState()
{
    QList<QtBrowserItem *> list = propertyTreeEditor->topLevelItems();
    QListIterator<QtBrowserItem *> it(list);
    while (it.hasNext()) {
        QtBrowserItem *item = it.next();
        QtProperty *prop = item->property();
        idToExpanded[propertyToId[prop]] = propertyTreeEditor->isExpanded(item);
    }
}


void SourcePropertyBrowser::setGlobalExpandState(bool expanded)
{
    QList<QtBrowserItem *> list = propertyTreeEditor->topLevelItems();
    QListIterator<QtBrowserItem *> it(list);
    while (it.hasNext()) {
        QtBrowserItem *item = it.next();
        QtProperty *prop = item->property();
        idToExpanded[propertyToId[prop]] = expanded;
        propertyTreeEditor->setExpanded(item, expanded);
    }
}


void SourcePropertyBrowser::valueChanged(QtProperty *property, const QPointF &value){

    if (!propertyToId.contains(property))
        return;

    if ( RenderingManager::getInstance()->notAtEnd(RenderingManager::getInstance()->getCurrentSource()) ) {
		Source *currentItem = *RenderingManager::getInstance()->getCurrentSource();

		QString id = propertyToId[property];

		if ( id == QString("Position") ) {
			currentItem->setX( value.x() );
			currentItem->setY( value.y() );
		}

		// example of rtti dependent property:
		if (currentItem->rtti() == Source::VIDEO_SOURCE) {

		}

    }
}


void SourcePropertyBrowser::valueChanged(QtProperty *property, const QColor &value){

    if (!propertyToId.contains(property))
        return;

    if ( RenderingManager::getInstance()->notAtEnd(RenderingManager::getInstance()->getCurrentSource()) ) {
		Source *currentItem = *RenderingManager::getInstance()->getCurrentSource();

		QString id = propertyToId[property];

		if ( id == QString("Color") ) {
			currentItem->setColor(value);
			property->setModified(false);
		}

    }
}


void SourcePropertyBrowser::updateProperties(SourceSet::iterator csi, bool force){

	if ( RenderingManager::getInstance()->isValid(csi) && (*csi)->getProperty() != 0 ) {

		// ok, we got a valid top property; show all the subProperties into the browser:
	    QList<QtProperty *> list = (*csi)->getProperty()->subProperties();
	    QListIterator<QtProperty *> it(list);
	    while (it.hasNext()) {
	    	QtProperty *p = it.next();
	    	if ( force || p->isModified() )
	    	{
	    		QString id = propertyToId[p];

	    		if ( id == QString("Position") ) {
	    			pointManager->setValue(p, QPointF( (*csi)->getX(), (*csi)->getY()));
	    			p->setModified(false);
	    		}

	    	}
	    }

	} else {
		//qDebug("not showProperties");
	}
}


void SourcePropertyBrowser::ctxMenuGroup(const QPoint &pos){

    QMenu *menu = new QMenu;
    menu->addAction(tr("Switch to Tree view"), this, SLOT(switchToTreeView()));
    menu->exec(mapToGlobal(pos));

}


void SourcePropertyBrowser::ctxMenuTree(const QPoint &pos){

    QMenu *menu = new QMenu;
    menu->addAction(tr("Expand All"), this, SLOT(expandAll()));
    menu->addAction(tr("Collapse All"), this, SLOT(collapseAll()));
    menu->addAction(tr("Switch to Groups view"), this, SLOT(switchToGroupView()));
    menu->exec(mapToGlobal(pos));

}

void SourcePropertyBrowser::switchToTreeView(){

	propertyGroupEditor->setVisible(false);
	layout->removeWidget(propertyGroupEditor);

    updateProperties(RenderingManager::getInstance()->getCurrentSource(), true);
    layout->addWidget(propertyTreeEditor);
    propertyTreeEditor->setVisible(true);
}

void SourcePropertyBrowser::switchToGroupView(){

	propertyTreeEditor->setVisible(false);
	layout->removeWidget(propertyTreeEditor);

    updateProperties(RenderingManager::getInstance()->getCurrentSource(), true);
    layout->addWidget(propertyGroupEditor);
    propertyGroupEditor->setVisible(true);
}


