#include "PropertyBrowser.moc"

#include <QPair>
#include <QVBoxLayout>
#include <QScrollArea>
#include <QMenu>

#include <QtTreePropertyBrowser>
#include <QtButtonPropertyBrowser>
#include <QtGroupBoxPropertyBrowser>
#include <QtDoublePropertyManager>
#include <QtIntPropertyManager>
#include <QtStringPropertyManager>
#include <QtColorPropertyManager>
#include <QtRectFPropertyManager>
#include <QtPointFPropertyManager>
#include <QtSizePropertyManager>
#include <QtEnumPropertyManager>
#include <QtBoolPropertyManager>
#include <QtTimePropertyManager>
#include <QtDoubleSpinBoxFactory>
#include <QtCheckBoxFactory>
#include <QtSpinBoxFactory>
#include <QtSliderFactory>
#include <QtLineEditFactory>
#include <QtEnumEditorFactory>
#include <QtCheckBoxFactory>
#include <QtTimeEditFactory>
#include <QtColorEditorFactory>


PropertyBrowser::PropertyBrowser(QWidget *parent) :
    QWidget(parent)
{
    layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setObjectName(QString::fromUtf8("verticalLayout"));

    // property Group Box
    propertyGroupEditor = new QtGroupBoxPropertyBrowser(this);
    propertyGroupEditor->setObjectName(QString::fromUtf8("Property Groups"));
    propertyGroupEditor->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(propertyGroupEditor, SIGNAL(customContextMenuRequested(const QPoint &)), this, SLOT(ctxMenuGroup(const QPoint &)));

    propertyGroupArea = new QScrollArea(this);
    propertyGroupArea->setFrameShape(QFrame::NoFrame);
    propertyGroupArea->setWidgetResizable(true);
    propertyGroupArea->setWidget(propertyGroupEditor);
    propertyGroupArea->setVisible(false);

    // property TREE
    propertyTreeEditor = new QtTreePropertyBrowser(this);
    propertyTreeEditor->setObjectName(QString::fromUtf8("Property Tree"));
    propertyTreeEditor->setContextMenuPolicy(Qt::CustomContextMenu);
    propertyTreeEditor->setResizeMode(QtTreePropertyBrowser::Interactive);
    connect(propertyTreeEditor, SIGNAL(customContextMenuRequested(const QPoint &)), this, SLOT(ctxMenuTree(const QPoint &)));

    // TODO ; read default from application config
    propertyTreeEditor->setVisible(true);
    layout->addWidget(propertyTreeEditor);

    // create the property managers for every possible types
    groupManager = new QtGroupPropertyManager(this);
    doubleManager = new QtDoublePropertyManager(this);
    intManager = new QtIntPropertyManager(this);
    stringManager = new QtStringPropertyManager(this);
    colorManager = new QtColorPropertyManager(this);
    pointManager = new QtPointFPropertyManager(this);
    enumManager = new QtEnumPropertyManager(this);
    boolManager = new QtBoolPropertyManager(this);
    rectManager = new QtRectFPropertyManager(this);

    connect(colorManager, SIGNAL(valueChanged(QtProperty *, const QColor &)),
                this, SLOT(propertyValueChanged(QtProperty *, const QColor &)));
    connect(enumManager, SIGNAL(valueChanged(QtProperty *, int)),
                this, SLOT(propertyValueChanged(QtProperty *, int)));
    connect(intManager, SIGNAL(valueChanged(QtProperty *, int)),
                this, SLOT(propertyValueChanged(QtProperty *, int)));
    connect(boolManager, SIGNAL(valueChanged(QtProperty *, bool)),
                this, SLOT(propertyValueChanged(QtProperty *, bool)));
    connect(doubleManager, SIGNAL(valueChanged(QtProperty *, double)),
                this, SLOT(propertyValueChanged(QtProperty *, double)));

    // special managers which are not associated with a factory (i.e non editable)
    infoManager = new QtStringPropertyManager(this);
    sizeManager = new QtSizePropertyManager(this);

    // specify the factory for each of the property managers
    QtDoubleSpinBoxFactory *doubleSpinBoxFactory = new QtDoubleSpinBoxFactory(this);    // for double
    QtCheckBoxFactory *checkBoxFactory = new QtCheckBoxFactory(this);					// for bool
    QtSpinBoxFactory *spinBoxFactory = new QtSpinBoxFactory(this);                      // for int
    QtSliderFactory *sliderFactory = new QtSliderFactory(this);							// for int
    QtLineEditFactory *lineEditFactory = new QtLineEditFactory(this);					// for text
    QtEnumEditorFactory *comboBoxFactory = new QtEnumEditorFactory(this);				// for enum
    QtColorEditorFactory *colorFactory = new QtColorEditorFactory(this);				// for color

    propertyTreeEditor->setFactoryForManager(doubleManager, doubleSpinBoxFactory);
    propertyTreeEditor->setFactoryForManager(intManager, spinBoxFactory);
    propertyTreeEditor->setFactoryForManager(stringManager, lineEditFactory);
    propertyTreeEditor->setFactoryForManager(colorManager, colorFactory);
    propertyTreeEditor->setFactoryForManager(pointManager->subDoublePropertyManager(), doubleSpinBoxFactory);
    propertyTreeEditor->setFactoryForManager(enumManager, comboBoxFactory);
    propertyTreeEditor->setFactoryForManager(boolManager, checkBoxFactory);
    propertyTreeEditor->setFactoryForManager(rectManager->subDoublePropertyManager(), doubleSpinBoxFactory);

    propertyGroupEditor->setFactoryForManager(doubleManager, doubleSpinBoxFactory);
    propertyGroupEditor->setFactoryForManager(intManager, sliderFactory);
    propertyGroupEditor->setFactoryForManager(stringManager, lineEditFactory);
    propertyGroupEditor->setFactoryForManager(colorManager, colorFactory);
    propertyGroupEditor->setFactoryForManager(pointManager->subDoublePropertyManager(), doubleSpinBoxFactory);
    propertyGroupEditor->setFactoryForManager(enumManager, comboBoxFactory);
    propertyGroupEditor->setFactoryForManager(boolManager, checkBoxFactory);
    propertyGroupEditor->setFactoryForManager(rectManager->subDoublePropertyManager(), doubleSpinBoxFactory);

    // actions of context menus
    defaultValueAction = new QAction(tr("Default value"), this);
    QObject::connect(defaultValueAction, SIGNAL(triggered()), this, SLOT(defaultValue() ) );

    menuTree.addAction(defaultValueAction);
    menuTree.addAction(tr("Reset all"), this, SLOT(resetAll()));
    menuTree.addSeparator();
    menuTree.addAction(tr("Switch to Groups view"), this, SLOT(switchToGroupView()));
    menuTree.addAction(tr("Expand tree"), this, SLOT(expandAll()));
    menuTree.addAction(tr("Collapse tree"), this, SLOT(collapseAll()));

    menuGroup.addAction(defaultValueAction);
    menuGroup.addAction(tr("Reset all"), this, SLOT(resetAll()));
    menuGroup.addSeparator();
    menuGroup.addAction(tr("Switch to Tree view"), this, SLOT(switchToTreeView()));
}


void PropertyBrowser::setHeaderVisible(bool visible){

    propertyTreeEditor->setHeaderVisible(visible);

}

void PropertyBrowser::setPropertyEnabled(QString propertyName, bool enabled){

    if (idToProperty.contains(propertyName))
        idToProperty[propertyName]->setEnabled(enabled);

}


void PropertyBrowser::connectManagers()
{
    connect(doubleManager, SIGNAL(valueChanged(QtProperty *, double)),
                this, SLOT(valueChanged(QtProperty *, double)));
    connect(stringManager, SIGNAL(valueChanged(QtProperty *, const QString &)),
                this, SLOT(valueChanged(QtProperty *, const QString &)));
    connect(colorManager, SIGNAL(valueChanged(QtProperty *, const QColor &)),
                this, SLOT(valueChanged(QtProperty *, const QColor &)));
    connect(pointManager, SIGNAL(valueChanged(QtProperty *, const QPointF &)),
                this, SLOT(valueChanged(QtProperty *, const QPointF &)));
    connect(enumManager, SIGNAL(valueChanged(QtProperty *, int)),
                this, SLOT(enumChanged(QtProperty *, int)));
    connect(intManager, SIGNAL(valueChanged(QtProperty *, int)),
                this, SLOT(valueChanged(QtProperty *, int)));
    connect(boolManager, SIGNAL(valueChanged(QtProperty *, bool)),
                this, SLOT(valueChanged(QtProperty *, bool)));
    connect(rectManager, SIGNAL(valueChanged(QtProperty *, const QRectF &)),
                this, SLOT(valueChanged(QtProperty *, const QRectF &)));
}

void PropertyBrowser::disconnectManagers()
{
    disconnect(doubleManager, SIGNAL(valueChanged(QtProperty *, double)),
                this, SLOT(valueChanged(QtProperty *, double)));
    disconnect(stringManager, SIGNAL(valueChanged(QtProperty *, const QString &)),
                this, SLOT(valueChanged(QtProperty *, const QString &)));
    disconnect(colorManager, SIGNAL(valueChanged(QtProperty *, const QColor &)),
                this, SLOT(valueChanged(QtProperty *, const QColor &)));
    disconnect(pointManager, SIGNAL(valueChanged(QtProperty *, const QPointF &)),
                this, SLOT(valueChanged(QtProperty *, const QPointF &)));
    disconnect(enumManager, SIGNAL(valueChanged(QtProperty *, int)),
                this, SLOT(enumChanged(QtProperty *, int)));
    disconnect(intManager, SIGNAL(valueChanged(QtProperty *, int)),
                this, SLOT(valueChanged(QtProperty *, int)));
    disconnect(boolManager, SIGNAL(valueChanged(QtProperty *, bool)),
                this, SLOT(valueChanged(QtProperty *, bool)));
    disconnect(rectManager, SIGNAL(valueChanged(QtProperty *, const QRectF &)),
                this, SLOT(valueChanged(QtProperty *, const QRectF &)));

}



void PropertyBrowser::addProperty(QtProperty *property)
{
    // add to the group view
    propertyGroupEditor->addProperty(property);

    // add to the tree view
    QtBrowserItem *item = propertyTreeEditor->addProperty(property);
    if (idToExpanded.contains(property->propertyName()))
        propertyTreeEditor->setExpanded(item, idToExpanded[property->propertyName()]);
    else
        propertyTreeEditor->setExpanded(item, false);
}


void PropertyBrowser::updateExpandState(QList<QtBrowserItem *> list)
{
    QListIterator<QtBrowserItem *> it(list);
    while (it.hasNext()) {
        QtBrowserItem *item = it.next();
        idToExpanded[item->property()->propertyName()] = propertyTreeEditor->isExpanded(item);

        updateExpandState(item->children());
    }
}

void PropertyBrowser::restoreExpandState(QList<QtBrowserItem *> list)
{
    QListIterator<QtBrowserItem *> it(list);
    while (it.hasNext()) {
        QtBrowserItem *item = it.next();
        if (idToExpanded.contains(item->property()->propertyName()))
            propertyTreeEditor->setExpanded(item, idToExpanded[item->property()->propertyName()]);

        restoreExpandState(item->children());
    }
}

void PropertyBrowser::setGlobalExpandState(QList<QtBrowserItem *> list, bool expanded)
{
    QListIterator<QtBrowserItem *> it(list);
    while (it.hasNext()) {
        QtBrowserItem *item = it.next();
        idToExpanded[item->property()->propertyName()] = expanded;
        propertyTreeEditor->setExpanded(item, expanded);

        setGlobalExpandState(item->children(), expanded);
    }
}

void PropertyBrowser::expandAll()
{
    setGlobalExpandState(propertyTreeEditor->topLevelItems(), true);
}

void PropertyBrowser::collapseAll()
{
    setGlobalExpandState(propertyTreeEditor->topLevelItems(), false);
}


void PropertyBrowser::enumChanged(QString propertyname, int value)
{
    enumManager->setValue(idToProperty[propertyname], value);
}

void PropertyBrowser::valueChanged(QString propertyname, const QColor &value)
{
    colorManager->setValue(idToProperty[propertyname], value);
}

void PropertyBrowser::valueChanged(QString propertyname, bool value)
{
    boolManager->setValue(idToProperty[propertyname], value);
}

void PropertyBrowser::valueChanged(QString propertyname, int value)
{
    intManager->setValue(idToProperty[propertyname], value);
}

void PropertyBrowser::valueChanged(QString propertyname, double value)
{
    doubleManager->setValue(idToProperty[propertyname], value);
}

void PropertyBrowser::propertyValueChanged(QtProperty *property, bool value)
{
    emit( propertyChanged( property->propertyName(), value) );
}

void PropertyBrowser::propertyValueChanged(QtProperty *property, int value)
{
    emit( propertyChanged( property->propertyName(), value) );
}

void PropertyBrowser::propertyValueChanged(QtProperty *property, const QColor &value)
{
    emit( propertyChanged( property->propertyName(), value) );
}

void PropertyBrowser::propertyValueChanged(QtProperty *property, double value)
{
    emit( propertyChanged( property->propertyName(), value) );
}


void PropertyBrowser::ctxMenuGroup(const QPoint &pos)
{
    defaultValueAction->setEnabled( false );

    if (propertyGroupEditor->currentItem() &&
        propertyGroupEditor->currentItem()->children().count() == 0 &&
        !propertyGroupEditor->currentItem()->property()->isItalics() )
        defaultValueAction->setEnabled( true );

    menuGroup.exec( propertyGroupEditor->mapToGlobal(pos) );
}


void PropertyBrowser::ctxMenuTree(const QPoint &pos)
{
    defaultValueAction->setEnabled(false);
    if (propertyTreeEditor->currentItem() &&
        propertyTreeEditor->currentItem()->children().count() == 0 &&
        !propertyTreeEditor->currentItem()->property()->isItalics() )
        defaultValueAction->setEnabled( true );

    menuTree.exec( propertyTreeEditor->mapToGlobal(pos) );
}

void PropertyBrowser::switchToTreeView(){

    propertyGroupArea->setVisible(false);
    layout->removeWidget(propertyGroupArea);

    layout->addWidget(propertyTreeEditor);
    propertyTreeEditor->setVisible(true);
}

void PropertyBrowser::switchToGroupView(){

    propertyTreeEditor->setVisible(false);
    layout->removeWidget(propertyTreeEditor);

    layout->addWidget(propertyGroupArea);
    propertyGroupArea->setVisible(true);
}

QString getSizeString(float num)
{
    QStringList list;
    list << "KB" << "MB" << "GB" << "TB";

    QStringListIterator i(list);
    QString unit("bytes");

    while(num >= 1024.0 && i.hasNext())
     {
        unit = i.next();
        num /= 1024.0;
    }
    return QString().setNum(num,'f',2)+" "+unit;
}

