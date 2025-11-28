#include <iostream>   
#include <unistd.h>     
#include <pthread.h>    
#include "BlockQueue.hpp"  // 包含“自定义阻塞队列类头文件” ---> 线程安全的生产者-消费者队列


/*-------------------------------------生产者线程入口函数 ---> 向阻塞队列中生产int类型数据-------------------------------------*/
void *productor(void *args)
{
    //1.初始化生产数据的初始值
    int data = 1; 

    //2.将void*参数转换为BlockQueue<int>指针，获取阻塞队列对象
    BlockQueue<int> *bq = static_cast<BlockQueue<int>*>(args);

    //3.
    while(true)
    {
        // //3.3：休眠1秒，控制生产节奏
        // sleep(1); 

        //3.2：打印消费的数据
        std::cout << "生产了一个数据：" << data << std::endl;

        //3.3：向阻塞队列中放入数据 ---> 若队列已满，线程会阻塞等待
        bq->Push(data);

        //3.4：生产数据递增
        data++; 


    }
}

/*-------------------------------------消费者线程入口函数 ---> 从阻塞队列中消费int类型数据-------------------------------------*/
void *consumer(void *args)
{
    //1.将void*参数转换为BlockQueue<int>指针，获取阻塞队列对象
    BlockQueue<int> *bq = static_cast<BlockQueue<int>*>(args);

    //2.
    while(true)
    {
        //2.1：休眠1秒，控制生产节奏
        sleep(1); 

        //2.2：从阻塞队列中取出数据 ---> 若队列为空，线程会阻塞等待
        int data = bq->Pop();

        //2.3：打印消费的数据
        std::cout << "消费了一个数据：" << data << std::endl;

    }
}



/*----------------------------------------主函数----------------------------------------*/
int main()
{
    /*----------------------------第一步：创建阻塞队列对象----------------------------*/
    //1.申请阻塞队列（存储int类型数据）
    BlockQueue<int> *bq = new BlockQueue<int>();

    /*----------------------------第二步：定义并创建线程----------------------------*/
    //2.构建生产者和消费者线程ID
    pthread_t p, c;

    //3.创建生产者线程 ---> 执行productor函数，传入阻塞队列指针作为参数
    pthread_create(&p, nullptr, productor, bq);
    //4.创建消费者线程 ---> 执行consumer函数，传入阻塞队列指针作为参数
    pthread_create(&c, nullptr, consumer, bq);

    /*----------------------------第三步：等待回收线程----------------------------*/
    //5.等待生产者&消费者线程执行完毕
    pthread_join(p, nullptr);
    pthread_join(c, nullptr);

    return 0;
}