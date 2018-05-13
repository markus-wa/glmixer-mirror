#ifndef VIDEOCLOCK_H
#define VIDEOCLOCK_H


class VideoClock
{
    bool _paused;
    double _speed;
    double _time_on_start;
    double _time_on_pause;
    double _min_frame_delay;
    double _max_frame_delay;
    double _frame_base;
    double _requested_speed;

public:
    VideoClock();
    void reset(double deltat, double timebase = -1.0);
    void pause(bool);
    void setSpeed(double);
    void applyRequestedSpeed();

    bool paused() const;
    double time() const;
    double speed() const;
    double timeBase() const;
    double minFrameDelay() const;
    double maxFrameDelay() const;
};

#endif // VIDEOCLOCK_H
