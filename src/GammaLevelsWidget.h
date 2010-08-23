

#ifndef LEVELSDIALOG_H_
#define LEVELSDIALOG_H_

#include <QDialog>
#include <QBrush>
#include <QPen>
#include <QPixmap>

#include "SourceSet.h"

#include "ui_GammaLevelsDialog.h"



#define NUM_POINTS_PLOT 40

class GammaLevelsWidget : public QWidget, Ui::GammaLevelsWidget {

    Q_OBJECT

public:

    GammaLevelsWidget(QWidget *parent);
    virtual ~GammaLevelsWidget();

    void setValues(float gamma, float minInput, float maxInput, float minOutput, float maxOutput);

    float minOutput();
    float maxOutput();
    float minInput();
    float maxInput();
    float gamma();

    void showEvent ( QShowEvent * event );

public Q_SLOTS:

    void connectSource(SourceSet::iterator);
    void on_gammaSlider_sliderMoved(int);
    void on_inSplit_splitterMoved ( int pos, int index );
    void on_outSplit_splitterMoved ( int pos, int index );
    void on_resetButton_clicked ();

private:

    class GammaPlotArea *plot;
    Source *source;
};


#endif //LEVELSDIALOG_H_
