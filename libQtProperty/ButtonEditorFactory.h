#ifndef BUTTONEDITORFACTORY_H
#define BUTTONEDITORFACTORY_H

#include <QToolButton>
#include <QLabel>

#include "qtpropertybrowser.h"
#include "ButtonPropertyManager.h"

class ButtonEdit : public QWidget
{
    Q_OBJECT
public:
    ButtonEdit(QWidget *parent = 0);
    void setValueText(QString text);
    void setButtonText(QString text);

signals:
    void buttonClicked();

private:
    QToolButton *button;
    QLabel *value;
};

class ButtonEditorFactory : public QtAbstractEditorFactory<ButtonPropertyManager>
{
    Q_OBJECT

public:
    ButtonEditorFactory(QObject *parent = 0)
        : QtAbstractEditorFactory<ButtonPropertyManager>(parent) { }

    virtual ~ButtonEditorFactory();

protected:
    virtual void connectPropertyManager(ButtonPropertyManager *manager);
    virtual void disconnectPropertyManager(ButtonPropertyManager *manager);
    virtual QWidget *createEditor(ButtonPropertyManager *manager, QtProperty *property, QWidget *parent);

private slots:
    void slotPropertyChanged(QtProperty *property, const QString &value);
    void slotLabelChanged(QtProperty *property, const QString &value);
    void slotSetValue (const QString &value = QString());
    void slotEditorDestroyed(QObject *object);

private:
    QMap<QtProperty *, QList<ButtonEdit *> > theCreatedButtons;
    QMap<ButtonEdit *, QtProperty *> theButtonsToProperty;
};

#endif // BUTTONEDITORFACTORY_H
