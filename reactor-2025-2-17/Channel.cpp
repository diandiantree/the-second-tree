#include "Channel.h"
#include "EventLoop.h"
#include "Epoll.h"

Channel::Channel(EventLoop* loop, int fd) : loop_(loop), fd_(fd) {}
Channel::~Channel() //在析构函数中,不要销毁ep_,也不能关闭fd_，因为这两个东西不属于Channel类,Channel类只是需要他们使用它们而已
{
    if(inepoll_)
    {remove();}
}
int Channel::fd() { return fd_; }
void Channel::useET() { event_ = event_ | EPOLLET; }
void Channel::enabreading()
{
    event_ = event_ | EPOLLIN;                      // 为fd_添加读事件
    loop_->getep()->updatechannel(this);            // 把fd_添加到epoll树上
}
void Channel::enabwriting()
{
    event_ = event_ | EPOLLOUT;                      // 为fd_添加写事件
    loop_->getep()->updatechannel(this);             // 把fd_添加到epoll树上
}
void Channel::setinepoll() { inepoll_ = true; }
void Channel::setoutepoll()
{
    inepoll_ = false;
}
void Channel::setrevents(uint32_t ev) { revent_ = ev; }
bool Channel::inpoll() { return inepoll_; }
uint32_t Channel::event() { return event_; }
uint32_t Channel::revent() { return revent_; }
void Channel::handleevent()
{
    if (revent() & EPOLLRDHUP)                     // 对方已关闭，有些系统检测不到，可以使用EPOLLIN，recv()返回0。
    {
        remove();                                  // 从epoll树上删除fd_
        closecallback_();
    }                                              //  普通数据  带外数据
    else if (revent() & (EPOLLIN | EPOLLPRI))        // 接收缓冲区中有数据可以读。
    {
        readcallback_();                           //使用了函数回调，listenfd的channel和clientfd的channel的回调函数不同，提前设置好了
    }
    else if (revent() & EPOLLOUT)                  // 有数据需要写，暂时没有代码，以后再说
    {
        writecallback_();                           //使用了函数回调
    }
    else                                           // 其它事件，都视为错误
    {
        remove();                                // 从epoll树上删除fd_
        errorcallback_();
    }
}
void Channel::setreadcallback(std::function<void()> fn)
{
    readcallback_ = fn;
}
void Channel::setclosecallback(std::function<void()> fn)
{
    closecallback_ = fn;
}
void Channel::seterrorcallback(std::function<void()> fn)
{
    errorcallback_ = fn;
}
void Channel::setwritecallback(std::function<void()> fn)
{
    writecallback_ = fn;
}
void Channel::disabreading()
{
    event_ = event_ & ~EPOLLIN;                      // 为fd_删除读事件
    loop_->getep()->updatechannel(this);                    // 把fd_从epoll树上跟更新
}
void Channel::disabwriting()
{
    event_ = event_ & ~EPOLLOUT;                  // 为fd_删除写事件
    loop_->getep()->updatechannel(this);                 // 把fd_从epoll树上更新
}
void Channel::disableall()
{
    event_ = 0;                                // 为fd_删除所有事件
    loop_->getep()->updatechannel(this);                // 把fd_从epoll树上更新
}
void Channel::remove()
{
    disableall();                           // 为fd_删除所有事件
    setoutepoll();
    loop_->getep()->removechannel(this);                // 从红黑树上删除fd_
}