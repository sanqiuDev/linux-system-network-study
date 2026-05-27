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

        //5.“锁的get方法” ---> 获取互斥锁的地址
        pthread_mutex_t *Get()
        {
            return &_mutex;
        }
    };

    /*------------------------------------- 锁守卫类 -------------------------------------*/
    //2.锁守卫类（RAII机制）---> 用于自动管理互斥锁的加锁与解锁，确保在异常或函数退出时，锁能被正确释放，避免死锁
    class LockGuard
    {
    private:
        //1.引用要管理的Mutex对象，确保生命周期与Mutex一致
        Mutex &_mutex;

    public:
        //1.“构造函数” ---> 接收Mutex对象引用，自动调用加锁操作
        LockGuard(Mutex &mutex):_mutex(mutex)
        {
            _mutex.Lock(); // 构造时自动加锁，进入临界区
        }

        //2.“析构函数” ---> 自动调用解锁操作，无需手动解锁
        ~LockGuard()
        {
            _mutex.Unlock(); // 无论函数正常返回还是因异常退出，析构函数都会执行
        }

        //3.禁用“拷贝构造”和“赋值运算符” ---> 避免锁守卫对象被复制导致的异常解锁
        LockGuard(const LockGuard &) = delete;
        LockGuard &operator=(const LockGuard &) = delete;
    };
}