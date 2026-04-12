//模拟数据不一致的真实现象抢票 
 
#include <stdio.h>    
#include <unistd.h> 
#include <pthread.h>    // 提供“POSIX线程库接口” ---> 线程创建、互斥锁等

//1.定义“共享变量” ---> 表示总票数，所有线程会共同操作此变量
int ticket = 1000;

/*------------------------------------新线程执行函数------------------------------------*/
void *route(void *arg)
{
    //1.将void*参数转换为字符串指针，获取线程标识
    char *id = (char *)arg; 

    //2.新线程不停地执行循环中的动作
    while (1)
    {
        //2.2：如果还有剩余票
        if (ticket > 0)  
        {
            usleep(1000);  // 模拟抢票过程的耗时（微秒级休眠，放大线程竞争现象）
            printf("%s sells ticket:%d\n", id, ticket); //打印：输出当前线程抢到的票号

            //第一步：进行抢票 ---> 票数递减
            ticket--;    

        }
        //2.3：如果已经没有票的话
        else
        {
            //第一步：票数为0退出循环
            break;  
        }
    }

    //3.线程执行完毕，返回空指针
    return nullptr;  
}


/*------------------------------------主函数------------------------------------*/
int main(void)
{
    //1.定义四个线程ID变量
    pthread_t t1, t2, t3, t4;  

    //2.创建四个新线程 ---> 均执行route函数，分别传入不同的线程标识
    pthread_create(&t1, NULL, route, (void *)"thread 1");
    pthread_create(&t2, NULL, route, (void *)"thread 2");
    pthread_create(&t3, NULL, route, (void *)"thread 3");
    pthread_create(&t4, NULL, route, (void *)"thread 4");

    //3.等待四个线程全部执行完毕
    pthread_join(t1, NULL);
    pthread_join(t2, NULL);
    pthread_join(t3, NULL);
    pthread_join(t4, NULL);

    //4.主线程退出
    return 0;  
}