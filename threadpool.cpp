#include <iostream>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <queue>
#include <chrono>
#include <vector>
#include <functional>
using namespace std;

class Threadpool{
public:
    static Threadpool& Get_pl(int num=0){
        static Threadpool pl(num);
        return pl;
    }
    template<class F,class... Args>
    void enqueue(F &&f,Args&&... args){
        function<void()> task=bind(forward<F>(f),forward<Args>(args)...);//通过bind把函数与参数绑定在一起，为什么要使用完美转发？
        unique_lock<mutex> lck(mtx);
        q.emplace(move(task));
        lck.unlock();//如果不主动解锁会怎么样，好像也不会怎么样
        cv.notify_one();//有任务了快去完成
    }//把任务添加进任务队列的一个函数，也是提供给用户的一个接口
private:
    Threadpool(int num=0):stop(false){
        for(int i=0;i<num;i++){
            threads.emplace_back([this]{
                while(1){
                unique_lock<mutex> lck(mtx);
                cv.wait(lck,[this]()->bool{return !q.empty()||stop==true;});//无任务或者线程池终止了，这里就要等待
                if(stop&&q.empty()){
                    return;//如果线程池终止了，就return掉
                }
                //去从任务队列里面取任务
                function<void()> task(move(q.front()));//使用不使用move的区别？？不使用move就调用task的copy构造函数，比较慢
                q.pop();
                //取到了任务就要解开锁
                lck.unlock();
                //然后执行task函数，去完成任务
                task();
                }//while循环的作用，确保执行完一个任务后，再次执行下一个任务或者等待
            });//lambda表达式,整个lambda表达式被作为一个参数去给了thread的构造函数，仔细想想，thread的构造函数的参数是什么，不就是一个函数吗
        }
    }
    ~Threadpool(){
        {
            unique_lock<mutex> lck(mtx);
            stop=true;
        }
        cv.notify_all();//唤醒所有线程去工作，把任务队列里面的任务取完，得益于while循环可以源源不断的去处理任务
        for(auto& t:threads){
            t.join();
        }//等待所有线程执行完
    }
private:
    vector<thread> threads;//线程池首先需要一个线程数组
    queue<function<void()>> q;//一个任务队列，任务是什么，任务就是一些函数，线程不就是去执行一个函数吗，就相当于处理了一个任务
    mutex mtx;
    condition_variable cv;
    bool stop;//表示线程池什么时候终止
    Threadpool(const Threadpool&)=delete;
    void operator=(const Threadpool&)=delete;
    Threadpool(Threadpool&&)=delete;
    void operator=(Threadpool&&)=delete;
};

int main(){
    Threadpool& pl=Threadpool::Get_pl(3);
    for(int i=0;i<10;i++){
        cout<<"task:"<<i<<"is added"<<endl;
        pl.enqueue([i]{
            cout<<"task:"<<i<<"is running"<<endl;
            this_thread::sleep_for(chrono::seconds(1));
            cout<<"task:"<<i<<"is done"<<endl;
        });
    }
    return 0;
}