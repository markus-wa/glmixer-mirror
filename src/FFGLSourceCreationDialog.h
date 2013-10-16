#ifndef FFGLSOURCECREATIONDIALOG_H
#define FFGLSOURCECREATIONDIALOG_H

#include <QDialog>
#include <QDomElement>
#include <QFileInfo>

namespace Ui {
class FFGLSourceCreationDialog;
}

class FFGLSourceCreationDialog : public QDialog
{
    Q_OBJECT
    
public:
    explicit FFGLSourceCreationDialog(QWidget *parent = 0);
    ~FFGLSourceCreationDialog();

    QFileInfo getFreeframePluginFileInfo();
    QDomElement getFreeframePluginConfiguration();
    int getSelectedWidth();
    int getSelectedHeight();

public Q_SLOTS:

    void done(int r);

    void browse();
    void updatePlugin(QString);
    void updateSourcePreview(QDomElement config = QDomElement());
    void selectSizePreset(int preset);

protected:
    void showEvent(QShowEvent *);

private:

    Ui::FFGLSourceCreationDialog *ui;
    class FFGLPluginBrowser *pluginBrowser;
    class FFGLPluginSourceStack *pluginBrowserStack;
    class FFGLSource *s;
    class SourceDisplayWidget *preview;

    QString _filename;
    QDomElement pluginConfiguration;
};

#endif // FFGLSOURCECREATIONDIALOG_H
