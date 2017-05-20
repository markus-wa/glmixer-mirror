#ifndef TIMELINESLIDER_H
#define TIMELINESLIDER_H

#include <QWidget>
#include <QFrame>
#include <QPair>

class TimelineSlider : public QFrame
{
    Q_OBJECT

    Q_PROPERTY(double value READ value WRITE setValue NOTIFY valueChanged)
    Q_PROPERTY(double minimum READ minimum WRITE setMinimum)
    Q_PROPERTY(double maximum READ maximum WRITE setMaximum)
    Q_PROPERTY(double begin READ begin WRITE setBegin)
    Q_PROPERTY(double end READ end WRITE setEnd)
    Q_PROPERTY(double step READ step WRITE setStep)
    Q_PROPERTY(double margin READ margin WRITE setMargin)

public:
    explicit TimelineSlider(QWidget *parent = 0);
    ~TimelineSlider();

    // properties
    double value() const { return cur_value; }
    double minimum() const { return min_value; }
    double maximum() const { return max_value; }
    double begin() const { return range_value.first; }
    double end() const { return range_value.second; }
    double step() const { return step_value; }
    int margin() const { return margin_pixel; }

    // utility
    static QString getStringFromTime(double time);
    QPair<double,double> range() const { return range_value; }
    void setRange(QPair<double,double> r);

    void setLabelFont(const QString &fontFamily, int pointSize);

    // TODO : context menu with
    // - reset
    // - go to time :
//    static double getTimeFromString(QString line);

public slots:
    // Properties
    void setValue(double v);
    void requestValue(double v);
    void setMinimum(double v);
    void setMaximum(double v);
    void setStep(double v);
    void setBegin(double v);
    void setEnd(double v);
    void setMargin(int pixels);

    // utilities
    void reset();
    void setBeginToMinimum();
    void setEndToMaximum();
    void setBeginToCurrent();
    void setEndToCurrent();

signals:
    void valueRequested(double);
    void valueChanged(double);
    void beginChanged(double);
    void endChanged(double);

protected:
    void paintEvent(QPaintEvent *e);
    void mouseDoubleClickEvent ( QMouseEvent * event );
    void mouseMoveEvent ( QMouseEvent * event );
    void mousePressEvent ( QMouseEvent * event );
    void mouseReleaseEvent (QMouseEvent *);
    void wheelEvent ( QWheelEvent * event );

private:

    // internal use
    void drawWidget(QPainter &qp);
    void updateMarks();
    typedef enum {
        CURSOR_NONE = 0,
        CURSOR_OVER,
        CURSOR_CURRENT,
        CURSOR_RANGE_MIN,
        CURSOR_RANGE_MAX
    } cursor;
    cursor cursor_state;
    cursor mouseOver(QMouseEvent * e);
    int getPosFromVal(double v);
    double getValFromPos(int p);

    QWidget *m_parent;
    double cur_value;
    double min_value;
    double max_value;
    double step_value;
    double user_value;
    QPair<double,double> range_value;
    double mark_value;
    int margin_pixel;

    int HEIGHT_TIME_BAR;
    int DISTANCE_MARK_TEXT;
    int LINE_MARK_LENGHT;
    int RANGE_MARK_HEIGHT;
    QRect  draw_area;
    QFont labelFont;
};

#endif // TIMELINESLIDER_H
