#pragma once

class Timer
{
public:
    Timer(float length) : length(length), time(0), timeout(false) {};

    void step(float deltaTime)
    {
        time += deltaTime;
        if (time > length)
        {
            time -= length;
            timeout = true;
        }
    }

    bool isTimeout() const { return timeout; };
    float getTime() const { return time; };
    float getLength() const { return length; };
    void reset()
    {
        time = 0;
        timeout = false;
    };
    void setTimeout(bool set) { timeout = set; };
    void setLength(float _length) { length = _length; };

private:
    float time;
    float length;
    bool timeout;
};

class Ruler
{
public:
    Ruler(float length) : length(length), distance(0), over(false) {};

    void step(float deltaDistance)
    {
        distance += deltaDistance;
        if (distance > length)
        {
            distance -= length;
            over = true;
        }
    }

    bool isOver() const { return over; };
    float getTime() const { return distance; };
    float getLength() const { return length; };
    void reset()
    {
        distance = 0;
        over = false;
    };
    void setTimeout(bool set) { over = set; };
    void setLength(float _length) { length = _length; };

private:
    float distance;
    float length;
    bool over;
};
