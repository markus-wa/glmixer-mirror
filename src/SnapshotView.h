#ifndef SNAPSHOTVIEW_H
#define SNAPSHOTVIEW_H

#include "View.h"
class Source;

class SnapshotView : public View
{

public:
    SnapshotView();
    virtual ~SnapshotView();


    // View implementation
    void paint();
    void setModelview();
    void resize(int w, int h);
    bool mousePressEvent(QMouseEvent *event);
    bool mouseDoubleClickEvent(QMouseEvent * event);
    bool mouseMoveEvent(QMouseEvent *event);
    bool mouseReleaseEvent(QMouseEvent * event );
    bool wheelEvent(QWheelEvent * event );
    bool keyPressEvent ( QKeyEvent * event );

    void setAction(ActionType a);
    bool getSourcesAtCoordinates(int mouseX, int mouseY, bool clic = true);

    void setVisible(bool on, View *activeview = 0);
    bool visible() { return _visible;}

    void setTargetSnapshot(QString id);

private:

    void grabSource(Source *s, int x, int y);

    bool _visible;
    View *_view;
    double _factor, _begin, _end, _y;
    QPixmap _destination;
    Source *_renderSource;

    QMap<Source *, QVector< QPair<double,double> > > _snapshots;

};

#endif // SNAPSHOTVIEW_H
