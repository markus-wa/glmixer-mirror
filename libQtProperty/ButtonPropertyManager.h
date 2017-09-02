#ifndef BUTTONPROPERTYMANAGER_H
#define BUTTONPROPERTYMANAGER_H

#include <QObject>
#include <QMap>

#include "qtpropertybrowser.h"

class ButtonPropertyManager : public QtAbstractPropertyManager
{
    Q_OBJECT

public:
    ButtonPropertyManager(QObject *parent = 0)
        : QtAbstractPropertyManager(parent)  { }

    QString value(const QtProperty *property) const;
    QString label(const QtProperty *property) const;

public slots:
    void setValue(QtProperty *property, const QString &val);
    void setLabel(QtProperty *property, const QString &val);

signals:
    void valueChanged(QtProperty *property, const QString &val);
    void labelChanged(QtProperty *property, const QString &val);

protected:
    virtual QString valueText(const QtProperty *property) const { return value(property); }
    virtual void initializeProperty(QtProperty *property) { theValues[property] = Data(); }
    virtual void uninitializeProperty(QtProperty *property) { theValues.remove(property); }

private:

    struct Data
    {
        QString value;
        QString label;
    };

    QMap<const QtProperty *, Data> theValues;
};

#endif // BUTTONPROPERTYMANAGER_H
