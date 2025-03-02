#include "TimeStamp.h"

TimeStamp::TimeStamp()
{
    timer=time(0);
}
TimeStamp::TimeStamp(int64_t timer_)
{
    timer=timer_;
}

TimeStamp TimeStamp::Now()
{
    return TimeStamp();
}

time_t TimeStamp::toint() const
{
    return timer;
}
std::string TimeStamp::tostring() const
{
    return std::string(ctime(&timer));
}
std::string TimeStamp::todate() const
{
    return std::string(ctime(&timer)).substr(0,10);
}
std::string TimeStamp::totime() const
{
    return std::string(ctime(&timer)).substr(20,4)+"-"+std::string(ctime(&timer)).substr(11,8);
}