#include "EventLoop.h"


EventLoop::EventLoop():ep_(new Epoll){}         // 创建epoll句柄（epoll的监视模块是一颗红黑树）
EventLoop::~EventLoop()
{
    delete ep_;
}
void EventLoop::run()
{
    while (true)                                 // 事件循环
    {
    std::vector<Channel*> channels=ep_->loop();  // 等待监视的fd有事件发生
    if(channels.size()==0)                                          // 如果有超时回调函数，就调用
    {
        timeoutcallback_(this);
    }
    for (auto& ch:channels)                      // 遍历epoll返回的数组evs
        {
            ch->handleevent();
        }
    }
}
Epoll* EventLoop::getep()
{
    return ep_;
}
void EventLoop::settimeoutcallback(std::function<void(EventLoop*)> fn)
{
    timeoutcallback_=fn;
}