#pragma once  

#include <iostream>       // 提供标准输入输出功能（此处未直接使用，为潜在扩展预留）
#include <pthread.h>      // 提供POSIX线程库的条件变量底层接口（pthread_cond_*系列函数）
#include "Mutex.hpp"      // 包含自定义Mutex类头文件（条件变量需与互斥锁配合使用）

using namespace MutexModule;  // 使用MutexModule命名空间，简化自定义Mutex类的调用

// 条件变量模块命名空间：封装条件变量相关功能，避免命名冲突，提高代码模块化
namespace CondModule
{
    // 条件变量封装类：对POSIX底层条件变量（pthread_cond_t）进行面向对象封装
    // 核心作用：实现线程间的"等待-唤醒"同步机制，需与Mutex互斥锁配合使用
    class Cond
    {
    private:
        //1.底层条件变量句柄（POSIX标准类型，封装系统资源）
        pthread_cond_t _cond;  

    public:
        //1.“构造函数” ---> 调用底层接口pthread_cond_init初始化条件变量
        Cond()
        {
            pthread_cond_init(&_cond, nullptr);
        }

        //2.“析构函数” ---> 调用底层接口pthread_cond_destroy销毁条件变量，避免资源泄漏
        ~Cond()
        {
            pthread_cond_destroy(&_cond);
        }

        //3.“等待条件变量” ---> 让当前线程阻塞，直到被唤醒
        void Wait(Mutex &mutex) //参数：mutex - 自定义Mutex类对象（必须与等待的临界资源绑定的互斥锁）
        {
            /* 调用底层接口pthread_cond_wait，核心逻辑：
            *      1. 自动释放mutex关联的底层锁（pthread_mutex_t），让其他线程可访问临界资源
            *      2. 阻塞当前线程，直到被Signal()或Broadcast()唤醒
            *      3. 线程被唤醒后，自动重新获取mutex的锁，确保唤醒后仍在临界区内
            */
           
            int n = pthread_cond_wait(&_cond, mutex.Get());
            (void)n;  // 忽略返回值，避免编译警告（实际项目可根据返回值处理错误）
        }

        //4.“唤醒一个等待线程” ---> 唤醒在当前条件变量上等待的任意一个线程
        void Signal()
        {
            // 适用场景：仅需一个线程处理临界资源（如消费者取走一个任务后，唤醒一个生产者）
            int n = pthread_cond_signal(&_cond);
            (void)n;  // 忽略返回值，避免编译警告
        }

        //5.“唤醒所有等待线程” ---> 唤醒在当前条件变量上等待的所有线程
        void Broadcast()
        {
            // 适用场景：多个线程需同时处理临界资源（如队列新增任务后，唤醒所有等待的消费者）
            int n = pthread_cond_broadcast(&_cond);
            (void)n;  // 忽略返回值，避免编译警告
        }
    };
};