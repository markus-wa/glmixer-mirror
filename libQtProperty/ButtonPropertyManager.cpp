#include "ButtonPropertyManager.moc"

QString ButtonPropertyManager::value(const QtProperty *property) const
{
    if (!theValues.contains(property))
        return QString();
    return theValues[property].value;
}

QString ButtonPropertyManager::label(const QtProperty *property) const
{
    if (!theValues.contains(property))
        return QString();
    return theValues[property].label;
}

void ButtonPropertyManager::setLabel(QtProperty *property, const QString &val)
{
    if (!theValues.contains(property))
        return;

    Data data = theValues[property];
    if (data.label == val)
        return;

    data.label = val;
    theValues[property] = data;

    emit propertyChanged(property);
    emit labelChanged(property, data.label);
}


void ButtonPropertyManager::setValue(QtProperty *property, const QString &val)
{

    if (!theValues.contains(property))
        return;

    Data data = theValues[property];

    // change value only if valid string
    if ( !val.isNull()) {

        if (data.value == val)
            return;

        data.value = val;
        theValues[property] = data;
    }

    emit propertyChanged(property);
    emit valueChanged(property, val);
}

