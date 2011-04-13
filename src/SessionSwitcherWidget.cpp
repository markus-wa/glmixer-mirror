/*
 * SessionSwitcherWidget.cpp
 *
 *  Created on: Oct 1, 2010
 *      Author: bh
 */

#include <QDomDocument>

#include "common.h"
#include "SessionSwitcher.h"
#include "OutputRenderWindow.h"
#include "SourceDisplayWidget.h"

#include "SessionSwitcherWidget.moc"


void addFile(QStandardItemModel *model, const QString &name, const QDateTime &date, const QString &filename, const standardAspectRatio allowedAspectRatio)
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
    // get number of sources in the session
    int nbElem = srcconfig.childNodes().count();

    // get aspect ratio
    QString aspectRatio;
	standardAspectRatio ar = (standardAspectRatio) srcconfig.attribute("aspectRatio", "0").toInt();
	switch(ar) {
	case ASPECT_RATIO_FREE:
		aspectRatio = "free";
		break;
	case ASPECT_RATIO_16_10:
		aspectRatio = "16:10";
		break;
	case ASPECT_RATIO_16_9:
		aspectRatio = "16:9";
		break;
	case ASPECT_RATIO_3_2:
		aspectRatio = "3:2";
		break;
	default:
	case ASPECT_RATIO_4_3:
		aspectRatio = "4:3";
	}

    file.close();

    Qt::ItemFlags flags = Qt::ItemIsSelectable;
    if (allowedAspectRatio == ASPECT_RATIO_FREE || ar == allowedAspectRatio)
    	flags |= Qt::ItemIsEnabled;

    model->insertRow(0);
    model->setData(model->index(0, 0), name);
    model->setData(model->index(0, 0), filename, Qt::UserRole);
    model->itemFromIndex (model->index(0, 0))->setFlags (flags);

    model->setData(model->index(0, 1), nbElem);
    model->setData(model->index(0, 1), filename, Qt::UserRole);
    model->itemFromIndex (model->index(0, 1))->setFlags (flags);

    model->setData(model->index(0, 2), aspectRatio);
    model->setData(model->index(0, 2), filename, Qt::UserRole);
    model->itemFromIndex (model->index(0, 2))->setFlags (flags);

    model->setData(model->index(0, 3), date.toString("yy/MM/dd hh:mm"));
    model->setData(model->index(0, 3), filename, Qt::UserRole);
    model->itemFromIndex (model->index(0, 3))->setFlags (flags);

}

void fillFolderModel(QStandardItemModel *model, const QString &path, const standardAspectRatio allowedAspectRatio)
{
    QDir dir(path);
    QFileInfoList fileList = dir.entryInfoList(QStringList("*.glm"), QDir::Files);

    // empty list
    model->removeRows(0, model->rowCount());

    // fill list
    for (int i = 0; i < fileList.size(); ++i) {
         QFileInfo fileInfo = fileList.at(i);
         addFile(model, fileInfo.completeBaseName(), fileInfo.lastModified(), fileInfo.absoluteFilePath(), allowedAspectRatio);
    }
}


SessionSwitcherWidget::SessionSwitcherWidget(QWidget *parent, QSettings *settings) : QWidget(parent),
																					appSettings(settings), m_iconSize(48,48), allowedAspectRatio(ASPECT_RATIO_FREE) {

	transitionSelection = new QComboBox;
	transitionSelection->addItem("Transition - Disabled");
	transitionSelection->addItem("Transition - Black");
	transitionSelection->addItem("Transition - Last frame");
	transitionSelection->addItem("Transition - Custom color");
	transitionSelection->addItem("Transition - Media file");
	transitionSelection->setToolTip("Select the transition type");

	transitionTab = new QTabWidget(this);
	transitionTab->setToolTip("Choose how you control the transition");
	transitionTab->setSizePolicy(QSizePolicy(QSizePolicy::Preferred, QSizePolicy::Maximum));
	transitionTab->addTab( new QWidget(), "Manual");
	QGridLayout *g = new QGridLayout;

    sessionLabel = new QLabel;
    sessionLabel->setText("Current");
    sessionLabel->setSizePolicy( QSizePolicy::Preferred, QSizePolicy::Expanding);
    sessionLabel->setAlignment(Qt::AlignLeading|Qt::AlignLeft|Qt::AlignBottom);
    g->addWidget(sessionLabel, 0, 0);

    overlayPreview = new SourceDisplayWidget(this);
    overlayPreview->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    overlayPreview->setMinimumSize(QSize(80, 60));
    g->addWidget(overlayPreview, 0, 1);

    overlayLabel = new QLabel;
    overlayLabel->setText("Next");
    overlayLabel->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Expanding);
    overlayLabel->setAlignment(Qt::AlignBottom|Qt::AlignRight|Qt::AlignTrailing);
    g->addWidget(overlayLabel, 0, 2);

    transitionSlider = new QSlider;
    transitionSlider->setMinimum(-100);
    transitionSlider->setMaximum(101);
    transitionSlider->setValue(-100);
    transitionSlider->setOrientation(Qt::Horizontal);
    transitionSlider->setTickPosition(QSlider::TicksAbove);
    transitionSlider->setTickInterval(100);
    g->addWidget(transitionSlider, 1, 0, 1, 3);

    transitionTab->widget(0)->setLayout(g);

	QLabel *transitionDurationLabel;
	transitionDuration = new QSpinBox;
	transitionDuration->setSingleStep(200);
	transitionDuration->setMaximum(5000);
	transitionDuration->setValue(1000);
	transitionDurationLabel = new QLabel(tr("&Duration (ms):"));
	transitionDurationLabel->setBuddy(transitionDuration);

    // create the curves into the transition easing curve selector
	easingCurvePicker = createCurveIcons();
	easingCurvePicker->setViewMode(QListView::IconMode);
	easingCurvePicker->setWrapping (false);
    easingCurvePicker->setIconSize(m_iconSize);
    easingCurvePicker->setFixedHeight(m_iconSize.height()+34);
	easingCurvePicker->setCurrentRow(3);

	transitionTab->addTab( new QWidget(), "Automatic");
	g = new QGridLayout;
	g->addWidget(transitionDurationLabel, 1, 0);
	g->addWidget(transitionDuration, 1, 1);
    g->addWidget(easingCurvePicker, 2, 0, 1, 2);
    transitionTab->widget(1)->setLayout(g);

    folderModel = new QStandardItemModel(0, 4, this);
    folderModel->setHeaderData(0, Qt::Horizontal, QObject::tr("Filename"));
    folderModel->setHeaderData(1, Qt::Horizontal, QObject::tr("n"));
    folderModel->setHeaderData(2, Qt::Horizontal, QObject::tr("W:H"));
    folderModel->setHeaderData(3, Qt::Horizontal, QObject::tr("Last modified"));

    proxyFolderModel = new QSortFilterProxyModel;
    proxyFolderModel->setDynamicSortFilter(true);
    proxyFolderModel->setFilterKeyColumn(0);
    proxyFolderModel->setSourceModel(folderModel);

    folderValidator *v = new folderValidator(this);
    QToolButton *dirButton = new QToolButton;
    dirButton->setToolTip("Add a folder to the list");
	QIcon icon;
	icon.addFile(QString::fromUtf8(":/glmixer/icons/fileopen.png"), QSize(), QIcon::Normal, QIcon::Off);
	dirButton->setIcon(icon);

    QToolButton *dirDeleteButton = new QToolButton;
    dirDeleteButton->setToolTip("Remove a folder from the list");
	QIcon icon2;
	icon2.addFile(QString::fromUtf8(":/glmixer/icons/fileclose.png"), QSize(), QIcon::Normal, QIcon::Off);
	dirDeleteButton->setIcon(icon2);

    customButton = new QToolButton;
	customButton->setIcon( QIcon() );
	customButton->setVisible(false);

	folderHistory = new QComboBox;
	folderHistory->setToolTip("List of folders containing session files");
	folderHistory->setEditable(true);
	folderHistory->setValidator(v);
	folderHistory->setInsertPolicy (QComboBox::InsertAtTop);
	folderHistory->setMaxCount(MAX_RECENT_FOLDERS);
	folderHistory->setMaximumWidth(250);
	folderHistory->setSizeAdjustPolicy(QComboBox::AdjustToContents);

    QTreeView *proxyView;
    proxyView = new QTreeView;
    proxyView->setToolTip("Double click on a session to initiate the transition");
    proxyView->setRootIsDecorated(false);
    proxyView->setAlternatingRowColors(true);
    proxyView->setSortingEnabled(true);
    proxyView->sortByColumn(0, Qt::AscendingOrder);
    proxyView->setModel(proxyFolderModel);
    proxyView->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    proxyView->resizeColumnToContents(1);
    proxyView->resizeColumnToContents(2);
    proxyView->setSizePolicy(QSizePolicy(QSizePolicy::Preferred, QSizePolicy::Expanding));

    QLabel *filterPatternLabel;
    QLineEdit *filterPatternLineEdit;
    filterPatternLineEdit = new QLineEdit;
    filterPatternLineEdit->setText("");
    filterPatternLabel = new QLabel(tr("&Filename filter:"));
    filterPatternLabel->setBuddy(filterPatternLineEdit);

    connect(filterPatternLineEdit, SIGNAL(textChanged(QString)), this, SLOT(nameFilterChanged(QString)));
    connect(dirButton, SIGNAL(clicked()),  this, SLOT(openFolder()));
    connect(dirDeleteButton, SIGNAL(clicked()),  this, SLOT(discardFolder()));
    connect(customButton, SIGNAL(clicked()),  this, SLOT(customizeTransition()));
    connect(proxyView, SIGNAL(doubleClicked(QModelIndex)), this, SLOT(openFileFromFolder(QModelIndex) ));
    connect(folderHistory, SIGNAL(currentIndexChanged(QString)), this, SLOT(folderChanged(QString)));
    connect(transitionSelection, SIGNAL(currentIndexChanged(int)), this, SLOT(selectTransitionType(int)));
    connect(transitionDuration, SIGNAL(valueChanged(int)), RenderingManager::getSessionSwitcher(), SLOT(setTransitionDuration(int)));
    connect(easingCurvePicker, SIGNAL(currentRowChanged (int)), RenderingManager::getSessionSwitcher(), SLOT(setTransitionCurve(int)));
    connect(transitionSlider, SIGNAL(valueChanged(int)), this, SLOT(manualTransitionAdjustment(int)));

    QGridLayout *mainLayout = new QGridLayout;
    mainLayout->addWidget(transitionSelection, 0, 0, 1, 3);
    mainLayout->addWidget(customButton, 0, 3);
    mainLayout->addWidget(transitionTab, 1, 0, 1, 4);
    mainLayout->addWidget(proxyView, 2, 0, 1, 4);
    mainLayout->addWidget(folderHistory, 3, 0, 1, 2);
    mainLayout->addWidget(dirButton, 3, 2);
    mainLayout->addWidget(dirDeleteButton, 3, 3);
    mainLayout->addWidget(filterPatternLabel, 4, 0);
    mainLayout->addWidget(filterPatternLineEdit, 4, 1, 1, 3);
    setLayout(mainLayout);

    // restore the settings
	restoreSettings();
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

    folderHistory->updateGeometry ();
    fillFolderModel(folderModel, text, allowedAspectRatio);
}

void SessionSwitcherWidget::openFolder()
{
  QString dirName = QFileDialog::getExistingDirectory(this, tr("Select a directory"), QDir::currentPath());
  if ( dirName.isEmpty() )
	return;

   folderHistory->insertItem(0, dirName);
   folderHistory->setCurrentIndex(0);

}

void SessionSwitcherWidget::discardFolder()
{
	QStringList folders = appSettings->value("recentFolderList").toStringList();
	folders.removeAll( folderHistory->currentText() );
    appSettings->setValue("recentFolderList", folders);
	folderHistory->removeItem(folderHistory->currentIndex());

	updateFolder();
}

void SessionSwitcherWidget::updateFolder()
{
	folderChanged( folderHistory->currentText() );
	folderHistory->updateGeometry ();
}

void SessionSwitcherWidget::openFileFromFolder(const QModelIndex & index)
{
	if (transitionTab->currentIndex() == 1)
		emit switchSessionFile(proxyFolderModel->data(index, Qt::UserRole).toString());
}

void  SessionSwitcherWidget::selectTransitionType(int t)
{
	SessionSwitcher::transitionType tt = (SessionSwitcher::transitionType) CLAMP(SessionSwitcher::TRANSITION_NONE, t, SessionSwitcher::TRANSITION_CUSTOM_MEDIA);
	RenderingManager::getSessionSwitcher()->setTransitionType( tt );

	customButton->setStyleSheet("");
	transitionTab->setEnabled(tt != SessionSwitcher::TRANSITION_NONE);

	if ( tt == SessionSwitcher::TRANSITION_CUSTOM_COLOR ) {
		QPixmap c = QPixmap(16, 16);
		c.fill(RenderingManager::getSessionSwitcher()->transitionColor());
		customButton->setIcon( QIcon(c) );
		customButton->setVisible(true);
		customButton->setToolTip("Choose color");

	} else if ( tt == SessionSwitcher::TRANSITION_CUSTOM_MEDIA ) {
		customButton->setIcon(QIcon(QString::fromUtf8(":/glmixer/icons/fileopen.png")));

		if ( !QFileInfo(RenderingManager::getSessionSwitcher()->transitionMedia()).exists() )
			customButton->setStyleSheet("QToolButton { border: 1px solid red }");
		customButton->setVisible(true);
		customButton->setToolTip("Choose media");
	} else
		customButton->setVisible(false);

	// set the overlay source
	overlayPreview->setSource(RenderingManager::getSessionSwitcher()->overlaySource);

}


void SessionSwitcherWidget::customizeTransition()
{
	if (RenderingManager::getSessionSwitcher()->getTransitionType() == SessionSwitcher::TRANSITION_CUSTOM_COLOR ) {

		QColor color = QColorDialog::getColor(RenderingManager::getSessionSwitcher()->transitionColor(), parentWidget());
		if (color.isValid()) {
			RenderingManager::getSessionSwitcher()->setTransitionColor(color);
			selectTransitionType( (int) SessionSwitcher::TRANSITION_CUSTOM_COLOR);
		}
	}
	else if (RenderingManager::getSessionSwitcher()->getTransitionType() == SessionSwitcher::TRANSITION_CUSTOM_MEDIA ) {

		QString oldfile = RenderingManager::getSessionSwitcher()->transitionMedia();
		QString newfile = QFileDialog::getOpenFileName(this, tr("Open File"), oldfile.isEmpty() ? QDir::currentPath() : oldfile,
											tr("Video (*.mov *.avi *.wmv *.mpeg *.mp4 *.mpg *.vob *.swf *.flv);;Image (*.png *.jpg *.jpeg *.tif *.tiff *.gif *.tga *.sgi *.bmp)"));
		if ( QFileInfo(newfile).exists()) {
			RenderingManager::getSessionSwitcher()->setTransitionMedia(newfile);
			customButton->setStyleSheet("");
		} else {
			// not a valid file ; show a warning only if the QFileDialog did not return null (cancel)
			if (!newfile.isNull())
				qCritical( "The file %s does not exist.", qPrintable(newfile) );
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
    QVariant variant = RenderingManager::getSessionSwitcher()->transitionColor();
    appSettings->setValue("transitionColor", variant);
    appSettings->setValue("transitionMedia", RenderingManager::getSessionSwitcher()->transitionMedia());
    appSettings->setValue("transitionTab", transitionTab->currentIndex());
}

void SessionSwitcherWidget::restoreSettings()
{
	QStringList folders = appSettings->value("recentFolderList").toStringList();
	if (folders.empty())
		folderHistory->addItem(QDir::currentPath());
	else
		folderHistory->addItems(folders);
    folderHistory->setCurrentIndex(0);

    RenderingManager::getSessionSwitcher()->setTransitionColor( appSettings->value("transitionColor").value<QColor>());

    QString mediaFileName = appSettings->value("transitionMedia", "").toString();
    if (QFileInfo(mediaFileName).exists())
    	RenderingManager::getSessionSwitcher()->setTransitionMedia(mediaFileName);

    transitionSelection->setCurrentIndex(appSettings->value("transitionSelection", "0").toInt());
    transitionDuration->setValue(appSettings->value("transitionDuration", "1000").toInt());
    easingCurvePicker->setCurrentRow(appSettings->value("transitionCurve", "3").toInt());
    transitionTab->setCurrentIndex(appSettings->value("transitionTab", 0).toInt());

    connect(transitionSelection, SIGNAL(currentIndexChanged(int)), this, SLOT(saveSettings()));
    connect(transitionDuration, SIGNAL(valueChanged(int)), this, SLOT(saveSettings()));
    connect(easingCurvePicker, SIGNAL(currentRowChanged (int)), this, SLOT(saveSettings()));
    connect(transitionTab, SIGNAL(currentChanged(int)), this, SLOT(saveSettings()));
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



void SessionSwitcherWidget::setAllowedAspectRatio(const standardAspectRatio ar)
{
	allowedAspectRatio = ar;
	updateFolder();
}

void SessionSwitcherWidget::manualTransitionAdjustment(int t)
{
	// apply transition
	RenderingManager::getSessionSwitcher()->setAlpha( 1.f - float( ABS(t) ) / 100.f);

	// detect change of session
	if ( t == 0 ){

	}
	// detect end of transition
	if ( t > 100 ) {
		transitionSlider->setValue(-100);
	}
}
