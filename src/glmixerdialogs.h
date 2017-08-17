#ifndef GLMIXERDIALOGS_H
#define GLMIXERDIALOGS_H

#include <QtGui>
#include "Source.h"

QString getStringFromTime(double time);
double getTimeFromString(QString line);
void setupAboutDialog(QDialog *AboutGLMixer);

class CaptureDialog: public QDialog {

    QImage img;
    QComboBox *presetsSizeComboBox;

public:

    int getWidth();

    CaptureDialog(QWidget *parent, QImage capture, QString caption);

    QSize sizeHint() const {
        return QSize(400, 300);
    }
};


class RenderingSourceDialog: public QDialog {

    QToolButton *recursiveButton;

public:

    bool getRecursive();

    RenderingSourceDialog(QWidget *parent);

    QSize sizeHint() const {
        return QSize(400, 300);
    }
};


class SourceFileEditDialog: public QDialog {

    Source *s;
    class PropertyBrowser *specificSourcePropertyBrowser;
    class SourceDisplayWidget *sourcedisplay;
#ifdef GLM_FFGL
    class FFGLPluginBrowser *pluginBrowser;
#endif

public:
    SourceFileEditDialog(QWidget *parent, Source *source, QString caption);
    ~SourceFileEditDialog();
    QSize sizeHint() const {
        return QSize(400, 500);
    }
};



class TimeInputDialog: public QDialog {

    Q_OBJECT

public:

    double getTime() { return _t; }

    TimeInputDialog(QWidget *parent, double time, double min, double max, double fps, bool asframe);

    QSize sizeHint() const {
        return QSize(300, 100);
    }

public slots:

    void validateTimeInput(const QString &s);
    void validateFrameInput(const QString &s);

private:

    QPushButton *Ok;
    QLineEdit *Entry;
    QLabel *Info;
    double _t, _min, _max, _fps;
};


#ifdef GLM_LOGS

class LoggingWidget: public QWidget {

    Q_OBJECT

public:

    LoggingWidget(QWidget *parent = 0);

    QSize sizeHint() const {
        return QSize(600, 250);
    }
    virtual void closeEvent ( QCloseEvent * event );
    
    QByteArray saveState() const;
    bool restoreState(const QByteArray &state);    

signals:
    void saveLogs();
    void isVisible(bool);

public slots:

    void Log(int, QString);
    void on_copyLogsToClipboard_clicked();
    void on_saveLogsToFile_clicked();
    void on_openLogsFolder_clicked();
    void on_logTexts_doubleClicked();

private:

    void setupui();
    
    QVBoxLayout *logsVerticalLayout;
    QHBoxLayout *logsHorizontalLayout;
    QSpacerItem *logsHorizontalSpacer;
    QToolButton *saveLogsToFile;
    QToolButton *openLogsFolder;
    QToolButton *copyLogsToClipboard;
    QToolButton *toolButtonClearLogs;
    QTreeWidget *logTexts;

};
#endif  // GLM_LOGS


#endif // GLMIXERDIALOGS_H

