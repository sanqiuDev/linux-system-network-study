#include <iostream>       
#include <semaphore.h>    // 提供POSIX信号量底层接口（sem_t及sem_*系列函数）
#include <pthread.h>      // 提供POSIX线程库接口（信号量常用于多线程同步，需依赖线程库）
 
// 信号量模块命名空间 ---> 封装信号量相关功能，避免命名冲突，提升代码模块化程度
namespace SemModule
{
    //1.信号量默认初始值（1对应"互斥锁"语义，0对应"同步锁"语义）
    const int defaultvalue = 1;  
    /* 初始值含义：
    *     1：适用于互斥场景（同一时间仅允许一个线程访问资源）
    *     N（N>1）：适用于限流场景（允许N个线程同时访问资源）
    *     0：适用于同步场景（初始无资源，需等待其他线程释放）
    */

    // 信号量封装类 ---> 对POSIX底层信号量（sem_t）进行面向对象封装，实现多线程间的同步与互斥（控制资源访问顺序、限制并发访问数量）
    class Sem
    {  
    private:
        //1.底层信号量句柄（POSIX标准类型，封装系统级信号量资源）
        sem_t _sem;  

    public:
        //1.“构造函数” ---> 初始化信号量
        Sem(unsigned int sem_value = defaultvalue)
        {
            sem_init(&_sem, 0, sem_value);
        }

        //2.“析构函数” ---> 销毁信号量，释放系统资源
        ~Sem()
        {
            sem_destroy(&_sem); //注意：销毁前需确保无线程正在等待该信号量，否则会导致未定义行为
        }


        //3.P操作（申请资源）---> 也叫wait操作，原子性地减少信号量值
        void P()
        {
            int n = sem_wait(&_sem);
            (void)n;  
        }

        //4.V操作（释放资源）---> 也叫post操作，原子性地增加信号量值
        void V()
        {
            int n = sem_post(&_sem);
            (void)n;  
        }
    };
}