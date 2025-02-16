#pragma once

#include "Epoll.h"

class EventLoop
{
private:
    Epoll *ep_;     //每个事件循环只有一个Epoll
    std::function<void(EventLoop*)> timeoutcallback_;  //超时回调函数
public:
    EventLoop();    //在构造函数中创建Epoll对象ep_
    ~EventLoop();   //在析构函数中销毁ep_
    void run();     //运行事件循环
    Epoll* getep();
    void settimeoutcallback(std::function<void(EventLoop*)> fn);  //设置超时回调函数
};