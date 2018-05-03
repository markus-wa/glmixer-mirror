#include <QtGui>

#include "TimelineSlider.moc"

const int PANEL_HEIGHT = 60;
const double MAXIMUM_VALUE = 100000.0;
const int GRAB_THRESHOLD = 5;

TimelineSlider::TimelineSlider(QWidget *parent) :
    QFrame(parent)
{
    m_parent = parent;
    setMinimumHeight(PANEL_HEIGHT);
    setMouseTracking(true);

    margin_pixel = 20;
    HEIGHT_TIME_BAR = 20;
    DISTANCE_MARK_TEXT = 10;
    LINE_MARK_LENGHT = 5;
    RANGE_MARK_HEIGHT = 25;
    draw_area = QRect(margin_pixel, 0, size().width() - margin_pixel * 2, size().height());

    overlayFont = font();
    overlayFont.setPointSize(9);
    overlayFont.setBold(true);
    overlayFont.setItalic(true);
    labelFont = font();
    labelFont.setPointSize(7);
    setFont(labelFont);

    reset();
}

TimelineSlider::~TimelineSlider()
{

}

void TimelineSlider::setValue(double v) {
    // ensure value is correct
    v = qBound(min_value, v, max_value);

    // round the value by the value step
    qint64 intval_before = (int) qRound64(cur_value / step_value);
    qint64 intval_after = (int) qRound64(v / step_value);
    // update value if different (module value step)
    if ( intval_before != intval_after ) {
        cur_value = (double) intval_after * step_value;
        // done
        repaint();
        emit valueChanged(cur_value);
    }
}


void TimelineSlider::requestValue(double v) {
    user_value = v;
    emit valueRequested(user_value);
    repaint();
}

void TimelineSlider::setStep(double v) {

    step_value = qMax( v, 0.001 );
    updateMarks();
}

void TimelineSlider::setMargin(int pixels) {
    margin_pixel = pixels;
    updateMarks();
}

void TimelineSlider::setMinimum(double v) {
    min_value = qBound(0.0, v, max_value);
    if (cur_value < min_value)
        setValue(min_value);
    updateMarks();
}

void TimelineSlider::setMaximum(double v) {
    max_value = qBound(min_value, v, MAXIMUM_VALUE);
    if (cur_value > max_value)
        setValue(max_value);
    updateMarks();
}


int TimelineSlider::getPosFromVal(double v)
{
    int w = draw_area.width();
    return (int) ( w * ( (v - min_value) / (max_value - min_value) ) ) + draw_area.left();
}

double TimelineSlider::getValFromPos(int p)
{
    int w = draw_area.width();
    return min_value + ( (double) (p - draw_area.left()) / (double) w ) * (max_value - min_value);
}

void TimelineSlider::reset() {
    speed = 1.0;
    cur_value = 0.0;
    min_value = 0.0;
    max_value = 1.0;
    step_value = 0.2;
    range_value.first = 0.0;
    range_value.second = 1.0;
    range_cursor.first = 0.0;
    range_cursor.second = 1.0;
    mark_value = 1.0;
    step_value_increment = -1.0;
    user_value = -1.0;
    cursor_state = CURSOR_NONE;
    repaint();
}


void TimelineSlider::setBeginToMinimum()
{
    setBegin(minimum());
}

void TimelineSlider::setEndToMaximum()
{
    setEnd(maximum());
}

void TimelineSlider::setBeginToCurrent()
{
    setBegin(value());
}

void TimelineSlider::setEndToCurrent()
{
    setEnd(value());
}


void TimelineSlider::setRange(QPair<double, double> r) {

    // ensure the range is correct
    if (r.first > r.second)
        r.first = r.second;
    r.first = qBound(min_value, r.first, max_value);
    r.second = qBound(min_value, r.second, max_value);

    // round the value by the value step
    qint64 intval_before = (int) qRound64(range_value.first / step_value);
    qint64 intval_after = (int) qRound64(r.first / step_value);
    // update value if different (module value step)
    if ( intval_before != intval_after ) {
        range_value.first = (double) intval_after * step_value;
        emit beginChanged(range_value.first);
    }

    // round the value by the value step
    intval_before = (int) qRound64(range_value.second / step_value);
    intval_after = (int) qRound64(r.second / step_value);
    // update value if different (module value step)
    if ( intval_before != intval_after ) {
        range_value.second = (double) intval_after * step_value;
        emit endChanged(range_value.second);
    }

    // set cursors
    range_cursor = r;
    repaint();
}

void TimelineSlider::setBegin(double v) {
    setRange(qMakePair( v, qMax(v, range_value.second) ));
}

void TimelineSlider::setEnd(double v) {
    setRange(qMakePair( qMin(v, range_value.first), v ));
}


void TimelineSlider::mouseDoubleClickEvent ( QMouseEvent * event )
{
    if (event->button() == Qt::LeftButton) {
        // mouse double clic
        switch (mouseOver(event)) {
        case CURSOR_CURRENT:
            break;
        case CURSOR_RANGE_MIN:
            setBegin(min_value);
            break;
        case CURSOR_RANGE_MAX:
            setEnd(max_value);
            break;
        case CURSOR_OVER:
        default:
        {
            // JUMP to position
            double pos = getValFromPos(event->pos().x());
            if ( pos > range_value.first && pos < range_value.second)
                requestValue( pos );
        }
        }
    }
}


TimelineSlider::cursor TimelineSlider::mouseOver(QMouseEvent * e)
{
    if ( e->pos().y() < height()/2 && qAbs( getPosFromVal(cur_value) - e->pos().x()) < GRAB_THRESHOLD ) {
        return CURSOR_CURRENT;
    }
    else if ( qAbs( getPosFromVal(range_cursor.first) - e->pos().x()) < GRAB_THRESHOLD ) {

        return CURSOR_RANGE_MIN;
    }
    else if ( qAbs( getPosFromVal(range_cursor.second) - e->pos().x()) < GRAB_THRESHOLD ) {

        return CURSOR_RANGE_MAX;
    }
    else if (  e->pos().y() < HEIGHT_TIME_BAR ) {

        return CURSOR_OVER;
    }
    return CURSOR_NONE;
}

void TimelineSlider::mouseMoveEvent ( QMouseEvent * event )
{
//    double v = user_value;

    switch (cursor_state) {
    case CURSOR_CURRENT:
        // slider for cursor position
        requestValue( getValFromPos(event->pos().x()) );
        break;
    case CURSOR_RANGE_MIN:
        // slider for min position
        range_cursor.first = qBound(min_value, getValFromPos(event->pos().x()), max_value) ;
        break;
    case CURSOR_RANGE_MAX:
        // slider for max position
        range_cursor.second = qBound(min_value, getValFromPos(event->pos().x()), max_value) ;
        break;
    default:
    {
        // mouse over : set cursor
        switch (mouseOver(event)) {
        case CURSOR_CURRENT:
            setCursor(Qt::SplitHCursor);
            break;
        case CURSOR_RANGE_MIN:
            setCursor(Qt::SizeHorCursor);
            break;
        case CURSOR_RANGE_MAX:
            setCursor(Qt::SizeHorCursor);
            break;
        case CURSOR_OVER:
            cursor_state = CURSOR_OVER;
            repaint();
        default:
        {
            unsetCursor();
//            if (event->pos().y() < 30)
//                over_value = getValFromPos(event->pos().x());
//            else
//                over_value = -1;
//            if ( v != over_value )
//                repaint();

        }
            break;
        }

    }
        break;
    }

//    setToolTip( over_value > 0.0 ? getStringFromTime(over_value) : QString::null);

    repaint();
}

void TimelineSlider::mousePressEvent ( QMouseEvent * event )
{
    if (event->button() == Qt::LeftButton) {
        // mouse clic : set state
        cursor_state = mouseOver(event);
    }
}

void TimelineSlider::mouseReleaseEvent ( QMouseEvent *event )
{

    switch (cursor_state) {
    case CURSOR_RANGE_MIN:
        // min position
        // (will repaint)
        range_cursor.first = qBound(min_value, getValFromPos(event->pos().x()), max_value) ;
        setBegin( range_cursor.first );
        break;
    case CURSOR_RANGE_MAX:
        // max position
        // (will repaint)
        range_cursor.second = qBound(min_value, getValFromPos(event->pos().x()), max_value) ;
        setEnd( range_cursor.second );
        break;
    default:
    {

    }
    }

    if (cursor_state != CURSOR_OVER)
        cursor_state = CURSOR_NONE;

//    repaint();
}


//void TimelineSlider::leaveEvent( QEvent * event )
//{

////    range_cursor = range_value;
////    cursor_state = CURSOR_NONE;

//    qDebug() << "LEAVE ";

//}

void TimelineSlider::wheelEvent ( QWheelEvent * event )
{
    // scroll forward only
    int numDegrees = event->delta() / 8;
    int numSteps = numDegrees / 15;
    if ( numSteps > 0 ) {
        // increment value and request it
        requestValue( value() + (double) numSteps * step_value);
        event->accept();
    }
}


void TimelineSlider::paintEvent(QPaintEvent *e) {

    QPainter qp(this);
    //  qp.setRenderHint(QPainter::Antialiasing, true);
    qp.setRenderHint(QPainter::TextAntialiasing, true);
    drawWidget(qp);

    QFrame::paintEvent(e);
}


void TimelineSlider::setLabelFont(const QString &fontFamily, int pointSize)
{
    overlayFont = QFont(fontFamily, pointSize + 2);
    overlayFont.setBold(true);
    overlayFont.setItalic(true);

    labelFont = QFont(fontFamily, pointSize);
    setFont(labelFont);

}

void TimelineSlider::drawWidget(QPainter &qp)
{
    // update draw area
    draw_area = QRect(margin_pixel, 0, size().width() - margin_pixel * 2, size().height());
    int zero_x =  draw_area.left();

    QColor COLOR_MARK(palette().color(QPalette::Highlight).dark(120));
    COLOR_MARK.setAlpha( isEnabled() ?  255 : 200);
    QPen PEN_MARK(COLOR_MARK, 3);
    QColor COLOR_RANGE = palette().color(QPalette::Window);
    COLOR_RANGE.setAlpha( isEnabled() ?  200 : 120);

    // draw timeline
    //  qp.setPen(PEN_TICS);
    //  qp.drawLine(0, HEIGHT_TIME_BAR, size().width(), HEIGHT_TIME_BAR);

    // compute pixel coordinates from values
    int currentPosition = getPosFromVal(cur_value);
    int userPosition = getPosFromVal(user_value);
    int rangeBegin = getPosFromVal( range_value.first );
    int rangeEnd = getPosFromVal( range_value.second );
    int rangeCursorBegin = getPosFromVal( range_cursor.first );
    int rangeCursorEnd = getPosFromVal( range_cursor.second );

    // compute useful coordinates
    int range_mark_y = draw_area.height() / 2 + RANGE_MARK_HEIGHT;

    int step_width = (int) qRound( step_value * ( (double)  size().width() / (max_value - min_value) ) );

    // draw marks and text
    qp.setPen(palette().color(QPalette::WindowText));
    qp.setFont(labelFont);
    QFontMetrics labelFontMetrics(labelFont);

    double t = min_value;
    int pos = zero_x;
    int lastpos = -zero_x;

    // draw first line
    qp.drawLine(pos, HEIGHT_TIME_BAR + LINE_MARK_LENGHT, pos, HEIGHT_TIME_BAR - LINE_MARK_LENGHT );

    while (t < max_value) {
        pos = getPosFromVal(t);
        qp.setPen(palette().color(QPalette::WindowText));

        // create label and compute its pixel width
        QString label = TimelineSlider::getStringFromTime( t );
        int width_text = labelFontMetrics.width(label) / 2;

        // draw mark and label if enough space
        if ( pos - width_text > lastpos + width_text )
        {
            qp.drawLine(pos, HEIGHT_TIME_BAR, pos, HEIGHT_TIME_BAR - LINE_MARK_LENGHT);

            // display discard label for borders
            if (pos - width_text + labelFontMetrics.width(label) < draw_area.right() - 3 )
                qp.drawText(pos - width_text, DISTANCE_MARK_TEXT, label);

            // remember last drawn label
            lastpos = pos;
        }
        // draw small mark otherwise
        else
            qp.drawLine(pos, HEIGHT_TIME_BAR, pos, HEIGHT_TIME_BAR - LINE_MARK_LENGHT / 2);

        if ( step_value_increment >0 ) {
            // draw intermediate steps if enough space
            qp.setPen(palette().color(QPalette::WindowText).lighter());
            // increment by the step increment
            double t2 = t + step_value_increment;
            while (t2 < t + mark_value) {
                pos = getPosFromVal(t2);
                qp.drawLine(pos, HEIGHT_TIME_BAR, pos, HEIGHT_TIME_BAR - LINE_MARK_LENGHT / 2);
                t2 += step_value_increment;
            }
        }

        // next mark
        t += mark_value;
    }

    // draw last value
    pos = getPosFromVal(max_value);
    QString label = TimelineSlider::getStringFromTime( max_value );
    int width_text = labelFontMetrics.width(label) / 2;
    qp.setPen(palette().color(QPalette::WindowText));
    qp.drawLine(pos, HEIGHT_TIME_BAR + LINE_MARK_LENGHT, pos, HEIGHT_TIME_BAR - LINE_MARK_LENGHT );
    qp.drawText(pos - width_text, DISTANCE_MARK_TEXT, label);

    // show cursor value
    // current value
    label = TimelineSlider::getStringFromTime( cur_value );
    int pos_text = currentPosition - labelFontMetrics.width(label) / 2;

    // draw position cursor
    qp.setPen(PEN_MARK);
    if ( cursor_state == CURSOR_CURRENT )
        qp.drawLine(userPosition, HEIGHT_TIME_BAR, userPosition, range_mark_y);
    else
        qp.drawLine(currentPosition, HEIGHT_TIME_BAR, currentPosition, range_mark_y);

    QPainterPath myPath;
    QPolygonF polygon;
    polygon << QPointF(currentPosition, HEIGHT_TIME_BAR + 1) << QPointF(currentPosition - 5, HEIGHT_TIME_BAR -4) << QPointF(currentPosition + 5, HEIGHT_TIME_BAR -4) << QPointF(currentPosition, HEIGHT_TIME_BAR + 1) ;
    myPath.addPolygon(polygon);
    qp.setPen(COLOR_MARK);
    qp.setBrush(COLOR_MARK);
    qp.drawPath(myPath);

    // draw area for cursor text
    qp.fillRect(pos_text - 2, 0, labelFontMetrics.width(label) + 4, HEIGHT_TIME_BAR - 4, COLOR_MARK);

    // draw text on top
    qp.setPen(palette().color(QPalette::HighlightedText));
    //      pos_text = qBound(0, pos_text, width() - metrics.width(label));
    qp.drawText(pos_text, DISTANCE_MARK_TEXT, label);


    // Draw range
    qp.fillRect(rangeBegin, range_mark_y, rangeEnd - rangeBegin, -RANGE_MARK_HEIGHT, COLOR_RANGE.darker(150));
    qp.setPen(COLOR_RANGE.darker(170));
    qp.drawLine(rangeBegin, HEIGHT_TIME_BAR, rangeBegin, range_mark_y);
    qp.drawLine(rangeEnd, HEIGHT_TIME_BAR, rangeEnd, range_mark_y);

    // draw cursor of begin and end
    qp.setPen(COLOR_RANGE.darker(200));
    qp.drawLine(rangeCursorBegin, HEIGHT_TIME_BAR, rangeCursorBegin, range_mark_y);
    qp.drawLine(rangeCursorEnd, HEIGHT_TIME_BAR, rangeCursorEnd, range_mark_y);
    qp.setPen(PEN_MARK);
    qp.drawPoint(rangeCursorBegin +2, range_mark_y -1);
    qp.drawPoint(rangeCursorEnd -2, range_mark_y -1);

    // compute position begin value
//    QString label_b = TimelineSlider::getStringFromTime( begin() );
    QString label_b = TimelineSlider::getStringFromTime( range_cursor.first );
    int pos_text_b = rangeCursorBegin - labelFontMetrics.width(label_b) -1;
    if (pos_text_b < 1)
        pos_text_b = rangeCursorBegin + 5;

    // compute position end value
//    QString label_e = TimelineSlider::getStringFromTime( end() );
    QString label_e = TimelineSlider::getStringFromTime( range_cursor.second );
    int pos_text_e = rangeCursorEnd;
    if (pos_text_e + labelFontMetrics.width(label) > width())
        pos_text_e = rangeCursorEnd - labelFontMetrics.width(label_e) - 5;

    qp.setPen(palette().color(QPalette::WindowText));
    // draw both labels if enough space between them
    if ( qAbs(pos_text_b - pos_text_e) > labelFontMetrics.width(label_e)  ) {
        qp.drawText(pos_text_e, range_mark_y - 2, label_e);
        qp.drawText(pos_text_b, range_mark_y - 2, label_b);
    }
    // otherwise draw the most central one
    else if ( pos_text_b == rangeBegin + 5) {
        qp.drawText(pos_text_e, range_mark_y - 2, label_e);
    }
    else
        qp.drawText(pos_text_b, range_mark_y - 2, label_b);

    // draw duration information
    QString label_d = TimelineSlider::getStringFromTime( (range_cursor.second - range_cursor.first) / speed );

    QFontMetrics overlayFontMetrics(overlayFont);
    if ( rangeEnd - rangeBegin > overlayFontMetrics.width(label_d)) {
        int pos_text_d = rangeBegin + (rangeEnd - rangeBegin) / 2 - overlayFontMetrics.width(label_d)/2;
        qp.setFont(overlayFont);
        qp.setPen(Qt::white);
        qp.drawText(pos_text_d, range_mark_y - (RANGE_MARK_HEIGHT/2) + overlayFontMetrics.height()/2, label_d);
    }

    // draw mouse over cursor value
//    if (cursor_state == CURSOR_OVER)
//    {

//        label = TimelineSlider::getStringFromTime( user_value );
//        pos = getPosFromVal(user_value);

//        qp.drawText(pos, range_mark_y - 2, label);
//    }

}


void TimelineSlider::updateMarks()
{
    double range = max_value - min_value;

    int s = (int) range;
    int h = s / 3600;
    int m = (s % 3600) / 60;
    s = (s % 3600) % 60;

    step_value_increment = -1.0;

    if ( h > 3 )                // more than 3h
        mark_value = 1800.0;
    else if ( h > 2 )           // between 2 and 3h
        mark_value = 900.0;
    else if ( h > 1 )           // between 1 and 2h
        mark_value = 600.0;
    else if ( h > 0 )           // between 1 and 2h
        mark_value = 300.0;
    else if ( m > 30 )          // between 30 and 60 min
        mark_value = 120.0;
    else if ( m > 15 )  {        // between 15 and 30 min
        mark_value = 60.0;
        step_value_increment = qMax(step_value, 20.0);
    }
    else if ( m > 7 )   {        // between 7 and 15 min
        mark_value = 30.0;
        step_value_increment = qMax(step_value, 10.0);
    }
    else if ( m > 3 )  {         // between 3 and 7 min
        mark_value = 15.0;
        step_value_increment = qMax(step_value, 5.0);
    }
    else if ( m > 1 )   {        // between 2 and 3 min
        mark_value = 10.0;
        step_value_increment = qMax(step_value, 2.0);
    }
    else if ( s > 29 )   {        // between 30s and 2 min
        mark_value = 5.0;
        step_value_increment = qMax(step_value, 1.0);
    }
    else if ( s > 19 )  {        // between 30 and 59 sec
        mark_value = 2.0;
        step_value_increment = qMax(step_value, 0.25);
    }
    else if ( s > 9 ) {         // between 10 and 29 sec
        mark_value = 1.0;
        step_value_increment = qMax(step_value, 0.1);
     }
    else if ( s > 4 ) {          // between 5 and 9 sec
        mark_value = step_value * 10.0;
        step_value_increment = step_value * 2.0;
    } 
    else                        // less than 4 sec
        mark_value = step_value;

    //qDebug()<<"> "  << h << m << s<<  mark_value << step_value << step_value_increment ;

    repaint();
}



QString TimelineSlider::getStringFromTime(double time)
{
    QString text;
    int s = (int) qRound64(time * 100.0) / 100;
    double ms = time - (double) s;
    int h = s / 3600;
    int m = (s % 3600) / 60;
    s = (s % 3600) % 60;
    int ds = (int) qRound64(ms * 1000.0) / 10 ;

    if (h>0)
        text = QString(" %1:%2:%3 ").arg(h, 2).arg(m, 2, 10, QChar('0')).arg(s, 2, 10, QChar('0'));
    else if (m>0)
        text = QString(" %1:%2 ").arg(m, 2, 10, QChar(' ')).arg(s, 2, 10, QChar('0'));
    else
        text = QString(" %1.%2 ").arg(s, 2, 10, QChar(' ')).arg(ds, 2, 10, QChar('0'));

    return text;
}

//double TimelineSlider::getTimeFromString(QString line)
//{
//    bool ok = false;
//    double h, m, s;

//    QStringList parts = line.split('h', QString::SkipEmptyParts);
//    if (parts.size()!=2)    return -1;
//    h = (double) parts[0].toInt(&ok);
//    if (!ok) return -1;
//    parts = parts[1].split('m', QString::SkipEmptyParts);
//    if (parts.size()!=2)    return -1;
//    m = (double) parts[0].toInt(&ok);
//    if (!ok) return -1;
//    parts = parts[1].split('s', QString::SkipEmptyParts);
//    s = parts[0].toDouble(&ok);
//    if (!ok) return -1;

//    return (h * 3600.0) + (m * 60.0) + s;
//}
