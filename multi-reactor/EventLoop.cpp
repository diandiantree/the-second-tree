#include "EventLoop.h"

int create_timerfd(int sec)
{
    int timerfd=timerfd_create(CLOCK_MONOTONIC,TFD_NONBLOCK|TFD_CLOEXEC);
    if(timerfd<0)
    {
    perror("timerfd_create error");
    }
    struct itimerspec howlong;          //设置超时时间
    bzero(&howlong,sizeof(howlong));    //清零
    howlong.it_value.tv_sec=sec;          //5s后超时
    howlong.it_value.tv_nsec=0;         //纳秒
    timerfd_settime(timerfd,0,&howlong,NULL); //设置超时时间
    return timerfd;
}
EventLoop::EventLoop(bool mainloop,int time_tvl,int time_out_):ep_(new Epoll),wakeupfd_(eventfd(0,EFD_NONBLOCK)),wakeup_channel(new Channel(this,wakeupfd_)),
                                                            timerfd_(create_timerfd(time_tvl)),timer_channel(new Channel(this,timerfd_)),mainloop_(mainloop),time_out(time_out_)
{
    wakeup_channel->setreadcallback(std::bind(&EventLoop::handle_wakeup,this)); //设置wakeupchannel_的读回调函数
    wakeup_channel->enabreading(); //为wakeupfd_添加读事件
    timer_channel->setreadcallback(std::bind(&EventLoop::handle_timer,this)); //设置timerchannel_的读回调函数
    timer_channel->enabreading(); //为timerfd_添加读事件
}     
EventLoop::~EventLoop()
{
}
void EventLoop::run()
{
    thread_id=syscall(SYS_gettid);               //获取事件循环的线程id
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
bool EventLoop::is_inloop_thread()
{
    return thread_id==syscall(SYS_gettid);//即是否为IO线程
}
void EventLoop::addtask(std::function<void()> fn)
{
    {
    std::lock_guard<std::mutex> lock(mutex_);
    task_queue.push(fn);
    }
    //唤醒IO线程，用eventfd唤醒
    wakeup();
}
void EventLoop::wakeup()
{
    uint64_t one=1;
    ssize_t n=write(wakeupfd_,&one,sizeof(one));
    if(n!=sizeof(one))
    {
    perror("wakeup write error");
    }
}
void EventLoop::handle_wakeup()
{
    uint64_t one=1;
    ssize_t n=read(wakeupfd_,&one,sizeof(one));//从eventfd中读取数据，如果不读取，eventfd会一直触发
    if(n!=sizeof(one))
    {
    perror("wakeup read error");
    }
    std::function<void()> fn;
    std::lock_guard<std::mutex> lock(mutex_);
    while(!task_queue.empty())
    {
        fn=std::move(task_queue.front());
        task_queue.pop();
        fn();
    }
}
void EventLoop::handle_timer()
{
    //闹钟响后，重新开始计时
    struct itimerspec howlong;          //设置超时时间
    bzero(&howlong,sizeof(howlong));    //清零
    howlong.it_value.tv_sec=5;      //5s后超时
    howlong.it_value.tv_nsec=0;         //纳秒
    timerfd_settime(timerfd_,0,&howlong,NULL); //设置超时时间
    if(mainloop_)
    {
        
    }
    else
    {
        time_t now=time(0);     //获取当前时间
        for (auto it = conns.begin(); it != conns.end(); )
        {
            if (it->second->timeout(now, time_out))
            {
                auto conn = it->second;
                {
                std::lock_guard<std::mutex> lck(M_mutex);
                it = conns.erase(it); // 删除当前连接，并返回指向下一个元素的迭代器
                }
                close_callback(conn);
                // ep_->removechannel(conn->fd());//没有删除epoll树上的fd，这样直接删是有问题的，如果只是因为超时去删除，没问题，但是如果因为客户端主动断开连接，已经在epoll树上删了fd就会报错
            }
            else
            {
                ++it; // 移动到下一个元素
            }
        }
    }
}
void EventLoop::add_connection(spConnection conn)
{
    std::lock_guard<std::mutex> lck(M_mutex);
    conns[conn->fd()]=conn;
}
void EventLoop::set_close_callback(std::function<void(spConnection)> fn)
{
    close_callback=fn;
}