#ifndef _THREAD_H_
#define _THREAD_H_

#include <iostream>     
#include <cstring>
#include <pthread.h>  
#include <functional> // 提供std::function ---> 用于封装线程执行的函数对象
 

// 定义线程模块的命名空间 ---> 避免全局命名冲突
namespace ThreadModule
{
    //1.定义静态变量 ---> 用于自动生成线程名称（潜在线程安全问题：多线程并发创建时可能导致命名重复，需加锁保护）
    static uint32_t number = 1;

    //2.线程类模板，支持接收任意类型T的参数
    template <typename T>
    class Thread
    {
    private:
    //3.定义线程执行函数的类型 ---> 接收T类型参数，无返回值的函数对象
    using func_t = std::function<void(T)>; 
    
    /*-------------------------- 私有属性 --------------------------*/
        //1.线程ID ---> POSIX线程库定义的标识符）
        //2.存储线程退出时的返回值
        pthread_t _tid; 
        void *res;   

        //3.线程是否为分离状态（true：分离，false：非分离）
        //4.线程是否正在运行（true：运行中，false：未运行/已停止）
        bool _isdetach;    
        bool _isrunning; 

        //5.线程名称 ---> 用于标识和调试
        std::string _name; 

        //6.线程要执行的函数对象（用户传入）
        //7.传递给线程函数的参数（用户传入）
        func_t _func;      
        T _data;

        /*-------------------------- 私有方法 --------------------------*/
        //1.标记线程为“分离状态”
        void EnableDetach()
        {
            std::cout << "线程被分离了" << std::endl;
            _isdetach = true;
        }

        //2.标记线程为“运行状态”
        void EnableRunning()
        {
            _isrunning = true;
        }

        //3.新线程入口函数
        // 疑问：为什么Routine函数这里要使用static使其变成静态成员函数呢？
        static void *Routine(void *args) //参数args：传递当前Thread对象的指针 ---> 用于在静态函数中访问非静态成员
        {
            //1.将void*类型的参数转换为当前Thread对象的指针
            Thread<T> *self = static_cast<Thread<T> *>(args);

            //2.标记线程为运行中状态
            self->EnableRunning();

            //3.若线程初始化时被设置为分离状态，则执行分离操作
            if (self->_isdetach)
            {
                self->Detach();
            }

            //4.回调执行用户传入的线程函数，并传递数据
            self->_func(self->_data);

            //5.线程执行完毕后返回空指针
            return nullptr; 
        }

    public:
        //1.“构造函数” ---> 初始化线程对象
        Thread(func_t func, T data) //参数：func - 线程要执行的函数对象；data - 传递给线程函数的参数
            : _tid(0),           // 线程ID初始化为0（无效值）
              res(nullptr),      // 线程返回值指针初始化为空

              _isdetach(false),  // 初始状态为非分离（需手动join回收资源）
              _isrunning(false), // 初始状态为未运行

              _func(func),       // 绑定用户传入的函数对象
              _data(data)        // 存储用户传入的参数
        {
            // 自动生成线程名称（格式：thread-1、thread-2...）
            _name = "thread-" + std::to_string(number++);
        }

        //2.“析构函数” ---> 目前为空实现，可根据需求添加资源清理逻辑
        ~Thread()
        {}


        //3.“创建线程”
        bool Create()
        {
            //1.若线程已在运行，直接返回失败
            if (_isrunning) 
            {
                return false;
            }

            //2.调用pthread_create创建线程
            int n = pthread_create(&_tid, nullptr, Routine, this); //注意： this：传递当前对象指针，供Routine函数访问类成员
            //情况一：若创建失败（返回非0错误码）
            if (n != 0) 
            {
                std::cerr << "create thread error: " << strerror(n) << std::endl; // 打印错误信息
                return false;
            }
            //情况二：创建成功
            else 
            {
                std::cout << _name << " create success" << std::endl;
                return true;
            }
        }

        //4.“取消线程” ---> 强制取消线程执行
        bool Cancel()
        {
            //1.仅当线程正在运行时，执行取消操作
            if (_isrunning) 
            {
                //1.1：调用pthread_cancel强制取消线程
                int n = pthread_cancel(_tid);
                //情况一：取消失败
                if (n != 0) 
                {
                    std::cerr << "cancel thread error: " << strerror(n) << std::endl;
                    return false;
                }
                //情况二：取消成功
                else 
                {
                    //1.更新运行状态为停止
                    _isrunning = false; 

                    //2.
                    std::cout << _name << " stop" << std::endl;

                    //3.
                    return true;
                }
            }

            //2.线程未运行，返回失败
            return false; 
        }

        //5.“等待线程” ---> 阻塞等待线程退出并回收资源
        void Join()
        {
            //1.若线程已分离，无法进行join操作
            if (_isdetach)
            {
                std::cout << "你的线程已经是分离的了,不能进行join" << std::endl;
                return;
            }

            //2.调用pthread_join等待线程退出，获取返回值
            int n = pthread_join(_tid, &res);
            //情况一：join失败
            if (n != 0) 
            {
                std::cerr << "join thread error: " << strerror(n) << std::endl;
            }
            //情况二：join成功
            else 
            {
                std::cout << "join success" << std::endl;
            }
        }
        //6.“线程分离” 
        void Detach()
        {
            //1.根据具体的情况进行线程分离的操作
            //情况一：如果线程已处于分离状态 ---> 直接返回
            if (_isdetach)  
            {
                return;
            }
            //情况二：如果线程正在运行 ---> 调用pthread_detach系统接口
            if (_isrunning) 
            {
                pthread_detach(_tid);
            }

            //情况三：如果线程没有运行 ---> 更新分离状态标志位
            EnableDetach(); 
        }


        //7.“获取线程的名称” 
        std::string Name()
        {
            return _name;
        }
    };
}

#endif

