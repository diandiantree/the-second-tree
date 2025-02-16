#pragma once

#include <iostream>
#include <ctime>
#include <string>

class TimeStamp
{
public:
    TimeStamp();                 //用当前时间初始化对象
    TimeStamp(int64_t timer_);   //用一个整数表示的时间初始化对象

    static TimeStamp Now();      //获取当前时间

    time_t toint() const;        //返回一个整数表示的时间
    std::string tostring() const;   //返回一个字符串表示的时间
    std::string todate() const;   //返回一个字符串表示的日期
    std::string totime() const;   //返回一个字符串表示的时间
private:
    time_t timer;//整数表示的时间，从1970年1月1日0时0分0秒到现在的秒数
};