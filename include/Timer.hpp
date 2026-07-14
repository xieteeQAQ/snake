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
    Ruler(float length) : length(length), distance(0), overCount(0) {};

    void step(float deltaDistance)
    {
        distance += deltaDistance;
        while (distance >= length)
        {
            distance -= length;
            ++overCount;
        }
    }

    bool isOver() const { return overCount > 0; };
    void consumeOver()
    {
        if (overCount > 0)
            --overCount;
    }
    float getDistance() const { return distance; };
    float getLength() const { return length; };
    void reset()
    {
        distance = 0;
        overCount = 0;
    };
    void setLength(float _length) { length = _length; };

private:
    float distance;
    float length;
    int overCount;
};

class Counter
{
public:
    Counter(float length) : length(length), number(0), over(false) {};

    void step(float deltaNum)
    {
        number += deltaNum;
        if (number >= length)
        {
            number -= length;
            over = true;
        }
    }

    bool isOver() const { return over; };
    float getNumber() const { return number; };
    float getLength() const { return length; };
    void reset()
    {
        number = 0;
        over = false;
    };
    void setOver(bool set) { over = set; };
    void setLength(float _length) { length = _length; };

private:
    float number;
    float length;
    bool over;
};
