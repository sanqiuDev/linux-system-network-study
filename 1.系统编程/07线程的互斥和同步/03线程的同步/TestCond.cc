//怎么使用条件变量？

#include <iostream>       
#include <vector>        
#include <string>        
#include <unistd.h>       
#include <pthread.h>     

//1.定义线程数量为5
#define NUM 5   

//2.共享计数器（临界资源，多线程需同步访问）         
int cnt = 1000;            

//3.全局互斥锁 ---> 保护共享资源cnt的访问，确保条件判断和修改的原子性
pthread_mutex_t glock = PTHREAD_MUTEX_INITIALIZER; 

//4.全局条件变量 ---> 用于线程间同步，实现"等待-唤醒"机制（线程等待特定条件，满足后被唤醒）
pthread_cond_t gcond = PTHREAD_COND_INITIALIZER;   

/*------------------------------------新线程执行函数------------------------------------*/
void *threadrun(void *args)
{
    //1.将void*参数转换为char指针，获取线程名称
    std::string name = static_cast<const char *>(args);  
    //2.循环进行计数
    while (true)
    {
        //2.1：加锁：进入临界区，准备访问共享资源cnt
        pthread_mutex_lock(&glock);
        
        //2.2：等待条件变量：
        pthread_cond_wait(&gcond, &glock); 
        /* 注意事项：
        *     1. 调用后自动释放glock，让其他线程有机会修改共享资源
        *     2. 线程进入阻塞状态，等待被pthread_cond_signal/broadcast唤醒
        *     3. 唤醒后会重新获取glock，继续执行后续代码（仍在临界区内）
        */
        
        //2.3：被唤醒后执行操作 ---> 打印线程名称和当前计数器值，然后递增计数器
        std::cout << name << " 计算: " << cnt << std::endl;
        cnt++;
        
        //2.4：解锁：退出临界区，释放对共享资源的独占
        pthread_mutex_unlock(&glock);
    }
}


/*------------------------------------主函数------------------------------------*/
int main()
{
    /*-----------------------第一步：创建vector容器-----------------------*/
    //1.定义一个存储pthread_t的vector容器 ---> 用vector存储所有线程ID，便于后续等待线程结束
    std::vector<pthread_t> threads;  

    /*-----------------------第二步：创建多线程-----------------------*/
    //2.创建NUM个线程
    for (int i = 0; i < NUM; i++)
    {
        //2.1：定义线程ID变量
        pthread_t tid;  
        //2.2：动态分配线程名称缓冲区            
        char *name = new char[64];   
        //2.3：格式化线程名称（如"thread-0"）
        snprintf(name, 64, "thread-%d", i);  

        //2.4：创建线程
        int n = pthread_create(&tid, nullptr, threadrun, name);
        if (n != 0)  
        {
            continue; // 线程创建失败则跳过
        }

        //2.5：存储成功创建的线程ID
        threads.push_back(tid); 
        sleep(1);  // 休眠1秒，让线程按顺序创建（便于观察输出）
    }

    sleep(3);  // 主线程休眠3秒，确保所有子线程都已启动并进入等待状态

    /*-----------------------第三步：唤醒等待的线程-----------------------*/
    //3.循环唤醒线程 ---> 每隔1秒唤醒所有等待的线程
    while(true)
    {
        /*--------------------【方法一：单播唤醒】--------------------*/
        //1.单播唤醒 ---> 仅唤醒一个在gcond上等待的线程
        std::cout << "唤醒一个线程... " << std::endl;
        pthread_cond_signal(&gcond);

        /*--------------------【方法二：广播唤醒】--------------------*/
        //2.广播唤醒 ---> 唤醒所有在gcond上等待的线程
        std::cout << "唤醒所有线程... " << std::endl;
        pthread_cond_broadcast(&gcond);
        
        sleep(1);  // 休眠1秒，控制唤醒频率
    }

    /*-----------------------第三步：等待回收线程-----------------------*/
    //4.等待所有线程结束
    for (auto &id : threads)
    {
        int m = pthread_join(id, nullptr);
        (void)m;  // 忽略返回值，避免编译警告
    }

    return 0;
}