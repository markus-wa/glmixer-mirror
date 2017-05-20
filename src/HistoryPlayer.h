#ifndef HISTORYPLAYER_H
#define HISTORYPLAYER_H

#include <QDebug>
#include <QRectF>
#include <QColor>
#include <QStack>
#include <QElapsedTimer>

#include "History.h"

class HistoryPlayer : public QObject
{
    Q_OBJECT

    friend class HistoryPlayerModel;

public:
    HistoryPlayer(History history, QObject *parent = 0);

    // duration
    double duration() const;

    // playback
    bool isPlaying() const { return _play; }
    bool isLoop() const { return _loop; }
    bool isReverse() const { return _reverse; }

signals:
    void playing(bool);

public slots:

    // navigate into history
    qint64 cursorPosition() const;
    void setCursorPosition(qint64 t = 0);
    void setCursorNextPosition(History::Direction dir);
    void setCursorNextPositionForward();
    void setCursorNextPositionBackward();

    // control playback
    void rewind();
    void play(bool on);
    void loop(bool on) { _loop = on; }
    void reverse(bool on) { _reverse = on; }

    // synchronize with display
    void updateCursor();

private:

    // history of events
    History _history;

    // replay toolbox
    History::EventMap::iterator _current;
    qint64 _currentTime;
    QElapsedTimer _timer;
    History::Direction _direction;
    bool _play, _loop, _reverse;

};

typedef HistoryPlayer * HistoryPlayerStar;
Q_DECLARE_METATYPE(HistoryPlayerStar);

#endif // HISTORYPLAYER_H
