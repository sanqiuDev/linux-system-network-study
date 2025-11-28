#include <iostream>        
#include <unistd.h>       
#include <pthread.h>      
#include "RingQueue.hpp"   // 包含“环形队列类头文件” ---> 多生产者-多消费者线程安全队列

/*-------------------------------------------线程参数结构体-------------------------------------------*/
// 线程参数结构体 ---> 用于向线程函数传递多个参数（pthread_create仅支持单个void*参数）
struct threaddata 
{
    //1.指向环形队列的指针 ---> 生产者和消费者共享的缓冲区
    RingQueue<int> *rq;   
     
    //2.线程名称 ---> 用于打印日志，区分不同 生产者/消费者
    std::string name;     
};


/*-------------------------------------------新线程入口函数-------------------------------------------*/
//1.生产者线程入口函数：生成int类型数据并写入环形队列
int data = 1;  // 生产数据的初始值（全局变量，所有生产者共享，递增生成连续数据）
void *productor(void *args) // 参数args：指向threaddata结构体的指针（包含队列指针和线程名称）
{
    //1.将void*类型的通用参数转换为threaddata指针，获取线程所需资源
    threaddata *td = static_cast<threaddata*>(args);

    //2.生产者循环：持续生产数据
    while (true) 
    {
        sleep(1);  // 模拟生产者生成数据的间隔（每1秒生产一个数据，控制生产节奏）
        // sleep(2);  // 备选节奏：每2秒生产一个数据（注释掉的可选方案）

        //1.日志打印 ---> 告知用户当前生产者生成的数据（包含线程名称和数据值）
        std::cout << td->name << " 生产了一个任务: " << data << std::endl;

        //2.1：生产数据 ---> 将当前data值写入环形队列
        td->rq->Equeue(data);

        //2.3：更新生产数据 ---> 下一个数据递增（生成连续的int数据，避免重复）
        data++;  
    }
}


//2.消费者线程入口函数 ---> 从环形队列中读取int类型数据并打印
void *consumer(void *args) //参数args：指向threaddata结构体的指针（包含队列指针和线程名称）
{
    //1.将void*类型的通用参数转换为threaddata指针，获取线程所需资源
    threaddata *td = static_cast<threaddata*>(args);

    //2.消费者循环：持续消费数据
    while (true)  
    {
        sleep(3);  // 模拟消费者处理数据的耗时（每3秒消费一个数据，控制消费节奏）

        //2.1：消费数据 ---> 从环形队列中取出数据
        int t = 0;  
        td->rq->Pop(&t);

        //2.2：处理数据 ---> 打印消费信息（数据已取出，属于当前线程上下文，与队列无关）
        std::cout << td->name << " 消费者拿到了一个数据:  " << t << std::endl;
    }
}




/*-------------------------------------------主函数-------------------------------------------*/
int main()
{
    /* 核心思路：基于环形队列实现"多生产者-多消费者"模型
    *    特点：多个生产者并发写入数据，多个消费者并发读取数据，队列自动处理同步与互斥
    *    关键：环形队列内部通过"信号量（同步）+ 双互斥锁（互斥）"保证线程安全，上层无需关心底层同步
    */

    /*-----------------------------第一步：创建环形队列对象-----------------------------*/
    //1.创建环形队列对象（存储int类型数据，默认容量5）
    RingQueue<int> *rq = new RingQueue<int>();
    
    /*-----------------------------第二步：创建生产者和消费者线程-----------------------------*/
    //2.定义线程ID数组 ---> p[3]存储3个生产者线程ID，c[2]存储2个消费者线程ID
    pthread_t  p[3], c[2];

    //3.---------------------- 创建生产者线程 ----------------------
    // 生产者线程1：初始化参数结构体
    //3.1：准备
    threaddata *ptd1 = new threaddata();
    ptd1->rq = rq;
    ptd1->name = "pthread-1"; 

    //3.2：创建
    pthread_create(p, nullptr, productor, ptd1);  


    // 生产者线程2：同理初始化参数
    threaddata *ptd2 = new threaddata();
    ptd2->rq = rq;
    ptd2->name = "pthread-2"; 
    pthread_create(p + 1, nullptr, productor, ptd2); 

    // 生产者线程3：同理初始化参数
    threaddata *ptd3 = new threaddata();
    ptd3->rq = rq;
    ptd3->name = "pthread-3"; 
    pthread_create(p + 2, nullptr, productor, ptd3); 

    //4.---------------------- 创建消费者线程 ----------------------
    // 消费者线程1：初始化参数结构体，绑定队列和线程名称
    //4.1：准备
    threaddata *ctd1 = new threaddata();
    ctd1->rq = rq;  
    ctd1->name = "cthread-1";  
    //4.2：创建
    pthread_create(c, nullptr, consumer, ctd1); 

    // 消费者线程2：同理初始化参数，绑定队列和名称
    threaddata *ctd2 = new threaddata();
    ctd2->rq = rq;
    ctd2->name = "cthread-2";
    pthread_create(c + 1, nullptr, consumer, ctd2);  


    /*-----------------------------第三步：等待线程执行完毕-----------------------------*/
    //5.等待3个生产者线程结束
    pthread_join(p[0], nullptr);
    pthread_join(p[1], nullptr);
    pthread_join(p[2], nullptr);

    //6.等待2个消费者线程结束
    pthread_join(c[0], nullptr);
    pthread_join(c[1], nullptr);

    return 0;
}
