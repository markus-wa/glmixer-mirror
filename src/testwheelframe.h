/*
 * testwheelframe.h
 *
 *  Created on: Jul 11, 2011
 *      Author: bh
 */

#ifndef TESTWHEELFRAME_H_
#define TESTWHEELFRAME_H_

#include <QWidget>

class TestWheelFrame: public QWidget
{
	Q_OBJECT

public:
	TestWheelFrame(QWidget * parent = 0, Qt::WindowFlags f = 0);

protected:
    void wheelEvent(QWheelEvent *event);
    void paintEvent(QPaintEvent *event);
    void showEvent(QShowEvent *event);

public Q_SLOTS:
	void setSpeed(int);
	void setCentered(bool);

private:
	int speed;
	bool centered;
	float scale;
	QPointF poscenter;
};

#endif /* TESTWHEELFRAME_H_ */
