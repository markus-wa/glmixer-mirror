#include "OpenSoundControlTranslator.moc"
#include "ui_OpenSoundControlTranslator.h"

#include "common.h"
#include "OpenSoundControlManager.h"

#include <QMenu>
#include <QDesktopServices>
#include <QNetworkInterface>


TranslationDelegate::TranslationDelegate(QObject *parent) :
    QItemDelegate(parent)
{
    QRegExp validRegex("[/A-z0-9]+");
    validator.setRegExp(validRegex);
}

QWidget *TranslationDelegate::createEditor(QWidget *parent,
                                    const QStyleOptionViewItem &option,
                                    const QModelIndex &index) const
{
    QLineEdit *editor = new QLineEdit(parent);
    editor->setValidator(&validator);
    return editor;
}


void TranslationDelegate::setEditorData(QWidget *editor,
                                 const QModelIndex &index) const
{
    QString value =index.model()->data(index, Qt::EditRole).toString();
        QLineEdit *line = static_cast<QLineEdit*>(editor);
        line->setText(value);
}


void TranslationDelegate::setModelData(QWidget *editor,
                                QAbstractItemModel *model,
                                const QModelIndex &index) const
{
    QLineEdit *line = static_cast<QLineEdit*>(editor);
    QString value = line->text();
    model->setData(index, value);
}

void TranslationDelegate::updateEditorGeometry(QWidget *editor,
                                        const QStyleOptionViewItem &option,
                                        const QModelIndex &index) const
{
    editor->setGeometry(option.rect);
}


TranslationTableModel::TranslationTableModel(QObject *parent)
    :QAbstractTableModel(parent)
{
    dictionnary = OpenSoundControlManager::getInstance()->getTranslationDictionnary();
}

int TranslationTableModel::rowCount(const QModelIndex & /*parent*/) const
{
   return dictionnary->size();
}

int TranslationTableModel::columnCount(const QModelIndex & /*parent*/) const
{
    return 2;
}

QVariant TranslationTableModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid())
        return QVariant();

    if (index.row() >= dictionnary->size() || index.row() < 0)
        return QVariant();

    if (role == Qt::DisplayRole || role == Qt::EditRole)
    {
        // reading keys
        if (index.column() == 0 )
            return dictionnary->at(index.row()).first;
        // reading value
        return dictionnary->at(index.row()).second;

    }

    return QVariant();
}

QVariant TranslationTableModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (role == Qt::DisplayRole)
    {
        if (orientation == Qt::Horizontal) {
            switch (section)
            {
            case 0:
                return QString("Inputs");
            case 1:
                return QString("Translations");
            }
        }
    }

    return QVariant();
}

Qt::ItemFlags TranslationTableModel::flags(const QModelIndex & index) const
{
//    return Qt::ItemIsSelectable |  Qt::ItemIsEditable | Qt::ItemIsEnabled ;
    if (!index.isValid())
        return Qt::ItemIsEnabled;

    return QAbstractTableModel::flags(index) | Qt::ItemIsEditable;
}

bool TranslationTableModel::setData(const QModelIndex & index, const QVariant & value, int role)
{
    if (role == Qt::EditRole)
    {
        // remember value from editor
        QPair<QString, QString> p = dictionnary->value(index.row());

        if (index.column() == 0)
            p.first = value.toString();
        else if (index.column() == 1)
            p.second = value.toString();
        else
            return false;

        dictionnary->replace(index.row(), p);
        emit(dataChanged(index, index));
        return true;
    }

    return false;
}

bool TranslationTableModel::insertRows(int position, int rows, const QModelIndex &index)
{
    Q_UNUSED(index);
    beginInsertRows(QModelIndex(), position, position+rows-1);

    for (int row=0; row < rows; row++) {
        QPair<QString, QString> pair(" ", " ");
        dictionnary->insert(position, pair);
    }

    endInsertRows();
    return true;
}

bool TranslationTableModel::removeRows(int position, int rows, const QModelIndex &index)
{
    Q_UNUSED(index);
    beginRemoveRows(QModelIndex(), position, position+rows-1);

    for (int row=0; row < rows; ++row) {
        dictionnary->removeAt(position);
    }

    endRemoveRows();
    return true;
}

bool TranslationTableModel::contains(QString before, QString after)
{
    return dictionnary->contains( QPair<QString, QString>(before, after) );
}


OpenSoundControlTranslator::OpenSoundControlTranslator(QSettings *settings, QWidget *parent) :
    QWidget(parent),
    ui(new Ui::OpenSoundControlTranslator),
    appSettings(settings)
{
    ui->setupUi(this);

    QFile file(":/style/default");
    file.open(QFile::ReadOnly);
    QString styleSheet = QLatin1String(file.readAll());
    setStyleSheet(styleSheet);

    // improve table view with input validator and context menu
    TranslationDelegate *itDelegate = new  TranslationDelegate;
    ui->tableTranslation->setItemDelegateForColumn(1, itDelegate);
    ui->tableTranslation->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(ui->tableTranslation, SIGNAL(customContextMenuRequested(QPoint)), this, SLOT(contextMenu(QPoint)));

    // set model to table view
    ui->tableTranslation->setModel(&translations);

    // Use embedded fixed size font
    ui->consoleOSC->setFontFamily(getMonospaceFont());
    ui->consoleOSC->setFontPointSize(10);

    QRegExp validRegex("[/A-z0-9]+");
    validator.setRegExp(validRegex);
    ui->afterTranslation->setValidator(&validator);

    // set GUI initial status from Manager
    ui->enableOSC->setChecked( OpenSoundControlManager::getInstance()->isEnabled());
    ui->OSCPort->setValue( OpenSoundControlManager::getInstance()->getPort() );
    ui->verboseLogs->setChecked( OpenSoundControlManager::getInstance()->isVerbose() );

    // connect GUI to Manager
    connect(ui->enableOSC, SIGNAL(toggled(bool)), this, SLOT(updateManager()) );
    connect(ui->OSCPort, SIGNAL(valueChanged(int)), this, SLOT(updateManager()) );

    // connect logs
    connect(OpenSoundControlManager::getInstance(), SIGNAL(log(QString)), this, SLOT(logMessage(QString)) );
    connect(OpenSoundControlManager::getInstance(), SIGNAL(error(QString)), this, SLOT(logError(QString)) );
}

OpenSoundControlTranslator::~OpenSoundControlTranslator()
{
    delete ui;
}

void OpenSoundControlTranslator::showEvent(QShowEvent *e)
{

    if (appSettings) {
        if (appSettings->contains("OSCTranslatorHeader")) {
            ui->tableTranslation->horizontalHeader()->restoreState(appSettings->value("OSCTranslatorHeader").toByteArray());
        }
    }

    QWidget::showEvent(e);
}

void OpenSoundControlTranslator::hideEvent(QHideEvent *e)
{
    if (appSettings) {
        appSettings->setValue("OSCTranslatorHeader", ui->tableTranslation->horizontalHeader()->saveState());
    }

    QWidget::hideEvent(e);
}

void OpenSoundControlTranslator::on_addTranslation_pressed()
{
    QString before = ui->beforeTranslation->text();
    QString after  = ui->afterTranslation->text();

    if (before.isEmpty() || after.isEmpty())
        return;

    if (translations.contains(before, after))
        return;

    // insert translation in table
    translations.insertRows(0, 1);
    QModelIndex index = translations.index(0, 0, QModelIndex());
    translations.setData(index, before, Qt::EditRole);
    index = translations.index(0, 1, QModelIndex());
    translations.setData(index, after, Qt::EditRole);

    // clear selection
    ui->tableTranslation->clearSelection();
}



void  OpenSoundControlTranslator::on_verboseLogs_toggled(bool on)
{
    OpenSoundControlManager::getInstance()->setVerbose(on);
}


void OpenSoundControlTranslator::updateManager()
{
    bool on = ui->enableOSC->isChecked();
    qint16 p = (qint16) ui->OSCPort->value();
    OpenSoundControlManager::getInstance()->setEnabled(on, p);


    ui->consoleOSC->setTextColor(QColor(160, 250, 160));
    if (on) {
        QStringList addresses;
        foreach( const QHostAddress &a, QNetworkInterface::allAddresses())
            if (a.protocol() == QAbstractSocket::IPv4Protocol)
                addresses << a.toString() + ":" + QString::number(p);

        logMessage(tr("Listening to ") + addresses.join(", "));
    }
    else
        logMessage(tr("Disabled"));
    ui->consoleOSC->setTextColor(Qt::white);

}

void OpenSoundControlTranslator::logError(QString m)
{
    ui->consoleOSC->setTextColor(QColor(250, 160, 160));
    ui->consoleOSC->append(m);
    ui->consoleOSC->setTextColor(Qt::white);
}

void OpenSoundControlTranslator::logMessage(QString m)
{
    ui->consoleOSC->append(m);
}

void OpenSoundControlTranslator::contextMenu(QPoint p)
{
    static QMenu *contextmenu = NULL;
    if (contextmenu == NULL) {
        contextmenu = new QMenu(this);
        QAction *remove = new QAction(tr("Remove"), this);
        connect(remove, SIGNAL(triggered()), this, SLOT(removeSelection()));
        contextmenu->addAction(remove);
    }
    contextmenu->popup(ui->tableTranslation->viewport()->mapToGlobal(p));
}

void OpenSoundControlTranslator::removeSelection()
{
   QModelIndexList selection = ui->tableTranslation->selectionModel()->selectedRows();
   translations.removeRows(selection[0].row(), selection.size());
}

void OpenSoundControlTranslator::on_OSCHelp_pressed()
{
    // glmixer help
//    QDesktopServices::openUrl(QUrl("https://sourceforge.net/p/glmixer/wiki/GLMixer_OSC_Specs/", QUrl::TolerantMode));

    // global help OSC
    QDesktopServices::openUrl(QUrl("http://opensoundcontrol.org/spec-1_0", QUrl::TolerantMode));
}
