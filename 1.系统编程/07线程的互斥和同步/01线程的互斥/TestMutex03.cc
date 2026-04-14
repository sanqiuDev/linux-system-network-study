//使用局部互斥锁解决数据不一致的抢票情况
 
#include <stdio.h>       
#include <string>         
#include <unistd.h>       
#include <pthread.h>      

//1.定义“共享变量” ---> 表示总票数，所有线程会共同操作此变量
int ticket = 1000;

/*------------------------------------线程数据类 ---> 封装线程所需的参数（线程名称和互斥锁指针）------------------------------------*/
class ThreadData
{
public:
    /*-------------------------属性-------------------------*/
    //1.线程名称 ---> 用于打印区分不同线程
    //2.互斥锁指针 ---> 指向共享的互斥锁，用于同步操作

    std::string name;        
    pthread_mutex_t *lockp; 

    /*-------------------------方法-------------------------*/
    //1.“构造函数” ---> 初始化线程名称和互斥锁指针
    ThreadData(const std::string &n, pthread_mutex_t &lock)
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
        //2.1：加锁：通过互斥锁指针锁定共享资源，确保同一时间只有一个线程执行后续操作
        pthread_mutex_lock(td->lockp);
        
        //2.3：情况一：检查是否还有剩余票
        if (ticket > 0)  
        {
            usleep(1000);  // 模拟抢票过程的耗时（微秒级休眠，放大线程竞争现象）
            printf("%s sells ticket:%d\n", td->name.c_str(), ticket); // 打印当前线程卖出的票号（使用线程名称区分）
            
            //第一步：票数递减，更新剩余票数
            ticket--;     
            
            //第二步：解锁：释放互斥锁，允许其他线程抢票
            pthread_mutex_unlock(td->lockp);
        }

        //2.4：情况二：票已售罄
        else  
        {
            //第一步；解锁（避免死锁）
            pthread_mutex_unlock(td->lockp);  

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
    //1.定义互斥锁 ---> 用于保护共享变量ticket
    pthread_mutex_t lock;  
    
    //2.初始化互斥锁 ---> 使用默认属性（NULL）
    pthread_mutex_init(&lock, nullptr); 

    /*-----------------------第二步：定义并初始化“新线程”-----------------------*/
    //3.定义四个线程ID变量 ---> 用于标识线程
    pthread_t t1, t2, t3, t4;  

    //4.创建线程数据对象
    ThreadData *td1 = new ThreadData("thread 1", lock); // 传递线程名称和互斥锁引用
    pthread_create(&t1, NULL, route, td1);

    ThreadData *td2 = new ThreadData("thread 2", lock);
    pthread_create(&t2, NULL, route, td2);

    ThreadData *td3 = new ThreadData("thread 3", lock);
    pthread_create(&t3, NULL, route, td3);

    ThreadData *td4 = new ThreadData("thread 4", lock);
    pthread_create(&t4, NULL, route, td4);

    /*-----------------------第三步：等待回收“新线程”-----------------------*/
    //5.等待四个子线程执行完毕（阻塞主线程，避免主线程提前退出）
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

    /*-----------------------第五步：销毁“互斥锁”-----------------------*/
    //7.销毁互斥锁，释放其占用的系统资源
    pthread_mutex_destroy(&lock);
    
    //8.主线程退出
    return 0;  
}