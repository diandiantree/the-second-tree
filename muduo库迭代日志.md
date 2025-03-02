# muduo库迭代日志

## 1、最简单的Socket编程

### Socket是什么？

套接字。为不同主机或同一主机之间的不同进程提供了一种通信机制，是网络编程接口。

定义

Socket 是对网络中不同主机上的应用进程之间进行双向通信的端点的抽象。它是网络编程接口，封装了底层复杂的网络通信细节，让开发者可以方便地进行网络数据的发送和接收。

工作模式

**C/S（客户端 / 服务器）模式**：这是最常见的模式。服务器端程序会在指定的 IP 地址和端口上监听客户端的连接请求。客户端程序则向服务器的 IP 地址和端口发起连接请求，一旦连接建立，双方就可以通过各自的 Socket 进行数据交换。例如，我们日常使用的网页浏览，浏览器就是客户端，网站服务器就是服务器端。

**P2P（对等）模式**：在这种模式下，各个节点之间地位平等，每个节点既可以作为客户端向其他节点发起连接，也可以作为服务器接受其他节点的连接请求，实现直接的数据交互。比如一些文件共享软件就采用了 P2P 模式。

分类

- **流式套接字（SOCK_STREAM）**：基于 TCP（传输控制协议）实现
- **数据报套接字（SOCK_DGRAM）**：基于 UDP（用户数据报协议）实现

工作流程

服务器端

1. **创建 Socket**：使用 `socket()` 函数创建一个套接字对象。
2. **绑定地址**：使用 `bind()` 函数将套接字与指定的 `IP` 地址和端口号绑定。
3. **监听连接**：使用 `listen()` 函数开始监听客户端的连接请求。
4. **接受连接**：使用 `accept()` 函数接受客户端的连接请求，返回一个新的套接字用于与该客户端进行通信。
5. **数据收发**：使用 `send()` 和 `recv()` 等函数进行数据的发送和接收。
6. **关闭连接**：使用 `close()` 函数关闭套接字，释放资源。

客户端

1. **创建 Socket**：同样使用 `socket()` 函数创建套接字。
2. **连接服务器**：使用 `connect()` 函数向服务器的 IP 地址和端口号发起连接请求。
3. **数据收发**：使用 `send()` 和 `recv()` 等函数与服务器进行数据交互。
4. **关闭连接**：使用 `close()` 函数关闭套接字。

### 创建单个socket到recv()函数阻塞再到内核接收数据的过程

1、当执行完socket函数后，操作系统会创建一个由文件管理系统的socket对象

2、该socket对象有接收缓冲区，发送缓冲区以及等待队列

3、当执行到`recv()`函数后，该进程会变为阻塞态，同时该进程被添加进socket的等待队列中

4、当网卡收到网线传过来的数据后，同时给cpu发送中断信号

5、cpu接收了中断信号，会执行中断程序

6、执行的中断程序有两个作用：1）把网卡中收到的数据写入内存后，再从内存写入socket的接收缓冲区

​						     2）唤醒socket等待队列中的进程，让它变为就绪态，即放入工作队列中，等待cpu的调度

7、`recv()` 函数会从 Socket 的接收缓冲区中读取数据，并将其复制到应用程序指定的内存区域

8、操作系统如何知道网络数据对应于哪个socket？

因为一个socket对应着一个端口号，而网络数据包中包含了 IP 和端口的信息，内核可以通过端口号找到对应的socket。当然，为了提高处理速度，操作系统会维护端口号到socket的索引结构，以快速读取。

## 2、`epoll`登场

上节存在问题：服务端不支持多个客户端与之连接并通信，使用`epoll`可以解决这个问题，先有select后有epoll

### `select`的流程

1、创建一个fds数组，数组里面是需要监视的socket的fd

2、将数组传给`select`，`select`就会监视这些socket，当某个socket有事件发生后，select就会返回，否则就阻塞

3、在`select`监视socket的时候，即调用`select`之后，操作系统就会把该进程分别加入到所有需要监视的等待队列中

4、当任意一个socket收到数据之后，中断程序就会唤醒所有进程（将进程从所有等待队列里面移除，加入到工作队列中去）

这种方法很简单粗暴，但存在三个缺点：

1、每次调用select都需要将进程加入到所有监视socket的等待队列，每次唤醒都需要从每个队列中移除。这里涉及了两次遍历，而且每次都要将整个 FDS 列表传递给内核，有一定的开销（其实FDS被copy了两次，由于select会改变FDS(bitmap)，所以用户态会copy一份，且传给内核再次copy一份）

2、只要有一个socket有事件，select就会返回，开销非常大

3、返回后进程也不知道那个socket有事件，还需要遍历FDS数组

### `epoll`原理

1、当某个进程调用`epoll_create()`后，操作系统会创建一个`eventpoll`对象，并返回一个文件描述符`epoll`的fd

2、`eventpoll`里面有监视列表和就绪队列，通过`epoll_ctl()`将需要监视的socket的fd加入到监视列表中，同时内核会把`eventpoll`加入到socket的等待队列中（select是把进程加入到等待队列中）

3、程序执行到`epoll_wait()`时，内核会将进程放入 `eventpoll` 的等待队列中，阻塞进程。当socket接收到数据，中断程序一方面修改就绪队列，另一方面唤醒 `eventpoll` 等待队列中的进程，进程再次进入运行态，如果就绪队列为空，阻塞进程

4、就绪队列是双向链表，监视列表是一颗红黑树

5、阻塞解除，并将这些已就绪的文件描述符通过一个结构体传出来，接着对传出的文件描述符进行判断：

1）若是监听的文件描述符，就调用accept()建立连接，此时不会阻塞，因为这文件描述符是已就绪的。

2）如是通信的文件描述符，就调用通信函数`read() write()`和已建立连接的客户端进行通信。

这样就可以在单线程的场景下实现服务器并发了

水平触发（Level Triggered，LT）

- **触发逻辑**：在水平触发模式下，只要文件描述符（如 `socket`）处于就绪状态（可读或可写），`epoll_wait` 就会持续通知应用程序。也就是说，只要读缓冲区中有数据未被读取，或者写缓冲区有空间可写，再次调用 `epoll_wait` 时它都会立即返回。
- **就绪队列情况**：当 `socket` 上的事件就绪时，该 `socket` 会被添加到 `eventpoll` 的就绪队列中。即使应用程序在一次 `epoll_wait` 返回后没有完全处理完该 `socket` 上的事件，它也不会从就绪队列中移除。只要条件满足（即读事件或写事件未处理完），下次 `epoll_wait` 检查时，这个 `socket` 依然会在就绪队列里，从而使 `epoll_wait` 再次返回。

边缘触发（Edge Triggered，ET）

- **触发逻辑**：边缘触发模式下，只有当文件描述符的状态发生变化时，`epoll_wait` 才会通知应用程序。具体来说，就是当 `socket` 从不可读变为可读，或者从不可写变为可写时，`epoll_wait` 才会返回，并且仅在状态变化的那个瞬间通知一次。
- **就绪队列情况**：当 `socket` 的状态发生变化变为就绪状态时，它会被添加到 `eventpoll` 的就绪队列中。一旦 `epoll_wait` 返回并将该事件告知应用程序后，这个 `socket` 就会从就绪队列中移除。后续即使该 `socket` 仍然处于就绪状态（例如读缓冲区还有数据），如果没有新的状态变化，`epoll_wait` 也不会再次返回。所以在边缘触发模式下，应用程序需要一次性把 `socket` 上的事件处理完，否则可能会导致数据丢失或处理不及时。

## 3、开始添加类，模块化

随着代码越来越多，程序越来越复杂且不易懂，而且一些代码不应该出现在main函数中，main函数只是为了开发业务，一些底层的代码应该用类封装起来

### 增加`Socket`类、`InetAddress`类、`Epoll`类

`InetAddress`类：存放ip地址和端口

`Socket`类：创建socket对象，把ip地址和端口绑定，负责监听，以及接收客户端连接

`Epoll`类：封装了`epoll`的一些功能，创建`epoll`，把需要监视的socket加入`epoll`以及从`epoll`树上删除socket，更新`epoll`需要监视的socket的读或写事件，运行`epoll_wait()`，如果有事件发生就返回一个`Channel*`数组

## 4、添加Channel类

如果说之前的代码封装的目的在于让代码更简洁易懂，那增加Channel类的原因可能不一样了

在我们调用完`epoll_create()`之后，会使用`epoll_ctl()`将需要监视的socket加入到红黑树上，`epoll_ctl()`有四个参数，第一个是`epoll`树的fd，第二个是一个宏，表示从`epoll`树上添加还是更改还是删除socket，第三个是需要监视的socket的fd，第四个则是一个结构体`epoll_event`

```c++
// 这是联合体, 多个变量共用同一块内存        
typedef union epoll_data {
    void        *ptr;
    int          fd;    // 通常情况下使用这个成员, 和epoll_ctl的第三个参数相同
    uint32_t     u32;
    uint64_t     u64;
} epoll_data_t;

struct epoll_event {
    uint32_t     events;      /* Epoll events Epoll需要监视的事件*/
    epoll_data_t data;        /* User data variable*/
};
int epoll_ctl(int epfd, int op, int fd, struct epoll_event *event);


```



一般情况下，我们会设置event里面的data的fd成员，但这样我们只使用联合体中成员fd,只能得到文件描述符。当我们想要获取关于该fd更多的信息时候，就不妥了。比如想获取这个socket是否以及在红黑树上了

那么，我们换一种思路，联合体`epoll_data`中还有个变量ptr,其是个`void*`指针。那就可以指向任何一块地址空间，也可以指向一个类对象，这就可以包含关于这个文件描述符的更多信息。


Channel类

```cpp
class Channel
{
private:
    int fd_;                	   //Channel拥有的fd,一个Channel只拥有一个fd
    EventLoop* loop_=nullptr;      //Channel对应的红黑树,Channel与Epoll是多对一的关系，一个Channel只对应一个Epoll
    bool inepoll_=false;    	  //Channel是否已经添加到epoll树上,如果未添加,调用epoll_ctl()的时候使用EPOLL_CTL_ADD;否则使用EPOLL_CTL_MOD;
    uint32_t event_=0;      	 //fd_需要监视的事件.listenfd和clientfd需要监视EPOLLIN,clientfd还可能需要监视EPOLLOUT
    uint32_t revent_=0;     	 //fd_已发生的事件
    std::function<void()> readcallback_;                //fd_读事件的回调函数
    std::function<void()> closecallback_;               //连接断开，关闭fd_的回调函数，回调Connection::closecallback_()
    std::function<void()> errorcallback_;               //fd_发生错误的回调函数，回调Connection::errorcallback_()
    std::function<void()> writecallback_;               //fd_写事件的回调函数,回调Connection::writecallback_()
public:
    Channel(EventLoop* loop,int fd);     //Channel是Accepter和Connection的基类，所以构造函数中有Epoll* ep参数
    ~Channel();            
    int fd();
    void useET();                  //采用边缘触发
    void enabreading();            //让epoll_wait()监视fd_的读事件，如果fd_可读，epoll_wait()返回，执行handleevent()
    void disabreading();           //让epoll_wait()不监视fd_的读事件
    void enabwriting();            //让epoll_wait()监视fd_的写事件
    void disabwriting();           //让epoll_wait()不监视fd_的写事件
    void disableall();             //让epoll_wait()不监视fd_的任何事件
    void remove();          	  //从事件循环中删除channel
    void setinepoll();             //把inepoll_成员的值设置为true
    void setoutepoll();
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
```

### `Channel`的成员变量

`fd_` 文件描述符FD,每个Channel对象自始至终只负责一个fd，但是它不拥有这个fd,即是也不会在析构的时候关闭这个fd；

`loop_` 一个`Epoll`类对象，后面升级为`EventLoop`对象，`EventLoop`是对`Epoll`的进一步封装，是`Epoll`的上层类，我们要通过`epoll`把fd添加到`epoll`上，一个`Channel` 只跟着一个`Epoll`,通过`enabreading()`函数添加FD到该`epoll`上。

`event_` 是`Channel`关心的IO事件，由用户设置(即是用户想监听该FD的哪类事件，如读事件)

`revent_` 是通过`epoll_wait()`返回的目前该fd活动的事件(即是我们要根据这个类型执行相对的操作,返回读事件就执行读操作)。

`inepoll_` 是用于判断该fd是否在`epoll`红黑树上，进而判断是使用`EPOLL_CTL_ADD`还是使用`EPOLL_CTL_MOD`

一个 `Channel` 可以对应一个客户端或服务器端的 `socket`，通过 `Channel` 类可以方便地管理和处理这些 `socket` 的事件。例如，在服务器端，对于监听 `socket` 和客户端 `socket` 可以分别创建不同的 `Channel` 对象，设置不同的事件和回调函数，实现对不同 `socket` 的精细化管理。

### `Channel`的使用

通过五个成员函数，`enabreading()`  `disabreading()` `enabwriting()` `disabwriting()` `disableall()` 

他们的逻辑都是一样的，设置好需要监视的事件，然后调用`epoll`的`updateChannel()` 去添加或者更新`epoll`树上的fd

`EPOLL`的`loop()`函数代码也有修改，有了`Channel`之后，返回的就是`vector<Channel*>`

## 5、添加类`EventLoop`，增加回调函数

### 为什么要增加`EventLoop`类？

1、`Epoll`类属于底层类，需要对其进一步封装

2、设计`Epoll`类的主要目的是为了让它管理红黑树，如添加、修改和删除Channel，创建或者关闭红黑树；但后续我们为了实现多线程和reactor模式，需要对事件循环传入更多的信息，如定时器`timerfd`，唤醒线程的`wakeupfd`（`eventfd`）等，还有获取当前红黑树循环所在的线程id等等，这些额外的信息不应该由`Epoll`类来管理，`Epoll`只负责红黑树就好了，多余的信息进一步封装为`EventLoop`类

`EventLoop`里的成员变量现在只有`Epoll* ep_`，并在其构造函数中`new`出来，并在析构函数中`delete`

### 修改`Channel`类

1、`Channel`类中原本有`Epoll*`，现在修改为`EventLoop*`

2、设置读事件的回调函数，由于服务端负责监听的Channel和客户端连接上来的Channel发生的读事件不一样，前者发生的读事件是指有新的Socket连接上来，后者则是对端有数据发送过来或者连接已断开，因此需要对不同的Channel设置不同的回调函数（由于现在的`Acceptor`和`Connection`类还没有创建出来，后续的`serverchannel` （对应`listen_socket`）和`clientchannel` （对应`client_socket`）的回调函数是分别在这两个类中设置的）

3、设置完之后，在Channel类中调用`handle_event()`函数，而这个函数在`EventLoop`中的`run()`被调用，逻辑是运行事件循环，如果有事件发生，会返回发生事件的`vector<Channel*>`，然后让`Channel`去自己处理这个事件

## 6、增加`Acceptor`类、`Connection`类

其实是先有的`TcpServer`类，再从`TcpServer`类中分离出的`Acceptor`类和`Connection`类

### `TcpServer`类简述

这个类是负责管理整个服务端，如创建监听的`socket`，创建客户端连接上来的`socket`，以及处理客户端发来的消息，同时设置一些回调函数，后续使用了多线程，因此还负责创建主从`EventLoop`

### 从`TcpServer`类中分离出`Acceptor`和`Connection`

`Acceptor`负责接收新的客户端连接（`create->bind->listen(listen_socket)`），同时会创建`serverchannel`并加入到`epoll`树上，新的连接到来后会回调`Tcpserver::newconnection()`函数

在`Tcpserver::newconnection()`函数里，会创建一个`Connection`对象

所以`Acceptor`和`Connection`中都有`EventLoop*`、`Socket*`以及`Channel*`，由于Connection是对已连接的socket的处理，因此后续还会增加更多对于`client_socket`的处理，所以总结`Acceptor`和`Connection`是对`Socket`的进一步封装，同时会回调`Tcpserver`的一些函数进行读事件的处理，这两个类的生命周期都由`Tcpserver`来控制

### 修改`TcpServer`类

1、添加了`Acceptor`类，使用`unique_ptr`进行管理

2、设置了两个回调函数，一个是`TcpServer`类中创建`Acceptor`对象后，会设置`Acceptor`的回调函数（调用`TcpServer::newconnection()`），另一个是在`Acceptor`中创建好`server_channel`后，会设置好`server_channel`的读回调函数（调用`Acceptor::newconnection()` `Acceptor::newconnection()`中又调用了`TcpServer::newconnection()`），同时调用`server_channel`的`enabreading()`，把`server_channel`加入到红黑树上去

### `Acceptor`类的作用

主要实现创建TCP服务端的传统步骤，并且接受客户端的连接`accept()`。这样，`TcpServer`类更加简洁，也分离出了`Acceptor`类

## 7、`Connection`类详解

### `Connection`的成员变量                                      

 `EventLoop* loop_` 对应的事件循环，但是这个事件循环是从外面传入的，不属于`Connection`，换言之`Connection`只是用他，并不管理他

 `std::unique_ptr<Socket> client_socket` `客户端连接的socket`

  `std::unique_ptr<Channel> clientchannel` `socket`对应的`Channel`

  `Buffer inputbuffer_` 接收缓冲区

  `Buffer outputbuffer_` 发送缓冲区

还有一些回调函数和后续添加的成员变量，先记录这些

### `Connection`的构造函数

```cpp
Connection::Connection(EventLoop* loop, int clientfd) : loop_(loop), disconnect_(false)

{

  client_socket = make_unique<Socket>(clientfd);

  clientchannel = make_unique<Channel>(loop_, clientfd);

  clientchannel->setreadcallback(std::bind(&Connection::onmessage, this));

  clientchannel->setwritecallback(std::bind(&Connection::writecallback, this));

  clientchannel->setclosecallback(std::bind(&Connection::closecallback, this));

  clientchannel->seterrorcallback(std::bind(&Connection::errorcallback, this));

  clientchannel->useET();             	//采用边缘触发  

  clientchannel->enabreading();          //为新客户端连接准备读事件，并添加到epoll中

}
```

与`Acceptor`的构造函数对比

```c++
Acceptor::Acceptor(EventLoop& loop, const string& ip, uint16_t port)
    : loop_(&loop),
      listen_socket(nullptr),
      serverchannel(nullptr)
{
    // 创建服务端用于监听的listen_socket,这里会调用Socket的构造函数，为什么要创建它，因为它是指针类型，需要new出来
    listen_socket = std::make_unique<Socket>(createnonblocking());
    // 设置listenfd的属性
    listen_socket->set_KEEPALIVE(true);
    listen_socket->set_NODELAY(true);
    listen_socket->set_REUSEADDR(true);
    listen_socket->set_REUSEPORT(true);

    InetAddress servaddr(ip, port); // 创建服务端的地址协议

    listen_socket->bind(servaddr);
    listen_socket->listen();

    serverchannel = std::make_unique<Channel>(loop_, listen_socket->fd());
    serverchannel->setreadcallback(std::bind(&Acceptor::newconnection, this)); // 设置回调函数
    serverchannel->enabreading(); // 让epoll_wait()监视fd_的读事件
}
```



处理逻辑基本一样，都是在构造函数中通过传入的事件循环`EventLoop`初始化自己的`EventLoop*`，然后创建相应的`socket`，创建相应的`Channel`，然后设置好相应的`Channel`的回调函数，最后把`Channel`加入到`epoll`树上

### `Connection`对象的创建时刻

`TcpServer`类中

使用 `std::unique_ptr<Acceptor> acceptor_` （一个`Tcpserver`类只有一个`Acceptor`对象）管理`Acceptor`对象

使用`std::map<int,spConnection> connections` 来管理`Connection`对象

在`TcpServer::newconnection()`中会创建`connection`对象，创建完之后设置一些回调函数，以及把`connection`对象加入到`map`容器中

### `Buffer`类增加的必要性

在搞清楚这一点之前，首先要搞明白——为什么IO 多路复用和非阻塞 IO 绑定在一起使用？

主要是为了充分发挥两者的优势，提高系统的并发处理能力和性能，以下是具体原因：

**避免阻塞影响并发处理**

- **IO 多路复用机制本身的特性**：像 `select`、`poll`、`epoll` 这类 IO 多路复用机制，能够同时监听多个文件描述符上的 IO 事件。若使用阻塞 IO，当某个文件描述符上的操作阻塞时，整个线程或进程都会被阻塞，无法对其他文件描述符上的事件做出响应。例如，在使用 `select` 监听多个 `socket` 时，如果其中一个 `socket` 在进行 `read` 操作时因没有数据而阻塞，那么 `select` 就无法继续处理其他 `socket` 上可能已经就绪的事件，这样就无法充分发挥 IO 多路复用的优势。
- **非阻塞 IO 的优势**：非阻塞 IO 允许程序在执行 IO 操作时，若数据未准备好，函数会立即返回，而不会阻塞线程或进程。这样，在使用 IO 多路复用机制时，程序可以继续检查其他文件描述符上的事件，从而实现对多个文件描述符的高效并发处理。  

**提高事件处理效率**

- **快速响应就绪事件**：结合非阻塞 IO 后，当 IO 多路复用机制检测到某个文件描述符上的事件就绪时，程序可以立即对该文件描述符执行非阻塞的 IO 操作。由于非阻塞 IO 不会等待数据完全准备好，而是快速返回结果，因此可以在短时间内处理多个就绪的文件描述符，提高了事件处理的效率。例如，在 `epoll` 检测到多个 `socket` 可读时，程序可以依次对这些 `socket` 进行非阻塞的 `read` 操作，快速读取数据。
- **减少不必要的等待**：非阻塞 IO 避免了在数据未准备好时的无谓等待，使得程序能够将时间和资源更多地用于处理其他有意义的任务。而 IO 多路复用机制则负责高效地检测哪些文件描述符上有事件发生，两者结合可以最大程度地减少系统的空闲时间，提高整体性能。

**实现高效的并发模型**

- **支持大量并发连接**：在高并发场景下，服务器需要同时处理大量的客户端连接。通过将 IO 多路复用和非阻塞 IO 结合使用，可以轻松应对大量的并发连接。例如，使用 `epoll` 和非阻塞 IO 可以在一个线程中高效地处理数千甚至数万个 `socket` 连接，而不需要为每个连接创建一个单独的线程或进程，从而减少了系统资源的开销。
- **构建灵活的并发架构**：这种组合方式使得程序可以根据实际需求灵活地构建并发架构。例如，可以使用线程池结合 IO 多路复用和非阻塞 IO，将不同的 IO 任务分配给不同的线程进行处理，进一步提高系统的并发处理能力和吞吐量。

**综上三点，我们使用非阻塞的IO，而非阻塞的IO就必须搭配`outputbuffer`使用，为什么？**

1、非阻塞IO的核心是不让事件循环阻塞在任意一个IO函数上，如`write()`、`read()`、`send()`等，只能阻塞在`epoll_wait()`中

2、但设想一个场景，如果你想向对端发送`100KB`数据，但是由于网络原因只发送了`80KB`，这时候肯定不能原地等待，那就阻塞了，这时候程序应该尽快返回，去处理别的事情，但剩下的`20KB`怎么办？

  对于应用程序而言，它只管生成数据，它不应该关心到底数据是一次性发送还是分成几次发送，这些应该由网络库来操心，程序只要调用`Connection::onmessage()`就行了，网络库会负责到底。网络库应该接管这剩余的`20kB`数据，把它保存在该`Connection`的 `outputbuffer` 里，然后注册 `POLLOUT` 事件，一旦 `socket` 变得可写就立刻发送数据。当然，这第二次 `write()`也不一定能完全写入 `20kB`，如果还有剩余，网络库应该继续关注`POLLOUT`事件;如果写完了`20kB`，网络库应该停止关注`POLLOUT`
  如果程序又写入了`50kB`，而这时候`outputbuffer`里还有待发送的 `20kB` 数据那么网络库不应该直接调用 `write()`，而应该把这 `50kB` 数据 `append` 在那 `20kB`数据之后，等 `socket` 变得可写的时候再一并写入。

3、如果 `outputbuffer` 里还有待发送的数据，而程序又想关闭连接(对程序而言调用`Connection::onmessage()`之后他就认为数据迟早会发出去)，那么这时候网络库不能立刻关闭连接，而要等数据发送完毕。

综上，要让程序在 write操作上不阻塞，网络库必须要给每个`Connection` 配置 `outputbuffer`

**为什么要使用`inputbuffer`？**

TCP是一个无边界的字节流协议，接收方必须要处理“收到的数据尚不构成一条完整的消息”和“一次收到两条消息的数据”等情况（粘包和分包）。

一个常见的场景是，发送方`send()`了两条`1kB`的消息(共`2kB`)，接收方收到数据的情况可能是:
一次性收到 `2kB` 数据
分两次收到，第一次 `600B`，第二次 `1400B`
分两次收到，第一次 `1400B`，第二次`600B`
分两次收到，第一次 `1kB`，第二次 `1kB`
分三次收到，第一次 `600B`，第二次`800B`，第三次`600B`
其他任何可能
网络库在处理 `socket`可读 事件的时候，必须一次性把`socket`里的数据读完(从操作系统buffer搬到应用层buffer)，否则会反复触发`POLLIN` 事件，造成busy-loop。那么网络库必然要应对“数据不完整”的情况，收到的数据先放到 `inputbuffer` 里，等构成一条完整的消息再通知程序的业务逻辑。所以，在TCP网络编程中，网络库必须要给每个`Connection` 配置`inputbuffer`

```c++
void Connection::onmessage()
{
    char buffer[1024];									 //定义一个长度为1024的字符串数组，用于从输出缓冲区里读数据在存到inputbuffer中
    while (true)      									 //一次读取buffer大小数据，由于采用边缘触发，必须用循环，直到全部的数据读取完毕
    {
        bzero(&buffer, sizeof(buffer));				       //清空buffer
        ssize_t nread = read(fd(), buffer, sizeof(buffer)); // 这行代码用了read()，也可以用recv()，一样
        if (nread > 0) 									 // 成功的读取到了数据。
        {
            inputbuffer_.append(buffer, nread); 		   //接收到数据后先放到接收缓冲区中
        }
        else if (nread == -1 && errno == EINTR) 		   // 读取数据的时候被信号中断，继续读取
        {
            continue;
        }
        else if (nread == -1 && ((errno == EAGAIN) || (errno == EWOULDBLOCK))) // 全部的数据已读取完毕,接收缓冲区中没有数据了
        {
            while (true)
            {
                std::string message = inputbuffer_.getmessage();			//那就从inputbuffer中取出来数据
                if (message == " ") break;								  //如果空了，就结束
                last_time = TimeStamp::Now(); 							   //每接收到一个报文，把时间戳更新为当前时间
                messagecallback_(shared_from_this(), message); 				//这里最终回调的是EchoServer::HandleMessage()
            }
            break;
        }
        else if (nread == 0) 												// 客户端连接已断开
        {
            clientchannel->remove(); 										 //从事件循环中删除channel
            closecallback(); 												//回调Tcpserver::closecallback
            break;
        }
    }
}
```

> 也就是说`Connection::onmessage()`的作用就是，从socket的接收缓冲区中读取数据然后放在`inputbuffer`里面，然后再从`inputbuffer`里按报文长度读取出报文，然后回调`EchoServer::HandleMessage()`（类`EchoServer`后续再增加）这个函数的作用是简单包装一下从`inputbuffer`里读出来的报文，然后发给`outputbuffer`，这时候就是注册socket的写事件的时机，即一旦`outputbuffer`中有数据，就注册写事件，而在`client_channel`处理写事件的回调函数中，会增加判断——如果`outputbuffer`为空，就停止关注写事件

补充：使用智能指针管理资源，使用`shared_ptr`管理共享资源：，使用`unique_ptr`管理个人资源

(1)共享资源：`Connection` 这里用到了别名`using spConnection=std::make_shared<Connection>`;

(2)如何在类中使用智能指针而非this指针，让类去继承一个模板类：`public std::enable_shared_from_this<类名>`，这样在该类中可以调用成员函数`shared_from_this()`

至此，单线程的reactor模型已基本构建完毕。

## 8、添加线程池

### 为什么要添加线程池？

1)单线程的Reactor模型不能发挥多核CPU的性能

2)运行多个事件循环，主事件循环运行在主线程中，从事件循环(`EventLoop::run()`)运行在线程池中

3)主线程负责创建客户端连接，然后把conn分配给线程池，2)和3)主要使得线程之间分工更加明确

### 为什么要添加工作线程？

IO线程负责从`inputbuffer`读取`message`并发送给`outputbuffer`，而工作线程只负责处理业务（现在的业务只是消息的计算）

如果把处理业务和IO都交给IO线程，处理业务如果很麻烦，会造成阻塞，这时候就不能即时响应新的`connection`的连接了

## 9、添加异步唤醒事件循环

### 工作线程处理完业务后怎么办？

应该直接调用`send`函数发送给对端或者发送给`outputbuffer`吗？

不应该，工作线程应只处理对于消息的计算，不应该去关心`send()`到哪里去，这些是IO线程应该操心的事情，因此在`EventLoop`类中增加`eventfd`，用于异步唤醒IO循环，而且如果不这样做的话，那IO线程和工作线程会同时操作`outputbuffer`，产生`race condition`

所以现在的处理逻辑就是，在`EventLoop`类中增加一个任务队列(相当于给每个IO线程中增加了一个任务队列，因为一个线程和一个`EventLoop`强绑定)和`eventfd`，当工作线程处理完数据后，把发送数据给`outputbuffer`的这个任务(函数)加入到对应的任务队列中去，同时向`eventfd`中写入数据，唤醒事件循环，具体调用流程如下：
1)假设这样一个场景，客户端现在已经有socket连接上来，某个IO线程阻塞在`epoll_wait()`函数

2)下一时刻，服务端收到了对端发来的报文，即`epoll`监视的`client_socket`发生了事件

3)`Epoll::loop()`函数返回一个`vector<Channel*>`，然后在`EventLoop::run()`函数中，对返回的`Channel*`调用`Channel::handleevent()`，之后会调用`client_socket`的读事件的回调函数（收到消息是读事件）

4）`client_socket`的读回调函数是`Connection::onmessage()`，这个函数的作用不再赘述，简单来讲就是从发送缓冲区搬到`inputbuffer`，再从`inputbuffer`中取出报文，取出报文后经过两次回调，`TcpServer`做了一次回调中介，最终回调`EchoServer::HandleMessage()`

5）在`EchoServer::HandleMessage()`中，会做一次判断，如果没有工作线程，那就直接处理数据，如果有就交给工作线程处理，处理函数是`EchoServer::Handledata()`

6）在`EchoServer::Handledata()`中，会对报文进行计算，然后调用`Connection::sendmessage()`，重点来了！在这个函数中会做判断，如果当前线程是IO线程，就直接交给IO线程，如果不是就把任务添加到任务队列中

7）假如不是IO线程，会调用`EventLoop::addtask()`，这个函数首先把任务加入到任务队列中，然后调用`EventLoop::wakeup()`函数，向`eventfd`中写入数据，在`EventLoop`的构造函数中就已经创建好了`wakeup_channel`，并注册好了读事件的回调函数，在向`eventfd`写入数据后，`epoll_wait()`会返回`wakeup_channel`，然后又一次执行`Channel::handleevent()`，并调用读事件回调函数——`EventLoop::handle_wakeup()`

8）在最后这个函数中，会把`eventfd`中的数据读出来（必须要读，否则会一直触发写事件（水平触发）），并从任务队列中取出来任务，执行最后的发送操作——`Connection::send_inloop()`

9）在`Connection::send_inloop()`函数中，先把处理完的数据放入到`outputbuffer`中，同时注册好`client_channel`的写事件，最终会调用写回调函数——`Connection::writecallback()`

## 10、添加定时器

增加定时器的目的是——定时清理空闲的connection对象

1）传统的做法:`alarm()`函数可设置定时器，触发`SIGALRM`信号

2）新版Linux内核把定时器和信号抽象为fd，让`epoll`统一监视
做法：
1)在事件循环中增加`map<int,spConnect>conns` 容器，存放运行在该事件循环上全部的`Connection`对象

2）如果闹钟时间到了，遍历`conns`，判断每个`Connection`对象是否超时

3）如果超时了，从`conns` 中删除`Connection`对象（需要加锁，主线程会向容器中添加`connection`对象，而从线程会从容器中删除`connection`对象）

4）再从`TcpServer::conns` 中删除Connection对象（需要加锁，主线程会向容器中添加`connection`对象，而从线程会从容器中删除`connection`对象，也就是说`TcpServer`类的容器和`EventLoop`类的容器同在`newconnection`函数中添加`connection`对象）

经过测试：上面这种方式有bug，即如果一个客户端连接正常的关闭，他会在`Tcpserver`的`connections`数组中被删除，但是却不能在`EventLoop`的`connection`数组中被删除，于是干脆删除`EventLoop`的`connection`数组，直接在`Tcpsever`中增加一个定时器，闹钟响了只删除`Tcpserver`里面的`connection`

