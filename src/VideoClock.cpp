
extern "C"
{
#include <libavutil/time.h>
#include <libavutil/common.h>
}

#include "VideoClock.moc"


/**
 * Get time using libav
 */
#define GETTIME (double) av_gettime() * av_q2d(AV_TIME_BASE_Q)



VideoClock::VideoClock(QObject *parent) : QObject(parent) {
    _requested_speed = -1.0;
    _speed = 1.0;
    _frame_base = 0.04;
    _time_on_start = 0.0;
    _time_on_pause = 0.0;
    _paused = false;
    // minimum is 50 % of time base
    _min_frame_delay = 0.5;
    // maximum is 200 % of time base
    _max_frame_delay = 2.0;
}

void VideoClock::reset(double deltat, double timebase) {

    // set frame base time ratio when provided
    if (timebase > 0)
        _frame_base = timebase;

    // set new time on start
    _time_on_start = GETTIME - ( deltat / _speed );

    // trick to reset time on pause
    _time_on_pause = _time_on_start + ( deltat / _speed );

}

double VideoClock::time() const {

    if (_paused)
        return (_time_on_pause - _time_on_start) * _speed;
    else
        return (GETTIME - _time_on_start) * _speed;

}

void VideoClock::pause(bool p) {

    if (p)
        _time_on_pause = GETTIME;
    else
        _time_on_start += GETTIME - _time_on_pause;

    _paused = p;
}

bool VideoClock::paused() const {
    return _paused;
}

double VideoClock::speed() const {
    return _speed;
}

double VideoClock::timeBase() const {
    return _frame_base / _speed;
}

void VideoClock::setSpeed(double s) {

    // limit range
    // request new speed
    _requested_speed = s > 10.0 ? 10.0 : s < 0.1 ? 0.1 : s;
}

void VideoClock::applyRequestedSpeed() {

    if ( _requested_speed > 0 ) {
        // trick to reset time on pause
        _time_on_pause = _time_on_start + ( time() / _speed );

        // replace time of start to match the change in speed
        _time_on_start = ( 1.0 - _speed / _requested_speed) * GETTIME + (_speed / _requested_speed) * _time_on_start;

        // set speed
        _speed = _requested_speed;
        _requested_speed = -1.0;
    }
}


double VideoClock::minFrameDelay() const{
    return _min_frame_delay * timeBase();
}

double VideoClock::maxFrameDelay() const {
    return  _max_frame_delay * timeBase();
}
