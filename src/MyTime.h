#pragma once
#include "time.h"

struct MyTime{
    int hour,min;
};

static bool timeEquals(tm tim, MyTime t)
{
    return tim.tm_hour == t.hour && tim.tm_min == t.min;
}

static bool timeEqualsOrGreater(tm tim, MyTime t)
{
    return tim.tm_hour > t.hour || (tim.tm_hour == t.hour && tim.tm_min >= t.min);
}

static bool timeEqualsOrSmaler(tm tim, MyTime t)
{
    return tim.tm_hour < t.hour || (tim.tm_hour == t.hour && tim.tm_min <= t.min);
}

static int getTimeDiff(int h1, int min1, int h2,int min2)
{
    int dif;
    int h = h1 - h2;
    int min = min1 - min2;
    if(min > 60)
    {
        h++;
        min -=60;
    }
    if(min < 0)
    {
        h--;
        min +=60;
    }
    return h*60 + min;
}

static int getTimeDiff(MyTime t1, MyTime t2)
{
    return getTimeDiff(t1.hour,t1.min, t2.hour,t2.min);
}

static int getTimeDiff(tm t1, MyTime t2)
{
    return getTimeDiff(t1.tm_hour,t1.tm_min,t2.hour,t2.min);
}

static bool timeInRange(MyTime * start, MyTime * end, tm inRange)
{
    bool ret = false;
    if(start->hour > end->hour)
    {
        if((inRange.tm_hour > start->hour && inRange.tm_hour <= 23) || (inRange.tm_hour >= 0 && end->hour > inRange.tm_hour))
            return true;
        else if((inRange.tm_hour == start->hour && inRange.tm_min >= start->min && inRange.tm_hour <= 23) || (inRange.tm_hour >= 0 && end->hour == inRange.tm_hour && inRange.tm_min <= end->min))
            return true;
        else
            return false;
    }
    else if(inRange.tm_hour > start->hour && inRange.tm_hour < end->hour)
        return true;
    else if((inRange.tm_hour == start->hour && inRange.tm_min >= start->min) || (inRange.tm_hour == end->hour && inRange.tm_min <= end->min))
        return true;
    return false;
}