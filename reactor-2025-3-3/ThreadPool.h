#pragma once

#include <thread>
#include <mutex>
#include <condition_variable>
#include <queue>
#include <chrono>
#include <vector>
#include <functional>
#include <atomic>
using namespace std;

class Threadpool{
public:
    static Threadpool& Get_pl(int num=0);
    template<class F,class... Args>  //Args是一个模板参数包，F是一个函数对象，Args是一个参数包
    void enqueue(F &&f,Args&&... args);
private:
    Threadpool(int num=0);
    ~Threadpool();
private:
    vector<thread> threads;//线程池首先需要一个线程数组
    queue<function<void()>> q;//一个任务队列，任务是什么，任务就是一些函数，线程不就是去执行一个函数吗，就相当于处理了一个任务
    mutex mtx;
    condition_variable cv;
    atomic<bool> stop;//表示线程池什么时候终止
    Threadpool(const Threadpool&)=delete;
    void operator=(const Threadpool&)=delete;
    Threadpool(Threadpool&&)=delete;
    void operator=(Threadpool&&)=delete;
public: 
    int size();
};
template<class F,class... Args>  //Args是一个模板参数包，F是一个函数对象，Args是一个参数包
void Threadpool::enqueue(F &&f,Args&&... args)
{
    function<void()> task=bind(forward<F>(f),forward<Args>(args)...);//通过bind把函数与参数绑定在一起，为什么要使用完美转发？因为这里的参数是一个参数包，不知道参数的具体类型，所以要使用完美转发
    unique_lock<mutex> lck(mtx);
    q.emplace(std::move(task));
    lck.unlock();//如果不主动解锁会怎么样，好像也不会怎么样
    cv.notify_one();//有任务了快去完成
}//把任务添加进任务队列的一个函数，也是提供给用户的一个接口