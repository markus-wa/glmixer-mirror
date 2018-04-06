
#include <QtGui>
#include <cmath>

#include "RenderingManager.h"
#include "GammaLevelsWidget.moc"

#define GammaCorrection(color, gamma) pow(color, 1.0 / gamma)
#define LevelsControlInputRange(color, minInput, maxInput)  qMin( qMax(color - minInput, 0.0) / (maxInput - minInput), 1.0)
#define LevelsControlInput(color, minInput, gamma, maxInput) GammaCorrection(LevelsControlInputRange(color, minInput, maxInput), gamma)
#define LevelsControlOutputRange(color, minOutput, maxOutput) ( minOutput*(1.0-color) + maxOutput*color  )
#define LevelsControl(color, minInput, gamma, maxInput, minOutput, maxOutput)   LevelsControlOutputRange(LevelsControlInput(color, minInput, gamma, maxInput), minOutput, maxOutput)



GammaLevelsWidget::GammaLevelsWidget(QWidget *parent) : QWidget(parent), source(0)
{
    setupUi(this);

    // replace dummy widget by plot
    gridLayoutGraphic->removeWidget ( plotWidget );
    delete plotWidget;
    plot = new GammaPlotArea(this);
    Q_CHECK_PTR(plot);
    plotWidget = (QWidget *) plot;
    gridLayoutGraphic->addWidget(plotWidget, 0, 1, 1, 1);
    QObject::connect(plot, SIGNAL(gammaChanged()), this, SLOT(updateColor()) );

    // setup plot
    plot->setPen(QPen( palette().color(QPalette::Mid) ));
    plot->setAntialiased(true);
    plot->setFont(QFont(getMonospaceFont(), 12, QFont::Bold));

}

void GammaLevelsWidget::setAntialiasing(bool antialiased)
{
    plot->setAntialiased(antialiased);
}

void GammaLevelsWidget::connectSource(SourceSet::iterator csi){

    if ( RenderingManager::getInstance()->isValid(csi) ) {
        setEnabled(true);
        source = *csi;
        setGammaColor(source->getGamma(), source->getGammaRed(), source->getGammaGreen(), source->getGammaBlue());
        setGammaLevels(source->getGammaMinInput(), source->getGammaMaxInput(), source->getGammaMinOuput(), source->getGammaMaxOutput());

    } else {
        setEnabled(false);
        source = 0;
        setGammaColor(1.0, 1.0, 1.0, 1.0);
        setGammaLevels(0.0, 1.0, 0.0, 1.0);
    }
}

void GammaLevelsWidget::on_curveMode_currentIndexChanged(int c)
{
    plot->activeCurve = (GammaPlotArea::gammaCurve) CLAMP(c, 0, 3);

    QString color;
    switch(c) {
    case 3:
        color = "0,0,255";
        break;
    case 2:
        color = "0,255,0";
        break;
    case 1:
        color = "255,0,0";
        break;
    case 0:
    default:
        color = "255,255,255";
        break;
    }

    verticalGradient->setStyleSheet(QString("QWidget{  background-color: qlineargradient(spread:pad, x1:0, y1:0, x2:0, y2:1, stop:0 rgba(%1, 255), stop:1 rgba(0, 0, 0, 255));} QWidget:disabled { background-color: palette(button);}; border-width: 1px; border-style: solid; border-color: black;").arg(color));

    plot->update();
}

void GammaLevelsWidget::setGammaColor(double gamma, double red, double green, double blue){

    plot->gamma[GammaPlotArea::CURVE_VALUE] = gamma;
    plot->gamma[GammaPlotArea::CURVE_RED] = red;
    plot->gamma[GammaPlotArea::CURVE_GREEN] = green;
    plot->gamma[GammaPlotArea::CURVE_BLUE] = blue;

    updateColor();
}

void GammaLevelsWidget::setGammaLevels(double minInput, double maxInput, double minOutput, double maxOutput){

    // setup values, with some sanity check
    plot->xmin = qBound(minInput, 0.0, 1.0);
    plot->xmax = qBound(maxInput, 0.0, 1.0);
    plot->xmin = qMin(plot->xmax, plot->xmin);
    plot->ymin = qBound(minOutput, 0.0, 1.0);
    plot->ymax = qBound(maxOutput, 0.0, 1.0);
    plot->ymin = qMin(plot->ymax, plot->ymin);

    QList<int> s = outSplit->sizes();
    int total = s[0] + s[1] + s[2];
    s[2] = plot->ymin * total;
    s[0] = total - total * plot->ymax;
    s[1] = total - s[0] - s[2];
    outSplit->setSizes(s);

    s = inSplit->sizes();
    total = s[0] + s[1] + s[2];
    s[0] = plot->xmin * total;
    s[2] = total - total * plot->xmax;
    s[1] = total - s[0] - s[2];
    inSplit->setSizes(s);

    updateLevels();
}

void GammaLevelsWidget::showEvent ( QShowEvent * event ){

    QWidget::showEvent(event);

    QList<int> s = outSplit->sizes();
    int total = s[0] + s[1] + s[2];
    s[2] = plot->ymin * total;
    s[0] = total - total * plot->ymax;
    s[1] = total - s[0] - s[2];
    outSplit->setSizes(s);

    s = inSplit->sizes();
    total = s[0] + s[1] + s[2];
    s[0] = plot->xmin * total;
    s[2] = total - total * plot->xmax;
    s[1] = total - s[0] - s[2];
    inSplit->setSizes(s);

    plot->update();
}


void GammaLevelsWidget::resetAll() {

    setGammaColor(1.0, 1.0, 1.0, 1.0);
    setGammaLevels(0.0, 1.0, 0.0, 1.0);
}

void GammaLevelsWidget::on_resetButton_clicked (){

    setGammaLevels(0.0, 1.0, 0.0, 1.0);
}

void GammaLevelsWidget::on_curveReset_clicked() {

    plot->gamma[plot->activeCurve] = 1.0;
    updateColor();
}


void GammaLevelsWidget::on_curvePreview_pressed()
{
    if (source) {
        source->_setGammaColor(1.0, 1.0, 1.0, 1.0);
        source->_setGammaLevels(0.0, 1.0, 0.0, 1.0);
    }
}

void GammaLevelsWidget::on_curvePreview_released()
{
    updateColor();
    updateLevels();
}

void GammaLevelsWidget::on_inSplit_splitterMoved ( int pos, int index )
{
    int min = 0, max = 0;
    inSplit->getRange ( index, &min, &max );

    if (index == 1){
        plot->xmin = (double)(pos-min) / (double)(max-min);
        plot->xmax = qMax( plot->xmax, (plot->xmin + 0.01f));
    } else {
        plot->xmax = (double)(pos-min) / (double)(max-min);
        plot->xmin = qMin( (plot->xmax - 0.01f), plot->xmin);
    }

    updateLevels();
}

void GammaLevelsWidget::on_outSplit_splitterMoved ( int pos, int index )
{
    int min = 0, max = 0;
    outSplit->getRange ( index, &min, &max );

    if (index == 2) {
        plot->ymin = 1.0 - (double)(pos-min) / (double)(max-min);
        plot->ymax = qMax(plot->ymax, plot->ymin);
    } else {
        plot->ymax = 1.0 - (double)(pos-min) / (double)(max-min);
        plot->ymin = qMin(plot->ymax, plot->ymin);
    }

    updateLevels();
}

void GammaLevelsWidget::updateColor()
{
    plot->update();

    if (source)
        source->setGammaColor(plot->gamma[GammaPlotArea::CURVE_VALUE], plot->gamma[GammaPlotArea::CURVE_RED],plot->gamma[GammaPlotArea::CURVE_GREEN],plot->gamma[GammaPlotArea::CURVE_BLUE]);

}

void GammaLevelsWidget::updateLevels()
{
    plot->update();

    if (source)
        source->setGammaLevels(plot->xmin, plot->xmax, plot->ymin, plot->ymax);
}

GammaPlotArea::GammaPlotArea(QWidget *parent) : QWidget(parent), activeCurve(CURVE_VALUE), xmin(0.0), xmax(1.0), ymin(0.0), ymax(1.0)
{
    antialiased = false;

    setBackgroundRole(QPalette::Base);
    setAutoFillBackground(true);

    // setup gamma
    gamma[CURVE_VALUE] = 1.0;
    gamma[CURVE_RED] = 1.0;
    gamma[CURVE_GREEN] = 1.0;
    gamma[CURVE_BLUE] = 1.0;

    // setup pens
    pens[CURVE_VALUE].first = QPen( palette().color(QPalette::Mid).dark() );
    pens[CURVE_VALUE].first.setWidth(3);
    pens[CURVE_VALUE].second = QPen( palette().color(QPalette::Mid) );
    pens[CURVE_VALUE].second.setWidth(2);

    pens[CURVE_RED].first = QPen( QColor(CURVE_COLOR_RED) );
    pens[CURVE_RED].first.setWidth(3);
    pens[CURVE_RED].second = QPen( QColor(CURVE_COLOR_RED).light() );
    pens[CURVE_RED].second.setWidth(2);

    pens[CURVE_GREEN].first = QPen( QColor(CURVE_COLOR_GREEN) );
    pens[CURVE_GREEN].first.setWidth(3);
    pens[CURVE_GREEN].second = QPen( QColor(CURVE_COLOR_GREEN).light() );
    pens[CURVE_GREEN].second.setWidth(2);

    pens[CURVE_BLUE].first = QPen( QColor(CURVE_COLOR_BLUE) );
    pens[CURVE_BLUE].first.setWidth(3);
    pens[CURVE_BLUE].second = QPen( QColor(CURVE_COLOR_BLUE).light()  );
    pens[CURVE_BLUE].second.setWidth(2);

}

QSize GammaPlotArea::minimumSizeHint() const
{
    return QSize(100, 50);
}

QSize GammaPlotArea::sizeHint() const
{
    return QSize(200, 200);
}

void GammaPlotArea::setPen(const QPen &pen)
{
    pens[CURVE_VALUE].first = QPen(pen);
    pens[CURVE_VALUE].first.setColor( pen.color().dark());
    pens[CURVE_VALUE].first.setWidth(3);
    pens[CURVE_VALUE].second = QPen(pen);
    pens[CURVE_VALUE].second.setWidth(2);

    update();
}


void GammaPlotArea::setAntialiased(bool antialiased)
{
    this->antialiased = antialiased;
    update();
}

void GammaPlotArea::mouseMoveEvent ( QMouseEvent * event )
{
    if (event->buttons() != Qt::NoButton)
    {
        int y = qBound( 0, event->y(), this->height());
        gamma[activeCurve] = ScaleToGamma( (1.0 - ((double) y / (double) this->height()) ) * 1000.0 );
        emit gammaChanged();
    }
}


void GammaPlotArea::wheelEvent ( QWheelEvent * event )
{
    static double min = ScaleToGamma( 0 );
    static double max = ScaleToGamma( 1000 );

    int lg = GammaToScale(gamma[activeCurve]) + event->delta() / 2;
    double g = ScaleToGamma( qBound(0, lg, 1000) );
    g = qBound(min, (double) qRound( 100.0 * g) / 100.0, max);
    gamma[activeCurve] =  qAbs(1.0 - g) < 0.1 ? 1.0 : g;
    emit gammaChanged();
}

void GammaPlotArea::paintEvent(QPaintEvent *e)
{
    QPainter painter(this);
    if (antialiased)
        painter.setRenderHint(QPainter::Antialiasing, true);

    // draw grid
    QPen p(Qt::DotLine);
    if (isEnabled())
        p.setColor(palette().color(QPalette::Mid).lighter(120));
    else
        p.setColor(palette().color(QPalette::Midlight));
    painter.setPen(p);
    for (int x = width()/4; x < width(); x += width()/4 + 1)
        painter.drawLine(x, 0, x, height());
    for (int y = height()/4; y < height(); y += height()/4 + 1)
        painter.drawLine(0, y, width(), y);

    // draw plot
    if (isEnabled()){

        int N = width() / DENSITY_POINTS_PLOT;
        int H = height() - 2; // 1 pixels margins at top and botom
        QPoint points[N];
        double incr = 1.0 / (double)( N - 1);

        // loop over all curves
        for(int c = 3; c > -1; --c) {
            // convert to curve index
            gammaCurve curve = (gammaCurve) c;

            // pass on the active curve
            if (curve == activeCurve)
                continue;

            // pen for inactive curves
            painter.setPen(pens[curve].second);

            // draw curve
            double x = 0.0, y = 0.0;
            for (int i = 0; i < N; i++, x += incr) {
                y = LevelsControl(x, xmin, gamma[curve], xmax, ymin, ymax);
                points[i].setX( qRound(x * (double) width() ) );
                points[i].setY( H - qRound( y * (double) H ) -1);
            }
            painter.drawPolyline(points, N);

        }

        // draw active curve
        painter.setPen(pens[activeCurve].first);
        double x = 0.0, y = 0.0;
        for (int i = 0; i < N; i++, x += incr) {
            y = LevelsControl(x, xmin, gamma[activeCurve], xmax, ymin, ymax);
            points[i].setX( qRound(x * (double) width() ) );
            points[i].setY( H - qRound( y * (double) H ) -1);
        }
        painter.drawPolyline(points, N);
        QPoint tp = points[N/2] + QPoint( gamma[activeCurve] < 1.0 ? -50 : 5, gamma[activeCurve] < 1.0 ? -12 : 20 );
        painter.drawText( tp, QString::number(gamma[activeCurve], 'f', 2));

    }

    // draw frame
    painter.setRenderHint(QPainter::Antialiasing, false);
    painter.setPen(palette().dark().color());
    painter.setBrush(Qt::NoBrush);
    painter.drawRect(QRect(0, 0, width() - 1, height() - 1));

    QWidget::paintEvent(e);
}

