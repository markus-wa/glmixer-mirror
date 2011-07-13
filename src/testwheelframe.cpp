/*
 * testwheelframe.cpp
 *
 *  Created on: Jul 11, 2011
 *      Author: bh
 */

#include "testwheelframe.moc"
#include "common.h"

#define MINSCALE 4.0
#define MAXSCALE 100.0

TestWheelFrame::TestWheelFrame(QWidget * parent, Qt::WindowFlags f):QWidget(parent, f), speed(120), centered(true), scale(10.f) {

}

void TestWheelFrame::setSpeed(int s) {
	speed = s;

}

void TestWheelFrame::setCentered(bool on) {
	centered = on;
	if (!centered)
		poscenter = rect().center();
	update();
}


void TestWheelFrame::showEvent(QShowEvent *event){

	poscenter = rect().center();
}

void TestWheelFrame::wheelEvent ( QWheelEvent * event ){

	QPointF deltapos = poscenter - event->pos();
	float s = scale;

	// zoom in or out
	scale += ((float) event->delta() * scale * MINSCALE ) / ( (float) speed * MAXSCALE);

	// translate if in mouse-centered zoom mode
    if (centered)
    	poscenter = event->pos() + deltapos * ( scale / s );

	update();
}

void TestWheelFrame::paintEvent(QPaintEvent *event){

    QPainter painter;
    painter.begin(this);

    // show highlight border if out of scale
    if ( scale < MINSCALE || scale > MAXSCALE ){
        painter.setPen(palette().color(QPalette::Highlight));
        QTimer::singleShot(50, this, SLOT(repaint()));
    } else
    	painter.setPen(QColor(Qt::black));

    // draw background with frame
    painter.setBrush(QColor(COLOR_BGROUND));
    painter.drawRect(event->rect().adjusted(0,0,-1,-1));

    // apply transformations
	painter.translate(poscenter);
	scale = CLAMP( scale, MINSCALE, MAXSCALE);
    painter.scale(scale, scale);

    // draw circles
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setPen(QColor(COLOR_DRAWINGS));
    painter.setBrush(Qt::NoBrush);
    for (float diameter = 0; diameter < 50.0; diameter += 2.0)
    	painter.drawEllipse(QRectF(-diameter / 2.0, -diameter / 2.0, diameter, diameter));


}