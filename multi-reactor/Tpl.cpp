#include "Tpl.h"
#include <sys/syscall.h>
#include <unistd.h>
#include <iostream>
Tpl::Tpl(int num):stop(false)
{
    for(int i=0;i<num;i++)
    {
        threads.emplace_back([this]
        {
        printf("create work_thread %ld\n",syscall(SYS_gettid));//这里的syscall(SYS_gettid)是获取线程的id
        while(1)
        {
            unique_lock<mutex> lck(mtx);    //这里的lck是一个局部变量，通过lck去上锁，避免多线程同时访问任务队列，同时读取同一个任务，保证线程安全
            cv.wait(lck,[this]()->bool{return !q.empty()||stop==true;});//无任务或者线程池终止了，这里就要等待
            if(stop&&q.empty())
            {
                return;//如果线程池终止了，就return掉
            }
            //去从任务队列里面取任务
            function<void()> task(move(q.front()));//使用不使用move的区别？？不使用move就调用task的copy构造函数，比较慢
            q.pop();
            lck.unlock();
            //然后执行task函数，去完成任务
            task();
        }//while循环的作用，确保执行完一个任务后，再次执行下一个任务或者等待
        });//lambda表达式,整个lambda表达式被作为一个参数去给了thread的构造函数，仔细想想，thread的构造函数的参数是什么，不就是一个函数吗
    }
}
Tpl::~Tpl()
{
    /*{
        unique_lock<mutex> lck(mtx);
        stop=true;  
    }*/
    stop.store(true);//这里使用原子变量就不需要加锁了，因为原子变量是线程安全的
    cv.notify_all();//唤醒所有线程去工作，把任务队列里面的任务取完，得益于while循环可以源源不断的去处理任务
    for(auto& t:threads)
    {
        t.join();
    }//等待所有线程执行完
}
Tpl& Tpl::Get_pl(int num){
    static Tpl pl(num);
    return pl;
}
int Tpl::size()
{
    return threads.size();
}

    