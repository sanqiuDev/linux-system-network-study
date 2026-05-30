#pragma once 

#include <iostream>         
#include <string>           // 提供std::string字符串类 ---> 用于线程名称存储
#include <vector>           // 提供std::vector容器 ---> 存储线程池中的工作线程
#include <queue>            // 提供std::queue容器 ---> 作为任务队列，存储待执行任务
 
#include "Thread.hpp"       // 包含自定义Thread类头文件 ---> 封装线程的创建、启动等操作
#include "Mutex.hpp"        // 包含自定义Mutex互斥锁类头文件 ---> 保护临界资源访问
#include "Cond.hpp"         // 包含自定义Cond条件变量类头文件 ---> 实现线程等待-唤醒同步
#include "Log.hpp"          // 包含日志模块头文件 ---> 用于打印日志，此处未显式使用，预留扩展

namespace ThreadPoolModule
{
    //1.引入依赖模块的命名空间 ---> 简化类的调用（无需写完整命名空间前缀）
    using namespace ThreadModule;
    using namespace MutexModule;
    using namespace CondModule;
    using namespace LogModule;

    //2.线程池默认线程数量
    static const int gnum = 5; 

    // 模板类：支持任意可调用对象类型T的线程池（T需满足"可调用"特性，如：std::function<void()>）
    template <typename T>
    class ThreadPool
    {
    private:
        /*----------------------------【属性】----------------------------*/
        //1.工作线程容器 ---> 存储所有预创建的工作线程
        //2.工作线程数量 ---> 线程池大小
        //3.任务队列 ---> 存储待执行的任务（FIFO顺序）
        //4.互斥锁 ---> 保护任务队列、_isrunning、_sleepernum的访问
        //5.条件变量 ---> 实现线程的等待-唤醒同步
        std::vector<Thread> _threads;
        int _num;                       
        std::queue<T> _taskq;          
        Cond _cond;                    
        Mutex _mutex;                  

        //6.线程池运行状态 ---> true=运行中，false=已停止
        //7.休眠线程计数器 ---> 记录当前处于休眠状态的线程数
        bool _isrunning;               
        int _sleepernum;               

        //8.单例实例指针 ---> 全局唯一，静态成员（类共享）
        //9.单例创建锁 ---> 保护单例实例的并发创建（静态成员）
        static ThreadPool<T> *inc;     
        static Mutex _lock;  
        /*说明：已经有锁了为什么这里还要再定义一个静态锁？
        *
        *    1.使用单例模式创建线程池的方法是静态方法
        *    2.静态方法无法访问非静态成员（非静态的锁）
        *    3.但是获取单例的方法需要加锁进行保护
        */


        /*----------------------------【方法】----------------------------*/
        //1.“禁用拷贝构造函数” ---> 避免线程池对象被拷贝（单例模式必须，防止多实例）
        ThreadPool(const ThreadPool<T> &) = delete;
        //2.“禁用赋值运算符重载” ---> 避免线程池对象被赋值（同上）
        ThreadPool<T> &operator=(const ThreadPool<T> &) = delete;

        //3.“构造函数” ---> 禁止外部直接创建线程池对象（单例模式核心）
        ThreadPool(int num = gnum)
            : _num(num),         // 初始化工作线程数量
              _isrunning(false), // 线程池运行状态：初始为未运行
              _sleepernum(0)     // 休眠线程计数：初始为0（无线程休眠）
        {
            //3.1：预创建num个工作线程，存入线程容器_threads
            for (int i = 0; i < num; i++)
            {
                _threads.emplace_back
                (
                    [this]()
                    {
                        this->HandlerTask(); // 工作线程核心逻辑：循环处理任务
                    }
                );
                /*说明：
                 *   1. emplace_back：直接在vector中构造Thread对象，避免拷贝
                 *   2. 线程执行函数：lambda表达式（捕获this指针，访问线程池成员）
                 */
            }
        }
        //4.“启动线程池” ---> 启动所有预创建的工作线程（私有方法，仅单例初始化时调用）
        void Start()
        {
            //1.若线程池已在运行，直接返回（避免重复启动）
            if (_isrunning) 
            {
                return;
            }

            //2. 标记线程池为运行状态
            _isrunning = true;

            //3.遍历线程容器，启动每个工作线程
            for (auto &thread : _threads)
            {
                //3.1：调用Thread类的Create()方法，创建并启动线程
                thread.Create(); 

                //3.2：打印日志，记录线程启动成功（包含线程名称）
                LOG(LogLevel::INFO) << "启动新线程成功: " << thread.Name();
            }
        }

        //5.“唤醒一个休眠线程” ---> 用于新任务入队时，唤醒一个线程处理任务（避免所有线程空等）
        void WakeUpOneThread()
        {
            //1.信号唤醒：仅唤醒一个在_cond上等待的线程（减少线程切换开销）
            _cond.Signal();                            

            //2.打印日志，记录唤醒操作
            LOG(LogLevel::INFO) << "唤醒一个休眠线程"; 
        }

        
        //6.“唤醒所有休眠线程” ---> 用于线程池退出时，唤醒所有等待任务的线程
        void WakeUpAllThread()
        {
            //1.加锁：保护_sleepernum的访问（避免并发修改）
            LockGuard lockguard(_mutex);     
            
            //2.若有休眠线程，才执行唤醒（避免无效操作）
            if (_sleepernum > 0)   
            {
                _cond.Broadcast();   // 广播唤醒：唤醒所有在_cond上等待的线程
            }       

            //3.打印日志，记录唤醒操作                        
            LOG(LogLevel::INFO) << "唤醒所有的休眠线程"; 
        }


    public:
        //1.“析构函数” 
        ~ThreadPool() {}

        //2.“单例模式” ---> 获取线程池唯一实例（懒汉模式，首次使用时创建）
        static ThreadPool<T> *GetInstance()
        {
            //1.双重检查锁定：第一重检查（无锁，提升效率）---> 第一波之后获取单例的线程会从这里直接退出
            if (inc == nullptr)  
            {
                //1.1：加锁：保护单例对象的创建（避免并发创建多实例）
                LockGuard lockguard(_lock);  

                //1.2：双重检查锁定：第二重检查（加锁后，确保仅创建一次）
                LOG(LogLevel::DEBUG) << "获取单例....";
                if (inc == nullptr)  // ---> 第一波之获取单例的线程中非第一个抢到锁的线程会从这里直接退出
                {
                    LOG(LogLevel::DEBUG) << "首次使用单例, 创建之....";

                    //1)创建线程池实例（调用私有构造函数）
                    inc = new ThreadPool<T>();  

                    //2)启动线程池（预创建的线程开始运行）
                    inc->Start();                
                }
            }

            //2.返回单例实例指针
            return inc;  
        }
        /* 解释：这里的单例模式为什么使用static？
        *
        *     1.GetInstance是类内的成员方法
        *     2.所以访问该方法就必须要有线程池对象才能进行访问
        *     3.而进程池对象的创建必须执行GetInstance方法才能创建
        *     4.为了防止这个悖论所以我们需要将GetInstance方式设置为静态方法
        *     5.这样的话GetInstance方法就能在不创建对象的时候直接用
        */



        //3.“停止线程池” ---> 标记运行状态为false，唤醒所有线程退出
        void Stop()
        {
            //1.若线程池已停止，直接返回（避免重复停止）
            if (!_isrunning)  
            {
                return;
            }
        
            //2.标记线程池为停止状态 ---> 注意：这个时候线程池已经关闭了，但是线程还没有退出
            _isrunning = false;  

            //3.唤醒所有休眠线程
            WakeUpAllThread();  
            /* 所有线程被唤醒之后的情况：
            *     1. 情况一：任务队列“非空” ---> 处理完任务 ---> 线程退出
            *     2. 情况二：任务队列“为空” ---> 线程直接退出
            */
        }

        //4.“等待所有工作线程退出” ---> 阻塞主线程，直到所有工作线程执行完毕
        void Join()
        {
            //1.遍历线程容器，调用每个线程的Join()方法，等待线程退出
            for (auto &thread : _threads)
            {
                thread.Join();
            }
        }

        //5.工作线程核心逻辑 ---> 循环从任务队列获取任务并执行，支持线程池优雅退出
        void HandlerTask()
        {
            //1.存储当前线程名称的缓冲区
            char name[128];  

            //2.获取当前线程名称
            pthread_getname_np(pthread_self(), name, sizeof(name));

            //3.线程主循环：持续处理任务或响应退出信号
            while (true)  
            {
                //3.1：存储从任务队列取出的任务（T为可调用对象类型）
                T t;  

                // 局部作用域：通过LockGuard实现RAII加锁，出作用域自动解锁
                {
                    //3.2：加锁：保护任务队列和状态变量的访问
                    LockGuard lockguard(_mutex);  

                    /* 当任务队列“为空”的时候，需要根据线程池的状态，决定线程该做什么：
                    *      1. 线程池仍在运行 ---> 线程休眠
                    *      2. 线程池停止运行 ---> 线程退出
                    *  当任务队列“非空”的时候：
                    *      3. ---> 线程处理队列中任务
                    */
                    //3.3：循环等待条件：任务队列为空 && 线程池仍在运行
                    while (_taskq.empty() && _isrunning) //注意：这种情况中使用了Wait，所以判断的时候要使用while防止伪唤醒
                    {
                        //1)休眠线程计数+1（记录当前线程进入休眠）
                        _sleepernum++;  

                        //2)阻塞等待：释放锁，直到被唤醒
                        _cond.Wait(_mutex);  

                        //3)休眠线程计数-1（线程被唤醒，退出休眠）
                        _sleepernum--;  
                    }

                    //3.4：线程被唤醒后，检查退出条件：任务队列为空 && 线程池停止运行
                    if (_taskq.empty() && !_isrunning)
                    {
                        //1)打印日志：记录线程退出原因
                        LOG(LogLevel::INFO) << name << " 退出了, 线程池退出&&任务队列为空";

                        //2)退出线程主循环，线程终止
                        break; 
                    }

                    //3.4：获取队首任务（拷贝到线程局部变量t）
                    t = _taskq.front();  // 执行到此处：任务队列非空（即使线程池停止，也需处理剩余任务）

                    //3.5： 从队列中移除已取出的任务
                    _taskq.pop();        
                }  //注意：LockGuard析构，自动解锁（任务处理无需持有锁，提升并发效率）

                //3.6：处理任务：调用可调用对象t（任务已脱离队列，在当前线程上下文执行）
                t(); //关键：任务处理在临界区外，避免长时间占用锁导致其他线程阻塞
            }
        }

        //6.“任务入队接口” ---> 将任务添加到任务队列，线程安全
        bool Enqueue(const T &in)
        {
            //1.仅当线程池运行时，允许入队（停止后拒绝新任务）
            if (_isrunning)  //这样做的目的是：防止线程池关闭之后还在任务队列中挤压任务，导致线程无法退出
            {
                //1.1：加锁：保护任务队列的并发访问
                LockGuard lockguard(_mutex);  
                
                //1.2：任务入队（添加到队列尾部）
                _taskq.push(in);              
                
                //1.3：若所有工作线程都在休眠，唤醒一个线程处理新任务（避免任务堆积）
                if (_threads.size() == _sleepernum)
                {
                    WakeUpOneThread();
                }

                //1.4：入队成功
                return true;  
            }
            //2.线程池已停止，入队失败
            return false;  
        }
    };

    //3.静态成员变量初始化：单例实例指针（初始为nullptr，首次使用时创建）
    template <typename T>
    ThreadPool<T> *ThreadPool<T>::inc = nullptr;

    //4.静态成员变量初始化：单例创建锁（全局唯一，确保单例创建的线程安全）
    template <typename T>
    Mutex ThreadPool<T>::_lock;

}