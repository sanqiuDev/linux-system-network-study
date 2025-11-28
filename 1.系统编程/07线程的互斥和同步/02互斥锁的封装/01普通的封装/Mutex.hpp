#pragma once         
#include <iostream> 
#include <pthread.h> 
       

// 定义命名空间MutexModule，封装互斥锁相关类，避免命名冲突
namespace MutexModule
{
    /*------------------------------------- 互斥锁封装类 -------------------------------------*/
    //1.互斥锁封装类 ---> 封装了pthread_mutex_t的创建、销毁、加锁、解锁操作
    class Mutex
    {
    private:
        //1.原生POSIX互斥锁对象，作为私有成员隐藏实现细节
        pthread_mutex_t _mutex;

    public:
        //1.“构造函数” ---> 调用pthread_mutex_init初始化互斥锁，使用默认属性（nullptr）
        Mutex()
        {
            pthread_mutex_init(&_mutex, nullptr); //注意：默认属性下为普通锁，仅支持同一进程内线程同步
        }

        //2.“析构函数” ---> 调用pthread_mutex_destroy销毁互斥锁，释放其占用的系统资源
        ~Mutex()
        {
            pthread_mutex_destroy(&_mutex); //注意：销毁前需确保锁已被释放（未被任何线程持有）
        }

        //3.“加锁操作” ---> 调用pthread_mutex_lock加锁，若锁被占用则阻塞等待
        void Lock()
        {
            int n = pthread_mutex_lock(&_mutex);
            (void)n; //忽略返回值
        }  

        //4.“解锁操作” ---> 调用pthread_mutex_unlock解锁，唤醒等待该锁的线程
        void Unlock()
        {
            int n = pthread_mutex_unlock(&_mutex);
            (void)n; //忽略返回值
        }
    };
}