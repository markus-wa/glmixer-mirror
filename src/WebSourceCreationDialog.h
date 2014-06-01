#ifndef WEBSOURCECREATIONDIALOG_H
#define WEBSOURCECREATIONDIALOG_H

#include <QDialog>
#include <QUrl>
#include <QRegExpValidator>
#include <QSettings>

namespace Ui {
class WebSourceCreationDialog;
}

class WebSourceCreationDialog : public QDialog
{
    Q_OBJECT

public:
    explicit WebSourceCreationDialog(QWidget *parent = 0, QSettings *settings = 0);
    ~WebSourceCreationDialog();

    QUrl getSelectedUrl();
    int getSelectedHeight();
    int getSelectedScroll();

public Q_SLOTS:

    void done(int r);

    void updateSourcePreview();
    void browseHtmlPage();
    void deleteHtmlPage();
    void validateWebUrl(QString);
    void setHeight(int);
    void setScroll(int);

protected:

    void showEvent(QShowEvent *);

private:
    Ui::WebSourceCreationDialog *ui;
    QRegExpValidator urlValidator;
    QUrl url;

    class WebSource *s;

    QSettings *appSettings;
};

#endif // WEBSOURCECREATIONDIALOG_H
