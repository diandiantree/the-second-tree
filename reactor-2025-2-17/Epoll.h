#pragma once

#include <sys/epoll.h>
#include <stdio.h>
#include <stdlib.h>
#include <strings.h> 
#include <string.h> 
#include <string>
#include <errno.h>
#include <unistd.h>
#include <vector>
#include <iostream>
#include "Channel.h"

class Channel;

class Epoll{
private:
    static const int MaxEvents=100; //epoll_wait返回事件数组的大小
    int epollfd_=-1;                //epoll句柄
    epoll_event events[MaxEvents];
public:
    Epoll();                            //构造函数中创建epollfd_
    ~Epoll();                           //析构函数中关闭epollfd_
    // void addfd(int fd,uint32_t event);  //把fd和事件的类型加入到epollfd中
    void updatechannel(Channel *ch);    //把channel添加/更新到红黑树上，channel里面有fd，也有需要监视的事件
    void removechannel(Channel *ch);    //把channel从红黑树上删除
    void removechannel(int fd);
    std::vector<Channel*> loop(int timeout=-1);  //运行epoll_wait,等待事件的发生，已发生的事件用vector容器返回
};