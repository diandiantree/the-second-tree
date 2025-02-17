#include "Epoll.h"

Epoll::Epoll()
{
    epollfd_=epoll_create(1);
    if(epollfd_==-1)
    {
        std::cout<<"epoll_create() failed"<<errno<<std::endl;
        exit(-1);
    }
}                         
Epoll::~Epoll()
{
    close(epollfd_);
}  
void Epoll::updatechannel(Channel *ch)              // 更新epoll树上的fd
{
    struct epoll_event ev;                            // 声明事件的数据结构。
    ev.data.ptr=ch;                                   // 指定事件的自定义数据，会随着epoll_wait()返回的事件一并返回
    ev.events=ch->event();                            // 让epoll监视listenfd的读事件，采用水平触发
    if(ch->inpoll())                                  // 如果此fd已经在树上了，就更新
    {
    if(epoll_ctl(epollfd_,EPOLL_CTL_MOD,ch->fd(),&ev)==-1) 
    {
        std::cout<<"epoll_ctl_mod failed"<<errno<<std::endl;
        exit(-1);
    }
    }
    else{                                                   // 没有在树上，就加入，注意用的宏不一样
    if(epoll_ctl(epollfd_,EPOLL_CTL_ADD,ch->fd(),&ev)==-1) 
    {
        std::cout<<"epoll_ctl_add failed"<<errno<<std::endl;
        exit(-1);
    }
    ch->setinepoll();                                       //添加完之后在树上了，把ch的inepoll_设置为true
    }
}                      
std::vector<Channel*> Epoll::loop(int timeout)              // 等待监视的fd有事件发生
{
    std::vector<Channel*> Channels;   //存放epoll_wait返回的事件
    bzero(events,sizeof(events));
    int infds=epoll_wait(epollfd_,events,MaxEvents,timeout);       // 等待监视的fd有事件发生。填-1表示不关心超时时间,填0表示立即返回，填大于0的数表示等待的毫秒数
                                                                   // 如果有事件发生，就返回发生事件的个数，如果没有事件发生，就返回0，如果出错，就返回-1
                                                                   // events是一个数组，用来存放发生的事件，传入空的events数组，epoll_wait()会把发生的事件填充到这个数组中
    // 返回失败
    if (infds < 0)
    {
        perror("epoll_wait() failed");
        exit(-1);
    }
    // 超时
    if (infds == 0)
    {
        printf("epoll_wait() timeout.\n");
        return Channels;
    }
    //如果有事件发生
    for(int i=0;i<infds;i++)
    {  
        Channel* ch=(Channel*)events[i].data.ptr;
        ch->setrevents(events[i].events);
        Channels.emplace_back(ch);
    }
    return Channels;
}
void Epoll::removechannel(Channel *ch)              // 从epoll树上删除fd
{
    std::cout<<"removeChannel"<<std::endl;
    if(ch->inpoll())                                // 如果fd在树上，就删除
    {
    if(epoll_ctl(epollfd_,EPOLL_CTL_DEL,ch->fd(),nullptr)==-1) 
    {
        std::cout<<"epoll_ctl_del failed"<<errno<<std::endl;
        exit(-1);
    }
    }
}
void Epoll::removechannel(int fd)              // 从epoll树上删除fd
{
    std::cout<<"removefd"<<std::endl;
                                  // 如果fd在树上，就删除
    if(epoll_ctl(epollfd_,EPOLL_CTL_DEL,fd,nullptr)==-1) 
    {
        std::cout<<"epoll_ctl_del failed"<<errno<<std::endl;
        exit(-1);
    }
}
