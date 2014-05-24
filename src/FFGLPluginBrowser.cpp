/*
 *   FFGLPluginBrowser
 *
 *   This file is part of GLMixer.
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

#include "FFGLPluginBrowser.moc"

#include <QVBoxLayout>

#include <QPair>
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
#include <QFileInfo>

#include "FFGLPluginSource.h"



FFGLPluginBrowser::FFGLPluginBrowser(QWidget *parent, bool allowRemove) : PropertyBrowser(parent), currentStack(0) {

    // append remove action if allowed
    if (allowRemove) {
        menuTree.addSeparator();
        // actions of context menus
        removeAction = new QAction(tr("Remove"), this);
        QObject::connect(removeAction, SIGNAL(triggered()), this, SLOT(removePlugin()) );

        menuTree.addAction(removeAction);
    }
    else
        removeAction = NULL;

    // create edit action
    editAction = new QAction(tr("Edit"), this);
    QObject::connect(editAction, SIGNAL(triggered()), this, SLOT(editPlugin()) );
    // insert edit action on top
    menuTree.insertAction(resetAction, editAction);

    // hide actions by default
    resetAction->setVisible(false);
    editAction->setVisible(false);

    setStyleSheet(QString::fromUtf8("QToolTip {\n"
        "	font: 8pt \"Monospace,Courier\";\n"
        "}"));
}


QtProperty *FFGLPluginBrowser::createPluginPropertyTree(FFGLPluginSource *plugin)
{
    QtProperty *property;

    // create the entry for this plugin
    QFileInfo pluginfile(plugin->fileName());
    // root in the tree view with a basic name
    QtProperty *pluginroot = groupManager->addProperty( pluginfile.baseName());
    pluginroot->setItalics(true);

    // keep correspondance between property and plugin
    propertyToPluginParameter[pluginroot] = QPair<FFGLPluginSource *, QString>(plugin, QString::null);

    // get the hash list of parameters for this plugin
    QVariantHash parameters = plugin->getParameters();

    // iterate over the list of parameters
    QHashIterator<QString, QVariant> i(parameters);
    while (i.hasNext()) {
        i.next();

        // depending on the type of the QVariant, add to the correspondig property manager
        switch ( i.value().type() ) {
            case QVariant::String:
            case QVariant::ByteArray:
              property = stringManager->addProperty( i.key() );
              idToProperty[property->propertyName()] = property;
              stringManager->setValue(property, i.value().toString());
              pluginroot->addSubProperty(property);
              break;
            case QVariant::Bool:
              property = boolManager->addProperty( i.key() );
              idToProperty[property->propertyName()] = property;
              boolManager->setValue(property, i.value().toBool());
              pluginroot->addSubProperty(property);
              break;
            default:
            case QVariant::Double:
              property = doubleManager->addProperty( i.key() );
              idToProperty[property->propertyName()] = property;
              doubleManager->setValue(property, i.value().toDouble());
              doubleManager->setRange(property, 0.0, 1.0);
              doubleManager->setSingleStep(property, 0.01);
              pluginroot->addSubProperty(property);
              break;
        }

        // keep correspondance between property and plugin
        propertyToPluginParameter[property] = QPair<FFGLPluginSource *, QString>(plugin, i.key());

    }

    // Get the informations on this plugin into a string list
    QStringList tooltipinformation;

    // iterate over the plugin infos
    QVariantHash informations = plugin->getInfo();
    QHashIterator<QString, QVariant> j(informations);
    while (j.hasNext()) {
       j.next();
       tooltipinformation << j.key().simplified().leftJustified(13, ' ') + j.value().toString().simplified();
    }
    // add info on the filename
    tooltipinformation << QString("File").leftJustified(13, ' ') + pluginfile.fileName();

    // put all these info in the tooltip
    pluginroot->setToolTip(tooltipinformation.join("\n"));

    // give definitive name to the root entry
    pluginroot->setPropertyName( informations["Name"].toString() );

    // done
    return pluginroot;
}


void FFGLPluginBrowser::clear()
{
    // remember expanding state
    updateExpandState(propertyTreeEditor->topLevelItems());

    // clear the GUI
    propertyTreeEditor->clear();
    propertyGroupEditor->clear();

    // empty the property managers
    stringManager->clear();
    boolManager->clear();
    doubleManager->clear();
    infoManager->clear();

}

void FFGLPluginBrowser::showProperties(FFGLPluginSourceStack *plugins)
{
    clear();

    // disconnect the managers to the corresponding value change
    disconnectManagers();

    // set the new current stack of plugins
    currentStack = plugins;

    // if a there is something to do
    if (currentStack && currentStack->count() > 0) {

        // loop over the stack
        for (FFGLPluginSourceStack::iterator it = currentStack->begin(); it != currentStack->end(); ++it )
            addProperty( createPluginPropertyTree(*it) );

        // reconnect the managers to the corresponding value change
        connectManagers();

        restoreExpandState(propertyTreeEditor->topLevelItems());
    }
}



bool FFGLPluginBrowser::canChange()
{
    return (currentStack && currentStack->count() > 0 );
}

void FFGLPluginBrowser::valueChanged(QtProperty *property, bool value)
{
    if ( propertyToPluginParameter.contains(property) ) {
        propertyToPluginParameter[property].first->setParameter(propertyToPluginParameter[property].second, QVariant(value));
    }
}

void FFGLPluginBrowser::valueChanged(QtProperty *property, int value)
{
    if ( propertyToPluginParameter.contains(property) ) {
        propertyToPluginParameter[property].first->setParameter(propertyToPluginParameter[property].second, QVariant(value));
    }
}

void FFGLPluginBrowser::valueChanged(QtProperty *property, double value)
{
    if ( propertyToPluginParameter.contains(property) ) {
        propertyToPluginParameter[property].first->setParameter(propertyToPluginParameter[property].second, QVariant(value));
    }
}

void FFGLPluginBrowser::valueChanged(QtProperty *property, const QString &value)
{
    if ( propertyToPluginParameter.contains(property) ) {
        propertyToPluginParameter[property].first->setParameter(propertyToPluginParameter[property].second, QVariant(value));
    }
}

void FFGLPluginBrowser::resetAll()
{
//    if (currentStack) {
//        // loop over the stack
//        for (FFGLPluginSourceStack::iterator it = currentStack->begin(); it != currentStack->end(); ++it )
//            (*it)->restoreDefaults();

//        // refresh display
//        showProperties(currentStack);
//    }

    if ( propertyTreeEditor->currentItem() ) {
        QtProperty *property = propertyTreeEditor->currentItem()->property();
        if ( propertyToPluginParameter.contains(property) ) {
            propertyToPluginParameter[property].first->restoreDefaults();
        }
    }
    // refresh display
    showProperties(currentStack);
}

void FFGLPluginBrowser::defaultValue()
{
    if ( propertyTreeEditor->currentItem() ) {
        QtProperty *property = propertyTreeEditor->currentItem()->property();
        if ( propertyToPluginParameter.contains(property) &&
             propertyToPluginParameter[property].second > 0) {

            // read default value
            QVariant val = propertyToPluginParameter[property].first->getParametersDefaults()[property->propertyName()];

            propertyToPluginParameter[property].first->setParameter(propertyToPluginParameter[property].second, val);

        }
    }

    // refresh display
    showProperties(currentStack);
}

void FFGLPluginBrowser::removePlugin()
{
    if ( propertyTreeEditor->currentItem() && currentStack ) {
        QtProperty *property = propertyTreeEditor->currentItem()->property();
        if ( propertyToPluginParameter.contains(property) ) {
            currentStack->removePlugin(propertyToPluginParameter[property].first);
        }
    }
    // refresh display
    showProperties(currentStack);

    emit pluginChanged();
}

void FFGLPluginBrowser::editPlugin()
{
    if ( propertyTreeEditor->currentItem() ) {
        QtProperty *property = propertyTreeEditor->currentItem()->property();
        if ( propertyToPluginParameter.contains(property) )
            emit edit(propertyToPluginParameter[property].first);
    }
}


void FFGLPluginBrowser::ctxMenuTree(const QPoint &pos)
{
    setReferenceURL();
    // edit is disabled by default
    editAction->setVisible(false);
    // reset is disabled by default
    resetAction->setVisible(false);
    // remove is disabled by default
    if(removeAction) removeAction->setEnabled(false);

    if ( propertyTreeEditor->currentItem() ) {
        QtProperty *property = propertyTreeEditor->currentItem()->property();
        // ok, a plugin is selected
        if ( propertyToPluginParameter.contains(property) ) {
            // allow to remove it if there is a remove action
            if(removeAction) removeAction->setEnabled(true);

            // enable the edit action for shadertoy plugins only
            if (propertyToPluginParameter[property].first->rtti() == FFGLPluginSource::SHADERTOY_PLUGIN)
                editAction->setVisible(true);
            else {
                // allow to reset values for freeframe plugins only
                resetAction->setVisible(true);
                setReferenceURL( QUrl::fromLocalFile( QFileInfo(propertyToPluginParameter[property].first->fileName()).canonicalPath()) );
            }
        }
    }

    PropertyBrowser::ctxMenuTree(pos);
}
