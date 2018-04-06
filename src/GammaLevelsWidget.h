

#ifndef LEVELSDIALOG_H_
#define LEVELSDIALOG_H_

#include <QDialog>
#include <QBrush>
#include <QPen>
#include <QPixmap>

#include "SourceSet.h"

#include "ui_GammaLevelsDialog.h"

// convert gamma log into linear scale [0-1000]
#define GammaToScale(gamma) (int)(200.0*log( exp(1000.0/200.0)/24.0 * gamma + 0.8 ))
#define ScaleToGamma(val) 24.0*(exp((float)(val)/200.0) -0.8)/exp(1000.0/200.0)
#define DENSITY_POINTS_PLOT 6
#define CURVE_COLOR_RED 250, 50, 50
#define CURVE_COLOR_GREEN 50, 220, 50
#define CURVE_COLOR_BLUE 50, 50, 250

class GammaLevelsWidget : public QWidget, Ui::GammaLevelsWidget {

    Q_OBJECT

public:

    GammaLevelsWidget(QWidget *parent);

    void setGammaColor(double gamma, double red, double green, double blue);
    void setGammaLevels(double minInput, double maxInput, double minOutput, double maxOutput);

    void showEvent ( QShowEvent * event );
    void setAntialiasing(bool antialiased);

    void resetAll();

public slots:

    void connectSource(SourceSet::iterator);
    void updateColor();
    void updateLevels();

    void on_curveMode_currentIndexChanged(int c);
    void on_inSplit_splitterMoved(int pos, int index);
    void on_outSplit_splitterMoved(int pos, int index);
    void on_resetButton_clicked();
    void on_curveReset_clicked();
    void on_curvePreview_pressed();
    void on_curvePreview_released();

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

    typedef enum {
        CURVE_VALUE = 0,
        CURVE_RED,
        CURVE_GREEN,
        CURVE_BLUE
    } gammaCurve;

signals:
    void gammaChanged();

protected:
    void paintEvent(QPaintEvent *event);
    void mouseMoveEvent ( QMouseEvent * event );
    void wheelEvent ( QWheelEvent * event );

    gammaCurve activeCurve;

    QMap<gammaCurve, double> gamma;
    double xmin, xmax, ymin, ymax;

private:
    QMap<gammaCurve, QPair<QPen,QPen> > pens;
    QBrush brush;
    bool antialiased;
    QFont labelFont;

};



#endif //LEVELSDIALOG_H_
