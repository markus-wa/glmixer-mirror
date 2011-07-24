

#ifndef LEVELSDIALOG_H_
#define LEVELSDIALOG_H_

#include <QDialog>
#include <QBrush>
#include <QPen>
#include <QPixmap>

#include "SourceSet.h"

#include "ui_GammaLevelsDialog.h"

// convert gamma log into linear scale [0-1000]
#define GammaToScale(gamma) (int)(200.f*log((exp(1000.f/200.f)-1.f)*(gamma/10.f)+1.f))
#define ScaleToGamma(val) (10.f*exp((float)(val)/200.f)-1.f)/(exp(1000.f/200.f)-1.f)
#define NUM_POINTS_PLOT 40

class GammaLevelsWidget : public QWidget, Ui::GammaLevelsWidget {

    Q_OBJECT

public:

    GammaLevelsWidget(QWidget *parent);

    void setValues(float gamma, float minInput, float maxInput, float minOutput, float maxOutput);

    float minOutput();
    float maxOutput();
    float minInput();
    float maxInput();
    float gamma();

    void showEvent ( QShowEvent * event );
    void setAntialiasing(bool antialiased);

public Q_SLOTS:

    void connectSource(SourceSet::iterator);
    void updateSource();

    void on_inSplit_splitterMoved ( int pos, int index );
    void on_outSplit_splitterMoved ( int pos, int index );
    void on_resetButton_clicked ();

private:
    class GammaPlotArea *plot;
    Source *source;
};


class GammaPlotArea : public QWidget
{

    Q_OBJECT
    friend class GammaLevelsWidget;

public:
    GammaPlotArea(QWidget *parent = 0);

    QSize minimumSizeHint() const;
    QSize sizeHint() const;

    void setPen(const QPen &pen);
    void setAntialiased(bool antialiased);

Q_SIGNALS:
	void gammaChanged();

protected:
    void paintEvent(QPaintEvent *event);
    void mouseMoveEvent ( QMouseEvent * event );
    void wheelEvent ( QWheelEvent * event );

    float gamma;
    float xmin, xmax, ymin, ymax;

private:
    QPoint points[NUM_POINTS_PLOT];
    QPen pen;
    QBrush brush;
    bool antialiased;

};



#endif //LEVELSDIALOG_H_
