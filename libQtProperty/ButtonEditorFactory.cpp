#include "ButtonEditorFactory.moc"

#include <QHBoxLayout>

ButtonEdit::ButtonEdit(QWidget *parent)
    : QWidget(parent)
{
    QHBoxLayout *layout = new QHBoxLayout(this);
    layout->setMargin(0);
    layout->setContentsMargins(3, 0, 3, 0);
    layout->setSpacing(3);
    value = new QLabel(this);
    value->setSizePolicy(QSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred));
    value->setText(QLatin1String("..."));

    button = new QToolButton(this);
    button->setSizePolicy(QSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed));
    button->setStyleSheet("QToolButton {"
                          "min-height: 10px;"
                          "min-width: 10px;"
                          "font-size: 10px;"
                          "border-style: solid;"
                          "border-width: 1px;"
                          "border-radius:1px;"
                          "padding: 0px;"
                          "border-color: palette(dark);"
                          "background-color: palette(button);"
                      "}");
    button->setText(QLatin1String("..."));

    layout->addWidget(value);
    layout->addWidget(button);

    setFocusProxy(button);
    setFocusPolicy(Qt::StrongFocus);

    connect(button, SIGNAL(clicked()), this, SIGNAL(buttonClicked()));
}

void ButtonEdit::setButtonText(QString text)
{
    button->setText(text);
}

void ButtonEdit::setValueText(QString text)
{
    value->setText(text);
}


ButtonEditorFactory::~ButtonEditorFactory()
{
    QList<ButtonEdit *> editors = theButtonsToProperty.keys();
    QListIterator<ButtonEdit *> it(editors);
    while (it.hasNext())
        delete it.next();
}

void ButtonEditorFactory::connectPropertyManager(ButtonPropertyManager *manager)
{
    connect(manager, SIGNAL(valueChanged(QtProperty *, const QString &)),
                this, SLOT(slotPropertyChanged(QtProperty *, const QString &)));
    connect(manager, SIGNAL(labelChanged(QtProperty *, const QString &)),
                this, SLOT(slotLabelChanged(QtProperty *, const QString &)));
}

void ButtonEditorFactory::disconnectPropertyManager(ButtonPropertyManager *manager)
{
    disconnect(manager, SIGNAL(valueChanged(QtProperty *, const QString &)),
                this, SLOT(slotPropertyChanged(QtProperty *, const QString &)));
    disconnect(manager, SIGNAL(labelChanged(QtProperty *, const QString &)),
                this, SLOT(slotLabelChanged(QtProperty *, const QString &)));
}

QWidget *ButtonEditorFactory::createEditor(ButtonPropertyManager *manager,
        QtProperty *property, QWidget *parent)
{
    ButtonEdit *button = new ButtonEdit(parent);
    button->setValueText(manager->value(property));
    button->setButtonText(manager->label(property));

    theCreatedButtons[property].append(button);
    theButtonsToProperty[button] = property;

    // TODO connect button
    connect(button, SIGNAL(buttonClicked()),
                this, SLOT(slotSetValue()));
    connect(button, SIGNAL(destroyed(QObject *)),
                this, SLOT(slotEditorDestroyed(QObject *)));
    return button;
}

void ButtonEditorFactory::slotPropertyChanged(QtProperty *property, const QString &value)
{
    if (!theCreatedButtons.contains(property) || value.isNull())
        return;

    QList<ButtonEdit *> buttons = theCreatedButtons[property];
    QListIterator<ButtonEdit *> itButtons(buttons);
    while (itButtons.hasNext())
        itButtons.next()->setValueText(value);
}

void ButtonEditorFactory::slotLabelChanged(QtProperty *property, const QString &value)
{
    if (!theCreatedButtons.contains(property))
        return;

    QList<ButtonEdit *> buttons = theCreatedButtons[property];
    QListIterator<ButtonEdit *> itButtons(buttons);
    while (itButtons.hasNext())
        itButtons.next()->setButtonText(value);
}

void ButtonEditorFactory::slotSetValue(const QString &value)
{
    QObject *object = sender();
    QMap<ButtonEdit *, QtProperty *>::ConstIterator itButtons =  theButtonsToProperty.constBegin();
    while (itButtons != theButtonsToProperty.constEnd()) {
        if (itButtons.key() == object) {
            QtProperty *property = itButtons.value();
            ButtonPropertyManager *manager = propertyManager(property);
            if (!manager)
                return;
            manager->setValue(property, value);
            return;
        }
        itButtons++;
    }
}

void ButtonEditorFactory::slotEditorDestroyed(QObject *object)
{
    QMap<ButtonEdit *, QtProperty *>::ConstIterator itButtons =
                theButtonsToProperty.constBegin();
    while (itButtons != theButtonsToProperty.constEnd()) {
        if (itButtons.key() == object) {
            ButtonEdit *editor = itButtons.key();
            QtProperty *property = itButtons.value();
            theButtonsToProperty.remove(editor);
            theCreatedButtons[property].removeAll(editor);
            if (theCreatedButtons[property].isEmpty())
                theCreatedButtons.remove(property);
            return;
        }
        itButtons++;
    }
}
