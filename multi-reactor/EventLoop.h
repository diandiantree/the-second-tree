#pragma once
#include "Epoll.h"
#include <queue>
#include <mutex>
#include <functional>
#include <unistd.h>
#include <sys/eventfd.h>
#include <sys/syscall.h>
#include "Channel.h"
#include <memory>
#include <map>
#include <sys/timerfd.h> //定时器
#include "Connection.h"

class Connection;
using spConnection=std::shared_ptr<Connection>;
class EventLoop
{
private:
    int timetvl;        //多少秒检查一下闹钟
    int time_out;       //多少秒超时
    Epoll* ep_;     //每个事件循环只有一个Epoll
    std::function<void(EventLoop*)> timeoutcallback_;  //超时回调函数
    pid_t thread_id;    //线程id
    std::queue<std::function<void()>> task_queue;  //任务队列
    std::mutex mutex_;  //任务队列同步的互斥锁
    int wakeupfd_;      //用于唤醒IO现场的eventfd
    std::unique_ptr<Channel> wakeup_channel;   //用于唤醒IO线程的channel
    int timerfd_;       //定时器fd
    std::unique_ptr<Channel> timer_channel;    //定时器channel
    bool mainloop_;     //是否是主事件循环
    std::mutex M_mutex; //保护conns的互斥锁
    std::map<int,spConnection> conns;
    std::function<void(spConnection)> close_callback;
public:
    EventLoop(bool mainloop,int time_tvl=5,int time_out_=10);    //在构造函数中创建Epoll对象ep_
    ~EventLoop();   //在析构函数中销毁ep_
    void run();     //运行事件循环
    Epoll* getep();
    void settimeoutcallback(std::function<void(EventLoop*)> fn);  //设置超时回调函数
    bool is_inloop_thread();        //判断当前线程是否为事件循环线程
    void addtask(std::function<void()> fn);     //添加任务到任务队列
    void wakeup();  //唤醒IO线程
    void handle_wakeup(); //处理wakeupfd_的读事件
    void handle_timer();  //处理定时器事件
    void add_connection(spConnection conn); //添加一个连接
    void set_close_callback(std::function<void(spConnection)> fn);
};