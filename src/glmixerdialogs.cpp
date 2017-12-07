#include "glmixerdialogs.moc"

#include "glmixer.h"
#include "RenderingManager.h"
#include "SourcePropertyBrowser.h"
#include "SourceDisplayWidget.h"
#ifdef GLM_FFGL
#include "FFGLPluginBrowser.h"
#endif

int CaptureDialog::getWidth()
{
    return presetsSizeComboBox->itemData(presetsSizeComboBox->currentIndex()).toInt();
}

CaptureDialog::CaptureDialog(QWidget *parent, QImage capture, QString caption): QDialog(parent), img(capture) {

    QVBoxLayout *verticalLayout;
    QLabel *Question, *Display, *Info, *Property;
    QDialogButtonBox *DecisionButtonBox;

    setObjectName(QString::fromUtf8("CaptureDialog"));
    setWindowTitle(tr( "GLMixer - Pixmap"));
    verticalLayout = new QVBoxLayout(this);
    verticalLayout->setSpacing(9);

    Question = new QLabel(this);
    Question->setText(caption);
    verticalLayout->addWidget(Question);

    Display = new QLabel(this);
    Display->setPixmap(QPixmap::fromImage(img).scaledToWidth(400));
    verticalLayout->addWidget(Display);

    Info = new QLabel(this);
    Info->setText(tr("Original size: %1 x %2 px").arg(img.width()).arg(img.height()) );
    verticalLayout->addWidget(Info);


    QGroupBox *sizeGroupBox = new QGroupBox(this);
    sizeGroupBox->setTitle(tr("Pixel resolution"));
    sizeGroupBox->setFlat(true);
    verticalLayout->addWidget(sizeGroupBox);

    QHBoxLayout *horizontalLayout = new QHBoxLayout(sizeGroupBox);

    Property = new QLabel(sizeGroupBox);
    Property->setText(tr("Width x Height "));
    horizontalLayout->addWidget(Property);

    presetsSizeComboBox = new QComboBox(sizeGroupBox);
    int w = img.width();
    double ar = (double) img.height() / (double) img.width();
    presetsSizeComboBox->addItem(QString::fromUtf8("%1 x %2 (original)").arg(w).arg((int)((double) w * ar)));
    w = (int) ( (double) img.width() * 0.8 );
    presetsSizeComboBox->addItem(QString::fromUtf8("%1 x %2 (80%)").arg(w).arg((int)((double) w * ar)), QVariant(w));
    w = (int) ( (double) img.width() * 0.6 );
    presetsSizeComboBox->addItem(QString::fromUtf8("%1 x %2 (60%)").arg(w).arg((int)((double) w * ar)), QVariant(w));
    w = (int) ( (double) img.width() * 0.4 );
    presetsSizeComboBox->addItem(QString::fromUtf8("%1 x %2 (40%)").arg(w).arg((int)((double) w * ar)), QVariant(w));
    w = (int) ( (double) img.width() * 0.2 );
    presetsSizeComboBox->addItem(QString::fromUtf8("%1 x %2 (20%)").arg(w).arg((int)((double) w * ar)), QVariant(w));
    horizontalLayout->addWidget(presetsSizeComboBox);

    DecisionButtonBox = new QDialogButtonBox(this);
    DecisionButtonBox->setStandardButtons(QDialogButtonBox::Cancel|QDialogButtonBox::Ok);
    verticalLayout->addWidget(DecisionButtonBox);

    QObject::connect(DecisionButtonBox, SIGNAL(accepted()), this, SLOT(accept()));
    QObject::connect(DecisionButtonBox, SIGNAL(rejected()), this, SLOT(reject()));
}


bool RenderingSourceDialog::getRecursive()
{
    return recursiveButton->isChecked();
}

RenderingSourceDialog::RenderingSourceDialog(QWidget *parent): QDialog(parent)
{

    QVBoxLayout *verticalLayout, *vl;
    QLabel *Question, *Display, *Info;
    QDialogButtonBox *DecisionButtonBox;
    QHBoxLayout *horizontalLayout;
    QToolButton *nonrecursiveButton;

    setObjectName(QString::fromUtf8("RenderingSourceDialog"));
    setWindowTitle(tr( "GLMixer - New Loopback Source"));
    verticalLayout = new QVBoxLayout(this);
    verticalLayout->setSpacing(9);

    Question = new QLabel(this);
    Question->setText(tr("Select loop-back mode:"));
    verticalLayout->addWidget(Question);


    horizontalLayout = new QHBoxLayout();
    horizontalLayout->setObjectName(QString::fromUtf8("horizontalLayout"));
    horizontalLayout->setContentsMargins(0, 0, 0, 0);

    vl = new QVBoxLayout(this);
    Display = new QLabel(this);
    Display->setText(tr("Recursive frames"));
    recursiveButton = new QToolButton(this);
    recursiveButton->setObjectName(QString::fromUtf8("recursiveButton"));
    recursiveButton->setCheckable(true);
    recursiveButton->setChecked(true);
    recursiveButton->setAutoExclusive(true);
    QPixmap i(QString::fromUtf8(":/glmixer/images/loopback_recursive.png")) ;
    recursiveButton->setIcon( QIcon(i) );
    recursiveButton->setIconSize( i.size() );

    vl->addWidget(Display);
    vl->addWidget(recursiveButton);
    horizontalLayout->addLayout(vl);


    vl = new QVBoxLayout(this);
    Display = new QLabel(this);
    Display->setText(tr("One frame"));

    nonrecursiveButton = new QToolButton(this);
    nonrecursiveButton->setObjectName(QString::fromUtf8("nonrecursiveButton"));
    nonrecursiveButton->setCheckable(true);
    nonrecursiveButton->setAutoExclusive(true);
    i = QPixmap(QString::fromUtf8(":/glmixer/images/loopback_non_recursive.png")) ;
    nonrecursiveButton->setIcon( QIcon(i) );
    nonrecursiveButton->setIconSize( i.size() );

    vl->addWidget(Display);
    vl->addWidget(nonrecursiveButton);
    horizontalLayout->addLayout(vl);

    verticalLayout->addLayout(horizontalLayout);

    Info = new QLabel(this);
    Info->setStyleSheet("font: italic 9pt");
    if (RenderingManager::getInstance()->useFboBlitExtension())
        Info->setText(tr("Rendering mode optimal (Frame Buffer Blit enabled)."));
    else
        Info->setText(tr("Rendering mode not optimal (Frame Buffer Blit disabled).") );
    verticalLayout->addWidget(Info);

    DecisionButtonBox = new QDialogButtonBox(this);
    DecisionButtonBox->setStandardButtons(QDialogButtonBox::Cancel|QDialogButtonBox::Ok);
    verticalLayout->addWidget(DecisionButtonBox);

    QObject::connect(DecisionButtonBox, SIGNAL(accepted()), this, SLOT(accept()));
    QObject::connect(DecisionButtonBox, SIGNAL(rejected()), this, SLOT(reject()));
}



SourceFileEditDialog::SourceFileEditDialog(QWidget *parent, Source *source, QString caption): QDialog(parent), s(source) {

    QVBoxLayout *verticalLayout;
    QDialogButtonBox *DecisionButtonBox;
    QTabWidget *tabs;

    setObjectName(QString::fromUtf8("SourceEditDialog"));
    setWindowTitle(caption);
    setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::MinimumExpanding);

    verticalLayout = new QVBoxLayout(this);
    verticalLayout->setSpacing(9);

    sourcedisplay = new SourceDisplayWidget(this, SourceDisplayWidget::GRID);
    sourcedisplay->setMinimumSize(QSize(160, 100));
    sourcedisplay->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Expanding);
    sourcedisplay->setSource(s);
    verticalLayout->addWidget(sourcedisplay);

    specificSourcePropertyBrowser = SourcePropertyBrowser::createSpecificPropertyBrowser(s, this);
    specificSourcePropertyBrowser->setDisplayPropertyTree(false);
    specificSourcePropertyBrowser->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Maximum);

#ifdef GLM_FFGL
    if (s->getFreeframeGLPluginStack()->empty()) {
        pluginBrowser = 0;
        verticalLayout->addWidget(specificSourcePropertyBrowser);
    }
    else {
        // show effects if there are plugins
        sourcedisplay->setEffectsEnabled(true);
        // create plugin browser
        pluginBrowser = new FFGLPluginBrowser(this);
        pluginBrowser->setDisplayPropertyTree(false);
        pluginBrowser->showProperties( s->getFreeframeGLPluginStack() );
        // show two tabs
        tabs = new QTabWidget(this);
        tabs->addTab(new QWidget(), "Source");
        tabs->addTab(new QWidget(), "GPU Plugins");
        verticalLayout->addWidget(tabs);
        QVBoxLayout *tabLayout;
        tabLayout = new QVBoxLayout(tabs);
        tabLayout->addWidget(specificSourcePropertyBrowser);
        tabs->widget(0)->setLayout(tabLayout);
        tabLayout = new QVBoxLayout(tabs);
        tabLayout->addWidget(pluginBrowser);
        tabs->widget(1)->setLayout(tabLayout);

        // enable the button for openning the shadertoy code
        QObject::connect(pluginBrowser, SIGNAL(edit(FFGLPluginSource *)), this, SLOT(accept()));
        QObject::connect(pluginBrowser, SIGNAL(edit(FFGLPluginSource *)), GLMixer::getInstance(), SLOT(editShaderToyPlugin(FFGLPluginSource *)) );
    }
#else
    verticalLayout->addWidget(specificSourcePropertyBrowser);
#endif

    DecisionButtonBox = new QDialogButtonBox(this);
    DecisionButtonBox->addButton("Done", QDialogButtonBox::AcceptRole);
    if (source->rtti() != Source::CLONE_SOURCE)
        DecisionButtonBox->addButton("Re-create", QDialogButtonBox::RejectRole);
    verticalLayout->addWidget(DecisionButtonBox);

    QObject::connect(DecisionButtonBox, SIGNAL(accepted()), this, SLOT(accept()));
    QObject::connect(DecisionButtonBox, SIGNAL(rejected()), this, SLOT(reject()));
    QObject::connect(DecisionButtonBox, SIGNAL(rejected()), parent, SLOT(replaceCurrentSource()));
}

SourceFileEditDialog::~SourceFileEditDialog() {
    delete specificSourcePropertyBrowser;
    delete sourcedisplay;
#ifdef GLM_FFGL
    if (pluginBrowser)
        delete pluginBrowser;
#endif
}


void setupAboutDialog(QDialog *AboutGLMixer)
{
    QPixmap pixmap(":/glmixer/images/glmixer_splash.png");
    QSplashScreen *splash = new QSplashScreen(pixmap);
    AboutGLMixer->resize(420, 320);
    AboutGLMixer->setWindowTitle("About GLMixer");
    QGridLayout *gridLayout = new QGridLayout(AboutGLMixer);
    gridLayout->setObjectName(QString::fromUtf8("gridLayout"));
    QToolButton *Icon = new QToolButton(AboutGLMixer);
    Icon->setIcon(QPixmap(QString::fromUtf8(":/glmixer/icons/glmixer_256x256.png")));
    Icon->setStyleSheet("QToolButton { border: 0px; icon-size: 128px; }");
    QObject::connect(Icon, SIGNAL(pressed()), splash, SLOT(show()));
    QObject::connect(Icon, SIGNAL(released()), splash, SLOT(hide()));
    QLabel *Title = new QLabel(AboutGLMixer);
    Title->setStyleSheet(QString::fromUtf8("font: 14pt \"Sans Serif\";"));
    QLabel *VERSION = new QLabel(AboutGLMixer);
    VERSION->setStyleSheet(QString::fromUtf8("font: 14pt \"Sans Serif\";"));
    QLabel *textsvn = new QLabel(AboutGLMixer);
    QLabel *SVN = new QLabel(AboutGLMixer);
    QTextBrowser *textBrowser = new QTextBrowser(AboutGLMixer);
    textBrowser->setOpenExternalLinks (true);
    QDialogButtonBox *validate = new QDialogButtonBox(AboutGLMixer);
    validate->setOrientation(Qt::Horizontal);
    validate->setStandardButtons(QDialogButtonBox::Close);

    gridLayout->addWidget(Icon, 0, 0, 1, 1);
    gridLayout->addWidget(Title, 0, 1, 1, 1);
    gridLayout->addWidget(VERSION, 0, 2, 1, 1);
    gridLayout->addWidget(textsvn, 1, 1, 1, 1);
    gridLayout->addWidget(SVN, 1, 2, 1, 1);
    gridLayout->addWidget(textBrowser, 2, 0, 1, 3);
    gridLayout->addWidget(validate, 3, 0, 1, 3);

    Icon->setText(QString());
    Title->setText(QObject::tr("Graphic Live Mixer"));
    textBrowser->setHtml(QObject::tr("<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 4.0//EN\" \"http://www.w3.org/TR/REC-html40/strict.dtd\">\n"
    "<html><head><meta name=\"qrichtext\" content=\"1\" /><style type=\"text/css\">\n"
    "p, li { white-space: pre-wrap; }\n"
    "</style></head><body style=\" font-family:'Sans Serif'; font-size:10pt; font-weight:400; font-style:normal;\">\n"
    "<p>GLMixer is a video mixing software for live performance.</p>\n"
    "<p>Author:	Bruno Herbelin<br>\n"
    "Contact:	bruno.herbelin@gmail.com<br>\n"
    "License: 	GNU GPL version 3</p>\n"
    "<p>Copyright 2009-%1 Bruno Herbelin</p>\n"
    "<p>Updates and source code at: <br>\n"
    "   	<a href=\"http://sourceforge.net/projects/glmixer//\"><span style=\" text-decoration: underline; color:#7d400a;\">http://sourceforge.net/projects/glmixer/</span>"
    "</a></p>"
    "<p>GLMixer is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation.</p>"
    "<p>GLMixer is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more details (see http://www.gnu.org/licenses).</p>"
    "</body></html>").arg(COMPILE_YEAR));

    VERSION->setText( QString("%1").arg(QCoreApplication::applicationVersion()) );

#ifdef GLMIXER_REVISION
    SVN->setText(QString("%1").arg(GLMIXER_REVISION));
    textsvn->setText(QObject::tr("SVN repository revision:"));
#endif

    QObject::connect(validate, SIGNAL(rejected()), AboutGLMixer, SLOT(reject()));

}


QString getStringFromTime(double time)
{
    int s = (int) time;
    time -= s;
    int h = s / 3600;
    int m = (s % 3600) / 60;
    s = (s % 3600) % 60;
    int ds = (int) qRound(time * 100.0);
    return QString("%1h %2m %3.%4s").arg(h, 2).arg(m, 2, 10, QChar('0')).arg(s, 2, 10, QChar('0')).arg(ds, 2, 10, QChar('0'));
}

double getTimeFromString(QString line)
{
    bool ok = false;
    double h, m, s;

    QStringList parts = line.split('h', QString::SkipEmptyParts);
    if (parts.size()!=2)    return -1;
    h = (double) parts[0].toInt(&ok);
    if (!ok) return -1;
    parts = parts[1].split('m', QString::SkipEmptyParts);
    if (parts.size()!=2)    return -1;
    m = (double) parts[0].toInt(&ok);
    if (!ok) return -1;
    parts = parts[1].split('s', QString::SkipEmptyParts);
    s = parts[0].toDouble(&ok);
    if (!ok) return -1;

    return (h * 3600.0) + (m * 60.0) + s;
}

class timeValidator : public QValidator
{
  public:
    timeValidator(QObject *parent) : QValidator(parent) { }

    QValidator::State validate ( QString & input, int & pos ) const {
      if( input.isEmpty() )
          return QValidator::Invalid;
      double t = getTimeFromString(input);
      if( t < 0 )
          return QValidator::Intermediate;
      return QValidator::Acceptable;
    }
};


TimeInputDialog::TimeInputDialog(QWidget *parent, double time, double min, double max, double fps, bool asframe): QDialog(parent), _t(time), _min(min), _max(max), _fps(fps)
{

        QVBoxLayout *verticalLayout;
        QLabel *Question, *Guide, *Property;
        QDialogButtonBox *DecisionButtonBox;

        setObjectName(QString::fromUtf8("TimeInputDialog"));
        setWindowTitle(tr( "GLMixer - Seek to time in video"));
        setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::MinimumExpanding);

        verticalLayout = new QVBoxLayout(this);
        verticalLayout->setSpacing(9);

        Question = new QLabel(this);
        verticalLayout->addWidget(Question);
        Guide = new QLabel(this);
        Guide->setStyleSheet("font-size: 9pt");
        verticalLayout->addWidget(Guide);
        Entry = new QLineEdit(this);
        verticalLayout->addWidget(Entry);
        Info = new QLabel(this);
        verticalLayout->addWidget(Info);
        DecisionButtonBox = new QDialogButtonBox(this);
        DecisionButtonBox->setStandardButtons(QDialogButtonBox::Cancel);
        Ok = DecisionButtonBox->addButton("Go", QDialogButtonBox::AcceptRole);
        verticalLayout->addWidget(DecisionButtonBox);

        QObject::connect(DecisionButtonBox, SIGNAL(accepted()), this, SLOT(accept()));
        QObject::connect(DecisionButtonBox, SIGNAL(rejected()), this, SLOT(reject()));

        if (asframe) {
            Question->setText(tr("Please enter target frame"));
            Guide->setText(tr("Values must be between %1 and %2").arg((int)(_min*_fps)).arg((int)(_max*_fps)));
            Entry->setText( QString::number((int)(_t*_fps)) );
            QValidator *validator = new QIntValidator((int)(_min*_fps), (int)(_max*_fps), this);
            Entry->setValidator(validator);
            QObject::connect(Entry, SIGNAL(textChanged(const QString &)), this, SLOT(validateFrameInput(const QString &)));
        }
        else {
            Question->setText(tr("Please enter target time"));
            Guide->setText(tr("Format must be '__h __m __s'\nValues must be between %1 and %2").arg(getStringFromTime(_min)).arg(getStringFromTime(_max)));
            Entry->setText( getStringFromTime(_t) );
            timeValidator *validator = new timeValidator(this);
            Entry->setValidator(validator);
            QObject::connect(Entry, SIGNAL(textChanged(const QString &)), this, SLOT(validateTimeInput(const QString &)));
        }

}

void TimeInputDialog::validateTimeInput(const QString &s)
{
    Ok->setEnabled(false);

    if( Entry->hasAcceptableInput()) {
        _t = getTimeFromString(s);
        Entry->setStyleSheet("");

        if ( _t > _max ) {
            Info->setText( tr("Target %1 is passed the maximum").arg(_t));
        }
        else if ( _t < _min ) {

            Info->setText( tr("Target %1 is before the minimum").arg(_t));
        }
        else {
            Info->setText( tr("Ready to jump to %1").arg(_t));
            Ok->setEnabled(true);
        }
    }
    else {
        Entry->setStyleSheet("color: rgb(155, 0, 5)");
        Info->setText("Invalid entry");
    }

}

void TimeInputDialog::validateFrameInput(const QString &s)
{
    Ok->setEnabled(false);

    bool ok = false;
    int f = s.toInt(&ok);

    if( ok && Entry->hasAcceptableInput()) {

        _t = (double) f / _fps;

        Entry->setStyleSheet("");
        Info->setText( tr("Ready to jump to %1").arg(_t));
        Ok->setEnabled(true);
    }
    else {
        Entry->setStyleSheet("color: rgb(155, 0, 5)");
        Info->setText("Invalid entry");
    }

}




#ifdef GLM_LOGS

void LoggingWidget::on_openLogsFolder_clicked() {

    QDesktopServices::openUrl( QUrl::fromLocalFile(QDir::tempPath()) );
}

void LoggingWidget::on_copyLogsToClipboard_clicked() {

    if (logTexts->topLevelItemCount() > 0) {
        QString logs;
        QTreeWidgetItemIterator it(logTexts->topLevelItem(0));
        while (*it) {
            logs.append( QString("%2: %1\n").arg((*it)->text(0)).arg((*it)->text(1)) );
            ++it;
        }
        QApplication::clipboard()->setText(logs);
    }
}

void LoggingWidget::on_saveLogsToFile_clicked() {

    emit saveLogs();
}


void LoggingWidget::on_logTexts_doubleClicked() {

    QString origin = logTexts->currentItem()->text(1);
    QFileInfo file(origin);
    if (file.exists())
        QDesktopServices::openUrl( QUrl::fromLocalFile(file.absolutePath()) );
}


void LoggingWidget::closeEvent ( QCloseEvent * event ) {

    emit isVisible(false);
    QWidget::closeEvent(event);
}

void LoggingWidget::Log(int type, QString msg)
{
    // create log entry
    QTreeWidgetItem *item  = new QTreeWidgetItem();
    logTexts->addTopLevelItem( item );

    // reads the text passed and split into object|message
    QStringList message = msg.split(QChar(124), QString::SkipEmptyParts);
    if (message.count() > 1 ) {
        message[0] = message[0].simplified();
        message[1] = message[1].simplified();
        item->setText(0, message[1]);
        if (message[1].endsWith("!"))
            item->setIcon(0, QIcon(":/glmixer/icons/info.png"));
        item->setText(1, message[0]);
        if (QFileInfo(message[0]).exists())
            item->setToolTip(1, tr("Double clic to open folder."));
    } else if (message.count() > 0 ) {
        message[0] = message[0].simplified();
        item->setText(0, message[0]);
        item->setText(1, QApplication::applicationName());
    } else {
        item->setText(0, msg);
        item->setIcon(0, QIcon(":/glmixer/icons/info.png"));
        item->setText(1, "");
    }
    // adjust color and show dialog according to message type
    switch ( (QtMsgType) type) {
    case QtWarningMsg:
         item->setBackgroundColor(0, QColor(50, 180, 220, 50));
         item->setBackgroundColor(1, QColor(50, 180, 220, 50));
         item->setIcon(0, QIcon(":/glmixer/icons/info.png"));
         break;
    case QtCriticalMsg:
        item->setBackgroundColor(0, QColor(220, 90, 50, 50));
        item->setBackgroundColor(1, QColor(220, 90, 50, 50));
        item->setIcon(0, QIcon(":/glmixer/icons/warning.png"));
        break;
    default:
        break;
    }
    // auto scroll to new item
    logTexts->setCurrentItem( item );
}

LoggingWidget::LoggingWidget(QWidget *parent) : QWidget(parent) {

    // Window title
    setObjectName(QString::fromUtf8("LoggingWindow"));
    QIcon icon;
    icon.addFile(QString::fromUtf8(":/glmixer/icons/glmixer.png"), QSize(), QIcon::Normal, QIcon::Off);
    setWindowIcon(icon);
    setWindowTitle(tr("GLMixer - Logs"));
    // style
    QFile file(":/style/default");
    file.open(QFile::ReadOnly);
    QString styleSheet = QLatin1String(file.readAll());
    setStyleSheet(styleSheet);
    // window content
    setupui();
    // signals & slots
    QMetaObject::connectSlotsByName(this);
    QObject::connect(toolButtonClearLogs, SIGNAL(clicked()), logTexts, SLOT(clear()));

}

void LoggingWidget::setupui() {

    logsVerticalLayout = new QVBoxLayout(this);
    logsVerticalLayout->setSpacing(3);
    logsVerticalLayout->setContentsMargins(6, 6, 6, 6);
    logsVerticalLayout->setObjectName(QString::fromUtf8("logsVerticalLayout"));
    logsHorizontalLayout = new QHBoxLayout();
    logsHorizontalLayout->setSpacing(3);
    logsHorizontalLayout->setObjectName(QString::fromUtf8("logsHorizontalLayout"));
    logsHorizontalSpacer = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

    logsHorizontalLayout->addItem(logsHorizontalSpacer);

    openLogsFolder = new QToolButton(this);
    openLogsFolder->setObjectName(QString::fromUtf8("openLogsFolder"));
    openLogsFolder->setToolTip(tr("Show location of log file in the system."));
    QIcon icon104;
    icon104.addFile(QString::fromUtf8(":/glmixer/icons/textopen.png"), QSize(), QIcon::Normal, QIcon::Off);
    openLogsFolder->setIcon(icon104);
    openLogsFolder->setIconSize(QSize(24, 24));

    logsHorizontalLayout->addWidget(openLogsFolder);

    saveLogsToFile = new QToolButton(this);
    saveLogsToFile->setObjectName(QString::fromUtf8("saveLogsToFile"));
    saveLogsToFile->setToolTip(tr("Save this log to text file."));
    QIcon icon103;
    icon103.addFile(QString::fromUtf8(":/glmixer/icons/textsave.png"), QSize(), QIcon::Normal, QIcon::Off);
    saveLogsToFile->setIcon(icon103);
    saveLogsToFile->setIconSize(QSize(24, 24));

    logsHorizontalLayout->addWidget(saveLogsToFile);

    copyLogsToClipboard = new QToolButton(this);
    copyLogsToClipboard->setObjectName(QString::fromUtf8("copyLogsToClipboard"));
    copyLogsToClipboard->setToolTip(tr("Copy log to clipboard."));
    QIcon icon105;
    icon105.addFile(QString::fromUtf8(":/glmixer/icons/textcopy.png"), QSize(), QIcon::Normal, QIcon::Off);
    copyLogsToClipboard->setIcon(icon105);
    copyLogsToClipboard->setIconSize(QSize(24, 24));

    logsHorizontalLayout->addWidget(copyLogsToClipboard);

    toolButtonClearLogs = new QToolButton(this);
    toolButtonClearLogs->setObjectName(QString::fromUtf8("toolButtonClearLogs"));
    toolButtonClearLogs->setToolTip(tr("Clear logs."));
    QIcon icon90;
    icon90.addFile(QString::fromUtf8(":/glmixer/icons/clean.png"), QSize(), QIcon::Normal, QIcon::Off);
    toolButtonClearLogs->setIcon(icon90);
    toolButtonClearLogs->setIconSize(QSize(24, 24));

    logsHorizontalLayout->addWidget(toolButtonClearLogs);

    logsVerticalLayout->addLayout(logsHorizontalLayout);

    logTexts = new QTreeWidget(this);
    logTexts->setObjectName(QString::fromUtf8("logTexts"));
    logTexts->setEditTriggers(QAbstractItemView::NoEditTriggers);
    logTexts->setProperty("showDropIndicator", QVariant(false));
    logTexts->setAlternatingRowColors(true);
    logTexts->setSelectionMode(QAbstractItemView::NoSelection);
    logTexts->setTextElideMode(Qt::ElideMiddle);
    logTexts->setRootIsDecorated(false);
    logTexts->setUniformRowHeights(true);
    logTexts->setItemsExpandable(false);
    logTexts->setWordWrap(true);
    logTexts->setHeaderHidden(false);
    logTexts->setExpandsOnDoubleClick(false);
    logTexts->header()->setVisible(true);
    // Use fixed size font
#ifdef Q_OS_WIN32
    logTexts->setFont(QFont(getMonospaceFont(), QApplication::font().pointSize() + 1));
#else
    logTexts->setFont(QFont(getMonospaceFont(), QApplication::font().pointSize() - 1));
#endif

    logsVerticalLayout->addWidget(logTexts);

    QTreeWidgetItem *___qtreewidgetitem = logTexts->headerItem();
    ___qtreewidgetitem->setText(1, tr("Origin"));
    ___qtreewidgetitem->setText(0, tr("Message"));
}


QByteArray LoggingWidget::saveState() const {
    QByteArray ba;
    QDataStream stream(&ba, QIODevice::WriteOnly);

    // get settings
    stream << saveGeometry();
    stream << logTexts->columnWidth(0);

    return ba;
}


bool LoggingWidget::restoreState(const QByteArray &state) {

    QByteArray sd = state;
    QDataStream stream(&sd, QIODevice::ReadOnly);

    // restore settings
    QByteArray geom;
    stream >> geom;
    restoreGeometry(geom);
    int w = 300;
    stream >> w;
    logTexts->setColumnWidth(0, w);

    return true;
}

#endif
