/*
 * SessionSwitcherWidget.cpp
 *
 *  Created on: Oct 1, 2010
 *      Author: bh
 */

#include <QDomDocument>

#include "common.h"
#include "SessionSwitcherWidget.moc"

class folderValidator : public QValidator
{
  public:
    folderValidator(QObject *parent) : QValidator(parent) { }

    QValidator::State validate ( QString & input, int & pos ) const {
      QDir d(input);
      if( d.exists ())
    	  return QValidator::Acceptable;
      if( d.isAbsolute ())
    	  return QValidator::Intermediate;

      return QValidator::Invalid;
    }
};



void addFile(QStandardItemModel *model, const QString &name, const QDateTime &date, const QString &filename)
{

   QFile file(filename);
   if ( !file.open(QFile::ReadOnly | QFile::Text) )
	return;

    QDomDocument doc;
    QString errorStr;
    int errorLine;
    int errorColumn;
    if ( !doc.setContent(&file, true, &errorStr, &errorLine, &errorColumn) )
    	return;

    QDomElement root = doc.documentElement();
    if ( root.tagName() != "GLMixer" )
        return;

    if ( root.hasAttribute("version") && root.attribute("version") != XML_GLM_VERSION )
        return;

    QDomElement srcconfig = root.firstChildElement("SourceList");
    if ( srcconfig.isNull() )
    	return;

    int nbElem = srcconfig.childNodes().count();

    file.close();

    model->insertRow(0);
    model->setData(model->index(0, 0), name);
    model->setData(model->index(0, 0), filename, Qt::UserRole);
    model->itemFromIndex (model->index(0, 0))->setFlags (Qt::ItemIsSelectable | Qt::ItemIsEnabled | Qt::ItemIsDragEnabled);
    model->setData(model->index(0, 1), nbElem);
    model->setData(model->index(0, 1), filename, Qt::UserRole);
    model->itemFromIndex (model->index(0, 1))->setFlags (Qt::ItemIsSelectable | Qt::ItemIsEnabled | Qt::ItemIsDragEnabled);
    model->setData(model->index(0, 2), date);
    model->setData(model->index(0, 2), filename, Qt::UserRole);
    model->itemFromIndex (model->index(0, 2))->setFlags (Qt::ItemIsSelectable | Qt::ItemIsEnabled | Qt::ItemIsDragEnabled);

}

void fillFolderModel(QStandardItemModel *model, const QString &path)
{
    QDir dir(path);
    QFileInfoList fileList = dir.entryInfoList(QStringList("*.glm"), QDir::Files);

    // empty list
    model->removeRows(0, model->rowCount());

    // fill list
    for (int i = 0; i < fileList.size(); ++i) {
         QFileInfo fileInfo = fileList.at(i);
	 addFile(model, fileInfo.completeBaseName(), fileInfo.lastModified (), fileInfo.absoluteFilePath() );
    }
}


SessionSwitcherWidget::SessionSwitcherWidget(QWidget *parent, QSettings *settings) : QWidget(parent), appSettings(settings) {

	setupFolderToolbox();

	QStringList folders = appSettings->value("recentFolderList").toStringList();
	if (folders.empty())
		folderHistory->addItem(QDir::currentPath());
	else
		folderHistory->addItems(folders);

    folderHistory->setCurrentIndex(0);
}


void SessionSwitcherWidget::setupFolderToolbox()
{
	transitionSelection = new QComboBox;
	transitionSelection->addItem("Transition - disabled");
	transitionSelection->addItem("Transition - to Black");
	transitionSelection->addItem("Transition - keep last frame");

    folderModel = new QStandardItemModel(0, 3, this);
    folderModel->setHeaderData(0, Qt::Horizontal, QObject::tr("Filename"));
    folderModel->setHeaderData(1, Qt::Horizontal, QObject::tr("Sources"));
    folderModel->setHeaderData(2, Qt::Horizontal, QObject::tr("Last modified"));

    proxyFolderModel = new QSortFilterProxyModel;
    proxyFolderModel->setDynamicSortFilter(true);
    proxyFolderModel->setFilterKeyColumn(0);
    proxyFolderModel->setSourceModel(folderModel);

    folderValidator *v = new folderValidator(this);
    QToolButton *dirButton = new QToolButton;
	QIcon icon;
	icon.addFile(QString::fromUtf8(":/glmixer/icons/fileopen.png"), QSize(), QIcon::Normal, QIcon::Off);
	dirButton->setIcon(icon);

	folderHistory = new QComboBox;
	folderHistory->setEditable(true);
	folderHistory->setValidator(v);
	folderHistory->setInsertPolicy (QComboBox::InsertAtTop);
	folderHistory->setMaxCount(MAX_RECENT_FOLDERS);

    QTreeView *proxyView;
    proxyView = new QTreeView;
    proxyView->setRootIsDecorated(false);
    proxyView->setAlternatingRowColors(true);
    proxyView->setSortingEnabled(true);
    proxyView->sortByColumn(0, Qt::AscendingOrder);
    proxyView->setModel(proxyFolderModel);

    QLabel *filterPatternLabel;
    QLineEdit *filterPatternLineEdit;
    filterPatternLineEdit = new QLineEdit;
    filterPatternLineEdit->setText("");
    filterPatternLabel = new QLabel(tr("&Filename filter:"));
    filterPatternLabel->setBuddy(filterPatternLineEdit);

    connect(filterPatternLineEdit, SIGNAL(textChanged(QString)),
            this, SLOT(nameFilterChanged(QString)));
    connect(dirButton, SIGNAL(clicked()),  this, SLOT(openFolder()));
//    connect(folderPathEdit, SIGNAL(editingFinished()), this, SLOT(editPathFolder()));
    connect(proxyView, SIGNAL(doubleClicked(QModelIndex)),
            this, SLOT(openFileFromFolder(QModelIndex) ));
    connect(folderHistory, SIGNAL(currentIndexChanged(QString)), this, SLOT(folderChanged(QString)));
    connect(transitionSelection, SIGNAL(currentIndexChanged(int)), this, SLOT(selectTransition(int)));

    QGridLayout *mainLayout = new QGridLayout;
    mainLayout->addWidget(transitionSelection, 0, 0, 1, 3);
    mainLayout->addWidget(proxyView, 1, 0, 1, 3);
    mainLayout->addWidget(folderHistory, 2, 0, 1, 2);
    mainLayout->addWidget(dirButton, 2, 2);
    mainLayout->addWidget(filterPatternLabel, 3, 0);
    mainLayout->addWidget(filterPatternLineEdit, 3, 1, 1, 2);
    setLayout(mainLayout);

}


void SessionSwitcherWidget::nameFilterChanged(const QString &s)
{
    QRegExp regExp(s, Qt::CaseInsensitive, QRegExp::FixedString);
    proxyFolderModel->setFilterRegExp(regExp);
}


void SessionSwitcherWidget::folderChanged( const QString & text )
{
    QStringList folders = appSettings->value("recentFolderList").toStringList();
    folders.removeAll(text);
    folders.prepend(text);
    while (folders.size() > MAX_RECENT_FOLDERS)
        folders.removeLast();
    appSettings->setValue("recentFolderList", folders);

    fillFolderModel(folderModel, text);
}

void SessionSwitcherWidget::openFolder()
{
  QString dirName = QFileDialog::getExistingDirectory(this, tr("Select a directory"), QDir::currentPath());
  if ( dirName.isEmpty() )
	return;

   folderHistory->insertItem(0, dirName);
   folderHistory->setCurrentIndex(0);

}


void SessionSwitcherWidget::openFileFromFolder(const QModelIndex & index){

	emit switchSessionFile(proxyFolderModel->data(index, Qt::UserRole).toString());

}

void  SessionSwitcherWidget::selectTransition(int transitionType){


}

