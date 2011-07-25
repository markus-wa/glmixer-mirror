/*
 * SessionSwitcherWidget.cpp
 *
 *  Created on: Oct 1, 2010
 *      Author: bh
 */

#include <QDomDocument>

#include "common.h"
#include "glmixer.h"
#include "SessionSwitcher.h"
#include "OutputRenderWindow.h"
#include "SourceDisplayWidget.h"

#include "SessionSwitcherWidget.moc"

class SearchingTreeView : public QTreeView
{
	QLineEdit *filter;

public:

	SearchingTreeView ( QWidget * parent = 0 ): QTreeView(parent) {
		filter = 0;
	}

	void keyPressEvent ( QKeyEvent * event ) {

		if (filter) {
			filter->hide();
			delete filter;
			filter = 0;
		}
		QSortFilterProxyModel *m = dynamic_cast<QSortFilterProxyModel *>(model());
		if (m) {
			if (event->key() == Qt::Key_Escape || event->key() == Qt::Key_Return)
				m->setFilterWildcard("");
			else if ( event->key() == Qt::Key_Space){
				filter = new QLineEdit(this);
				filter->show();
				filter->setFocus();
				QObject::connect(filter, SIGNAL(textChanged(const QString &)), parent(), SLOT(nameFilterChanged(const QString &)) );
//				filter->setText(event->text().simplified());
			}
		}
	}

	void leaveEvent ( QEvent * event ) {
		QSortFilterProxyModel *m = dynamic_cast<QSortFilterProxyModel *>(model());
		if (m)
			m->setFilterWildcard("");
		if (filter) {
			filter->hide();
			delete filter;
			filter = 0;
		}
		if (event)
			QTreeView::leaveEvent(event);
	}

};

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
																					appSettings(settings), m_iconSize(48,48), allowedAspectRatio(ASPECT_RATIO_FREE),
																					nextSessionSelected(false), suspended(false)
{
	QGridLayout *g;

	transitionSelection = new QComboBox;
	transitionSelection->addItem("Transition - Instantaneous");
	transitionSelection->addItem("Transition - Fade to black");
	transitionSelection->addItem("Transition - Fade to custom color   -->");
	transitionSelection->addItem("Transition - Fade with last frame");
	transitionSelection->addItem("Transition - Fade with media file   -->");
	transitionSelection->setToolTip("Select the transition type");
	transitionSelection->setCurrentIndex(-1);

	/**
	 * Tab automatic
	 */
	transitionTab = new QTabWidget(this);
	transitionTab->setToolTip("Choose how you control the transition");
	transitionTab->setSizePolicy(QSizePolicy(QSizePolicy::Preferred, QSizePolicy::Maximum));

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
    transitionTab->widget(0)->setLayout(g);

    /**
     * Tab manual
     */
    currentSessionLabel = new QLabel;
    currentSessionLabel->setText(tr("100% current"));
    currentSessionLabel->setSizePolicy( QSizePolicy::Preferred, QSizePolicy::Expanding);
    currentSessionLabel->setAlignment(Qt::AlignLeading|Qt::AlignLeft|Qt::AlignBottom);

    overlayPreview = new SourceDisplayWidget(this);
    overlayPreview->useAspectRatio(false);
    overlayPreview->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    overlayPreview->setMinimumSize(QSize(80, 60));

    nextSessionLabel = new QLabel;
    nextSessionLabel->setText(tr("No selection"));
    nextSessionLabel->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Expanding);
    nextSessionLabel->setAlignment(Qt::AlignBottom|Qt::AlignRight|Qt::AlignTrailing);
    nextSessionLabel->setStyleSheet("QLabel::disabled {\ncolor: rgb(128, 0, 0);\n}");

    transitionSlider = new QSlider;
    transitionSlider->setMinimum(-100);
    transitionSlider->setMaximum(101);
    transitionSlider->setValue(-100);
    transitionSlider->setOrientation(Qt::Horizontal);
    transitionSlider->setTickPosition(QSlider::TicksAbove);
    transitionSlider->setTickInterval(100);
    transitionSlider->setEnabled(false);

	transitionTab->addTab( new QWidget(), "Manual");
	g = new QGridLayout;
    g->addWidget(currentSessionLabel, 0, 0);
    g->addWidget(overlayPreview, 0, 1);
    g->addWidget(nextSessionLabel, 0, 2);
    g->addWidget(transitionSlider, 1, 0, 1, 3);
    transitionTab->widget(1)->setLayout(g);

    /**
     * Folder view
     */

    folderModel = new QStandardItemModel(0, 4, this);
    folderModel->setHeaderData(0, Qt::Horizontal, QObject::tr("Filename"));
    folderModel->setHeaderData(1, Qt::Horizontal, QObject::tr("n"));
    folderModel->setHeaderData(2, Qt::Horizontal, QObject::tr("W:H"));
    folderModel->setHeaderData(3, Qt::Horizontal, QObject::tr("Last modified"));

    proxyFolderModel = new QSortFilterProxyModel;
    proxyFolderModel->setDynamicSortFilter(true);
    proxyFolderModel->setFilterKeyColumn(0);
    proxyFolderModel->setSourceModel(folderModel);

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
	folderHistory->setValidator(new folderValidator(this));
	folderHistory->setInsertPolicy (QComboBox::InsertAtTop);
	folderHistory->setMaxCount(MAX_RECENT_FOLDERS);
	folderHistory->setMaximumWidth(250);
	folderHistory->setSizeAdjustPolicy(QComboBox::AdjustToContents);
	folderHistory->setDuplicatesEnabled(false);

    proxyView = new SearchingTreeView;
    proxyView->setRootIsDecorated(false);
    proxyView->setAlternatingRowColors(true);
    proxyView->setSortingEnabled(true);
    proxyView->sortByColumn(0, Qt::AscendingOrder);
    proxyView->setModel(proxyFolderModel);
    proxyView->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    proxyView->resizeColumnToContents(1);
    proxyView->resizeColumnToContents(2);
    proxyView->setSizePolicy(QSizePolicy(QSizePolicy::Preferred, QSizePolicy::Expanding));

    connect(dirButton, SIGNAL(clicked()),  this, SLOT(openFolder()));
    connect(dirDeleteButton, SIGNAL(clicked()),  this, SLOT(discardFolder()));
    connect(customButton, SIGNAL(clicked()),  this, SLOT(customizeTransition()));
    connect(folderHistory, SIGNAL(currentIndexChanged(const QString &)), this, SLOT(folderChanged(const QString &)));
    connect(transitionSelection, SIGNAL(currentIndexChanged(int)), this, SLOT(setTransitionType(int)));
    connect(transitionDuration, SIGNAL(valueChanged(int)), RenderingManager::getSessionSwitcher(), SLOT(setTransitionDuration(int)));
    connect(easingCurvePicker, SIGNAL(currentRowChanged (int)), RenderingManager::getSessionSwitcher(), SLOT(setTransitionCurve(int)));
    connect(transitionSlider, SIGNAL(valueChanged(int)), this, SLOT(transitionSliderChanged(int)));
    connect(transitionTab, SIGNAL(currentChanged(int)), this, SLOT(setTransitionMode(int)));

    QGridLayout *mainLayout = new QGridLayout;
    mainLayout->addWidget(transitionSelection, 0, 0, 1, 3);
    mainLayout->addWidget(customButton, 0, 3);
    mainLayout->addWidget(transitionTab, 1, 0, 1, 4);
    mainLayout->addWidget(proxyView, 2, 0, 1, 4);
    mainLayout->addWidget(folderHistory, 3, 0, 1, 2);
    mainLayout->addWidget(dirButton, 3, 2);
    mainLayout->addWidget(dirDeleteButton, 3, 3);
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

    folderHistory->updateGeometry ();
    fillFolderModel(folderModel, text, allowedAspectRatio);

    // setup transition according to new folder
    setAvailable();
}

void SessionSwitcherWidget::openFolder()
{
  QString dirName = QFileDialog::getExistingDirectory(this, tr("Select a directory"), QDir::currentPath(),
		  	  	  	  	  	  	  	  	  	  	  	  GLMixer::getInstance()->useSystemDialogs() ? QFileDialog::ShowDirsOnly : QFileDialog::ShowDirsOnly | QFileDialog::DontUseNativeDialog);
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

void SessionSwitcherWidget::startTransitionToSession(const QModelIndex & index)
{
	// transfer info to glmixer
	emit sessionTriggered(proxyFolderModel->data(index, Qt::UserRole).toString());

	// clear filter of proxyview
	proxyView->leaveEvent(0);

	// make sure no other events are accepted until the end of the transition
    disconnect(proxyView, SIGNAL(doubleClicked(QModelIndex)), this, SLOT(startTransitionToSession(QModelIndex) ));
    proxyView->setEnabled(false);
	QTimer::singleShot( transitionSelection->currentIndex() > 0 ? transitionDuration->value() : 100, this, SLOT(setAvailable()));
}

void SessionSwitcherWidget::startTransitionToNextSession()
{
	startTransitionToSession( proxyView->indexBelow (proxyView->currentIndex())	);
	proxyView->setCurrentIndex( proxyView->indexBelow (proxyView->currentIndex())	);
}
void SessionSwitcherWidget::startTransitionToPreviousSession()
{
	startTransitionToSession( proxyView->indexAbove (proxyView->currentIndex())	);
	proxyView->setCurrentIndex( proxyView->indexAbove (proxyView->currentIndex())	);
}

void SessionSwitcherWidget::selectSession(const QModelIndex & index)
{
	// read file name
	nextSession = proxyFolderModel->data(index, Qt::UserRole).toString();
    transitionSlider->setEnabled(true);
    currentSessionLabel->setEnabled(true);
    nextSessionLabel->setEnabled(true);
	// display that we can do transition to new selected session
    nextSessionLabel->setText(tr("0% %1").arg(QFileInfo(nextSession).baseName()));
}

void SessionSwitcherWidget::setTransitionType(int t)
{
	SessionSwitcher::transitionType tt = (SessionSwitcher::transitionType) CLAMP(SessionSwitcher::TRANSITION_NONE, t, SessionSwitcher::TRANSITION_CUSTOM_MEDIA);
	RenderingManager::getSessionSwitcher()->setTransitionType( tt );

	customButton->setStyleSheet("");
//	transitionTab->setEnabled(tt != SessionSwitcher::TRANSITION_NONE);
	transitionTab->setVisible(tt != SessionSwitcher::TRANSITION_NONE);
	// hack ; NONE transition type should emulate automatic transition mode
	setTransitionMode(tt == SessionSwitcher::TRANSITION_NONE ? 0 : transitionTab->currentIndex());

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

}


void SessionSwitcherWidget::customizeTransition()
{
	if (RenderingManager::getSessionSwitcher()->getTransitionType() == SessionSwitcher::TRANSITION_CUSTOM_COLOR ) {

		QColor color = QColorDialog::getColor(RenderingManager::getSessionSwitcher()->transitionColor(), parentWidget());
		if (color.isValid()) {
			RenderingManager::getSessionSwitcher()->setTransitionColor(color);
			setTransitionType( (int) SessionSwitcher::TRANSITION_CUSTOM_COLOR);
		}
	}
	else if (RenderingManager::getSessionSwitcher()->getTransitionType() == SessionSwitcher::TRANSITION_CUSTOM_MEDIA ) {

		QString oldfile = RenderingManager::getSessionSwitcher()->transitionMedia();
		QString newfile = QFileDialog::getOpenFileName(this, tr("Open File"), oldfile.isEmpty() ? QDir::currentPath() : oldfile,
											tr("Video (*.mov *.avi *.wmv *.mpeg *.mp4 *.mpg *.vob *.swf *.flv);;Image (*.png *.jpg *.jpeg *.tif *.tiff *.gif *.tga *.sgi *.bmp)"),
											0,  QFileDialog::DontUseNativeDialog);
		if ( QFileInfo(newfile).exists()) {
			RenderingManager::getSessionSwitcher()->setTransitionMedia(newfile);
			customButton->setStyleSheet("");
		} else {
			// not a valid file ; show a warning only if the QFileDialog did not return null (cancel)
			if (!newfile.isNull())
				qCritical() << newfile << tr("|File does not exist.");
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


void SessionSwitcherWidget::resetTransitionSlider()
{
	// enable / disable transition slider
    transitionSlider->setEnabled(nextSessionSelected);
    currentSessionLabel->setEnabled(nextSessionSelected);
    nextSessionLabel->setEnabled(nextSessionSelected);
	// enable / disable changing session
	proxyView->setEnabled(!nextSessionSelected);
	// clear the selection
	proxyView->clearSelection();

	// ensure correct re-display
	if (!nextSessionSelected) {
		RenderingManager::getSessionSwitcher()->setTransitionType(RenderingManager::getSessionSwitcher()->getTransitionType());
		nextSessionLabel->setText(tr("No selection"));
	}
}

void  SessionSwitcherWidget::setTransitionMode(int m)
{
	resetTransitionSlider();

	// mode is manual (and not with instantaneous transition selected)
	if ( m == 1  && transitionSelection->currentIndex() > 0) {
		RenderingManager::getSessionSwitcher()->manual_mode = true;
		// adjust slider to represent current transparency
		transitionSlider->setValue(RenderingManager::getSessionSwitcher()->overlay() * 100.f - (nextSessionSelected?0.f:100.f));
		// single clic to select next session
	    proxyView->setToolTip("Click on a session to choose target session");
	    disconnect(proxyView, SIGNAL(doubleClicked(QModelIndex)), this, SLOT(startTransitionToSession(QModelIndex) ));
	    connect(proxyView, SIGNAL(clicked(QModelIndex)), this, SLOT(selectSession(QModelIndex) ));
	}
	// mode is automatic
	else {
		RenderingManager::getSessionSwitcher()->manual_mode = false;
		// enable changing session
		proxyView->setEnabled(true);
		// double clic to activate transition to next session
	    proxyView->setToolTip("Double click on a session to initiate the transition");
	    connect(proxyView, SIGNAL(doubleClicked(QModelIndex)), this, SLOT(startTransitionToSession(QModelIndex) ));
	    disconnect(proxyView, SIGNAL(clicked(QModelIndex)), this, SLOT(selectSession(QModelIndex) ));
	}
}

void SessionSwitcherWidget::setAvailable()
{
	setTransitionMode(transitionTab->currentIndex());
}

void SessionSwitcherWidget::transitionSliderChanged(int t)
{
	// apply transition
	RenderingManager::getSessionSwitcher()->setTransparency( ABS(t) );

	if (suspended) {
		transitionSlider->setValue(0);
		return;
	}

	if ( nextSessionSelected ) {

		// display that we can do transition to new selected session
	    nextSessionLabel->setText(tr("%1% %2").arg(ABS(t)).arg(QFileInfo(nextSession).baseName()));

		// prevent coming back to previous
		if (t < 0) {
			transitionSlider->setValue(0);
		} else

		// detect end of transition
		if ( t > 100 ) {
			// reset
			nextSessionSelected = false;
			RenderingManager::getSessionSwitcher()->endTransition();
			// no target
			transitionSlider->setValue(-100);
			resetTransitionSlider();
		}


	} else {

		// detect change of session
		if ( t >= 0 ){
			nextSessionSelected = true;
			suspended = true;
			resetTransitionSlider();
			// request to load session file
			emit sessionTriggered(nextSession);

		} else if (t > -100 && RenderingManager::getSessionSwitcher()->getTransitionType() == SessionSwitcher::TRANSITION_CUSTOM_MEDIA) {
			overlayPreview->playSource(true);
		}
		// show percent of mixing
	    currentSessionLabel->setText(tr("%1% current").arg(ABS(t)));
	}
}


void SessionSwitcherWidget::setTransitionSourcePreview(Source *s)
{
	// set the overlay source
	overlayPreview->setSource(s);
}

void SessionSwitcherWidget::unsuspend()
{
	suspended = false;
}
