# 2025/2/16

1、将线程池与单线程的reactor模型结合起来，构成了多线程的reactor模型，在multi-reactor文件夹中，原有的合并在single-reactor中
原因:(1)单线程的Reactor模型不能发挥多核CPU的性能
     (2)运行多个事件循环，主事件循环运行在主线程中，从事件循环(EventLoop::run())运行在线程池中
     (3)主线程负责创建客户端连接，然后把conn分配给线程池，2和3主要使得线程之间分工更加明确
2、增加完主从线程后，进一步讲从线程分为IO线程和工作线程，其中IO线程负责从inputbuffer读取message并发送给outputbuffer，而工作线程只负责处理业务（现在的业务只是消息的计算）
原因：如果把处理业务和IO都交给从线程，处理业务如果很大，会造成阻塞，这时候就不能即时响应新的connection的连接了
3、使用智能指针管理资源，使用shared_ptr管理共享资源：，使用unique_ptr管理个人资源
(1)共享资源：Connection 这里用到了别名using spConnection=std::make_shared<Connection>;
(2)如何在类中使用智能指针而非this指针，让类去继承一个模板类：public std::enable_shared_from_this<类名>，这样在该类中可以调用成员函数shared_from_this()
4、增加了异步唤醒事件循环
原因:现有代码逻辑为：
(1)IO线程负责从接收缓冲区中按照报文大小读取报文然后交给工作线程去处理业务
(2)工作线程处理完业务之后将报文发送给发送缓冲区，同时注册channel的写事件，代表可以发了
(3)然后IO线程从发送缓冲区中发送报文给对端
这样就产生了一个问题：
(4)即IO线程和工作线程会同时操作发送缓冲区，造成资源竞争，可以加锁，但是加锁耗费的资源太大，采取另外一种方案
(5)可以让工作线程处理完业务后，把报文发送给发送缓冲区的任务交给IO线程去处理，如何交给ta
处理方案：
(6)采用eventfd，通知线程的方法有很多种（如socket、条件变量、eventfd、管道、信号量等）
(7)但是eventfd可以被epoll监视，所以采用它
(8)一个EventLoop就对应着一个事件循环，在EventLoop里面增加一个任务队列，同时在Connection的sendmessage函数里增加判断，如果只有IO线程就直接让IO线程去执行了，IO线程会阻塞在事件循环的epoll_wait函数中，如何唤醒呢，让epoll_wait监视eventfd
(9)如果没有IO线程，就把这个函数当作一个任务去交给IO线程的任务队列，通过EventLoop里的增加任务函数，同时增加唤醒函数，即向eventfd中写入数据，epoll会触发读事件，这时候只需要提前设置好相应的读事件的处理函数（从任务队列中取出数据，然后执行）即可
5、增加定时器定时清理空闲的connection对象
(1)传统的做法:alarm()函数可设置定时器，触发SIGALRM信号。
(2)新版Linux内核把定时器和信号抽象为fd，让epoll统一监视。
做法：
(3)在事件循环中增加map<int,spConnect>conns 容器，存放运行在该事件循环上全部的Connection对象
(4)如果闹钟时间到了，遍历conns，判断每个Connection对象是否超时
(5)如果超时了，从conns 中删除Connection对象（需要加锁，主线程会向容器中添加connection对象，而从线程会从容器中删除connection对象）
(6)再从TcpServer.conns 中删除Connection对象（需要加锁，主线程会向容器中添加connection对象，而从线程会从容器中删除connection对象，也就是说TcpServer类的容器和EventLoop类的容器同在newconnection函数中添加connection对象）

# 2025/2/17

1、更改了新连接输出日志的显示位置，新的socket连接上来之后，将在Socket::accept()函数中调用EchoServer::showinfo()函数，显示连接信息

2、采用信号让服务程序体面的退出，原本都是ctrl+c强行终止服务程序：
(1)在EchoServer中增加stop函数，在stop函数中调用Tcpserver::stop()函数，用于停止事件循环，即将EventLoop的标志位设置为true;

(2)同时在main函数中采用智能指针管理EchoServer对象，用裸指针会报错？暂不清楚原因

(3)在main函数中增加信号处理函数，处理收到2和15的信号，在里面调用EchoServer::stop()

(4)由于线程池采用单例模式，因此不必手动delete线程池指针

3、测试了一下性能:

在IO线程为2，工作线程为0的情况下，大约40秒可以处理10个客户端同时发送100万个报文并回复。虚拟机配置(4核心，2G内存)

4、在Channel的析构函数中会选择调用remove,这样在删除connection连接后也可以把对应的clinetfd从epoll树上删去

# 2025/3/2

1、将connection的shared_ptr()改为普通指针管理，因为使用shared_ptr()常出现bad_weak_ptr错误

2、改为普通指针后，又出现回调函数错误，因此在每个回调函数之前都增加if判断，回调函数不为空才执行回调函数

3、又一次做了性能测试：

在两台虚拟机上分别做了压力测试，同时运行50个客户端程序，每个客户端都向服务端发送10w个报文，服务端接收到报文之后，再加上`"reply"`发送回去

服务端虚拟机配置是4G内存，2颗2核cpu

平均用时约为4min10s，也就是250s可以处理500w个报文

做完报文测试后，不关闭服务端程序，做连接测试，同时连接500个客户端程序，查看服务端程序会不会崩溃或者出bug，当然每个客户端只发送10条报文

程序正常运行，没有出错！

4、增加了程序的流程图，UML类图

5、增加了muduo库迭代日志