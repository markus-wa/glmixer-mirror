#ifndef SNAPSHOTVIEW_H
#define SNAPSHOTVIEW_H

#include "View.h"

class SnapshotView : public View
{

public:
    SnapshotView();
    virtual ~SnapshotView();


    // View implementation
    void paint();
    void resize(int w, int h);
    bool mousePressEvent(QMouseEvent *event);
    bool mouseDoubleClickEvent(QMouseEvent * event);
    bool mouseMoveEvent(QMouseEvent *event);
    bool mouseReleaseEvent(QMouseEvent * event );
    bool wheelEvent(QWheelEvent * event );

    bool getSourcesAtCoordinates(int mouseX, int mouseY, bool clic = true);

    void setVisible(bool on, View *activeview = 0);
    bool visible() { return _visible;}

private:

    void grabSource(Source *s, int x, int y, int dx, int dy);

    bool _visible;
    View *_view;
    double _factor;
    QPixmap _destination;
    class Source *_renderSource;
};

#endif // SNAPSHOTVIEW_H
