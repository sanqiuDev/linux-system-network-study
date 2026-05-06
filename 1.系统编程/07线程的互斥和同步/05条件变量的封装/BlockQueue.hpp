#pragma once
 
#include <iostream>
#include <string>
#include <queue>     // 提供“std::queue容器” ---> 作为阻塞队列的底层存储结构
#include <pthread.h> // 提供“POSIX线程库接口” ---> 互斥锁、条件变量，用于线程同步

//新1：包含之前封装实现的：“互斥锁”和“条件变量”
#include "Mutex.hpp"
#include "Cond.hpp"

//新2：使用命名空间
using namespace MutexModule;
using namespace CondModule;

//1.阻塞队列的默认容量
const int defaultcap = 5; 

// 模板类：支持存储任意类型T的数据，实现线程安全的阻塞队列
template <typename T>
class BlockQueue
{
private:
    /*---------------------------属性---------------------------*/
    //1.底层存储容器队列 ---> 核心临界资源（多线程共享）
    //2.队列的最大容量 ---> 限制入队元素个数（阻塞队列）
    std::queue<T> _q; 
    int _cap;       

    //3.互斥锁 ---> 保护队列的访问，确保操作原子性
    //4.条件变量 ---> 用于队列满时阻塞生产者、唤醒生产者
    //5.条件变量 ---> 用于队列为空时阻塞消费者、唤醒消费者
    // pthread_mutex_t _mutex;    
    // pthread_cond_t _full_cond;  
    // pthread_cond_t _empty_cond;

    //新3：使用自己封装实现的类实例化对象
    Mutex _mutex;
    Cond _full_cond;
    Cond _empty_cond;

    //6.统计当前休眠的消费者线程数（优化唤醒逻辑）
    //7.统计当前休眠的生产者线程数（优化唤醒逻辑）
    int _csleep_num; 
    int _psleep_num; 

    /*---------------------------方法---------------------------*/
    //1.“判断队列是否已满” ---> 仅在临界区内调用，无需加锁
    bool IsFull() { return _q.size() >= _cap; }

    //2.“判断队列是否为空” ---> 仅在临界区内调用，无需加锁
    bool IsEmpty() { return _q.empty(); }


public:
    //1.“构造函数” ---> 
    BlockQueue(int cap = defaultcap)
        : _cap(cap),          // 初始化队列最大容量
          _csleep_num(0),     // 初始化休眠的消费者线程数为0
          _psleep_num(0)      // 初始化休眠的生产者线程数为0
    {
        //新5.什么也不需要做了因为我们封装实现的：“互斥锁”和“条件变量”的构造函数已经完成构造任务


        // //1.1：初始化互斥锁 ---> 保护队列（临界资源）的访问，使用默认属性（nullptr）
        // pthread_mutex_init(&_mutex, nullptr);

        // //1.2：初始化"队列满"条件变量 ---> 用于通知生产者队列已满需等待，消费者消费后唤醒
        // pthread_cond_init(&_full_cond, nullptr);

        // //1.3：初始化"队列空"条件变量 ---> 用于通知消费者队列为空需等待，生产者生产后唤醒
        // pthread_cond_init(&_empty_cond, nullptr);
    }

    //2.“析构函数” ---> 
    ~BlockQueue()
    {
        //新6.什么也不需要做了因为我们封装实现的：“互斥锁”和“条件变量”的析构函数已经完成析构任务

        // //2.1：销毁互斥锁
        // pthread_mutex_destroy(&_mutex);    

        // //2.2：销毁"队列满"条件变量
        // pthread_cond_destroy(&_full_cond);  

        // //2.3：销毁"队列空"条件变量
        // pthread_cond_destroy(&_empty_cond); 
    }

    //3.入队操作（生产者调用）---> 将数据添加到队列，队列满时阻塞生产者
    void Push(const T &in)
    {
        // //1.加锁：进入临界区，确保队列操作的原子性（避免多线程并发冲突）
        // pthread_mutex_lock(&_mutex);

        //新7：
        LockGuard lockguard(_mutex);

        /*--------------------------第一步：判断队列是否已满--------------------------*/
        //2.循环判断队列是否已满
        /* 细节说明：
        *     1. 使用while而非if，避免"伪唤醒"导致的错误
        *     2. 伪唤醒：线程被条件变量唤醒，但实际队列仍满（如多个生产者同时被唤醒，队列中的空位置被其他生产者占用）
        */
        while (IsFull())
        {
            //1.休眠的生产者线程数+1 ---> 队列已满生产者线程需要休眠等待 
            _psleep_num++; 
            std::cout << "生产者，进入休眠了: _psleep_num = " << _psleep_num << std::endl;

            /* 重点说明pthread_cond_wait的核心机制：
               1. 调用成功后，在挂起当前线程之前，会自动释放持有的_mutex锁
                  （允许其他线程访问队列，避免死锁）；
               2. 线程被唤醒（通过pthread_cond_signal/broadcast）后，会先重新申请_mutex锁，
                  申请成功后才从该函数返回，此时线程仍在临界区内；
               3. 若申请锁失败（如锁被其他线程持有），线程会在锁上阻塞，直到获取到锁；
               4. 存在"伪唤醒"风险：即使队列仍满，线程也可能被唤醒，因此必须用while循环重判条件。
            */

            // //2.等待"队列为满"条件变量：被消费者唤醒后才继续执行
            // pthread_cond_wait(&_full_cond, &_mutex);

            //新9：
            _full_cond.Wait(_mutex);

            //3.生产者线程被唤醒，休眠数-1
            _psleep_num--;  
        }

        /*--------------------------第二步：将数据添加到队列--------------------------*/
        //3.执行到此处“队列一定未满” ---> 将数据入队（100%确定，因while循环已过滤满队列情况）
        _q.push(in); 

        /*--------------------------第三步：唤醒消费者--------------------------*/
        //4.优化方案：仅当有消费者休眠时，才唤醒消费者 ---> 避免无效唤醒，减少系统开销
        if (_csleep_num > 0)
        {
            // // 唤醒一个等待"队列非空"的消费者线程
            // pthread_cond_signal(&_empty_cond); 

            //新10：
            _empty_cond.Signal();
            std::cout << "唤醒消费者..." << std::endl;
        }

        //4.备选方案1：直接唤醒消费者 ---> 无论是否有消费者休眠，可能产生无效唤醒
        // pthread_cond_signal(&_empty_cond); 

        //新8：
        // //5.解锁：退出临界区，释放队列的访问权，允许其他线程操作
        // pthread_mutex_unlock(&_mutex);

        //4.备选方案2：解锁后唤醒消费者 ---> 也可行，但唤醒时机稍晚，不影响正确性
        // pthread_cond_signal(&_empty_cond); 
    }

    //4.出队操作（消费者调用）---> 从队列取出数据，队列为空时阻塞消费者
    T Pop() //返回值：从队列取出的数据（T类型，按值返回）
    {
        // //1.加锁：进入临界区，确保队列操作的原子性
        // pthread_mutex_lock(&_mutex);

        //新11：
        LockGuard lockguard(_mutex);

        /*--------------------------第一步：判断队列是否为空--------------------------*/
        //2.循环判断队列是否为空（使用while而非if，避免伪唤醒）
        while (IsEmpty())
        {
            //2.1：休眠的消费者线程数+1 ---> 队列为空，消费者线程需要休眠等待
            _csleep_num++;

            // //2.2：等待"队列非空"条件变量：被生产者唤醒后才继续执行
            // pthread_cond_wait(&_empty_cond, &_mutex);

            //新12：
            _empty_cond.Wait(_mutex);

            //2.3：消费者线程被唤醒，休眠数-1
            _csleep_num--; 
        }

        /*--------------------------第二步：从队列中取出数据--------------------------*/
        //3.执行到此处“队列一定非空” ---> 取出队首数据
        //3.1：获取队首元素
        T data = _q.front(); 
        //3.2：移除队首元素
        _q.pop();            

        /*--------------------------第三步：唤醒生产者--------------------------*/
        //4.优化方案：仅当有生产者休眠时，才唤醒生产者（避免无效唤醒）
        if (_psleep_num > 0)
        {
            // // 唤醒一个等待"队列未满"的生产者线程
            // pthread_cond_signal(&_full_cond); 

            //新13：
            _full_cond.Signal();
            std::cout << "唤醒生产者" << std::endl; 
        }

        //4.备选方案：直接唤醒生产者 ---> 无论是否有生产者休眠，可能产生无效唤醒
        // pthread_cond_signal(&_full_cond);


        //新12：
        // //5.解锁：退出临界区
        // pthread_mutex_unlock(&_mutex);

        //6.返回取出的数据
        return data; 
    }
};