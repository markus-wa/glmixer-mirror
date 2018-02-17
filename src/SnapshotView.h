#ifndef SNAPSHOTVIEW_H
#define SNAPSHOTVIEW_H

#include "View.h"
#include <QElapsedTimer>

#define DEFAULT_ANIMATION_SPEED 0.001
#define MIN_ANIMATION_SPEED 0.0001
#define MAX_ANIMATION_SPEED 0.005
#define ICON_CURSOR_SCALE 0.75
#define ICON_BORDER_SCALE 0.6

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
//    bool mouseDoubleClickEvent(QMouseEvent * event);
    bool mouseMoveEvent(QMouseEvent *event);
    bool mouseReleaseEvent(QMouseEvent * event );
    bool wheelEvent(QWheelEvent * event );
    bool keyPressEvent(QKeyEvent * event );
    void setAction(ActionType a);
    bool getSourcesAtCoordinates(int mouseX, int mouseY, bool clic = true);

    // specific functions for snapshot view
    void activate(View *activeview, QString id, bool interpolate = true);
    void deactivate();
    bool isActive() { return _active;}
    void activateTarget(bool positive);

private:

    bool setTargetSnapshot(QString id);
    void grabSource(Source *s, int x, int y);
    void setCursorAction(QPoint mousepos);

    bool _active, _interpolate;
    View *_view;
    double _factor, _begin, _end, _y;
    QString _destinationId;
    QImage _departure, _destination;
    QElapsedTimer _animationTimer;
    double _animationSpeed;

    class RenderingSource *_renderSource;
    class CaptureSource *_departureSource, *_destinationSource;

    QMap<Source *, QVector< QPair<double,double> > > _snapshots;

};

#endif // SNAPSHOTVIEW_H
