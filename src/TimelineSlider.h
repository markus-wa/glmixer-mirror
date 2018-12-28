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
    Q_PROPERTY(double fadein READ fadein WRITE setFadein)
    Q_PROPERTY(double fadeout READ fadeout WRITE setFadeout)

public:
    explicit TimelineSlider(QWidget *parent = 0);
    ~TimelineSlider();

    // properties
    double value() const { return cur_value; }
    double minimum() const { return min_value; }
    double maximum() const { return max_value; }
    double step() const { return step_value; }

    QPair<double,double> range() const { return range_value; }
    void setRange(QPair<double,double> r);
    double begin() const { return range_value.first; }
    double end() const { return range_value.second; }

    QPair<double,double> fading() const { return range_fade; }
    void setFading(QPair<double,double> r);
    double fadein() const { return range_fade.first; }
    double fadeout() const { return range_fade.second; }

    // utility
    static QString getStringFromTime(double time);
    void setLabelFont(const QString &fontFamily, int pointSize);
    int margin() const { return margin_pixel; }
    void setMargin(int pixels);

    // TODO : context menu with
    // - reset
    // - go to time

public slots:
    // Properties
    void setValue(double v);
    void requestValue(double v);
    void setMinimum(double v);
    void setMaximum(double v);
    void setStep(double v);
    void setBegin(double v);
    void setEnd(double v);
    void setFadein(double v);
    void setFadeout(double v);

    // utilities
    void reset();
    void resetBegin();
    void resetEnd();
    void setBeginToCurrent();
    void setEndToCurrent();
    void setSpeed(double s) { speed = s; }

signals:
    void valueRequested(double);
    void valueChanged(double);
    void beginChanged(double);
    void endChanged(double);
    void fadeinChanged(double);
    void fadeoutChanged(double);

protected:
    void paintEvent(QPaintEvent *e);
    void mouseDoubleClickEvent ( QMouseEvent * event );
    void mouseMoveEvent ( QMouseEvent * event );
    void mousePressEvent ( QMouseEvent * event );
    void mouseReleaseEvent (QMouseEvent *);
    void wheelEvent ( QWheelEvent * event );
//    void leaveEvent( QEvent * event);

private:

    // internal use
    void drawWidget(QPainter &qp);
    void updateMarks();
    typedef enum {
        CURSOR_NONE = 0,
        CURSOR_OVER,
        CURSOR_CURRENT,
        CURSOR_RANGE_MIN,
        CURSOR_RANGE_MAX,
        CURSOR_FADING_MIN,
        CURSOR_FADING_MAX
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
    QPair<double,double> cursor_value;
    QPair<double,double> range_fade;
    QPair<double,double> cursor_fade;
    double mark_value;
    double step_value_increment;
    int margin_pixel;

    int HEIGHT_TIME_BAR;
    int DISTANCE_MARK_TEXT;
    int LINE_MARK_LENGHT;
    int RANGE_MARK_HEIGHT;
    QRect draw_area;
    QFont labelFont;
    QFont overlayFont;
    double speed;
};

#endif // TIMELINESLIDER_H
