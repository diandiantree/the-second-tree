#include "Tcpserver.h"
#include <memory>

//客户端与服务端的tcp连接断开后，还要再等一段时间，connection才会析构
//虽然connection析构的时候，会主动delete掉clientchannel和client_sock，但是有延迟
//所以在connection析构之前我们要把clientchannel主动从epoll中删除，要不然会一直触发读事件（水平触发：客户端连接已断开）
//所以我们要在connection析构之前，主动调用clientchannel->disableall()，把clientchannel从epoll中删除
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
Tcpserver::Tcpserver(const string& ip, uint16_t port, int Threadnum,int timeval_,int timeout_) : threadnum(Threadnum),timeval(timeval_),timeout(timeout_)
{
    main_eloop = make_unique<EventLoop>(true); //主事件循环
    main_eloop->settimeoutcallback(std::bind(&Tcpserver::epolltimeout, this, std::placeholders::_1)); //设置epoll_wait()超时的回调函数

    timer_fd=create_timerfd(timeval_);
    timer_channel=std::make_unique<Channel>(main_eloop.get(),timer_fd);
    timer_channel->setreadcallback(std::bind(&Tcpserver::handle_timer,this)); //设置timerchannel_的读回调函数
    timer_channel->enabreading(); //为timerfd_添加读事件

    acceptor_ = std::make_unique<Acceptor>(*main_eloop, ip, port);
    acceptor_->setnewconnection(std::bind(&Tcpserver::newconnection, this, std::placeholders::_1)); //不是线程安全的，因为传入了this指针，而this指针是指向当前对象的指针，但是如果此时有别的线程在调用Tcpserver的成员函数，就会出现问题，因为他还没有完全被构造出来

    tpl = &(Threadpool::Get_pl(threadnum)); //创建线程池，因为线程池是单例模式，所以这里不需要new
    //创建从事件循环，也就相当于创建任务队列中的任务
    for (int i = 0; i < threadnum; i++) {
        sub_eloops.emplace_back(make_unique<EventLoop>(false));
        sub_eloops[i]->settimeoutcallback(std::bind(&Tcpserver::epolltimeout, this, std::placeholders::_1)); //设置epoll_wait()超时的回调函数
        tpl->enqueue(std::bind(&EventLoop::run, sub_eloops[i].get())); //把从事件循环放到线程池中，线程池中的线程会去执行从事件循环的run()函数
    }
}

Tcpserver::~Tcpserver()
{
    stop();
}

void Tcpserver::start()
{
    main_eloop->run();
}
void Tcpserver::stop()
{
    //停止主事件循环与从事件循环
    main_eloop->stop();
    for(auto& sub:sub_eloops){
        sub->stop();
    }
    // 会自动结束IO线程
}
void Tcpserver::newconnection(int clientfd)
{
    auto connection = new Connection(sub_eloops[clientfd % threadnum].get(), clientfd);
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        connections[clientfd] = connection;//把conn放在map容器中
    }
    connection->setclosecallback(std::bind(&Tcpserver::closeconnection, this, std::placeholders::_1));
    connection->seterrorcallback(std::bind(&Tcpserver::errorconnection, this, std::placeholders::_1));
    connection->setmessagecallback(std::bind(&Tcpserver::onmessage, this, std::placeholders::_1, std::placeholders::_2));
    connection->setsendcompletioncallback(std::bind(&Tcpserver::sendcompletion, this, std::placeholders::_1));
}

void Tcpserver::closeconnection(Connection* connection)
{
    if (closeconnection_) {
        closeconnection_(connection);
    }
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        connections.erase(connection->fd());
    }
    delete connection; // 手动删除对象
}

void Tcpserver::errorconnection(Connection* connection)
{
    if (errorconnection_) {
        errorconnection_(connection);
    }
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        connections.erase(connection->fd());
    }
    delete connection; // 手动删除对象
}

void Tcpserver::onmessage(Connection* conn, std::string message)
{
    if (onmessage_) {
        onmessage_(conn, message);
    }
}

void Tcpserver::sendcompletion(Connection* conn)
{
    if (sendcompletion_) {
        sendcompletion_(conn);
    }
}

void Tcpserver::epolltimeout(EventLoop* loop)
{
    if (epolltimeout_) {
        epolltimeout_(loop);
    }
}

void Tcpserver::setcloseconnection(std::function<void(Connection*)> fn)
{
    closeconnection_ = std::move(fn);
}

void Tcpserver::seterrorconnection(std::function<void(Connection*)> fn)
{
    errorconnection_ = std::move(fn);
}

void Tcpserver::setmessagecallback(std::function<void(Connection*, std::string&)> fn)
{
    onmessage_ = std::move(fn);
}

void Tcpserver::setsendcompletioncallback(std::function<void(Connection*)> fn)
{
    sendcompletion_ = std::move(fn);
}

void Tcpserver::setepolltimeout(std::function<void(EventLoop*)> fn)
{
    epolltimeout_ = std::move(fn);
}
void Tcpserver::handle_timer()
{
    // 读取定时器文件描述符，防止一直触发
    uint64_t expirations;
    ssize_t n = read(timer_fd, &expirations, sizeof(expirations));
    if (n != sizeof(expirations))
    {
        perror("timerfd read error");
    }

    // 闹钟响后，重新开始计时
    struct itimerspec howlong;
    bzero(&howlong, sizeof(howlong));
    howlong.it_value.tv_sec = timeval;
    howlong.it_value.tv_nsec = 0;
    timerfd_settime(timer_fd, 0, &howlong, NULL);
    std::cout<<"闹钟响了"<<std::endl;

    time_t now = time(0); // 获取当前时间
    for (auto it = connections.begin(); it != connections.end(); )
    {
            if (it->second->timeout(now, timeout))
            {
                auto conn = it->second;
                std::cout<<"delete socket "<<conn->fd()<<std::endl;
                conn->setclosecallback(nullptr);
                conn->seterrorcallback(nullptr);
                conn->setmessagecallback(nullptr);
                conn->setsendcompletioncallback(nullptr);
                // for(auto &sub:sub_eloops){
                //     sub.get()->getep()->removechannel(conn->fd());//从epoll中删除监视的socket
                // }
                {
                    std::lock_guard<std::mutex> lck(m_mutex);
                    it = connections.erase(it); // 删除当前连接，并返回指向下一个元素的迭代器
                }
                delete conn; // 手动删除对象
            }
            else
            {
                ++it; // 移动到下一个元素
            }
        }
    
}