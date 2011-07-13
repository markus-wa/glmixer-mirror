/*
 * testbuttonframe.h
 *
 *  Created on: Jun 23, 2011
 *      Author: bh
 */

#ifndef TESTBUTTONFRAME_H_
#define TESTBUTTONFRAME_H_

#include <QWidget>
#include <QMap>
#include "View.h"

class TestButtonFrame : public QWidget
{
	Q_OBJECT
	
public:
	TestButtonFrame(QWidget * parent = 0, Qt::WindowFlags f = 0);

    QMap<View::UserInput,Qt::MouseButtons> buttonMap() { return qbuttonmap; }
    QMap<View::UserInput,Qt::KeyboardModifiers> modifierMap() { return qmodifiermap; }
    void setConfiguration(QMap<int, int> buttonmap, QMap<int, int> modifiermap);

protected:
    bool event(QEvent *event);
    void leaveEvent ( QEvent * );
    void mousePressEvent(QMouseEvent *event);
    void mouseMoveEvent(QMouseEvent *event);
    void paintEvent(QPaintEvent *event);

public Q_SLOTS:
	void reset();
	void unset();

Q_SIGNALS:
	void inputChanged(QString);

private:

    QMap<View::UserInput,Qt::MouseButtons> qbuttonmap;
    QMap<View::UserInput,Qt::KeyboardModifiers> qmodifiermap;
    QMap<View::UserInput,QRect> qareamap;
    View::UserInput hover;
    QColor assignedBrushColor, assignedPenColor, unassignedBrushColor, unassignedPenColor;
};

#endif /* TESTBUTTONFRAME_H_ */
