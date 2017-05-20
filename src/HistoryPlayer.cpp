#include <QtGlobal>
#include <QDebug>

#include "Source.h"
#include "RenderingManager.h"
#include "ViewRenderWidget.h"

#include "HistoryPlayer.moc"


//#define DEBUG_HISTORY

HistoryPlayer::HistoryPlayer(History history, QObject *parent) : QObject(parent),
    _history(history), _direction(History::FORWARD), _play(false), _loop(true), _reverse(true), _maximumSize(10000)
{
    _current = _history.begin();
    _currentTime = _current.key();
}

double HistoryPlayer::duration() const
{
    return _history.duration();
}

void HistoryPlayer::updateCursor()
{

    if (_direction == History::FORWARD)
    {
        _currentTime += _timer.restart();

        // execute all steps until the elapsed time
        while (_current != _history.end() && _current.key() < _currentTime) {
            _current.value().invoke(_direction);
            _current++;
        }

        // loop
        if (_current == _history.end()) {

            if (!_loop)
                play(false);

            if (_reverse) {
                _direction = _direction==History::FORWARD ? History::BACKWARD : History::FORWARD;
                _current--;
            } else {
                _current = _history.begin();
            }
        }
    }
    else
    {
        _currentTime -= _timer.restart();

        // execute all steps until the elapsed time
        while (_current.key() > _currentTime && _current != _history.begin()) {
            _current.value().invoke(_direction);
            _current--;
        }

        // loop
        if (_current == _history.begin()) {

            if (!_loop)
                play(false);

            if (_reverse) {
                _direction = _direction==History::FORWARD ? History::BACKWARD : History::FORWARD;
            } else {
                _current = _history.end() -1;
                _currentTime = _current.key();
            }
        }
    }

    // reset time on begin
    if (_current == _history.begin())
        _currentTime = _current.key();

}

void HistoryPlayer::play(bool on)
{
    // set play mode
    _play = on;

    if (_play) {
        // connect to ticking clock
        connect(RenderingManager::getRenderingWidget(), SIGNAL(tick()), SLOT(updateCursor()));
        _timer.start();
    }
    else {
        // disconnect ticking clock
        RenderingManager::getRenderingWidget()->disconnect(this);
    }

    emit playing(_play);
}

void HistoryPlayer::rewind()
{
    _current = _history.begin();
    _currentTime = _current.key();
    _direction = History::FORWARD;

    // invoke all keys for being of history
    _history.invoke(History::BACKWARD);

}


qint64 HistoryPlayer::cursorPosition() const
{
    return _current.key();
}

void HistoryPlayer::setCursorPosition(qint64 t)
{
//    if (t == 0) {
//        _current = _history.begin();
//        _currentTime = _current.key();
//    }
//    else {

//    }
//    _current.value().invoke(_direction);
}

void HistoryPlayer::setCursorNextPositionForward()
{
    // ignore if no event
    if (_history.empty())
        return;

    if ( _current != _history.end() ) {

        // invoke the previous event
        _current.value().invoke(History::FORWARD);

        // move the cursor to the previous event
        _current++;
    }

    // inform that history changed
    emit changed();
}

void HistoryPlayer::setCursorNextPositionBackward()
{
    // ignore if no event
    if (_history.empty())
        return;

    if ( _current == _history.end() )
        _current--;

    if ( _current != _history.begin() ) {

        // invoke the previous event
        _current.value().invoke(History::BACKWARD);

        // move the cursor to the previous event
        _current--;
    }
    // inform that history changed
    emit changed();
}

void HistoryPlayer::setCursorNextPosition(History::Direction dir)
{
    // Backward in time
    if (dir == History::BACKWARD) {
        setCursorNextPositionBackward();
    }
    // Forward in time
    else {
        setCursorNextPositionForward();
    }

}



