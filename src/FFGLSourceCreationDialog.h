#ifndef FFGLSOURCECREATIONDIALOG_H
#define FFGLSOURCECREATIONDIALOG_H

#include <QDialog>

namespace Ui {
class FFGLSourceCreationDialog;
}

class FFGLSourceCreationDialog : public QDialog
{
    Q_OBJECT
    
public:
    explicit FFGLSourceCreationDialog(QWidget *parent = 0);
    ~FFGLSourceCreationDialog();

    QString getFreeframeFileName();
    int getSelectedWidth();
    int getSelectedHeight();

public Q_SLOTS:

    void done(int r);

    void browse();
    void updatePlugin(QString);
    void updateSourcePreview();
    void selectSizePreset(int preset);

protected:
    void showEvent(QShowEvent *);

private:

    Ui::FFGLSourceCreationDialog *ui;
    class FFGLPluginBrowser *pluginBrowser;
    class FFGLSource *s;
    class SourceDisplayWidget *preview;

    QString _filename;

};

#endif // FFGLSOURCECREATIONDIALOG_H
