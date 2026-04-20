#include <iostream>      
#include <unistd.h>      
#include <pthread.h>      
#include "Mutex.hpp"     
 
//1.使用MutexModule命名空间，简化Mutex类的调用
using namespace MutexModule;  

//2.定义共享变量总票数 ---> 所有线程会共同操作此变量（临界资源）
int ticket = 1000;

/*------------------------------------线程数据类 ---> 封装线程所需的参数（线程名称和互斥锁指针）------------------------------------*/
class ThreadData
{
public:
    /*-------------------------属性-------------------------*/
    //1.线程名称 ---> 用于打印区分不同线程
    //2.自定义Mutex类的指针（指向共享的互斥锁，实现同步）

    std::string name;        
    Mutex *lockp; //注意：之前我们使用POSIX线程库中的接口的时候，定义互斥锁指针是这么写的：pthread_mutex_t *lockp; 

    /*-------------------------方法-------------------------*/
    //1.“构造函数” ---> 初始化线程名称和互斥锁指针
    ThreadData(const std::string &n, Mutex &lock)
        : name(n),       // 初始化线程名称
          lockp(&lock)   // 存储互斥锁的地址（用于后续加锁/解锁）
    {}

    //2.“析构函数” ---> 空实现，无动态资源需释放
    ~ThreadData() {}     
};


/*------------------------------------新线程执行函数------------------------------------*/
void *route(void *arg)
{
    //1.将void*参数转换为ThreadData指针，获取线程所需的参数
    ThreadData *td = static_cast<ThreadData *>(arg);
    
    //2.循环抢票，直到票售罄
    while (1)  
    {
        // //2.1：加锁：调用自定义Mutex类的Lock方法（内部封装pthread_mutex_lock）
        // td->lockp->Lock();

        //2.1：RAII风格加锁：创建LockGuard对象时自动调用Mutex的Lock()
        LockGuard guard(*td->lockp);  
        /* 说明：
        *    1.当guard出作用域（如循环体结束、break、异常）时，自动调用Mutex的Unlock()
        *    2.优点：即使代码中忘记解锁，也能通过析构函数保证锁的释放，避免死锁
        */
        
        //2.2：情况一：检查是否还有剩余票
        if (ticket > 0)  
        {
            usleep(1000);  // 模拟抢票过程的耗时（微秒级休眠，放大线程竞争现象）
            printf("%s sells ticket:%d\n", td->name.c_str(), ticket); // 打印当前线程卖出的票号（使用线程名称区分）
            
            //第一步：票数递减，更新剩余票数
            ticket--;     
            
            // //第二步：解锁：调用自定义Mutex类的Unlock方法（内部封装pthread_mutex_unlock）
            // td->lockp->Unlock();
        }

        //2.3：情况二：票已售罄
        else  
        {
            // //第一步：解锁（必须释放锁，否则会导致死锁）
            // td->lockp->Unlock();  

            //第二步：退出循环，结束线程
            break;  
        }
    }
    
    //3.线程执行完毕，返回空指针
    return nullptr;  // 注意：此处未释放ThreadData对象的内存，可能导致内存泄漏（需在主线程join后释放）
}


/*------------------------------------主函数------------------------------------*/
int main(void)
{
    /*-----------------------第一步：定义并初始化“互斥锁”-----------------------*/
    //1.创建自定义Mutex对象（封装了pthread_mutex_t，自动初始化锁）
    Mutex lock;  

    /*-----------------------第二步：定义并创建“新线程”-----------------------*/
    //2.定义四个线程ID变量 ---> 用于标识线程
    pthread_t t1, t2, t3, t4;  

    //3.创建线程数据对象
    ThreadData *td1 = new ThreadData("thread 1", lock); // 传递线程名称和Mutex对象引用
    //4.创建线程t1
    pthread_create(&t1, NULL, route, td1);

    // 同理创建线程t2、t3、t4
    ThreadData *td2 = new ThreadData("thread 2", lock);
    pthread_create(&t2, NULL, route, td2);

    ThreadData *td3 = new ThreadData("thread 3", lock);
    pthread_create(&t3, NULL, route, td3);

    ThreadData *td4 = new ThreadData("thread 4", lock);
    pthread_create(&t4, NULL, route, td4);

    /*-----------------------第三步：等待回收“新线程”-----------------------*/
    //5.等待四个子线程执行完毕 ---> 阻塞主线程，避免主线程提前退出导致子线程被终止
    pthread_join(t1, NULL);
    pthread_join(t2, NULL);
    pthread_join(t3, NULL);
    pthread_join(t4, NULL);

    /*-----------------------第四步：释放“ThreadData对象”-----------------------*/
    //6.释放ThreadData对象的内存 ---> 避免内存泄漏
    delete td1;
    delete td2;
    delete td3;
    delete td4;

    //注意：Mutex对象lock的析构函数会自动销毁内部的互斥锁（无需手动调用pthread_mutex_destroy）

    //7.主线程退出
    return 0;  
}