#include "EventLoop.h"
EventLoop::EventLoop(bool mainloop):stop_(false),ep_(new Epoll),mainloop_(mainloop)
{
    if(mainloop_==false){
        wakeupfd_=eventfd(0,EFD_NONBLOCK|EFD_CLOEXEC);
        wakeup_channel=std::make_unique<Channel>(this,wakeupfd_);
        wakeup_channel->setreadcallback(std::bind(&EventLoop::handle_wakeup,this)); //设置wakeupchannel_的读回调函数
        wakeup_channel->enabreading(); //为wakeupfd_添加读事件
    }
}     
EventLoop::~EventLoop()
{
    delete ep_;
}
void EventLoop::run()
{
    thread_id = syscall(SYS_gettid); // 获取事件循环的线程id
    while (!stop_) // 事件循环
    {
        std::vector<Channel*> channels = ep_->loop(); // 等待监视的fd有事件发生
        if (channels.size() == 0 && timeoutcallback_) // 如果有超时回调函数，就调用
        {
            timeoutcallback_(this);
        }
        for (auto& ch : channels) // 遍历epoll返回的数组evs
        {
            ch->handleevent();
        }
    }
}
void EventLoop::stop()
{
    stop_=true;
    if(!mainloop_){
    wakeup();
}//因为事件循环函数run阻塞在ep_->loop()函数中，除非闹钟响了或者有事件发生epoll_wait才会立即返回，所有调用wakeup函数立即唤醒事件循环
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