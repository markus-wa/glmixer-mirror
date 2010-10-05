/*
 * SessionSwitcherWidget.cpp
 *
 *  Created on: Oct 1, 2010
 *      Author: bh
 */

#include <QDomDocument>

#include "common.h"
#include "RenderingManager.h"
#include "OutputRenderWindow.h"

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


SessionSwitcherWidget::SessionSwitcherWidget(QWidget *parent, QSettings *settings) : QWidget(parent), appSettings(settings), m_iconSize(48,48) {

	setupFolderToolbox();

	restoreSettings();
}


void SessionSwitcherWidget::setupFolderToolbox()
{
	transitionSelection = new QComboBox;
	transitionSelection->addItem("Transition - Disabled");
	transitionSelection->addItem("Transition - Background");
	transitionSelection->addItem("Transition - Last frame");
	transitionSelection->addItem("Transition - Custom color");
	transitionSelection->addItem("Transition - Media file");

	QLabel *transitionDurationLabel;
	transitionDuration = new QSpinBox;
	transitionDuration->setSingleStep(200);
	transitionDuration->setMaximum(5000);
	transitionDuration->setValue(1000);
	transitionDurationLabel = new QLabel(tr("&Duration (ms):"));
	transitionDurationLabel->setBuddy(transitionDuration);
	transitionDuration->setEnabled(false);

    // create the curves into the transition easing curve selector
	easingCurvePicker = createCurveIcons();
	easingCurvePicker->setViewMode(QListView::IconMode);
	easingCurvePicker->setWrapping (false);
    easingCurvePicker->setIconSize(m_iconSize);
    easingCurvePicker->setFixedHeight(m_iconSize.height()+30);
	easingCurvePicker->setEnabled(false);
	easingCurvePicker->setCurrentRow(3);

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

    customButton = new QToolButton;
	customButton->setIcon( QIcon() );
	customButton->setVisible(false);

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
    proxyView->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    QLabel *filterPatternLabel;
    QLineEdit *filterPatternLineEdit;
    filterPatternLineEdit = new QLineEdit;
    filterPatternLineEdit->setText("");
    filterPatternLabel = new QLabel(tr("&Filename filter:"));
    filterPatternLabel->setBuddy(filterPatternLineEdit);

    connect(filterPatternLineEdit, SIGNAL(textChanged(QString)), this, SLOT(nameFilterChanged(QString)));
    connect(dirButton, SIGNAL(clicked()),  this, SLOT(openFolder()));
    connect(customButton, SIGNAL(clicked()),  this, SLOT(customizeTransition()));
    connect(proxyView, SIGNAL(doubleClicked(QModelIndex)), this, SLOT(openFileFromFolder(QModelIndex) ));
    connect(folderHistory, SIGNAL(currentIndexChanged(QString)), this, SLOT(folderChanged(QString)));
    connect(transitionSelection, SIGNAL(currentIndexChanged(int)), this, SLOT(selectTransitionType(int)));
    connect(transitionDuration, SIGNAL(valueChanged(int)), OutputRenderWindow::getInstance(), SLOT(setTransitionDuration(int)));
    connect(easingCurvePicker, SIGNAL(currentRowChanged (int)), OutputRenderWindow::getInstance(), SLOT(setTransitionCurve(int)));

    QGridLayout *mainLayout = new QGridLayout;
    mainLayout->addWidget(transitionSelection, 0, 0, 1, 2);
    mainLayout->addWidget(customButton, 0, 2);

    mainLayout->addWidget(transitionDurationLabel, 1, 0);
    mainLayout->addWidget(transitionDuration, 1, 1, 1, 2);

    mainLayout->addWidget(easingCurvePicker, 2, 0, 1, 3);
    mainLayout->addWidget(proxyView, 3, 0, 1, 3);
    mainLayout->addWidget(folderHistory, 4, 0, 1, 2);
    mainLayout->addWidget(dirButton, 4, 2);
    mainLayout->addWidget(filterPatternLabel, 5, 0);
    mainLayout->addWidget(filterPatternLineEdit, 5, 1, 1, 2);
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

void  SessionSwitcherWidget::selectTransitionType(int t){

	OutputRenderWindow::transitionType tt = (OutputRenderWindow::transitionType) CLAMP(OutputRenderWindow::TRANSITION_NONE, t, OutputRenderWindow::TRANSITION_CUSTOM_MEDIA);
	OutputRenderWindow::getInstance()->setTransitionType( tt );

	customButton->setStyleSheet("");
	transitionDuration->setEnabled(tt != OutputRenderWindow::TRANSITION_NONE);
	easingCurvePicker->setEnabled(tt != OutputRenderWindow::TRANSITION_NONE);

	if ( tt == OutputRenderWindow::TRANSITION_CUSTOM_COLOR ) {
		QPixmap c = QPixmap(16, 16);
		c.fill(OutputRenderWindow::getInstance()->transitionColor());
		customButton->setIcon( QIcon(c) );
		customButton->setVisible(true);

	} else if ( tt == OutputRenderWindow::TRANSITION_CUSTOM_MEDIA ) {
		customButton->setIcon(QIcon(QString::fromUtf8(":/glmixer/icons/fileopen.png")));

		if ( !QFileInfo(OutputRenderWindow::getInstance()->transitionMedia()).exists() )
			customButton->setStyleSheet("QToolButton { border: 1px solid red }");
		customButton->setVisible(true);
	} else
		customButton->setVisible(false);

}


void SessionSwitcherWidget::customizeTransition()
{
	if (OutputRenderWindow::getInstance()->getTransitionType() == OutputRenderWindow::TRANSITION_CUSTOM_COLOR ) {

		QColor color = QColorDialog::getColor(OutputRenderWindow::getInstance()->transitionColor(), parentWidget());
		if (color.isValid()) {
			OutputRenderWindow::getInstance()->setTransitionColor(color);
			selectTransitionType( (int) OutputRenderWindow::TRANSITION_CUSTOM_COLOR);
		}
	}
	else if (OutputRenderWindow::getInstance()->getTransitionType() == OutputRenderWindow::TRANSITION_CUSTOM_MEDIA ) {

		QString oldfile = OutputRenderWindow::getInstance()->transitionMedia();
		QString newfile = QFileDialog::getOpenFileName(this, tr("Open File"), oldfile.isEmpty() ? QDir::currentPath() : oldfile,
											tr("Video (*.mov *.avi *.wmv *.mpeg *.mp4 *.mpg *.vob *.swf *.flv);;Image (*.png *.jpg *.jpeg *.tif *.tiff *.gif *.tga *.sgi *.bmp)"));
		if ( QFileInfo(newfile).exists()) {
			OutputRenderWindow::getInstance()->setTransitionMedia(newfile);
			customButton->setStyleSheet("");
		} else {
			// not a valid file ; show a warning only if the QFileDialog did not return null (cancel)
			if (!newfile.isNull())
				qCritical( qPrintable( tr("The file %1 does not exist.").arg(newfile)) );
			// if no valid oldfile neither; show icon in red
			if (oldfile.isEmpty())
				customButton->setStyleSheet("QToolButton { border: 1px solid red }");
		}
	}
	// remember
	saveSettings();
}


void SessionSwitcherWidget::saveSettings()
{
    appSettings->setValue("transitionSelection", transitionSelection->currentIndex());
    appSettings->setValue("transitionDuration", transitionDuration->value());
    appSettings->setValue("transitionCurve", easingCurvePicker->currentRow());
    QVariant variant = OutputRenderWindow::getInstance()->transitionColor();
    appSettings->setValue("transitionColor", variant);
    appSettings->setValue("transitionMedia", OutputRenderWindow::getInstance()->transitionMedia());

}

void SessionSwitcherWidget::restoreSettings()
{
	QStringList folders = appSettings->value("recentFolderList").toStringList();
	if (folders.empty())
		folderHistory->addItem(QDir::currentPath());
	else
		folderHistory->addItems(folders);
    folderHistory->setCurrentIndex(0);

    OutputRenderWindow::getInstance()->setTransitionColor( appSettings->value("transitionColor").value<QColor>());

    QString mediaFileName = appSettings->value("transitionMedia", "").toString();
    if (QFileInfo(mediaFileName).exists())
    	OutputRenderWindow::getInstance()->setTransitionMedia(mediaFileName);

    transitionSelection->setCurrentIndex(appSettings->value("transitionSelection", "0").toInt());
    transitionDuration->setValue(appSettings->value("transitionDuration", "1000").toInt());
    easingCurvePicker->setCurrentRow(appSettings->value("transitionCurve", "3").toInt());

    connect(transitionSelection, SIGNAL(currentIndexChanged(int)), this, SLOT(saveSettings()));
    connect(transitionDuration, SIGNAL(valueChanged(int)), this, SLOT(saveSettings()));
    connect(easingCurvePicker, SIGNAL(currentRowChanged (int)), this, SLOT(saveSettings()));
}

QListWidget *SessionSwitcherWidget::createCurveIcons()
{
	QListWidget *easingCurvePicker = new QListWidget;
    QPixmap pix(m_iconSize);
    QPainter painter(&pix);
    QLinearGradient gradient(0,0, 0, m_iconSize.height());
    gradient.setColorAt(0.0, QColor(240, 240, 240));
    gradient.setColorAt(1.0, QColor(224, 224, 224));
    QBrush brush(gradient);
    const QMetaObject &mo = QEasingCurve::staticMetaObject;
    QMetaEnum metaEnum = mo.enumerator(mo.indexOfEnumerator("Type"));
    // Skip QEasingCurve::Custom
    for (int i = 0; i < QEasingCurve::NCurveTypes - 3; ++i) {
        painter.fillRect(QRect(QPoint(0, 0), m_iconSize), brush);
        QEasingCurve curve((QEasingCurve::Type)i);
        painter.setPen(QColor(0, 0, 255, 64));
        qreal xAxis = m_iconSize.height()/1.15;
        qreal yAxis = m_iconSize.width()/5.5;
        painter.drawLine(0, xAxis, m_iconSize.width(),  xAxis);
        painter.drawLine(yAxis, 0, yAxis, m_iconSize.height());

        qreal curveScale = m_iconSize.height()/1.4;

        painter.setPen(Qt::NoPen);

        // start point
        painter.setBrush(Qt::red);
        QPoint start(yAxis, xAxis - curveScale * curve.valueForProgress(0));
        painter.drawRect(start.x() - 1, start.y() - 1, 3, 3);

        // end point
        painter.setBrush(Qt::blue);
        QPoint end(yAxis + curveScale, xAxis - curveScale * curve.valueForProgress(1));
        painter.drawRect(end.x() - 1, end.y() - 1, 3, 3);

        QPainterPath curvePath;
        curvePath.moveTo(start);
        for (qreal t = 0; t <= 1.0; t+=1.0/curveScale) {
            QPoint to;
            to.setX(yAxis + curveScale * t);
            to.setY(xAxis - curveScale * curve.valueForProgress(t));
            curvePath.lineTo(to);
        }
        painter.setRenderHint(QPainter::Antialiasing, true);
        painter.strokePath(curvePath, QColor(32, 32, 32));
        painter.setRenderHint(QPainter::Antialiasing, false);
        QListWidgetItem *item = new QListWidgetItem;
        item->setIcon(QIcon(pix));
//        item->setText(metaEnum.key(i));
        easingCurvePicker->addItem(item);
    }
    return easingCurvePicker;
}

