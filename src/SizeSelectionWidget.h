#ifndef SIZESELECTIONWIDGET_H
#define SIZESELECTIONWIDGET_H

#include <QWidget>

namespace Ui {
class SizeSelectionWidget;
}

class SizeSelectionWidget : public QWidget
{
    Q_OBJECT

public:
    explicit SizeSelectionWidget(QWidget *parent = 0);
    ~SizeSelectionWidget();

    int getWidth() const;
    int getHeight() const;
    
    void setPreset(int preset);
    int getPreset() const;

signals:
    void widthChanged(int);
    void heightChanged(int);
    void sizeChanged();

public slots:
    void setSizePreset(int preset);

private:
    Ui::SizeSelectionWidget *ui;
};

#endif // SIZESELECTIONWIDGET_H
