#pragma once

#include <sys/epoll.h>
#include "Epoll.h"
#include "Socket.h"
#include <functional>

class Epoll;
class Channel
{
private:
    int fd_;                //Channel拥有的fd,一个Channel只拥有一个fd
    Epoll *ep_=nullptr;     //Channel对应的红黑树,Channel与Epoll是多对一的关系，一个Channel只对应一个Epoll
    bool inepoll_=false;    //Channel是否已经添加到epoll树上,如果未添加,调用epoll_ctl()的时候使用EPOLL_CTL_ADD;否则使用EPOLL_CTL_MOD;
    uint32_t event_=0;      //fd_需要监视的事件.listenfd和clientfd需要监视EPOLLIN,clientfd还可能需要监视EPOLLOUT
    uint32_t revent_=0;     //fd_已发生的事件
    std::function<void()> readcallback_;                //fd_读事件的回调函数
    std::function<void()> closecallback_;               //连接断开，关闭fd_的回调函数，回调Connection::closecallback_()
    std::function<void()> errorcallback_;               //fd_发生错误的回调函数，回调Connection::errorcallback_()
    std::function<void()> writecallback_;               //fd_写事件的回调函数,回调Connection::writecallback_()
public:
    Channel(Epoll*ep,int fd);  
    ~Channel();            
    int fd();
    void useET();                  //采用边缘触发
    void enabreading();            //让epoll_wait()监视fd_的读事件，如果fd_可读，epoll_wait()返回，执行handleevent()
    void disabreading();           //让epoll_wait()不监视fd_的读事件
    void enabwriting();            //让epoll_wait()监视fd_的写事件
    void disabwriting();           //让epoll_wait()不监视fd_的写事件
    void setinepoll();             //把inepoll_成员的值设置为true
    void setrevents(uint32_t ev);  //设置revents_成员的值为参数ev
    bool inpoll();                 //返回inepoll_成员
    uint32_t event();              //返回event_成员
    uint32_t revent();             //返回revent_成员
    void handleevent();            //事件处理函数,epoll_wait返回的时候，执行它
    void setreadcallback(std::function<void()> fn);     //设置fd_读事件的回调函数
    void setclosecallback(std::function<void()> fn);    
    void seterrorcallback(std::function<void()> fn);    
    void setwritecallback(std::function<void()> fn);
};